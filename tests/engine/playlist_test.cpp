/**
 * @file
 * @brief Unit tests for what is to be played and where in it playback has reached.
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

#include <engine/playlist.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <set>
#include <vector>

namespace {

using wiola::engine::Playlist;
using Repeat = wiola::engine::Playlist::Repeat;

/// Three tracks, named so that a test can say which one it is standing at.
std::vector<std::filesystem::path> three()
{
    return {std::filesystem::path{"first.wav"}, std::filesystem::path{"second.wav"},
        std::filesystem::path{"third.wav"}};
}

} // namespace

TEST(Playlist, StandsNowhereWhileItIsEmpty)
{
    const Playlist playlist;

    EXPECT_TRUE(playlist.empty());
    EXPECT_EQ(playlist.size(), 0U);
    EXPECT_TRUE(playlist.current().empty());
    EXPECT_FALSE(playlist.position().has_value());
}

TEST(Playlist, GoesNowhereWhileItIsEmpty)
{
    Playlist playlist;

    EXPECT_FALSE(playlist.next());
    EXPECT_FALSE(playlist.previous());
    EXPECT_FALSE(playlist.go_to(0));
}

TEST(Playlist, StandsAtTheFirstOfWhatItIsGiven)
{
    Playlist playlist;

    playlist.set(three());

    EXPECT_EQ(playlist.size(), 3U);
    EXPECT_EQ(playlist.current(), std::filesystem::path{"first.wav"});
    EXPECT_EQ(playlist.position(), 0U);
}

TEST(Playlist, StandsAtTheFirstTrackAdded)
{
    Playlist playlist;

    playlist.add(std::filesystem::path{"only.wav"});
    playlist.add(std::filesystem::path{"after.wav"});

    EXPECT_EQ(playlist.size(), 2U);
    EXPECT_EQ(playlist.current(), std::filesystem::path{"only.wav"});
}

TEST(Playlist, WalksForwardsAndBack)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.next());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"second.wav"});

    ASSERT_TRUE(playlist.next());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"third.wav"});

    ASSERT_TRUE(playlist.previous());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"second.wav"});
}

/// The end of the list is the end of playback, and it stays where it stopped.
TEST(Playlist, GoesNoFurtherThanTheEnd)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(2));

    EXPECT_FALSE(playlist.next());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"third.wav"});

    ASSERT_TRUE(playlist.go_to(0));

    EXPECT_FALSE(playlist.previous());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"first.wav"});
}

TEST(Playlist, ComesRoundWhenEverythingRepeats)
{
    Playlist playlist;
    playlist.set(three());
    playlist.set_repeat(Repeat::all);

    ASSERT_TRUE(playlist.go_to(2));

    EXPECT_TRUE(playlist.next());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"first.wav"});

    EXPECT_TRUE(playlist.previous());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"third.wav"});
}

TEST(Playlist, StaysWhereItIsWhenOneTrackRepeats)
{
    Playlist playlist;
    playlist.set(three());
    playlist.set_repeat(Repeat::track);

    ASSERT_TRUE(playlist.go_to(1));

    EXPECT_TRUE(playlist.next());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"second.wav"});

    EXPECT_TRUE(playlist.previous());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"second.wav"});
}

TEST(Playlist, StandsWhereItIsSent)
{
    Playlist playlist;
    playlist.set(three());

    EXPECT_TRUE(playlist.go_to(2));
    EXPECT_EQ(playlist.current(), std::filesystem::path{"third.wav"});
    EXPECT_EQ(playlist.position(), 2U);

    EXPECT_FALSE(playlist.go_to(3));
    EXPECT_EQ(playlist.current(), std::filesystem::path{"third.wav"});
}

/// Shuffling changes what comes next, not what is playing now.
TEST(Playlist, KeepsTheTrackItStandsAtWhenShuffled)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(1));

    playlist.shuffle(1234);

    EXPECT_TRUE(playlist.shuffled());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"second.wav"});
    EXPECT_EQ(playlist.position(), 1U);
}

/// An order, not a fresh choice each time: every track comes once before any comes twice.
TEST(Playlist, ShufflingIsAnOrderOverEveryTrack)
{
    Playlist playlist;
    playlist.set(three());
    playlist.set_repeat(Repeat::none);
    playlist.shuffle(99);
    ASSERT_TRUE(playlist.go_to(0));

    std::set<std::filesystem::path> seen{playlist.current()};

    while (playlist.next())
        seen.insert(playlist.current());

    EXPECT_EQ(seen.size(), 3U);
}

/// The same seed is the same order, so what a listener heard can be heard again.
TEST(Playlist, ShufflesTheSameWayForTheSameSeed)
{
    const auto order_for = [](std::uint_fast32_t seed) {
        Playlist playlist;
        playlist.set(three());
        playlist.shuffle(seed);

        std::vector<std::filesystem::path> order;

        do
            order.push_back(playlist.current());
        while (playlist.next());

        return order;
    };

    EXPECT_EQ(order_for(7), order_for(7));
}

TEST(Playlist, PutsThemBackInOrder)
{
    Playlist playlist;
    playlist.set(three());
    playlist.shuffle(5);

    ASSERT_TRUE(playlist.go_to(2));

    playlist.unshuffle();

    EXPECT_FALSE(playlist.shuffled());
    EXPECT_EQ(playlist.current(), std::filesystem::path{"third.wav"});
    EXPECT_EQ(playlist.position(), 2U);

    ASSERT_TRUE(playlist.go_to(0));
    ASSERT_TRUE(playlist.next());

    EXPECT_EQ(playlist.current(), std::filesystem::path{"second.wav"});
}

TEST(Playlist, StandsNowhereOnceCleared)
{
    Playlist playlist;
    playlist.set(three());

    playlist.clear();

    EXPECT_TRUE(playlist.empty());
    EXPECT_FALSE(playlist.position().has_value());
    EXPECT_TRUE(playlist.current().empty());
}
