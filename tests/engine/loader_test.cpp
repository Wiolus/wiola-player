/**
 * @file
 * @brief Unit tests for opening files off the thread that asked.
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

#include <engine/load/loader.hpp>

#include <fixtures/wav.hpp>

#include <codec/open.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <thread>

namespace {

using wiola::codec::OpenResult;
using wiola::engine::Loader;

/// Waits for whatever was asked for, the way a caller that draws comes back for it.
std::optional<wiola::codec::Opened> wait_for(Loader& loader)
{
    while (loader.busy())
        std::this_thread::yield();

    return loader.take();
}

} // namespace

TEST(Loader, HasNothingBeforeItIsAsked)
{
    Loader loader;

    EXPECT_FALSE(loader.busy());
    EXPECT_FALSE(loader.take().has_value());
}

TEST(Loader, OpensAFileAndHandsItOver)
{
    Loader loader;

    loader.start(wiola::testing::write_wav("wiola_loader.wav"));

    const std::optional<wiola::codec::Opened> opened{wait_for(loader)};

    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->result, OpenResult::opened);
    ASSERT_NE(opened->decoder, nullptr);
    EXPECT_GT(opened->decoder->num_frames(), wiola::audio::Frames{});
}

TEST(Loader, SaysWhyAFileWouldNotOpen)
{
    Loader loader;

    loader.start(std::filesystem::path{"no-such-track.wav"});

    const std::optional<wiola::codec::Opened> opened{wait_for(loader)};

    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->result, OpenResult::unreadable);
    EXPECT_EQ(opened->decoder, nullptr);
}

/// The answer is handed over once: a caller that comes back again gets nothing, rather than the
/// same track twice.
TEST(Loader, HandsTheAnswerOverOnlyOnce)
{
    Loader loader;

    loader.start(wiola::testing::write_wav("wiola_loader.wav"));

    ASSERT_TRUE(wait_for(loader).has_value());
    EXPECT_FALSE(loader.take().has_value());
}

/// A listener who picks another file has said which one they meant, so the first answer is
/// dropped rather than played.
TEST(Loader, KeepsOnlyTheFileAskedForLast)
{
    Loader loader;

    const std::filesystem::path first{wiola::testing::write_wav("wiola_loader_first.wav")};
    const std::filesystem::path last{wiola::testing::write_wav("wiola_loader_last.wav")};

    loader.start(first);
    loader.start(last);

    const std::optional<wiola::codec::Opened> opened{wait_for(loader)};

    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->result, OpenResult::opened);

    // Only one answer is kept, whichever of the two the reading reached first.
    EXPECT_FALSE(loader.take().has_value());
}

TEST(Loader, TakesAnotherFileAfterOneIsDone)
{
    Loader loader;

    loader.start(wiola::testing::write_wav("wiola_loader.wav"));
    ASSERT_TRUE(wait_for(loader).has_value());

    loader.start(wiola::testing::write_wav("wiola_loader_again.wav"));

    EXPECT_TRUE(wait_for(loader).has_value());
}

/// Nothing of a file crosses back until it is whole, so a loader thrown away mid-read is not a
/// decoder half-built.
TEST(Loader, CanBeThrownAwayWhileItIsReading)
{
    Loader loader;

    loader.start(wiola::testing::write_wav("wiola_loader.wav"));
}
