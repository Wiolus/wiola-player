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

#include <engine/queue/playlist.hpp>

#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <set>
#include <vector>

namespace {

using wiola::engine::Playlist;
using wiola::engine::Track;
using Repeat = wiola::engine::Playlist::Repeat;

/// Three tracks, named so that a test can say which one it is standing at.
std::vector<Track> three()
{
    return {Track::of(std::filesystem::path{"first.wav"}),
        Track::of(std::filesystem::path{"second.wav"}),
        Track::of(std::filesystem::path{"third.wav"})};
}

/// The file a track is, which is what a test says it stands at.
const std::filesystem::path& path_of(const Playlist& playlist)
{
    return playlist.current().path;
}

} // namespace

TEST(Playlist, StandsNowhereWhileItIsEmpty)
{
    const Playlist playlist;

    EXPECT_TRUE(playlist.empty());
    EXPECT_EQ(playlist.tracks().size(), 0U);
    EXPECT_TRUE(path_of(playlist).empty());
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

    EXPECT_EQ(playlist.tracks().size(), 3U);
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"first.wav"});
    EXPECT_EQ(playlist.position(), 0U);
}

TEST(Playlist, StandsAtTheFirstTrackAdded)
{
    Playlist playlist;

    playlist.add(Track::of(std::filesystem::path{"only.wav"}));
    playlist.add(Track::of(std::filesystem::path{"after.wav"}));

    EXPECT_EQ(playlist.tracks().size(), 2U);
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"only.wav"});
}

TEST(Playlist, WalksForwardsAndBack)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.next());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});

    ASSERT_TRUE(playlist.next());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});

    ASSERT_TRUE(playlist.previous());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});
}

/// The end of the list is the end of playback, and it stays where it stopped.
TEST(Playlist, GoesNoFurtherThanTheEnd)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(2));

    EXPECT_FALSE(playlist.next());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});

    ASSERT_TRUE(playlist.go_to(0));

    EXPECT_FALSE(playlist.previous());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"first.wav"});
}

TEST(Playlist, ComesRoundWhenEverythingRepeats)
{
    Playlist playlist;
    playlist.set(three());
    playlist.set_repeat(Repeat::all);

    ASSERT_TRUE(playlist.go_to(2));

    EXPECT_TRUE(playlist.next());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"first.wav"});

    EXPECT_TRUE(playlist.previous());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});
}

TEST(Playlist, StaysWhereItIsWhenOneTrackRepeats)
{
    Playlist playlist;
    playlist.set(three());
    playlist.set_repeat(Repeat::track);

    ASSERT_TRUE(playlist.go_to(1));

    EXPECT_TRUE(playlist.next());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});

    EXPECT_TRUE(playlist.previous());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});
}

TEST(Playlist, StandsWhereItIsSent)
{
    Playlist playlist;
    playlist.set(three());

    EXPECT_TRUE(playlist.go_to(2));
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});
    EXPECT_EQ(playlist.position(), 2U);

    EXPECT_FALSE(playlist.go_to(3));
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});
}

/// Shuffling changes what comes next, not what is playing now.
TEST(Playlist, KeepsTheTrackItStandsAtWhenShuffled)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(1));

    playlist.shuffle(1234);

    EXPECT_TRUE(playlist.shuffled());
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});
    EXPECT_EQ(playlist.position(), 1U);
}

/// An order, not a fresh choice each time: every track comes once before any comes twice.
TEST(Playlist, ShufflingIsAnOrderOverEveryTrack)
{
    Playlist playlist;
    playlist.set(three());
    playlist.set_repeat(Repeat::none);
    playlist.shuffle(99);

    // Back to the start of the order: shuffling means the first track is not first any more.
    while (playlist.previous()) { }

    std::set<std::filesystem::path> seen{path_of(playlist)};

    while (playlist.next())
        seen.insert(path_of(playlist));

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
            order.push_back(path_of(playlist));
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
    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});
    EXPECT_EQ(playlist.position(), 2U);

    ASSERT_TRUE(playlist.go_to(0));
    ASSERT_TRUE(playlist.next());

    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});
}

/// A track nothing is known about is a track with a name: the file's own.
TEST(Playlist, NamesATrackAfterItsFileUntilItIsToldOtherwise)
{
    Playlist playlist;

    playlist.add(Track::of(std::filesystem::path{"/music/first.wav"}));

    EXPECT_EQ(playlist.current().title, "first") << "a name is not an extension";
    EXPECT_TRUE(playlist.current().artist.empty());
    EXPECT_EQ(playlist.current().duration, wiola::units::Time{});
}

