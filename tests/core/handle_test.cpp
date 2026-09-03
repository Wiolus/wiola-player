/**
 * @file
 * @brief Tests for a capability handed out once, and the end that holds it.
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

#include <core/handle.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace {

using wiola::core::Claimable;
using wiola::core::Handle;

/// Something that counts, with the counting handed out rather than reachable from the thing
/// itself. The shape every handle in this project has.
class Counter {
public:
    /// The end that counts. There is one: every one taken after the first counts nothing.
    class Tally final : public Handle<Counter> {
    public:
        void add() noexcept
        {
            if (owner())
                owner()->total_ += 1;
        }

        [[nodiscard]] int total() const noexcept { return owner() ? owner()->total_ : -1; }

    private:
        friend class Counter;

        using Handle::Handle;
    };

    [[nodiscard]] Tally tally() noexcept { return tally_taken_.claim() ? Tally{*this} : Tally{}; }

    [[nodiscard]] int total() const noexcept { return total_; }

private:
    Claimable tally_taken_;
    int total_{0};
};

/// One that gives its end back when it is let go, the way a device does.
class Returning {
public:
    class Ticket final : public Handle<Returning> {
    public:
        Ticket(Ticket&&) noexcept = default;

        Ticket& operator=(Ticket&& other) noexcept
        {
            if (this != &other) {
                release();
                Handle::operator=(std::move(other));
            }

            return *this;
        }

        ~Ticket() { release(); }

    private:
        friend class Returning;

        using Handle::Handle;

        void release() noexcept
        {
            if (owner())
                owner()->taken_.release();

            clear();
        }
    };

    [[nodiscard]] Ticket ticket() noexcept { return taken_.claim() ? Ticket{*this} : Ticket{}; }

    [[nodiscard]] bool claimed() const noexcept { return taken_.claimed(); }

private:
    Claimable taken_;
};

} // namespace

TEST(Claimable, StartsUnclaimed)
{
    const Claimable claimable;

    EXPECT_FALSE(claimable.claimed());
}

TEST(Claimable, GivesItToTheFirstCallerOnly)
{
    Claimable claimable;

    EXPECT_TRUE(claimable.claim());
    EXPECT_TRUE(claimable.claimed());

    EXPECT_FALSE(claimable.claim());
    EXPECT_FALSE(claimable.claim());
}

TEST(Claimable, CanBeClaimedAgainOnceReleased)
{
    Claimable claimable;

    ASSERT_TRUE(claimable.claim());

    claimable.release();

    EXPECT_FALSE(claimable.claimed());
    EXPECT_TRUE(claimable.claim());
}

/// Copying one would be two threads holding what belongs to one.
TEST(Handle, CannotBeCopied)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Counter::Tally>);
    EXPECT_FALSE(std::is_copy_assignable_v<Counter::Tally>);
}

/// Moving is how the end is handed to another thread.
TEST(Handle, CanBeMoved)
{
    EXPECT_TRUE(std::is_move_constructible_v<Counter::Tally>);
    EXPECT_TRUE(std::is_move_assignable_v<Counter::Tally>);
}

TEST(Handle, ReachesWhatItWasHandedOutFor)
{
    Counter counter;
    Counter::Tally tally{counter.tally()};

    EXPECT_TRUE(static_cast<bool>(tally));

    tally.add();
    tally.add();

    EXPECT_EQ(counter.total(), 2);
    EXPECT_EQ(tally.total(), 2);
}

/// A second one is inert rather than an error: it reaches nothing and does nothing.
TEST(Handle, GivesNothingToASecondTaker)
{
    Counter counter;
    Counter::Tally first{counter.tally()};
    Counter::Tally second{counter.tally()};

    EXPECT_TRUE(static_cast<bool>(first));
    EXPECT_FALSE(static_cast<bool>(second));

    second.add();

    EXPECT_EQ(counter.total(), 0) << "a second handle reached the counter";
    EXPECT_EQ(second.total(), -1);

    first.add();

    EXPECT_EQ(counter.total(), 1);
}

/// What is moved from must not still reach what it handed over.
TEST(Handle, LeavesNothingBehindWhenMovedFrom)
{
    Counter counter;
    Counter::Tally first{counter.tally()};
    Counter::Tally second{std::move(first)};

    EXPECT_TRUE(static_cast<bool>(second));

    // NOLINTNEXTLINE(bugprone-use-after-move) - that it reaches nothing is the point.
    EXPECT_FALSE(static_cast<bool>(first));

    second.add();

    EXPECT_EQ(counter.total(), 1);
}

TEST(Handle, MoveAssignmentHandsTheEndOver)
{
    Counter counter;
    Counter::Tally first{counter.tally()};
    Counter::Tally empty;

    empty = std::move(first);

    EXPECT_TRUE(static_cast<bool>(empty));

    empty.add();

    EXPECT_EQ(counter.total(), 1);
}

TEST(Handle, LettingOneGoGivesTheEndBack)
{
    Returning returning;

    {
        const Returning::Ticket ticket{returning.ticket()};

        EXPECT_TRUE(returning.claimed());
        EXPECT_TRUE(static_cast<bool>(ticket));
    }

    EXPECT_FALSE(returning.claimed()) << "the end was not given back";

    const Returning::Ticket again{returning.ticket()};

    EXPECT_TRUE(static_cast<bool>(again)) << "the next taker was refused";
}

/// Being given another end to hold gives the first one back.
TEST(Handle, ReturnsWhatItHeldWhenGivenAnother)
{
    Returning first;
    Returning second;

    Returning::Ticket ticket{first.ticket()};

    ASSERT_TRUE(first.claimed());

    ticket = second.ticket();

    EXPECT_FALSE(first.claimed()) << "the first end was not given back";
    EXPECT_TRUE(second.claimed());
}

/// Moving from one that returns its end must not return it twice.
TEST(Handle, ReturnsTheEndOnceWhenMovedFrom)
{
    Returning returning;

    {
        Returning::Ticket first{returning.ticket()};
        const Returning::Ticket second{std::move(first)};

        EXPECT_TRUE(returning.claimed()) << "moving gave the end back too early";
    }

    EXPECT_FALSE(returning.claimed());
    EXPECT_TRUE(static_cast<bool>(returning.ticket()));
}
