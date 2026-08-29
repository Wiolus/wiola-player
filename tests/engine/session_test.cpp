/**
 * @file
 * @brief Unit tests for the session.
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

#include "wav_fixture.hpp"

#include <audio/output.hpp>
#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <codec/open.hpp>
#include <engine/session.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <thread>

namespace {

using namespace std::chrono_literals;
using wiola::audio::Frames;
using wiola::codec::OpenResult;
using State = wiola::engine::Playback::State;
using wiola::engine::Session;
namespace units = wiola::units;

/// An output that opens anywhere, and drains what it is given on a thread of its own.
class TestOutput final : public wiola::audio::Output {
public:
    explicit TestOutput(wiola::audio::Source& source) noexcept
        : source_{source}
    {
    }

    ~TestOutput() override { stop(); }

    bool start() noexcept override
    {
        if (running_.exchange(true))
            return true;

        thread_ = std::thread{[this] { pull(); }};

        return true;
    }

    void stop() noexcept override
    {
        running_.store(false);

        if (thread_.joinable())
            thread_.join();
    }

    [[nodiscard]] bool running() const noexcept override { return running_.load(); }

    [[nodiscard]] Frames frames_played() const noexcept override { return Frames{frames_.load()}; }

private:
    void pull()
    {
        const wiola::audio::StreamSpec spec{source_.spec()};
        std::array<float, 512> block{};

        while (running_.load()) {
            const std::size_t num_rendered{source_.render(block)};

            if (num_rendered == 0) {
                std::this_thread::sleep_for(1ms);
                continue;
            }

            frames_.fetch_add(spec.frames_per(num_rendered).count());
        }
    }

    wiola::audio::Source& source_;
    std::atomic<bool> running_{false};
    std::atomic<std::size_t> frames_{0};
    std::thread thread_;
};

/// A session that plays nowhere real, and counts the outputs it was asked to build.
struct Fixture {
    Fixture()
        : session{[this](wiola::audio::Source& source) {
            ++num_outputs;
            return std::make_unique<TestOutput>(source);
        }}
    {
    }

    std::size_t num_outputs{0};
    Session session;
};

TEST(Session, StartsWithNothingLoaded)
{
    const Fixture fixture;

    EXPECT_FALSE(fixture.session.loaded());
    EXPECT_EQ(fixture.session.state(), State::idle);
    EXPECT_FALSE(fixture.session.playing());
    EXPECT_EQ(fixture.session.total_time(), units::Time{});
    EXPECT_EQ(fixture.session.time_played(), units::Time{});
}

/// Nothing loaded is not a reason to refuse to be asked.
TEST(Session, AnswersWhileNothingIsLoaded)
{
    Fixture fixture;

    EXPECT_FALSE(fixture.session.toggle());

    fixture.session.stop();
    fixture.session.seek(units::Time{1.0});

    EXPECT_FALSE(fixture.session.loaded());
    EXPECT_EQ(fixture.num_outputs, 0U);
}

TEST(Session, LoadsATrack)
{
    Fixture fixture;

    ASSERT_EQ(fixture.session.load(wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);

    EXPECT_TRUE(fixture.session.loaded());
    EXPECT_EQ(fixture.num_outputs, 1U);
    EXPECT_NEAR(fixture.session.total_time().get<units::Sec>(), wiola::testing::source_seconds,
        0.01);
}

TEST(Session, RefusesAFileItCannotRead)
{
    Fixture fixture;

    EXPECT_EQ(fixture.session.load(std::filesystem::path{"no-such-track.wav"}),
        OpenResult::unreadable);
    EXPECT_FALSE(fixture.session.loaded());
    EXPECT_EQ(fixture.num_outputs, 0U);
}

/// What was playing goes when a file that cannot be read is opened, rather than playing on under
/// a window that says something else is loaded.
TEST(Session, DropsWhatWasLoadedWhenTheNextFileFails)
{
    Fixture fixture;

    ASSERT_EQ(fixture.session.load(wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    EXPECT_EQ(fixture.session.load(std::filesystem::path{"no-such-track.wav"}),
        OpenResult::unreadable);

    EXPECT_FALSE(fixture.session.loaded());
    EXPECT_EQ(fixture.session.state(), State::idle);
}

/// Everything a track is played through is cut for its format, so opening another one builds it
/// again.
TEST(Session, BuildsAnOutputForEveryTrack)
{
    Fixture fixture;
    const std::filesystem::path path{wiola::testing::write_wav("wiola_session.wav")};

    ASSERT_EQ(fixture.session.load(path), OpenResult::opened);
    ASSERT_EQ(fixture.session.load(path), OpenResult::opened);

    EXPECT_EQ(fixture.num_outputs, 2U);
}

TEST(Session, PlaysAndPauses)
{
    Fixture fixture;

    ASSERT_EQ(fixture.session.load(wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);

    ASSERT_TRUE(fixture.session.toggle());
    EXPECT_EQ(fixture.session.state(), State::playing);
    EXPECT_TRUE(fixture.session.playing());

    ASSERT_TRUE(fixture.session.toggle());
    EXPECT_EQ(fixture.session.state(), State::paused);

    ASSERT_TRUE(fixture.session.toggle());
    EXPECT_EQ(fixture.session.state(), State::playing);
}

TEST(Session, StopsAtTheBeginning)
{
    Fixture fixture;

    ASSERT_EQ(fixture.session.load(wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    ASSERT_TRUE(fixture.session.toggle());

    fixture.session.seek(units::Time{1.0});
    fixture.session.stop();

    EXPECT_EQ(fixture.session.state(), State::stopped);
    EXPECT_EQ(fixture.session.time_played(), units::Time{});
}

TEST(Session, SeeksWhereItIsAsked)
{
    Fixture fixture;

    ASSERT_EQ(fixture.session.load(wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);

    fixture.session.seek(units::Time{1.0});

    EXPECT_NEAR(fixture.session.time_played().get<units::Sec>(), 1.0, 0.01);
}

/// A setting belongs to the listener, not to what is playing.
TEST(Session, KeepsItsSettingsAcrossTracks)
{
    Fixture fixture;
    const std::filesystem::path path{wiola::testing::write_wav("wiola_session.wav")};

    fixture.session.volume().set_gain(0.25F);
    fixture.session.equalizer().set_band_gain(3, 6.0F);

    ASSERT_EQ(fixture.session.load(path), OpenResult::opened);

    EXPECT_FLOAT_EQ(fixture.session.volume().gain(), 0.25F);
    EXPECT_FLOAT_EQ(fixture.session.equalizer().band_gain(3), 6.0F);
}

} // namespace
