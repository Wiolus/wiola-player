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
    session.open(path);

    while (session.reading())
        std::this_thread::yield();

    session.catch_up();

    return session.open_result();
}

/// Turns the session over until `settled` says it has caught up, the way a window that draws
/// does. Fails rather than hangs when it never does.
template<typename Predicate>
bool poll_until(Session& session, Predicate settled)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};

    while (std::chrono::steady_clock::now() < deadline) {
        session.catch_up();

        if (settled())
            return true;

        std::this_thread::yield();
    }

    return settled();
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

    EXPECT_FALSE(fixture.session.play_or_pause());

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
    ASSERT_TRUE(fixture.session.play_or_pause());
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
    ASSERT_TRUE(fixture.session.play_or_pause());

    EXPECT_EQ(fixture.session.open(wiola::testing::write_wav("wiola_other.wav")),
        OpenResult::loading);
    EXPECT_EQ(fixture.session.state(), State::playing);
    EXPECT_TRUE(fixture.session.loaded());

    while (fixture.session.reading())
        std::this_thread::yield();

    fixture.session.catch_up();

    EXPECT_EQ(fixture.session.open_result(), OpenResult::opened);
    EXPECT_EQ(fixture.session.state(), State::idle) << "the new track is loaded, not playing";
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

    ASSERT_TRUE(fixture.session.play_or_pause());
    EXPECT_EQ(fixture.session.state(), State::playing);
    EXPECT_TRUE(fixture.session.playing());

    ASSERT_TRUE(fixture.session.play_or_pause());
    EXPECT_EQ(fixture.session.state(), State::paused);

    ASSERT_TRUE(fixture.session.play_or_pause());
    EXPECT_EQ(fixture.session.state(), State::playing);
}

TEST(Session, StopsAtTheBeginning)
{
    Fixture fixture;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);
    ASSERT_TRUE(fixture.session.play_or_pause());

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

/// The point of a list: a track running out is the cue to play the next one.
TEST(Session, PlaysTheNextTrackWhenOneEnds)
{
    Fixture fixture;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));
    ASSERT_TRUE(fixture.session.play_or_pause());

    EXPECT_TRUE(poll_until(fixture.session, [&fixture, &next] {
        return fixture.session.track() == next;
    })) << "the next track was never played";
    EXPECT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.playing(); }));
}

/// The end of the list is the end of playback: it stays where it stopped.
TEST(Session, StaysAtTheEndOfTheList)
{
    Fixture fixture;

    const std::filesystem::path only{wiola::testing::write_wav("wiola_session.wav")};

    ASSERT_EQ(load_and_wait(fixture.session, only), OpenResult::opened);
    ASSERT_TRUE(fixture.session.play_or_pause());

    ASSERT_TRUE(poll_until(fixture.session,
        [&fixture] { return fixture.session.state() == State::ended; }));

    fixture.session.catch_up();

    EXPECT_FALSE(fixture.session.reading()) << "there was nowhere to go, and it went";
    EXPECT_EQ(fixture.session.state(), State::ended);
    EXPECT_EQ(fixture.session.track(), only);
}

/// A listener who pressed stop did not ask for the next track.
TEST(Session, PlaysNothingNextWhenTheListenerStops)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));
    ASSERT_TRUE(fixture.session.play_or_pause());

    fixture.session.stop();
    fixture.session.catch_up();

    // Nothing was even begun: waiting to see whether a track arrives would pass whether or not
    // one was on its way.
    EXPECT_FALSE(fixture.session.reading())
        << "a listener's stop was taken as a cue to read the next track";

    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return !fixture.session.reading(); }));

    EXPECT_EQ(fixture.session.track(), first);
    EXPECT_EQ(fixture.session.state(), State::stopped);
}

TEST(Session, PlaysTheNextTrackAndTheOneBefore)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));

    ASSERT_TRUE(fixture.session.next_track());
    ASSERT_TRUE(poll_until(fixture.session,
        [&fixture, &next] { return fixture.session.track() == next; }));

    ASSERT_TRUE(fixture.session.previous_track());
    EXPECT_TRUE(poll_until(fixture.session,
        [&fixture, &first] { return fixture.session.track() == first; }));
}

