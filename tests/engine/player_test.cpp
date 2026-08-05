/**
 * @file
 * @brief Tests for playing one decoder through a device.
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

#include <engine/player.hpp>

#include <codec/open.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace {

using namespace std::chrono_literals;
using wiola::engine::Player;

std::unique_ptr<wiola::codec::Decoder> fixture()
{
    return wiola::codec::open_file(std::filesystem::path{WIOLA_TEST_DATA_DIR} / "tone.flac");
}

/// Waits for `predicate`, so a test never depends on how fast a device happens to run.
template<typename Predicate>
bool eventually(Predicate predicate, std::chrono::milliseconds limit = 2s)
{
    for (auto waited = 0ms; waited < limit; waited += 10ms) {
        if (predicate())
            return true;

        std::this_thread::sleep_for(10ms);
    }

    return predicate();
}

} // namespace

TEST(Player, PlaysToTheEndOnItsOwn)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_TRUE(player.playing());

    player.wait();

    EXPECT_TRUE(player.finished());
    EXPECT_TRUE(source->exhausted());
}

/// The point of the thread: the caller is free while the audio is still going.
TEST(Player, StartReturnsBeforePlaybackEnds)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_FALSE(player.finished());
}

TEST(Player, StopEndsPlaybackAtOnce)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    const auto before = std::chrono::steady_clock::now();

    player.stop();
    player.wait();

    // Stopping does not play out what is buffered, so it returns in poll intervals rather than
    // in the fraction of a second the ring holds.
    EXPECT_LT(std::chrono::steady_clock::now() - before, 100ms);
    EXPECT_TRUE(player.finished());
    EXPECT_FALSE(player.playing());
}

TEST(Player, StopIsHarmlessAfterPlaybackHasEnded)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    player.wait();
    player.stop();
    player.stop();

    EXPECT_TRUE(player.finished());
}

TEST(Player, PauseSilencesAndResumeContinues)
{
    const auto source = fixture();
    ASSERT_NE(source, nullptr);

    Player player{*source};

    if (!player.start())
        GTEST_SKIP() << "no playback device on this machine";

    player.pause();
    EXPECT_FALSE(player.playing());

    // Pausing is not ending: the player is still there to be resumed.
    EXPECT_FALSE(player.finished());

    player.pause();
    EXPECT_FALSE(player.playing());

    ASSERT_TRUE(player.resume());
    EXPECT_TRUE(player.playing());

    EXPECT_TRUE(eventually([&player] { return player.finished(); }));
}
