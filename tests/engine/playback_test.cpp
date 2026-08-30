/**
 * @file
 * @brief Unit tests for the playback state machine.
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

#include <engine/playback.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <stop_token>
#include <thread>

namespace {

using State = wiola::engine::Playback::State;
using wiola::engine::Playback;

} // namespace

TEST(Playback, StartsIdle)
{
    const Playback playback;

    EXPECT_EQ(playback.state(), State::idle);
    EXPECT_FALSE(playback.playing());
    EXPECT_FALSE(playback.finished());
}

TEST(Playback, BeginsPlaying)
{
    Playback playback;

    EXPECT_TRUE(playback.begin());
    EXPECT_EQ(playback.state(), State::playing);
    EXPECT_TRUE(playback.playing());
}

TEST(Playback, RefusesToBeginWhatIsAlreadyUnderWay)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());
    EXPECT_FALSE(playback.begin());

    ASSERT_TRUE(playback.pause());
    EXPECT_FALSE(playback.begin());
    EXPECT_EQ(playback.state(), State::paused);
}

/// Playing a track again is beginning again: a final state is where `begin` is expected from.
TEST(Playback, BeginsAgainAfterPlaybackHasFinished)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());
    playback.finish(State::ended);

    EXPECT_TRUE(playback.begin());
    EXPECT_EQ(playback.state(), State::playing);
}

TEST(Playback, PausesOnlyWhatIsPlaying)
{
    Playback playback;

    EXPECT_FALSE(playback.pause());

    ASSERT_TRUE(playback.begin());

    EXPECT_TRUE(playback.pause());
    EXPECT_EQ(playback.state(), State::paused);

    // There is nothing left to silence, so the second one has nothing to report.
    EXPECT_FALSE(playback.pause());
    EXPECT_EQ(playback.state(), State::paused);
}

TEST(Playback, ResumesOnlyWhatIsPaused)
{
    Playback playback;

    EXPECT_FALSE(playback.resume());

    ASSERT_TRUE(playback.begin());
    EXPECT_FALSE(playback.resume());

    ASSERT_TRUE(playback.pause());
    EXPECT_TRUE(playback.resume());
    EXPECT_EQ(playback.state(), State::playing);
}

TEST(Playback, FinishesFromPaused)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());
    ASSERT_TRUE(playback.pause());

    playback.finish(State::ended);

    EXPECT_EQ(playback.state(), State::ended);
    EXPECT_TRUE(playback.finished());
}

/// A listener who asked to stop outranks a source that ran out a moment later.
TEST(Playback, KeepsTheFinalStateItReachedFirst)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());

    playback.finish(State::stopped);
    playback.finish(State::ended);

    EXPECT_EQ(playback.state(), State::stopped);
}

/// The one a plain store would get wrong: playback that has ended is not paused by a listener
/// who was a moment too late, and is not resumed back to life afterwards either.
TEST(Playback, RefusesToPauseOrResumeWhatHasFinished)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());
    playback.finish(State::ended);

    EXPECT_FALSE(playback.pause());
    EXPECT_EQ(playback.state(), State::ended);

    EXPECT_FALSE(playback.resume());
    EXPECT_EQ(playback.state(), State::ended);
}

/// A device that went away is as final as a track that ran out, and outranks it if it got there
/// first: what a listener needs to be told is that the device is gone.
TEST(Playback, KeepsAFaultAsFinal)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());

    playback.finish(State::faulted);

    EXPECT_EQ(playback.state(), State::faulted);
    EXPECT_TRUE(playback.finished());
    EXPECT_FALSE(playback.playing());

    playback.finish(State::ended);
    EXPECT_EQ(playback.state(), State::faulted);

    EXPECT_FALSE(playback.pause());
    EXPECT_FALSE(playback.resume());
    EXPECT_EQ(playback.state(), State::faulted);
}

/// A fault is not the end of the road: the listener may try the track again once the device is
/// back.
TEST(Playback, BeginsAgainAfterAFault)
{
    Playback playback;

    ASSERT_TRUE(playback.begin());
    playback.finish(State::faulted);

    EXPECT_TRUE(playback.begin());
    EXPECT_EQ(playback.state(), State::playing);
}

/// What this is for: a listener asking from one thread while playback ends on another.
/// Once it is over it stays over, however many asks land on the moment it ended.
TEST(Playback, NeverLeavesAFinalStateBehind)
{
    constexpr int num_rounds{200};
    constexpr int num_checks{100};

    for (int round = 0; round < num_rounds; ++round) {
        Playback playback;

        ASSERT_TRUE(playback.begin());

        const std::jthread listener{[&playback](const std::stop_token& stop) {
            while (!stop.stop_requested()) {
                static_cast<void>(playback.pause());
                static_cast<void>(playback.resume());
            }
        }};

        // A different point in the listener's loop each round, so the end of the track lands
        // between its steps rather than always in the same place.
        for (int spin = 0; spin < round; ++spin)
            std::this_thread::yield();

        playback.finish(State::ended);

        // The listener is still asking; none of it may take.
        for (int check = 0; check < num_checks; ++check) {
            ASSERT_EQ(playback.state(), State::ended) << "at round " << round;
            std::this_thread::yield();
        }
    }
}

/// The other pair that lands together: a listener pressing stop as the track runs out. Both
/// finals are asked for at once, from two threads, and whichever wins is the one that stays.
TEST(Playback, SettlesOnOneFinalStateWhenBothAreAskedAtOnce)
{
    constexpr int num_rounds{500};
    constexpr int num_checks{50};

    for (int round = 0; round < num_rounds; ++round) {
        Playback playback;
        std::atomic<bool> go{false};

        ASSERT_TRUE(playback.begin());

        // Both threads wait on the same flag, so the two asks land within nanoseconds of each
        // other rather than whenever the second thread happens to start.
        const std::jthread decoder{[&playback, &go] {
            while (!go.load(std::memory_order_acquire)) { }

            playback.finish(State::ended);
        }};

        go.store(true, std::memory_order_release);

        for (int spin = 0; spin < round % 4; ++spin)
            std::this_thread::yield();

        playback.finish(State::stopped);

        const State settled{playback.state()};
        ASSERT_TRUE(settled == State::ended || settled == State::stopped) << "at round " << round;

        // The other thread may still be inside its own finish; it may not take.
        for (int check = 0; check < num_checks; ++check) {
            ASSERT_EQ(playback.state(), settled) << "at round " << round;
            std::this_thread::yield();
        }
    }
}
