/**
 * @file
 * @brief Tests for playing one decoder through a device.
 * @author Roman Glaz
 * @copyright © 2026, <vokerlee@gmail.com>
 *
 * Wiola is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wiola is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wiola. If not, see <http://www.gnu.org/licenses/>.
 */

#include <engine/transport/player.hpp>

#include <fixtures/rig.hpp>
#include <fixtures/wav.hpp>

#include <audio/device/output.hpp>
#include <audio/dsp/buffer_source.hpp>
#include <audio/dsp/chain.hpp>
#include <codec/decode/decoder.hpp>
#include <codec/decode/open.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <pcm/stream_spec.hpp>

#include <fakes/output.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <numbers>
#include <ranges>
#include <span>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;
using namespace wiola::units::literals;
using wiola::audio::Chain;
using wiola::engine::Player;
using wiola::pcm::StreamSpec;
using wiola::testing::Rig;
using State = wiola::engine::Playback::State;
namespace units = wiola::units;

/// Waits for `predicate`, so a test never depends on how fast a device happens to run.
template<typename Predicate>
bool eventually(Predicate predicate, std::chrono::milliseconds limit = 5s)
{
    for (auto waited = 0ms; waited < limit; waited += 10ms) {
        if (predicate())
            return true;

        std::this_thread::sleep_for(10ms);
    }

    return predicate();
}

} // namespace

TEST(Player, PlaysToTheEndOnItsOwn)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const wiola::codec::Decoder* played{source.get()};
    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    EXPECT_TRUE(player.playing());

    player.wait();

    EXPECT_TRUE(player.finished());
    EXPECT_TRUE(played->exhausted());
}

/// The point of the thread: the caller is free while the audio is still going.
TEST(Player, StartReturnsBeforePlaybackEnds)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    EXPECT_FALSE(player.finished());
}

TEST(Player, StopEndsPlaybackAtOnce)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    const auto before = std::chrono::steady_clock::now();

    player.stop();
    player.wait();

    // Stopping does not play out what is buffered, so it returns in poll intervals rather than
    // in the fraction of a second the ring holds.
    EXPECT_LT(std::chrono::steady_clock::now() - before, 100ms);
    EXPECT_TRUE(player.finished());
    EXPECT_FALSE(player.playing());
}

TEST(Player, StopIsHarmlessAfterPlaybackHasEnded)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    player.wait();
    player.stop();
    player.stop();

    EXPECT_TRUE(player.finished());
}

TEST(Player, PauseSilencesAndResumeContinues)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    ASSERT_TRUE(player.pause());
    EXPECT_FALSE(player.playing());

    // Pausing is not ending: the player is still there to be resumed.
    EXPECT_FALSE(player.finished());

    // There is nothing left to silence, so the second one has nothing to report.
    EXPECT_FALSE(player.pause());
    EXPECT_FALSE(player.playing());

    ASSERT_TRUE(player.resume());
    EXPECT_TRUE(player.playing());

    EXPECT_TRUE(eventually([&player] { return player.finished(); }));
}

/// The fixture is a quarter second, so seeking past halfway leaves very little to play.
TEST(Player, SeekMovesTheSource)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    ASSERT_TRUE(player.pause());
    player.seek(1300_ms);

    // Seeking does not start a paused player.
    EXPECT_FALSE(player.playing());

    const auto began = std::chrono::steady_clock::now();

    ASSERT_TRUE(player.resume());
    player.wait();

    // Only a source that moved runs out this soon: from where it was, the rest would take the
    // length of the track.
    EXPECT_LT(std::chrono::steady_clock::now() - began, player.total_time().chrono() / 2);
}

TEST(Player, SeekBeyondTheEndIsIgnored)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    ASSERT_TRUE(player.pause());
    player.seek(3600_s);

    // The request is refused by the decoder, so playback is left where it was rather than ended.
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(player.finished());
}

TEST(Player, TimePlayedStartsAtTheBeginning)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    EXPECT_EQ(player.time_played(), units::Time{});
}

TEST(Player, TimePlayedReachesTheEndOfTheSource)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const double length{static_cast<double>(source->num_frames().count()) /
        source->spec().sample_rate.get<wiola::units::Hz>()};
    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    player.wait();

    EXPECT_NEAR(player.time_played().get<wiola::units::Sec>(), length, 0.02);
}

