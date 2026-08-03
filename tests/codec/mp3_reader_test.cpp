/**
 * @file
 * @brief Tests for the MP3 reader.
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

#include <codec/mp3_reader.hpp>

#include <gtest/gtest.h>

#include <array>
#include <filesystem>

using wiola::codec::Mp3Reader;

namespace {

std::filesystem::path fixture(const char* name)
{
    return std::filesystem::path{WIOLA_TEST_DATA_DIR} / name;
}

/// Reads a decoder to the end and reports how many samples it produced.
std::size_t drain(Mp3Reader& reader)
{
    std::array<float, 1024> block{};
    std::size_t total{0};

    for (std::size_t num_rendered{reader.render(block)}; num_rendered > 0;
        num_rendered = reader.render(block))
        total += num_rendered;

    return total;
}

} // namespace

TEST(Mp3Reader, ReadsFormatAndSamples)
{
    const auto reader = Mp3Reader::open(fixture("tone.mp3"));

    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->spec().sample_rate, wiola::units::Frequency{44100});
    EXPECT_EQ(reader->spec().num_channels, 2u);
    EXPECT_GT(reader->num_frames(), 0u);
    EXPECT_EQ(drain(*reader), reader->num_frames() * 2);
    EXPECT_TRUE(reader->exhausted());
}

TEST(Mp3Reader, RejectsWhatIsNotMp3)
{
    EXPECT_EQ(Mp3Reader::open("/nonexistent/wiola.mp3"), nullptr);
}
