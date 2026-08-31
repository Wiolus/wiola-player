/**
 * @file
 * @brief Unit tests for the end of an output that starts and stops it.
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

#include <audio/output.hpp>

#include <fakes/output.hpp>

#include <gtest/gtest.h>

#include <utility>

namespace {

using wiola::audio::Output;
using wiola::testing::FakeOutput;

} // namespace

TEST(OutputControl, StartsAndStopsWhatItWasTakenFrom)
{
    FakeOutput output;
    Output::Control control{output.control()};

    EXPECT_TRUE(control.start());
    EXPECT_TRUE(output.running());
    EXPECT_EQ(output.num_starts(), 1);

    control.stop();

    EXPECT_FALSE(output.running());
    EXPECT_EQ(output.num_stops(), 1);
}

TEST(OutputControl, SaysWhenTheOutputWouldNotStart)
{
    FakeOutput output;
    output.refuse_starts();
    Output::Control control{output.control()};

    EXPECT_FALSE(control.start());
    EXPECT_FALSE(output.running());
    EXPECT_EQ(output.num_starts(), 1);
}

/// Two threads driving one device leaves it in a state neither asked for, so there is one
/// control: a second is inert, which shows as a device that will not start.
TEST(OutputControl, HandsOutOneControlOnly)
{
    FakeOutput output;
    Output::Control first{output.control()};
    Output::Control second{output.control()};

    EXPECT_FALSE(second.start());
    EXPECT_EQ(output.num_starts(), 0) << "a second control reached the output";

    second.stop();
    EXPECT_EQ(output.num_stops(), 0);

    // The one that was taken first still drives it.
    EXPECT_TRUE(first.start());
    EXPECT_EQ(output.num_starts(), 1);
}

/// Moving is how the device passes to another thread, so what is left behind must not still
/// reach it.
TEST(OutputControl, LeavesNothingBehindWhenItIsMoved)
{
    FakeOutput output;
    Output::Control control{output.control()};
    Output::Control moved{std::move(control)};

    EXPECT_FALSE(control.start()); // NOLINT(bugprone-use-after-move) - the point of the test
    EXPECT_EQ(output.num_starts(), 0) << "the control that was moved from reached the output";

    EXPECT_TRUE(moved.start());
    EXPECT_EQ(output.num_starts(), 1);
}

TEST(OutputControl, LeavesNothingBehindWhenItIsMoveAssigned)
{
    FakeOutput output;
    FakeOutput other;

    Output::Control control{output.control()};
    Output::Control target{other.control()};

    target = std::move(control);

    EXPECT_FALSE(control.start()); // NOLINT(bugprone-use-after-move) - the point of the test
    EXPECT_EQ(output.num_starts(), 0);

    EXPECT_TRUE(target.start());
    EXPECT_EQ(output.num_starts(), 1);
    EXPECT_EQ(other.num_starts(), 0);
}

/// A device outlives the tracks played through it, so what drove it for one track gives it back
/// for the next.
TEST(OutputControl, HandsOutAnotherOnceTheHeldOneIsGone)
{
    FakeOutput output;

    {
        Output::Control first{output.control()};

        ASSERT_TRUE(first.start());
        ASSERT_EQ(output.num_starts(), 1);
    }

    Output::Control next{output.control()};

    EXPECT_TRUE(next.start()) << "the device was not given back";
    EXPECT_EQ(output.num_starts(), 2);
}

/// Moving one about does not give the device back: it is still held, just elsewhere.
TEST(OutputControl, KeepsTheDeviceWhileTheControlIsMovedAbout)
{
    FakeOutput output;

    Output::Control control{output.control()};
    Output::Control moved{std::move(control)};

    Output::Control second{output.control()};

    EXPECT_FALSE(second.start()) << "moving a control gave the device away";
    EXPECT_EQ(output.num_starts(), 0);

    EXPECT_TRUE(moved.start());
}

/// The one that was moved from holds nothing, so its end is not the device coming free.
TEST(OutputControl, GivesTheDeviceBackWhenTheOneHoldingItEnds)
{
    FakeOutput output;
    Output::Control moved_from{output.control()};

    {
        const Output::Control holder{std::move(moved_from)};
    }

    Output::Control next{output.control()};

    EXPECT_TRUE(next.start());
    EXPECT_EQ(output.num_starts(), 1);
}

/// Being given another device to drive gives the first one back.
TEST(OutputControl, GivesBackWhatItHeldWhenItIsGivenAnother)
{
    FakeOutput first;
    FakeOutput second;

    Output::Control control{first.control()};
    control = second.control();

    Output::Control back{first.control()};

    EXPECT_TRUE(back.start()) << "the first device was never given back";
    EXPECT_EQ(first.num_starts(), 1);
}