/// Position follows the device, so a seek moves it even though nothing has been played since.
TEST(Player, TimePlayedFollowsASeek)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    ASSERT_TRUE(player.pause());
    player.seek(200_ms);

    EXPECT_TRUE(eventually([&player] {
        return player.time_played().get<wiola::units::Sec>() > 0.19;
    }));
    EXPECT_LT(player.time_played().get<wiola::units::Sec>(), 0.22);
}

TEST(Player, ResumeRefusesBeforeStart)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    EXPECT_EQ(player.state(), State::idle);
    EXPECT_FALSE(player.resume());
    EXPECT_FALSE(player.playing());
}

/// A source that ran out and a listener who pressed stop are both final, and a playlist has to
/// tell them apart: one is the cue to play the next thing, the other is not.
TEST(Player, DistinguishesEndingFromBeingStopped)
{
    auto ended = wiola::testing::open_fixture();
    ASSERT_NE(ended, nullptr);

    Rig first_rig{std::move(ended)};
    Player& first{first_rig.player};

    ASSERT_TRUE(first.start());

    first.wait();
    EXPECT_EQ(first.state(), State::ended);
    EXPECT_TRUE(first.finished());

    auto stopped = wiola::testing::open_fixture();
    Rig second_rig{std::move(stopped)};
    Player& second{second_rig.player};

    ASSERT_TRUE(second.start());
    second.stop();
    second.wait();

    EXPECT_EQ(second.state(), State::stopped);
    EXPECT_TRUE(second.finished());
}

TEST(Player, TimePlayedReportsASeekThatHasNotBeenAppliedYet)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    // No device is needed: nothing has started, so nothing can apply the request.
    player.seek(1000_ms);

    EXPECT_NEAR(player.time_played().get<units::Sec>(), 1.0, 0.01);
}

TEST(Player, StartsWhereASeekAskedForBeforeIt)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const wiola::pcm::Frames total{source->num_frames()};
    const wiola::codec::Decoder* begun{source.get()};
    Rig rig{std::move(source)};
    Player& player{rig.player};

    player.seek(1000_ms);

    ASSERT_TRUE(player.start());

    EXPECT_GT(player.time_played().get<units::Sec>(), 0.9);

    // The source belongs to the decoding thread while it runs, and to us once it has stopped.
    player.stop();
    player.wait();

    // Beginning a second into the source left the decoder with only the rest of it to read.
    EXPECT_LT(begun->num_frames_left(), wiola::pcm::Frames{total.count() / 2});
}

TEST(Player, KeepsASeekAskedForAfterStopping)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    player.stop();
    player.wait();

    // A stopped player has no thread to take the request up, so start is what honors it.
    player.seek(1000_ms);
    EXPECT_NEAR(player.time_played().get<units::Sec>(), 1.0, 0.01);

    ASSERT_TRUE(player.start());
    EXPECT_GT(player.time_played().get<units::Sec>(), 0.9);
}

/// A source that takes its time moving, so the gap between asking for a seek and it being carried
/// out is wide enough to look into. Silence is enough: nothing here listens.
class SlowSource final : public wiola::codec::Decoder {
public:
    SlowSource()
        : Decoder{
              wiola::pcm::StreamSpec{units::Frequency{44100.0}, 2},
              wiola::pcm::Frames{44100 * 4}
    }
    {
    }

protected:
    std::size_t decode(std::span<float> output, wiola::pcm::Frames num_frames) override
    {
        std::fill_n(output.begin(), num_frames.count() * 2, 0.0F);

        return num_frames.count();
    }

    bool seek_frame(wiola::pcm::Frames /*frame_index*/) override
    {
        std::this_thread::sleep_for(200ms);

        return true;
    }
};

/// A source short enough to run out while what it decoded is still being played, and one that
/// says how often it was moved.
class ShortSource final : public wiola::codec::Decoder {
public:
    ShortSource()
        : Decoder{
              wiola::pcm::StreamSpec{units::Frequency{44100.0}, 2},
              wiola::pcm::Frames{4410}
    }
    {
    }

