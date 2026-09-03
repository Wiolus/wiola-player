/**
 * @file
 * @brief Tests for the ceiling an output takes.
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

#include <audio/dsp/clip.hpp>

#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <limits>
#include <span>

namespace {

using wiola::audio::Clip;
using wiola::pcm::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

} // namespace

TEST(Clip, LeavesWhatAnOutputTakesAlone)
{
    Clip clip;
    std::array block{1.0F, -1.0F, 0.5F, -0.25F, 0.0F};
    const auto untouched = block;

    clip.process(block);

    EXPECT_EQ(block, untouched);
}

TEST(Clip, FlattensWhatReachesPastFullScale)
{
    Clip clip;
    std::array block{1.5F, -1.5F, 4.0F, -9.0F};

    clip.process(block);

    EXPECT_FLOAT_EQ(block[0], 1.0F);
    EXPECT_FLOAT_EQ(block[1], -1.0F);
    EXPECT_FLOAT_EQ(block[2], 1.0F);
    EXPECT_FLOAT_EQ(block[3], -1.0F);
}

/// A stream carries whatever it carries; an infinity in one is still held to the ceiling.
TEST(Clip, FlattensAnInfinity)
{
    Clip clip;
    std::array block{std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()};

    clip.process(block);

    EXPECT_FLOAT_EQ(block[0], 1.0F);
    EXPECT_FLOAT_EQ(block[1], -1.0F);
}

TEST(Clip, AcceptsNothingToDo)
{
    Clip clip;

    clip.process(std::span<float>{});

    std::array block{2.0F};
    clip.process(block);

    EXPECT_FLOAT_EQ(block[0], 1.0F);
}

/// Nothing here follows the stream, so being told about one changes nothing.
TEST(Clip, IsUnchangedByTheFormat)
{
    Clip clip;
    clip.configure(stereo);

    std::array block{2.0F, -2.0F};
    clip.process(block);

    EXPECT_FLOAT_EQ(block[0], 1.0F);
    EXPECT_FLOAT_EQ(block[1], -1.0F);
}
