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

#include <fixtures/wav.hpp>

#include <audio/output.hpp>

#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <codec/open.hpp>
#include <engine/session.hpp>
#include <fakes/output.hpp>
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

/// Loads and waits for it, the way a window that draws does: asking, then taking the answer up
/// on a later turn.
OpenResult load_and_wait(Session& session, const std::filesystem::path& path)
{
    session.load(path);

    while (session.loading())
        std::this_thread::yield();

    session.poll();

    return session.last_result();
}

/// A session that plays nowhere real, and counts the outputs it was asked to build.
struct Fixture {
    Fixture()
        : session{[this](wiola::audio::Source& source) {
            ++num_outputs;
            return consuming ? std::make_unique<wiola::testing::FakeOutput>(source)
                             : std::make_unique<wiola::testing::FakeOutput>();
        }}
    {
    }

    /// Set before loading, when a test needs a track that cannot run out from under it.
    bool consuming{true};

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

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);

    EXPECT_TRUE(fixture.session.loaded());
    EXPECT_EQ(fixture.num_outputs, 1U);
    EXPECT_NEAR(fixture.session.total_time().get<units::Sec>(), wiola::testing::source_seconds,
        0.01);
}

TEST(Session, RefusesAFileItCannotRead)
{
    Fixture fixture;

    EXPECT_EQ(load_and_wait(fixture.session, std::filesystem::path{"no-such-track.wav"}),
        OpenResult::unreadable);
    EXPECT_FALSE(fixture.session.loaded());
    EXPECT_EQ(fixture.num_outputs, 0U);
}

/// What was playing goes when a file that cannot be read is opened, rather than playing on under
/// a window that says something else is loaded.
/// A file that will not open is no reason to silence the one that is playing: the listener still
/// has the track they had, and is told what went wrong with the other.
/// Reading ahead is for the track after this one: it is read and kept, and what is playing
/// carries on until it is asked for.
TEST(Session, KeepsATrackReadAheadUntilItIsAskedFor)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(load_and_wait(fixture.session, first), OpenResult::opened);
    ASSERT_TRUE(fixture.session.toggle());

    ASSERT_EQ(fixture.session.read_ahead(next), OpenResult::loading);

    while (fixture.session.loading())
        std::this_thread::yield();

    fixture.session.poll();

    EXPECT_TRUE(fixture.session.ready());
    EXPECT_EQ(fixture.session.track(), first) << "reading ahead took over what was playing";
    EXPECT_EQ(fixture.session.state(), State::playing);

    EXPECT_TRUE(fixture.session.install());

    EXPECT_FALSE(fixture.session.ready());
    EXPECT_EQ(fixture.session.track(), next);
    EXPECT_EQ(fixture.session.state(), State::idle) << "a track put on is loaded, not playing";
}

TEST(Session, InstallsNothingWhenNothingWasRead)
{
    Fixture fixture;

    EXPECT_FALSE(fixture.session.ready());
    EXPECT_FALSE(fixture.session.install());
    EXPECT_FALSE(fixture.session.loaded());
}

/// A listener picking a file has said which track they meant, so one read ahead of them is
/// dropped rather than played next.
TEST(Session, DropsATrackReadAheadWhenAnotherIsPicked)
{
    Fixture fixture;

    const std::filesystem::path ahead{wiola::testing::write_wav("wiola_session_ahead.wav")};
    const std::filesystem::path picked{wiola::testing::write_wav("wiola_session.wav")};

    ASSERT_EQ(fixture.session.read_ahead(ahead), OpenResult::loading);

    while (fixture.session.loading())
        std::this_thread::yield();

    fixture.session.poll();
    ASSERT_TRUE(fixture.session.ready());

    ASSERT_EQ(load_and_wait(fixture.session, picked), OpenResult::opened);

    EXPECT_FALSE(fixture.session.ready());
    EXPECT_EQ(fixture.session.track(), picked);
}

TEST(Session, NamesNoTrackBeforeOneIsLoaded)
{
    const Fixture fixture;

    EXPECT_TRUE(fixture.session.track().empty());
}

TEST(Session, NamesTheTrackItLoaded)
{
    Fixture fixture;

    const std::filesystem::path path{wiola::testing::write_wav("wiola_session.wav")};

    ASSERT_EQ(load_and_wait(fixture.session, path), OpenResult::opened);

    EXPECT_EQ(fixture.session.track(), path);
}