/// Adding is for the queue, not for what is on: a track playing keeps playing.
TEST(Session, AddsToTheQueueWithoutDisturbingWhatIsPlaying)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path added{wiola::testing::write_wav("wiola_session_added.wav")};

    ASSERT_EQ(load_and_wait(fixture.session, first), OpenResult::opened);
    ASSERT_TRUE(fixture.session.play_or_pause());

    fixture.session.add({added});

    EXPECT_EQ(fixture.session.playlist().tracks().size(), 2U);
    EXPECT_EQ(fixture.session.track(), first);
    EXPECT_EQ(fixture.session.state(), State::playing);
}

/// Adding to an empty queue is the first thing a listener does, and they mean to play it.
TEST(Session, ReadsTheFirstTrackAddedToAnEmptyQueue)
{
    Fixture fixture;

    const std::filesystem::path added{wiola::testing::write_wav("wiola_session.wav")};

    fixture.session.add({added});

    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));
    EXPECT_EQ(fixture.session.track(), added);
    EXPECT_EQ(fixture.session.state(), State::idle);
}

/// The queue says what comes next, not what is on: taking out the track that is playing leaves
/// it playing.
TEST(Session, KeepsPlayingATrackTakenOutOfTheQueue)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));
    ASSERT_TRUE(fixture.session.play_or_pause());

    EXPECT_TRUE(fixture.session.remove(0));

    EXPECT_EQ(fixture.session.playlist().tracks().size(), 1U);
    EXPECT_EQ(fixture.session.track(), first);
    EXPECT_EQ(fixture.session.state(), State::playing);
    EXPECT_FALSE(fixture.session.remove(1));
}

TEST(Session, EmptiesTheQueueAndEndsPlayback)
{
    Fixture fixture;
    fixture.consuming = false;

    ASSERT_EQ(fixture.session.open({wiola::testing::write_wav("wiola_session.wav"),
                  wiola::testing::write_wav("wiola_session_next.wav")}),
        OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));
    ASSERT_TRUE(fixture.session.play_or_pause());

    fixture.session.clear();

    EXPECT_TRUE(fixture.session.playlist().empty());
    EXPECT_EQ(fixture.session.state(), State::stopped);

    // What was playing is still loaded, so it can be played again without opening it afresh.
    EXPECT_TRUE(fixture.session.loaded());
}

/// What a listener picking a track out of their queue asks for.
TEST(Session, PlaysTheTrackItIsSentTo)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));

    ASSERT_TRUE(fixture.session.play_track(1));
    EXPECT_TRUE(poll_until(fixture.session,
        [&fixture, &next] { return fixture.session.track() == next; }));

    EXPECT_EQ(fixture.session.playlist().position(), 1U);
    EXPECT_FALSE(fixture.session.play_track(2)) << "there is no third track to be sent to";
}

/// The queue is what was given, in the order it was given: shuffling changes what plays next,
/// not the list a listener is looking at.
TEST(Session, ShowsTheQueueInTheOrderItWasGiven)
{
    Fixture fixture;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    fixture.session.shuffle(true);

    ASSERT_EQ(fixture.session.playlist().tracks().size(), 2U);
    EXPECT_EQ(fixture.session.playlist().tracks().front(), first);
    EXPECT_EQ(fixture.session.playlist().tracks().back(), next);
}

TEST(Session, GoesNowhereBeyondTheEndsOfTheList)
{
    Fixture fixture;

    ASSERT_EQ(load_and_wait(fixture.session, wiola::testing::write_wav("wiola_session.wav")),
        OpenResult::opened);

    EXPECT_FALSE(fixture.session.next_track());
    EXPECT_FALSE(fixture.session.previous_track());
}

/// Skipping while a track plays plays the one skipped to; skipping while nothing does, does not.
TEST(Session, KeepsPlayingOrNotWhenTheTrackChanges)
{
    Fixture fixture;
    fixture.consuming = false;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_session.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_session_next.wav")};

    ASSERT_EQ(fixture.session.open({first, next}), OpenResult::loading);
    ASSERT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.loaded(); }));
    ASSERT_TRUE(fixture.session.play_or_pause());

    ASSERT_TRUE(fixture.session.next_track());
    EXPECT_TRUE(poll_until(fixture.session, [&fixture] { return fixture.session.playing(); }));

    fixture.session.stop();
    ASSERT_TRUE(fixture.session.previous_track());
    ASSERT_TRUE(poll_until(fixture.session,
        [&fixture, &first] { return fixture.session.track() == first; }));

    EXPECT_FALSE(fixture.session.playing());
}
