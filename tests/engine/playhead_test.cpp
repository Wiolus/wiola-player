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

#include <audio/stream_spec.hpp>
#include <engine/transport/playhead.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <utility>

namespace {

using namespace std::chrono_literals;
using wiola::audio::Frames;
using wiola::engine::Playhead;

/// How long a threaded test waits before giving up, so a playhead that stops settling fails
/// rather than hangs.
constexpr auto patience = 5s;

constexpr std::size_t num_threaded_seeks{100000};

/// Where the seek numbered `index` asks to go. They ascend, so what is reported may never fall.
constexpr Frames target_of(std::size_t index) noexcept
{
    return Frames{index * 10};
}

TEST(Playhead, StartsAtTheBeginning)
{
    Playhead head;
    auto applier{head.applier()};

    EXPECT_EQ(head.position(Frames{0}), Frames{0});
    EXPECT_FALSE(head.seek_outstanding());
    EXPECT_EQ(applier.num_pushed(), 0U);
}

TEST(Playhead, FollowsWhatTheOutputPlayed)
{
    const Playhead head;

    EXPECT_EQ(head.position(Frames{1024}), Frames{1024});
}

TEST(Playhead, CountsFromWherePlaybackBegan)
{
    Playhead head;
    auto applier{head.applier()};

    applier.begin_at(Frames{500}, Frames{}, applier.claim());

    EXPECT_EQ(head.position(Frames{100}), Frames{600});
}

TEST(Playhead, HoldsASeekUntilItIsCarriedOut)
{
    Playhead head;
    auto applier{head.applier()};

    head.request_seek(Frames{4000});

    EXPECT_TRUE(head.seek_outstanding());

    applier.begin_at(Frames{4000}, Frames{}, applier.claim());

    EXPECT_FALSE(head.seek_outstanding());
}

/// The place asked for is where playback is from the moment it is asked for, so a bar does not
/// spring back to the old one while the seek is being carried out.
TEST(Playhead, ReportsASeekBeforeItIsCarriedOut)
{
    Playhead head;
    auto applier{head.applier()};

    applier.begin_at(Frames{1000}, Frames{}, applier.claim());
    head.request_seek(Frames{8000});

    EXPECT_EQ(head.position(Frames{500}), Frames{8000});

    applier.begin_at(Frames{8000}, Frames{}, applier.claim());

    EXPECT_EQ(head.position(Frames{0}), Frames{8000});
}

/// A claim answers the seeks asked for when it was taken. One asked for while it is being carried
/// out is a seek of its own, and stays outstanding.
TEST(Playhead, KeepsASeekAskedForWhileOneIsCarriedOut)
{
    Playhead head;
    auto applier{head.applier()};

    head.request_seek(Frames{2000});

    const Playhead::Claim claim{applier.claim()};

    head.request_seek(Frames{9000});
    applier.begin_at(claim.target, Frames{}, claim);

    EXPECT_TRUE(head.seek_outstanding());
    EXPECT_EQ(head.position(Frames{0}), Frames{9000});
}

TEST(Playhead, TakesTheLatestPlaceAsked)
{
    Playhead head;
    auto applier{head.applier()};

    head.request_seek(Frames{100});
    head.request_seek(Frames{700});

    EXPECT_EQ(applier.claim().target, Frames{700});
    EXPECT_EQ(head.position(Frames{0}), Frames{700});
}

TEST(Playhead, CountsWhatWasPushed)
{
    Playhead head;
    auto applier{head.applier()};

    applier.push(64);
    applier.push(32);

    EXPECT_EQ(applier.num_pushed(), 96U);
}

/// What was pushed for the old place has not been heard and never will be.
TEST(Playhead, ForgetsWhatWasPushedWhenPlaybackBeginsElsewhere)
{
    Playhead head;
    auto applier{head.applier()};

    applier.push(4096);
    applier.begin_at(Frames{2000}, Frames{}, applier.claim());

    EXPECT_EQ(applier.num_pushed(), 0U);
}

TEST(Playhead, ClaimsNothingWhenNoSeekWasAsked)
{
    Playhead head;
    auto applier{head.applier()};

    applier.push(10);

    const Playhead::Claim claim{applier.claim()};

    EXPECT_FALSE(claim.outstanding);

    // Beginning where it already is leaves the position alone and the count reset.
    applier.begin_at(Frames{0}, Frames{}, claim);

    EXPECT_EQ(head.position(Frames{50}), Frames{50});
    EXPECT_EQ(applier.num_pushed(), 0U);
}

/// The output counts on and is never wound back, so beginning somewhere means measuring from
/// what it had reached by then.
TEST(Playhead, CountsFromWhatTheOutputHadAlreadyPlayed)
{
    Playhead head;
    auto applier{head.applier()};

    applier.begin_at(Frames{1000}, Frames{700}, applier.claim());

    EXPECT_EQ(head.position(Frames{700}), Frames{1000});
    EXPECT_EQ(head.position(Frames{750}), Frames{1050});
}

TEST(Playhead, MeasuresFromTheLatestPlaceItBeganAt)
{
    Playhead head;
    auto applier{head.applier()};

    applier.begin_at(Frames{1000}, Frames{700}, applier.claim());
    ASSERT_EQ(head.position(Frames{800}), Frames{1100});

    applier.begin_at(Frames{5000}, Frames{800}, applier.claim());

    EXPECT_EQ(head.position(Frames{800}), Frames{5000});
    EXPECT_EQ(head.position(Frames{900}), Frames{5100});
}

/// Two threads counting pushes race on a number that is not atomic, so there is one applier: a
/// second is inert, which shows as a seek nobody carries out.
TEST(Playhead, HandsOutOneApplierOnly)
{
    Playhead head;
    auto first{head.applier()};
    auto second{head.applier()};

    head.request_seek(Frames{500});

    EXPECT_FALSE(second.claim().outstanding) << "a second applier reached the playhead";

    second.push(64);
    EXPECT_EQ(second.num_pushed(), 0U);
    EXPECT_EQ(first.num_pushed(), 0U);

    // The one taken first does the work.
    const Playhead::Claim claim{first.claim()};
    ASSERT_TRUE(claim.outstanding);

    first.begin_at(claim.target, Frames{}, claim);
    EXPECT_FALSE(head.seek_outstanding());
}

/// Moving is how the work passes to another thread, so what is left behind must not still do it.
TEST(Playhead, LeavesNothingBehindWhenTheApplierIsMoved)
{
    Playhead head;
    auto applier{head.applier()};
    auto moved{std::move(applier)};

    head.request_seek(Frames{500});

    // NOLINTNEXTLINE(bugprone-use-after-move) - the point of the test
    EXPECT_FALSE(applier.claim().outstanding) << "the applier that was moved from still claims";

    applier.push(64); // NOLINT(bugprone-use-after-move) - as above
    EXPECT_EQ(applier.num_pushed(), 0U);

    const Playhead::Claim claim{moved.claim()};
    ASSERT_TRUE(claim.outstanding);

    moved.begin_at(claim.target, Frames{}, claim);
    moved.push(64);

    EXPECT_EQ(moved.num_pushed(), 64U);
    EXPECT_EQ(head.position(Frames{}), Frames{500});
}

/// What the playhead is for: one thread asking for places while another carries them out, with
/// every request answered and the last one left in force.
TEST(Playhead, CarriesEverySeekBetweenTwoThreads)
{
    Playhead head;
    auto applier{head.applier()};
    std::atomic<bool> asking{true};

    const std::jthread seeker{[&head, &asking](const std::stop_token& stop) {
        for (std::size_t i = 1; i <= num_threaded_seeks && !stop.stop_requested(); ++i)
            head.request_seek(target_of(i));

        asking.store(false);
    }};

    // This thread is the one that decodes: it carries out whatever it claims.
    const auto deadline = std::chrono::steady_clock::now() + patience;

    while (asking.load() || head.seek_outstanding()) {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline)
            << "at " << head.position(Frames{}).count();

        const Playhead::Claim claim{applier.claim()};

        if (claim.outstanding)
            applier.begin_at(claim.target, Frames{}, claim);
        else
            std::this_thread::yield();
    }

    EXPECT_FALSE(head.seek_outstanding());
    EXPECT_EQ(head.position(Frames{0}), target_of(num_threaded_seeks));
}

/// A place asked for is never taken back: whether a reader catches the seek outstanding or
/// carried out, what it reads is at least what it read before.
TEST(Playhead, NeverReportsAPlaceItHasLeft)
{
    Playhead head;
    auto applier{head.applier()};
    std::atomic<bool> asking{true};

    const std::jthread reader{[&head, &asking] {
        Frames last{};

        while (asking.load()) {
            const Frames seen{head.position(Frames{})};

            EXPECT_GE(seen, last);
            last = seen;
        }
    }};

    for (std::size_t i = 1; i <= num_threaded_seeks; ++i) {
        head.request_seek(target_of(i));

        const Playhead::Claim claim{applier.claim()};

        if (claim.outstanding)
            applier.begin_at(claim.target, Frames{}, claim);
    }

    asking.store(false);
}

} // namespace