/// The name follows what is playing, not what was asked for: a file that would not open leaves
/// both alone.
TEST(Session, KeepsTheNameOfTheTrackAFailedLoadDidNotReplace)
{
    Fixture fixture;

    const std::filesystem::path path{wiola::testing::write_wav("wiola_session.wav")};

    ASSERT_EQ(load_and_wait(fixture.session, path), OpenResult::opened);
    ASSERT_EQ(load_and_wait(fixture.session, std::filesystem::path{"no-such-track.wav"}),
        OpenResult::unreadable);

    EXPECT_EQ(fixture.session.track(), path);
}

TEST(Session, KeepsWhatWasLoadedWhenTheNextFileFails)
{
    Fixture fixture;

    // The track must still be playing when this test asks, so the output plays none of it: one
    // that drains at the speed it is fed runs a track out in the time these lines take.
    fixture.consuming = false;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    ASSERT_TRUE(fixture.session.toggle());
    ASSERT_EQ(fixture.session.state(), State::playing);

    EXPECT_EQ(load_and_wait(fixture.session, std::filesystem::path{"no-such-track.wav"}),
        OpenResult::unreadable);

    EXPECT_TRUE(fixture.session.loaded());
    EXPECT_EQ(fixture.session.state(), State::playing);
}

/// Asking says nothing about the answer: what was playing is still playing until a file has been
/// read all the way through.
TEST(Session, KeepsPlayingWhileTheNextFileIsRead)
{
    Fixture fixture;

    // The track must still be playing when this test asks, so the output plays none of it: one
    // that drains at the speed it is fed runs a track out in the time these lines take.
    fixture.consuming = false;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    ASSERT_TRUE(fixture.session.toggle());

    EXPECT_EQ(fixture.session.load(wiola::testing::write_wav("wiola_other.wav")),
        OpenResult::loading);
    EXPECT_EQ(fixture.session.state(), State::playing);
    EXPECT_TRUE(fixture.session.loaded());

    while (fixture.session.loading())
        std::this_thread::yield();

    fixture.session.poll();

    EXPECT_EQ(fixture.session.last_result(), OpenResult::opened);
    EXPECT_EQ(fixture.session.state(), State::idle) << "the new track is loaded, not playing";
}

/// What a device is pointed at is never a track that has been let go: putting a track on takes
/// the last one away first, so an ordering slip is silence rather than a read of what is gone.
TEST(Session, PlaysNothingWhileOneTrackGivesWayToTheNext)
{
    Fixture fixture;
    fixture.consuming = false;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    ASSERT_TRUE(fixture.session.toggle());

    ASSERT_EQ(fixture.session.read_ahead(wiola::testing::write_wav("wiola_session_next.wav")),
        OpenResult::loading);

    while (fixture.session.loading())
        std::this_thread::yield();

    fixture.session.poll();
    ASSERT_TRUE(fixture.session.ready());

    EXPECT_TRUE(fixture.session.install());
    EXPECT_EQ(fixture.session.state(), State::idle);
}

/// A device is opened for a format, and a track in the same one plays through the device that is
/// already open: a track change that closed and opened a device would be heard.
TEST(Session, KeepsTheDeviceAcrossTracksOfTheSameFormat)
{
    Fixture fixture;
    const std::filesystem::path path{wiola::testing::write_wav("wiola_session.wav")};

    ASSERT_EQ(load_and_wait(fixture.session, path), OpenResult::opened);
    ASSERT_EQ(load_and_wait(fixture.session, path), OpenResult::opened);

    EXPECT_EQ(fixture.num_outputs, 1U);
}

/// A format it cannot play is a device it has to open again: what is open was cut for the rate
/// of the track before.
TEST(Session, OpensAnotherDeviceWhenTheFormatChanges)
{
    Fixture fixture;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    ASSERT_EQ(fixture.num_outputs, 1U);

    ASSERT_EQ(load_and_wait(fixture.session,
                  wiola::testing::write_wav("wiola_session_48k.wav", 48000)),
        OpenResult::opened);

    EXPECT_EQ(fixture.num_outputs, 2U);
}

TEST(Session, PlaysAndPauses)
{
    Fixture fixture;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
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

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
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

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
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

    ASSERT_EQ(load_and_wait(fixture.session, path), OpenResult::opened);

    EXPECT_FLOAT_EQ(fixture.session.volume().gain(), 0.25F);
    EXPECT_FLOAT_EQ(fixture.session.equalizer().band_gain(3), 6.0F);
}

} // namespace