/// What is found out about a track later replaces what was assumed, and moves nothing.
TEST(Playlist, TakesWhatIsFoundOutAboutATrack)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(1));

    Track told{Track::of(std::filesystem::path{"second.wav"})};
    told.title = "Together Forever";
    told.artist = "Rick Astley";
    told.album = "Whenever You Need Somebody";
    told.duration = wiola::units::Time{std::chrono::seconds{205}};

    EXPECT_TRUE(playlist.describe(1, std::move(told)));

    EXPECT_EQ(playlist.current().title, "Together Forever");
    EXPECT_EQ(playlist.current().artist, "Rick Astley");
    EXPECT_EQ(playlist.position(), 1U) << "saying what a track is moved the list";
    EXPECT_EQ(playlist.tracks().size(), 3U);

    EXPECT_FALSE(playlist.describe(3, Track{}));
}

/// A window redrawing ten times a second needs to know that something changed without working
/// out what.
TEST(Playlist, CountsEveryChangeToWhatIsQueued)
{
    Playlist playlist;

    const std::size_t empty{playlist.revision()};

    playlist.set(three());
    const std::size_t filled{playlist.revision()};
    EXPECT_NE(filled, empty);

    playlist.add(Track::of(std::filesystem::path{"fourth.wav"}));
    const std::size_t added{playlist.revision()};
    EXPECT_NE(added, filled);

    ASSERT_TRUE(playlist.describe(0, Track::of(std::filesystem::path{"first.wav"})));
    const std::size_t described{playlist.revision()};
    EXPECT_NE(described, added) << "what was found out about a track went unnoticed";

    ASSERT_TRUE(playlist.remove(0));
    EXPECT_NE(playlist.revision(), described);

    playlist.shuffle(11);
    const std::size_t shuffled{playlist.revision()};
    EXPECT_NE(shuffled, described);

    // Standing somewhere else is not a change to what is queued.
    ASSERT_TRUE(playlist.go_to(1));
    EXPECT_EQ(playlist.revision(), shuffled);
}

TEST(Playlist, TakesATrackOut)
{
    Playlist playlist;
    playlist.set(three());

    EXPECT_TRUE(playlist.remove(1));

    ASSERT_EQ(playlist.tracks().size(), 2U);
    EXPECT_EQ(playlist.tracks().front().path, std::filesystem::path{"first.wav"});
    EXPECT_EQ(playlist.tracks().back().path, std::filesystem::path{"third.wav"});

    EXPECT_FALSE(playlist.remove(2));
}

/// Taking out what came before it does not move what it stands at.
TEST(Playlist, KeepsWhatItStandsAtWhenSomethingBeforeItGoes)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(2));
    ASSERT_TRUE(playlist.remove(0));

    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});
    EXPECT_EQ(playlist.position(), 1U);
}

/// Taking out the one it stands at leaves it standing at whatever followed.
TEST(Playlist, StandsAtWhatFollowedTheTrackTakenOut)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(1));
    ASSERT_TRUE(playlist.remove(1));

    EXPECT_EQ(path_of(playlist), std::filesystem::path{"third.wav"});
    EXPECT_EQ(playlist.position(), 1U);
}

/// Nothing followed the last one, so it stands at the end of what is left.
TEST(Playlist, StandsAtTheEndWhenTheLastTrackGoes)
{
    Playlist playlist;
    playlist.set(three());

    ASSERT_TRUE(playlist.go_to(2));
    ASSERT_TRUE(playlist.remove(2));

    EXPECT_EQ(path_of(playlist), std::filesystem::path{"second.wav"});
    EXPECT_EQ(playlist.position(), 1U);
}

TEST(Playlist, StandsNowhereOnceTheLastTrackIsTakenOut)
{
    Playlist playlist;

    playlist.add(Track::of(std::filesystem::path{"only.wav"}));
    ASSERT_TRUE(playlist.remove(0));

    EXPECT_TRUE(playlist.empty());
    EXPECT_FALSE(playlist.position().has_value());
}

/// A shuffled order holds places in the list, so taking one out has to mend the rest of them.
TEST(Playlist, KeepsAShuffledOrderWholeWhenATrackGoes)
{
    Playlist playlist;
    playlist.set(three());
    playlist.shuffle(3);

    ASSERT_TRUE(playlist.remove(1));
    ASSERT_EQ(playlist.tracks().size(), 2U);

    playlist.set_repeat(Repeat::none);

    // Back to the start of the order, which after shuffling is not the start of the list.
    while (playlist.previous()) { }

    std::set<std::filesystem::path> seen{path_of(playlist)};

    while (playlist.next())
        seen.insert(path_of(playlist));

    EXPECT_EQ(seen.size(), 2U);
    EXPECT_EQ(seen.count(std::filesystem::path{"second.wav"}), 0U);
}

TEST(Playlist, StandsNowhereOnceCleared)
{
    Playlist playlist;
    playlist.set(three());

    playlist.clear();

    EXPECT_TRUE(playlist.empty());
    EXPECT_FALSE(playlist.position().has_value());
    EXPECT_TRUE(path_of(playlist).empty());
}
