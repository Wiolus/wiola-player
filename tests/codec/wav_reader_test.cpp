/**
 * @file
 * @brief Tests for the RIFF/WAVE reader.
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

#include <wav_reader.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>

using wiola::codec::WavReader;

namespace {

/// The fixture is a quarter second of 440 Hz stereo, so 11025 frames at 44100 Hz.
constexpr std::size_t tone_num_frames{11025};

std::filesystem::path fixture(const char* name)
{
    return std::filesystem::path{WIOLA_TEST_DATA_DIR} / name;
}

/// Reads a decoder to the end and reports how many samples it produced.
std::size_t drain(WavReader& reader)
{
    std::array<float, 1024> block{};
    std::size_t total{0};

    for (std::size_t num_rendered{reader.render(block)}; num_rendered > 0;
        num_rendered = reader.render(block))
        total += num_rendered;

    return total;
}

} // namespace

TEST(WavReader, ReadsFormatAndSamples)
{
    const auto reader = WavReader::open(fixture("tone.wav"));

    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->spec().sample_rate, wiola::units::Frequency{44100});
    EXPECT_EQ(reader->spec().num_channels, 2u);
    EXPECT_EQ(reader->num_frames(), tone_num_frames);
    EXPECT_EQ(drain(*reader), tone_num_frames * 2);
    EXPECT_TRUE(reader->exhausted());
}

/// The fixture carries a LIST chunk between `fmt ` and `data`. Counting it as audio, or starting
/// at a fixed byte 44, would put the frame count off.
TEST(WavReader, StepsOverChunksThatAreNotAudio)
{
    const auto reader = WavReader::open(fixture("tone.wav"));

    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->num_frames(), tone_num_frames);
}

TEST(WavReader, RendersFloatSamplesInRange)
{
    auto reader = WavReader::open(fixture("tone.wav"));
    ASSERT_NE(reader, nullptr);

    std::array<float, 256> block{};
    EXPECT_EQ(reader->render(block), block.size());

    EXPECT_TRUE(std::ranges::any_of(block, [](float v) { return v != 0.0F; }));
    EXPECT_TRUE(std::ranges::all_of(block, [](float v) { return v >= -1.0F && v <= 1.0F; }));
}

TEST(WavReader, StopsWhenTheDataChunkRunsOut)
{
    auto reader = WavReader::open(fixture("tone.wav"));
    ASSERT_NE(reader, nullptr);

    EXPECT_EQ(drain(*reader), tone_num_frames * 2);
    EXPECT_EQ(reader->num_frames_left(), 0u);

    std::array<float, 16> block{};
    EXPECT_EQ(reader->render(block), 0u);
}

TEST(WavReader, RejectsWhatIsNotWav)
{
    EXPECT_EQ(WavReader::open(fixture("tone.mp3")), nullptr);
    EXPECT_EQ(WavReader::open("/nonexistent/wiola.wav"), nullptr);
}