    [[nodiscard]] int num_seeks() const noexcept { return num_seeks_.load(); }

protected:
    std::size_t decode(std::span<float> output, wiola::pcm::Frames num_frames) override
    {
        std::fill_n(output.begin(), num_frames.count() * 2, 0.0F);

        return num_frames.count();
    }

    bool seek_frame(wiola::pcm::Frames /*frame_index*/) override
    {
        num_seeks_.fetch_add(1);

        return true;
    }

private:
    std::atomic<int> num_seeks_{0};
};

/// The source runs out long before the device has played what it decoded. A seek asked for in
/// that gap is carried out there and then, rather than waiting for the next start.
TEST(Player, SeeksWhileTheLastOfItIsStillBeingPlayed)
{
    auto source = std::make_unique<ShortSource>();
    const ShortSource* moved{source.get()};

    const StreamSpec spec{source->spec()};
    wiola::lockfree::SPSCRingBuffer<float> buffer{
        spec.samples_per(units::Time{std::chrono::milliseconds{250}})};

    // An output that plays nothing holds the player in that gap for as long as the test needs.
    wiola::testing::FakeOutput output;
    Player player{std::move(source), buffer.producer(), output};

    ASSERT_TRUE(player.start());
    ASSERT_EQ(moved->num_seeks(), 0);

    player.seek(50_ms);

    EXPECT_TRUE(eventually([moved] { return moved->num_seeks() == 1; }));
    EXPECT_FALSE(player.finished());

    player.stop();
    player.wait();
}

/// Carrying a seek out takes time - a device to stop, a file to move through - and for all of it
/// the place asked for is where playback is going. Reporting the old one meanwhile shows as a bar
/// that springs back before it settles.
TEST(Player, TimePlayedNeverFallsBackWhileASeekIsCarriedOut)
{
    Rig rig{std::make_unique<SlowSource>()};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    player.seek(2000_ms);

    // Watched from the moment it is asked for until well after the source has finished moving.
    double lowest{player.time_played().get<units::Sec>()};

    for (auto waited = 0ms; waited < 400ms; waited += 1ms) {
        lowest = std::min(lowest, player.time_played().get<units::Sec>());
        std::this_thread::sleep_for(1ms);
    }

    EXPECT_GT(lowest, 1.9);
}

TEST(Player, PlaysAgainAfterEnding)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    player.wait();
    ASSERT_EQ(player.state(), State::ended);

    // Starting a finished player winds it back rather than refusing.
    ASSERT_TRUE(player.start());
    EXPECT_EQ(player.state(), State::playing);
    EXPECT_LT(player.time_played().get<wiola::units::Sec>(), 0.5);

    player.wait();
    EXPECT_EQ(player.state(), State::ended);
}

TEST(Player, RefusesToStartWhilePlaying)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    EXPECT_FALSE(player.start());
    EXPECT_EQ(player.state(), State::playing);

    ASSERT_TRUE(player.pause());
    EXPECT_FALSE(player.start());
    EXPECT_EQ(player.state(), State::paused);
}

/// A player needs somewhere to play, not a sound card: with an output of its own playback
/// runs the same on a machine that has none.
TEST(Player, RunsOnAnyOutput)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const StreamSpec spec{source->spec()};
    wiola::lockfree::SPSCRingBuffer<float> buffer{
        spec.samples_per(units::Time{std::chrono::milliseconds{250}})};
    wiola::testing::FakeOutput output;
    Player player{std::move(source), buffer.producer(), output};

    ASSERT_TRUE(player.start());
    EXPECT_EQ(player.state(), State::playing);
    EXPECT_TRUE(output.running());

    player.stop();
    player.wait();

    EXPECT_EQ(player.state(), State::stopped);
    EXPECT_FALSE(output.running());
}

/// What has been heard is the output's count, not what has been decoded.
TEST(Player, FollowsTheOutputRatherThanTheDecoder)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const StreamSpec spec{source->spec()};
    wiola::lockfree::SPSCRingBuffer<float> buffer{
        spec.samples_per(units::Time{std::chrono::milliseconds{250}})};
    wiola::testing::FakeOutput output;
    Player player{std::move(source), buffer.producer(), output};

    ASSERT_TRUE(player.start());
    EXPECT_EQ(player.time_played().get<units::Sec>(), 0.0);

    output.play(wiola::pcm::Frames{
        static_cast<std::size_t>(spec.sample_rate.get<units::Hz>()) / 2});

    EXPECT_NEAR(player.time_played().get<units::Sec>(), 0.5, 1e-6);

    player.stop();
    player.wait();
}

