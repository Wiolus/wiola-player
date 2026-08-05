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

#include <engine/player.hpp>

#include <codec/open.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;
using namespace wiola::units::literals;
using wiola::engine::Player;
using wiola::engine::PlayerState;
namespace units = wiola::units;

/// Seconds of audio a test source holds. It has to exceed what the player buffers ahead, or
/// priming swallows the whole source before playback even starts and nothing can be observed.
constexpr double source_seconds{1.5};
constexpr std::uint32_t source_rate{44100};
constexpr std::uint16_t source_channels{2};

void append(std::string& out, std::uint32_t value, std::size_t num_bytes)
{
    for (std::size_t i = 0; i < num_bytes; ++i)
        out += static_cast<char>((value >> (8 * i)) & 0xFFU);
}

/// Writes a few seconds of tone as a WAV file and opens it.
std::unique_ptr<wiola::codec::Decoder> fixture()
{
    const auto num_frames{static_cast<std::size_t>(source_seconds * source_rate)};

    std::string samples;
    for (std::size_t i = 0; i < num_frames; ++i) {
        const auto value = static_cast<std::uint32_t>(static_cast<std::int16_t>(12000 *
            std::sin(2 * std::numbers::pi * 440 * i / source_rate)));

        for (std::uint16_t channel = 0; channel < source_channels; ++channel)
            append(samples, value, 2);
    }

    std::string fmt;
    append(fmt, 1, 2);
    append(fmt, source_channels, 2);
    append(fmt, source_rate, 4);
    append(fmt, source_rate * source_channels * 2, 4);
    append(fmt, source_channels * 2, 2);
    append(fmt, 16, 2);

    std::string body{"WAVEfmt "};
    append(body, static_cast<std::uint32_t>(fmt.size()), 4);
    body += fmt + "data";
    append(body, static_cast<std::uint32_t>(samples.size()), 4);
    body += samples;

    std::string file{"RIFF"};
    append(file, static_cast<std::uint32_t>(body.size()), 4);
    file += body;

    const std::filesystem::path path{std::filesystem::temp_directory_path() / "wiola_player.wav"};
    std::ofstream out{path, std::ios::binary};
    out.write(file.data(), static_cast<std::streamsize>(file.size()));
    out.close();

    return wiola::codec::open_file(path);
}

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
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_TRUE(player.playing());

    player.wait();

    EXPECT_TRUE(player.finished());
    EXPECT_TRUE(source->exhausted());
}

/// The point of the thread: the caller is free while the audio is still going.
TEST(Player, StartReturnsBeforePlaybackEnds)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_FALSE(player.finished());
}

TEST(Player, StopEndsPlaybackAtOnce)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

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
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    player.wait();
    player.stop();
    player.stop();

    EXPECT_TRUE(player.finished());
}

TEST(Player, PauseSilencesAndResumeContinues)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

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
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    const std::size_t total{source->num_frames()};
    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    ASSERT_TRUE(player.pause());
    player.seek(1300_ms);

    // Seeking near the end leaves the decoder with little left to read.
    EXPECT_TRUE(eventually([&source, total] { return source->num_frames_left() < total / 4; }));

    // Seeking does not start a paused player.
    EXPECT_FALSE(player.playing());
}

TEST(Player, SeekBeyondTheEndIsIgnored)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    ASSERT_TRUE(player.pause());
    player.seek(3600_s);

    // The request is refused by the decoder, so playback is left where it was rather than ended.
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(player.finished());
}

TEST(Player, PositionStartsAtTheBeginning)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    EXPECT_EQ(player.position(), units::Time{});
}

TEST(Player, PositionReachesTheEndOfTheSource)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    const double length{static_cast<double>(source->num_frames()) /
        source->spec().sample_rate.get<wiola::units::Hz>()};
    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    player.wait();

    EXPECT_NEAR(player.position().get<wiola::units::Sec>(), length, 0.02);
}

/// Position follows the device, so a seek moves it even though nothing has been played since.
TEST(Player, PositionFollowsASeek)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    ASSERT_TRUE(player.pause());
    player.seek(200_ms);

    EXPECT_TRUE(eventually([&player] {
        return player.position().get<wiola::units::Sec>() > 0.19;
    }));
    EXPECT_LT(player.position().get<wiola::units::Sec>(), 0.22);
}

TEST(Player, ResumeRefusesBeforeStart)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    EXPECT_EQ(player.state(), PlayerState::idle);
    EXPECT_FALSE(player.resume());
    EXPECT_FALSE(player.playing());
}

/// A source that ran out and a listener who pressed stop are both final, and a playlist has to
/// tell them apart: one is the cue to play the next thing, the other is not.
TEST(Player, DistinguishesEndingFromBeingStopped)
{
    const auto ended = fixture();
    ASSERT_NE(ended, nullptr);

    Player first{*ended};

    if (!first.start())
        GTEST_SKIP() << "no playback device on this machine";

    first.wait();
    EXPECT_EQ(first.state(), PlayerState::ended);
    EXPECT_TRUE(first.finished());

    const auto stopped = fixture();
    Player second{*stopped};

    ASSERT_TRUE(second.start());
    second.stop();
    second.wait();

    EXPECT_EQ(second.state(), PlayerState::stopped);
    EXPECT_TRUE(second.finished());
}

TEST(Player, PlaysAgainAfterEnding)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    player.wait();
    ASSERT_EQ(player.state(), PlayerState::ended);

    // Starting a finished player winds it back rather than refusing.
    ASSERT_TRUE(player.start());
    EXPECT_EQ(player.state(), PlayerState::playing);
    EXPECT_LT(player.position().get<wiola::units::Sec>(), 0.5);

    player.wait();
    EXPECT_EQ(player.state(), PlayerState::ended);
}

TEST(Player, RefusesToStartWhilePlaying)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_FALSE(player.start());
    EXPECT_EQ(player.state(), PlayerState::playing);

    ASSERT_TRUE(player.pause());
    EXPECT_FALSE(player.start());
    EXPECT_EQ(player.state(), PlayerState::paused);
}
