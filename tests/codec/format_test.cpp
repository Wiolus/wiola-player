/**
 * @file
 * @brief Unit tests for the leading bytes of a file.
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

#include <format.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string_view>

namespace {

using wiola::codec::FileHead;
using wiola::codec::Marker;

/// Writes `bytes` to a file of its own and reads its head back.
FileHead head_of(const char* name, std::string_view bytes)
{
    const std::filesystem::path path{std::filesystem::temp_directory_path() / name};
    std::ofstream out{path, std::ios::binary};

    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.close();

    return FileHead::read(path);
}

TEST(FileHead, IsEmptyWhenThereIsNoFile)
{
    const FileHead head{FileHead::read("/nonexistent/wiola.wav")};

    EXPECT_TRUE(head.empty());
    EXPECT_FALSE(head.carries(Marker{0, "RIFF"}));
}

TEST(FileHead, IsEmptyForAnEmptyFile)
{
    EXPECT_TRUE(head_of("wiola_head_empty.bin", "").empty());
}

TEST(FileHead, IsEmptyForADirectory)
{
    EXPECT_TRUE(FileHead::read(std::filesystem::temp_directory_path()).empty());
}

TEST(FileHead, CarriesAMarkerWhereItSaysItIs)
{
    const FileHead head{head_of("wiola_head_riff.bin", "RIFF\1\2\3\4WAVEfmt ")};

    EXPECT_FALSE(head.empty());
    EXPECT_TRUE(head.carries(Marker{0, "RIFF"}));
    EXPECT_TRUE(head.carries(Marker{8, "WAVE"}));
}

/// The offset is part of the marker: the same bytes elsewhere are not it.
TEST(FileHead, DoesNotCarryAMarkerAtAnotherOffset)
{
    const FileHead head{head_of("wiola_head_riff.bin", "RIFF\1\2\3\4WAVEfmt ")};

    EXPECT_FALSE(head.carries(Marker{4, "RIFF"}));
    EXPECT_FALSE(head.carries(Marker{0, "WAVE"}));
}

TEST(FileHead, CarriesNothingPastWhatWasRead)
{
    const FileHead head{head_of("wiola_head_short.bin", "RIFF")};

    EXPECT_FALSE(head.empty());
    EXPECT_TRUE(head.carries(Marker{0, "RIFF"}));

    // Beginning inside what was read is not enough; all of it has to be there.
    EXPECT_FALSE(head.carries(Marker{2, "FFxx"}));
    EXPECT_FALSE(head.carries(Marker{8, "WAVE"}));
}

/// Only the first bytes are kept, so a marker further in than that is never found.
TEST(FileHead, KeepsOnlyTheLeadingBytes)
{
    const FileHead head{head_of("wiola_head_long.bin", "0123456789abcdefghijklmnop")};

    EXPECT_TRUE(head.carries(Marker{12, "cdef"}));
    EXPECT_FALSE(head.carries(Marker{13, "defg"}));
    EXPECT_FALSE(head.carries(Marker{16, "ghij"}));
}

/// A signature is bytes, not text: the high ones compare as themselves.
TEST(FileHead, ComparesBytesRatherThanCharacters)
{
    const FileHead head{head_of("wiola_head_sync.bin", "\xFF\xFB\x90\x64rest")};

    EXPECT_TRUE(head.carries(Marker{0, "\xFF\xFB"}));
    EXPECT_FALSE(head.carries(Marker{0, "\x7F\x7B"}));
}

} // namespace
