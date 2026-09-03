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

#include <codec/decode/mp3_reader.hpp>

#include <audio/stream_spec.hpp>

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <vector>

using wiola::audio::Frames;
using wiola::codec::Mp3Reader;

using wiola::audio::Frames;

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

/// Reads `num_frames` frames from the start of `reader`, so a seek can be checked against the
/// samples that reading all the way there would have produced.
std::vector<float> read_frames(Mp3Reader& reader, std::size_t num_frames)
{
    const std::size_t num_channels{reader.spec().num_channels};
    std::vector<float> samples;
    std::array<float, 1024> block{};

    while (samples.size() < num_frames * num_channels) {
        const std::size_t num_rendered{reader.render(block)};

        if (num_rendered == 0)
            break;

        samples.insert(samples.end(), block.begin(),
            block.begin() + static_cast<std::ptrdiff_t>(num_rendered));
    }

    samples.resize(num_frames * num_channels);

    return samples;
}

} // namespace

TEST(Mp3Reader, ReadsFormatAndSamples)
{
    const auto reader = Mp3Reader::open(fixture("tone.mp3"));

    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->spec().sample_rate, wiola::units::Frequency{44100});
    EXPECT_EQ(reader->spec().num_channels, 2u);
    EXPECT_GT(reader->num_frames(), Frames{0});
    EXPECT_EQ(drain(*reader), reader->spec().samples_per(reader->num_frames()));
    EXPECT_TRUE(reader->exhausted());
}

TEST(Mp3Reader, RejectsWhatIsNotMp3)
{
    EXPECT_EQ(Mp3Reader::open("/nonexistent/wiola.mp3"), nullptr);
}

/// Seeking must land on the very sample that reading all the way there would have reached.
/// Nothing above the decoder can correct for a seek that lands somewhere else.
TEST(Mp3Reader, SeekLandsWhereReadingWouldHave)
{
    constexpr std::size_t target{4000};
    constexpr std::size_t num_compared{64};

    auto straight = Mp3Reader::open(fixture("tone.mp3"));
    ASSERT_NE(straight, nullptr);
    const std::vector<float> expected{read_frames(*straight, target + num_compared)};

    auto sought = Mp3Reader::open(fixture("tone.mp3"));
    ASSERT_NE(sought, nullptr);
    ASSERT_TRUE(sought->seek(Frames{target}));

    const std::size_t num_channels{sought->spec().num_channels};
    const std::vector<float> actual{read_frames(*sought, num_compared)};

    ASSERT_EQ(actual.size(), num_compared * num_channels);
    EXPECT_TRUE(std::equal(actual.begin(), actual.end(), expected.begin() + target * num_channels));
}

TEST(Mp3Reader, SeekMovesThePosition)
{
    auto reader = Mp3Reader::open(fixture("tone.mp3"));
    ASSERT_NE(reader, nullptr);

    ASSERT_TRUE(reader->seek(Frames{4000}));
    EXPECT_EQ(reader->num_frames_left(), reader->num_frames() - Frames{4000});
    EXPECT_FALSE(reader->exhausted());

    ASSERT_TRUE(reader->seek(reader->num_frames()));
    EXPECT_EQ(reader->num_frames_left(), Frames{0});
    EXPECT_TRUE(reader->exhausted());

    std::array<float, 16> block{};
    EXPECT_EQ(reader->render(block), 0u);
}

TEST(Mp3Reader, SeekRewinds)
{
    auto reader = Mp3Reader::open(fixture("tone.mp3"));
    ASSERT_NE(reader, nullptr);

    const std::vector<float> first{read_frames(*reader, 64)};

    ASSERT_TRUE(reader->seek(Frames{0}));
    EXPECT_EQ(reader->num_frames_left(), reader->num_frames());
    EXPECT_EQ(read_frames(*reader, 64), first);
}

TEST(Mp3Reader, RefusesToSeekPastTheEnd)
{
    auto reader = Mp3Reader::open(fixture("tone.mp3"));
    ASSERT_NE(reader, nullptr);

    ASSERT_TRUE(reader->seek(Frames{4000}));
    EXPECT_FALSE(reader->seek(reader->num_frames() + Frames{1}));

    // A refused seek leaves the position alone.
    EXPECT_EQ(reader->num_frames_left(), reader->num_frames() - Frames{4000});
}