/// Everything a listener can ask for, as fast as it can be asked, against a track that is
/// playing. The tests above each name one interleaving; this one names none, and is there for
/// the ones nobody thought to name. It asserts almost nothing on its own - the thread sanitizer
/// is what reads the result.
TEST(Player, SurvivesATransportHammering)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    Rig rig{std::move(source)};
    Player& player{rig.player};

    ASSERT_TRUE(player.start());

    std::atomic<bool> asking{true};

    const std::jthread listener{[&player, &asking] {
        for (int i = 0; asking.load(); ++i) {
            static_cast<void>(player.pause());
            player.seek(units::Time{std::chrono::milliseconds{i % 1400}});
            static_cast<void>(player.resume());
        }
    }};

    std::this_thread::sleep_for(1s);
    asking.store(false);
}

/// The rule the device depends on: the caller opens it, and from the moment there is a decoding
/// thread, only that thread starts or stops it. A listener asking to pause moves the state and
/// nothing else. Checked here rather than left to a sanitizer, so it holds on any build.
TEST(Player, TouchesTheOutputFromTheDecodingThreadAlone)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const StreamSpec spec{source->spec()};
    wiola::lockfree::SPSCRingBuffer<float> buffer{
        spec.samples_per(units::Time{std::chrono::milliseconds{250}})};
    wiola::testing::FakeOutput output;
    Player player{std::move(source), buffer.producer(), output};

    ASSERT_TRUE(player.start());

    // The caller opened the device on the line above; everything from here is the decoding
    // thread's, and the pauses below are what would tempt another thread into it.
    output.watch();

    for (int i = 0; i < 20; ++i) {
        ASSERT_TRUE(player.pause());
        std::this_thread::sleep_for(5ms);

        ASSERT_TRUE(player.resume());
        std::this_thread::sleep_for(5ms);
    }

    player.stop();
    player.wait();

    const std::vector<std::thread::id> touching{output.touching_threads()};

    ASSERT_EQ(touching.size(), 1u) << "more than one thread started or stopped the output";
    EXPECT_NE(touching.front(), std::this_thread::get_id());
}

/// A device that will not start again is a fault, not a listener pressing stop: the two look the
/// same to a window that can only read the state, and only one of them is worth saying.
TEST(Player, ReportsAFaultWhenTheOutputWillNotStartAgain)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const StreamSpec spec{source->spec()};
    wiola::lockfree::SPSCRingBuffer<float> buffer{
        spec.samples_per(units::Time{std::chrono::milliseconds{250}})};
    wiola::testing::FakeOutput output;
    Player player{std::move(source), buffer.producer(), output};

    ASSERT_TRUE(player.start());
    ASSERT_TRUE(player.pause());

    // The device is silenced by the decoding thread, so the refusal is set once it has been:
    // otherwise a pause and a resume can both land before that thread looks.
    ASSERT_TRUE(eventually([&output] { return !output.running(); }));

    output.refuse_starts();

    ASSERT_TRUE(player.resume());

    EXPECT_TRUE(eventually([&player] { return player.finished(); }));
    EXPECT_EQ(player.state(), State::faulted);

    player.stop();
    player.wait();
}

/// A device that stops answering while a track plays is noticed, rather than left to look like
/// playback that carries on with nothing coming out.
TEST(Player, ReportsAFaultWhenTheDeviceStopsOnItsOwn)
{
    auto source = wiola::testing::open_fixture();
    ASSERT_NE(source, nullptr);

    const StreamSpec spec{source->spec()};
    wiola::lockfree::SPSCRingBuffer<float> buffer{
        spec.samples_per(units::Time{std::chrono::milliseconds{250}})};
    wiola::testing::FakeOutput output;
    Player player{std::move(source), buffer.producer(), output};

    ASSERT_TRUE(player.start());
    ASSERT_TRUE(player.playing());

    output.die();

    EXPECT_TRUE(eventually([&player] { return player.finished(); }))
        << "playback carried on with a device that had stopped";
    EXPECT_EQ(player.state(), State::faulted);

    player.stop();
    player.wait();
}
