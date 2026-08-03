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

#include <codec/wav_reader.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using wiola::codec::WavReader;

namespace {

void append(std::string& out, const void* data, std::size_t size)
{
    out.append(static_cast<const char*>(data), size);
}

void append_u32(std::string& out, std::uint32_t value)
{
    append(out, &value, sizeof(value));
}

void append_u16(std::string& out, std::uint16_t value)
{
    append(out, &value, sizeof(value));
}

/// Writes the low `bits_per_sample` bits of a sample, little-endian, as RIFF stores them.
void append_sample(std::string& out, std::int32_t sample, std::uint16_t bits_per_sample)
{
    const auto bits = static_cast<std::uint32_t>(sample);

    for (std::uint16_t shift = 0; shift < bits_per_sample; shift += 8)
        out += static_cast<char>((bits >> shift) & 0xFFU);
}

/// Builds a RIFF/WAVE file, optionally with a LIST chunk before the samples.
std::string make_wav(std::uint32_t sample_rate, std::uint16_t num_channels,
    std::uint16_t bits_per_sample, const std::vector<std::int32_t>& samples, bool with_list_chunk)
{
    std::string payload;
    for (std::int32_t sample : samples)
        append_sample(payload, sample, bits_per_sample);

    std::string fmt;
    append_u16(fmt, 1);
    append_u16(fmt, num_channels);
    append_u32(fmt, sample_rate);
    append_u32(fmt, sample_rate * num_channels * bits_per_sample / 8);
    append_u16(fmt, static_cast<std::uint16_t>(num_channels * bits_per_sample / 8));
    append_u16(fmt, bits_per_sample);

    std::string body{"WAVE"};
    body += "fmt ";
    append_u32(body, static_cast<std::uint32_t>(fmt.size()));
    body += fmt;

    if (with_list_chunk) {
        const std::string list{"INFOhello"};
        body += "LIST";
        append_u32(body, static_cast<std::uint32_t>(list.size() + 1));
        body += list;
        body += '\0';
    }

    body += "data";
    append_u32(body, static_cast<std::uint32_t>(payload.size()));
    body += payload;

    std::string file{"RIFF"};
    append_u32(file, static_cast<std::uint32_t>(body.size()));
    file += body;

    return file;
}

std::filesystem::path write_temp(const std::string& bytes, const char* name)
{
    const std::filesystem::path path{std::filesystem::temp_directory_path() / name};
    std::ofstream out{path, std::ios::binary};
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));

    return path;
}

} // namespace

TEST(WavReader, ReadsFormatFromTheFmtChunk)
{
    const auto path = write_temp(make_wav(44100, 2, 16, {1, -1, 2, -2}, false), "wiola_fmt.wav");
    const auto reader = WavReader::open(path);

    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->spec().sample_rate, wiola::units::Frequency{44100});
    EXPECT_EQ(reader->spec().num_channels, 2u);
    EXPECT_EQ(reader->num_frames(), 2u);
}

TEST(WavReader, SkipsChunksItDoesNotUnderstand)
{
    const auto path = write_temp(make_wav(48000, 1, 16, {7, 8, 9}, true), "wiola_list.wav");
    const auto reader = WavReader::open(path);

    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->spec().sample_rate, wiola::units::Frequency{48000});
    EXPECT_EQ(reader->num_frames(), 3u);
}

TEST(WavReader, ConvertsIntegerSamplesToFloat)
{
    const auto path =
        write_temp(make_wav(48000, 1, 16, {0, 16384, -16384}, false), "wiola_values.wav");
    auto reader = WavReader::open(path);
    ASSERT_NE(reader, nullptr);

    std::array<float, 3> block{};
    EXPECT_EQ(reader->render(block), 3u);

    EXPECT_FLOAT_EQ(block[0], 0.0F);
    EXPECT_FLOAT_EQ(block[1], 0.5F);
    EXPECT_FLOAT_EQ(block[2], -0.5F);
}

TEST(WavReader, ReadsDepthsOtherThanSixteenBits)
{
    const auto path =
        write_temp(make_wav(48000, 1, 24, {0, 0x400000, -0x400000}, false), "wiola_24bit.wav");
    auto reader = WavReader::open(path);
    ASSERT_NE(reader, nullptr);

    std::array<float, 3> block{};
    EXPECT_EQ(reader->render(block), 3u);

    EXPECT_FLOAT_EQ(block[0], 0.0F);
    EXPECT_FLOAT_EQ(block[1], 0.5F);
    EXPECT_FLOAT_EQ(block[2], -0.5F);
}

TEST(WavReader, StopsWhenTheDataChunkRunsOut)
{
    const auto path = write_temp(make_wav(48000, 2, 16, {1, 2, 3, 4}, false), "wiola_end.wav");
    auto reader = WavReader::open(path);
    ASSERT_NE(reader, nullptr);

    std::array<float, 16> block{};
    EXPECT_EQ(reader->render(block), 4u);
    EXPECT_TRUE(reader->exhausted());
    EXPECT_EQ(reader->num_frames_left(), 0u);
    EXPECT_EQ(reader->render(block), 0u);
}

TEST(WavReader, RejectsFilesItCannotPlay)
{
    EXPECT_EQ(WavReader::open("/nonexistent/wiola.wav"), nullptr);

    EXPECT_EQ(WavReader::open(write_temp("not a riff file at all", "wiola_bad.wav")), nullptr);
}
