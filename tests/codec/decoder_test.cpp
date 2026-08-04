/**
 * @file
 * @brief Tests for the FLAC and MP3 readers, and for reader selection.
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

#include <codec/open.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>

using wiola::codec::Decoder;

namespace {

std::filesystem::path fixture(const char* name)
{
    return std::filesystem::path{WIOLA_TEST_DATA_DIR} / name;
}

/// Reads a decoder to the end and reports how many samples it produced.
std::size_t drain(Decoder& decoder)
{
    std::array<float, 1024> block{};
    std::size_t total{0};

    for (std::size_t num_rendered{decoder.render(block)}; num_rendered > 0;
        num_rendered = decoder.render(block))
        total += num_rendered;

    return total;
}

} // namespace

TEST(OpenFile, OpensEveryFormatItSupports)
{
    for (const char* name : {"tone.wav", "tone.flac", "tone.mp3"}) {
        const auto decoder = wiola::codec::open_file(fixture(name));

        ASSERT_NE(decoder, nullptr) << name;
        EXPECT_EQ(decoder->spec().sample_rate, wiola::units::Frequency{44100}) << name;
        EXPECT_EQ(decoder->spec().num_channels, 2u) << name;
        EXPECT_GT(decoder->num_frames(), 0u) << name;
    }
}

/// The bytes outrank the filename, so a file that names itself is read as what it is rather than
/// as what it is called.
TEST(OpenFile, BelievesTheContentOverTheExtension)
{
    const std::filesystem::path misnamed{std::filesystem::temp_directory_path() / "wiola_lie.wav"};
    std::filesystem::copy_file(fixture("tone.flac"), misnamed,
        std::filesystem::copy_options::overwrite_existing);

    const auto decoder = wiola::codec::open_file(misnamed);

    ASSERT_NE(decoder, nullptr);
    EXPECT_EQ(decoder->spec().num_channels, 2u);
    EXPECT_EQ(drain(*decoder), decoder->num_frames() * 2);
}

/// MP3 announces nothing, so it is reached only after every format that could have has declined.
TEST(OpenFile, ReachesAFormatThatCannotAnnounceItself)
{
    const std::filesystem::path misnamed{std::filesystem::temp_directory_path() / "wiola_lie.flac"};
    std::filesystem::copy_file(fixture("tone.mp3"), misnamed,
        std::filesystem::copy_options::overwrite_existing);

    const auto decoder = wiola::codec::open_file(misnamed);

    ASSERT_NE(decoder, nullptr);
    EXPECT_GT(decoder->num_frames(), 0u);
}

TEST(OpenFile, ReturnsNullWhenNoReaderAccepts)
{
    const std::filesystem::path junk{std::filesystem::temp_directory_path() / "wiola_junk.wav"};
    std::ofstream{junk, std::ios::binary} << "not audio at all, by any reader";

    EXPECT_EQ(wiola::codec::open_file(junk), nullptr);
    EXPECT_EQ(wiola::codec::open_file("/nonexistent/wiola.ogg"), nullptr);
    EXPECT_EQ(wiola::codec::open_file(std::filesystem::temp_directory_path()), nullptr);
}

TEST(Decoder, PlaysThroughTheInterface)
{
    const std::unique_ptr<Decoder> decoder{wiola::codec::open_file(fixture("tone.flac"))};

    ASSERT_NE(decoder, nullptr);

    std::array<float, 256> block{};
    const std::size_t num_rendered{decoder->render(block)};

    EXPECT_GT(num_rendered, 0u);
    EXPECT_LE(num_rendered, block.size());
    EXPECT_TRUE(std::ranges::any_of(block, [](float v) { return v != 0.0F; }));
}
