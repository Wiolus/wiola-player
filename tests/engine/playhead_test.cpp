/**
 * @file
 * @brief Unit tests for the playhead.
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

#include <engine/playhead.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

namespace {

using namespace std::chrono_literals;
using wiola::engine::Playhead;

/// How long a threaded test waits before giving up, so a playhead that stops settling fails
/// rather than hangs.
constexpr auto patience = 5s;

constexpr std::size_t num_threaded_seeks{2000};

/// Where the seek numbered `index` asks to go. They ascend, so what is reported may never fall.
constexpr std::size_t target_of(std::size_t index) noexcept
{
    return index * 10;
}

TEST(Playhead, StartsAtTheBeginning)
{
    const Playhead head;

    EXPECT_EQ(head.position(0), 0U);
    EXPECT_FALSE(head.seek_outstanding());
    EXPECT_EQ(head.num_pushed(), 0U);
}

TEST(Playhead, FollowsWhatTheOutputPlayed)
{
    const Playhead head;

    EXPECT_EQ(head.position(1024), 1024U);
}

TEST(Playhead, CountsFromWherePlaybackBegan)
{
    Playhead head;

    head.begin_at(500, head.claim());

    EXPECT_EQ(head.position(100), 600U);
}

TEST(Playhead, HoldsASeekUntilItIsCarriedOut)
{
    Playhead head;

    head.request_seek(4000);

    EXPECT_TRUE(head.seek_outstanding());

    head.begin_at(4000, head.claim());

    EXPECT_FALSE(head.seek_outstanding());
}

/// The place asked for is where playback is from the moment it is asked for, so a bar does not
/// spring back to the old one while the seek is being carried out.
TEST(Playhead, ReportsASeekBeforeItIsCarriedOut)
{
    Playhead head;

    head.begin_at(1000, head.claim());
    head.request_seek(8000);

    EXPECT_EQ(head.position(500), 8000U);

    head.begin_at(8000, head.claim());

    EXPECT_EQ(head.position(0), 8000U);
}

/// A claim answers the seeks asked for when it was taken. One asked for while it is being carried
/// out is a seek of its own, and stays outstanding.
TEST(Playhead, KeepsASeekAskedForWhileOneIsCarriedOut)
{
    Playhead head;

    head.request_seek(2000);

    const Playhead::Claim claim{head.claim()};

    head.request_seek(9000);
    head.begin_at(claim.target, claim);

    EXPECT_TRUE(head.seek_outstanding());
    EXPECT_EQ(head.position(0), 9000U);
}

TEST(Playhead, TakesTheLatestPlaceAsked)
{
    Playhead head;

    head.request_seek(100);
    head.request_seek(700);

    EXPECT_EQ(head.claim().target, 700U);
    EXPECT_EQ(head.position(0), 700U);
}

TEST(Playhead, CountsWhatWasPushed)
{
    Playhead head;

    head.push(64);
    head.push(32);

    EXPECT_EQ(head.num_pushed(), 96U);
}

/// What was pushed for the old place has not been heard and never will be.
TEST(Playhead, ForgetsWhatWasPushedWhenPlaybackBeginsElsewhere)
{
    Playhead head;

    head.push(4096);
    head.begin_at(2000, head.claim());

    EXPECT_EQ(head.num_pushed(), 0U);
}

TEST(Playhead, ClaimsNothingWhenNoSeekWasAsked)
{
    Playhead head;

    head.push(10);

    const Playhead::Claim claim{head.claim()};

    EXPECT_FALSE(claim.outstanding);

    // Beginning where it already is leaves the position alone and the count reset.
    head.begin_at(0, claim);

    EXPECT_EQ(head.position(50), 50U);
    EXPECT_EQ(head.num_pushed(), 0U);
}

/// What the playhead is for: one thread asking for places while another carries them out, with
/// every request answered and the last one left in force.
TEST(Playhead, CarriesEverySeekBetweenTwoThreads)
{
    Playhead head;
    std::atomic<bool> asking{true};

    const std::jthread seeker{[&head, &asking](const std::stop_token& stop) {
        for (std::size_t i = 1; i <= num_threaded_seeks && !stop.stop_requested(); ++i)
            head.request_seek(target_of(i));

        asking.store(false);
    }};

    // This thread is the one that decodes: it carries out whatever it claims.
    const auto deadline = std::chrono::steady_clock::now() + patience;

    while (asking.load() || head.seek_outstanding()) {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "at " << head.position(0);

        const Playhead::Claim claim{head.claim()};

        if (claim.outstanding)
            head.begin_at(claim.target, claim);
        else
            std::this_thread::yield();
    }

    EXPECT_FALSE(head.seek_outstanding());
    EXPECT_EQ(head.position(0), target_of(num_threaded_seeks));
}

/// A place asked for is never taken back: whether a reader catches the seek outstanding or
/// carried out, what it reads is at least what it read before.
TEST(Playhead, NeverReportsAPlaceItHasLeft)
{
    Playhead head;
    std::atomic<bool> asking{true};

    const std::jthread reader{[&head, &asking] {
        std::size_t last{0};

        while (asking.load()) {
            const std::size_t seen{head.position(0)};

            EXPECT_GE(seen, last);
            last = seen;
        }
    }};

    for (std::size_t i = 1; i <= num_threaded_seeks; ++i) {
        head.request_seek(target_of(i));

        const Playhead::Claim claim{head.claim()};

        if (claim.outstanding)
            head.begin_at(claim.target, claim);
    }

    asking.store(false);
}

} // namespace
