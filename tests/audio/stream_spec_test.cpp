/**
 * @file
 * @brief Unit tests for the PCM stream description.
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

#include <audio/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

namespace {

using wiola::audio::Frames;
using wiola::audio::StreamSpec;
using namespace wiola::units;

TEST(StreamSpec, DefaultsToStereoAt48kHz)
{
    constexpr StreamSpec spec{};

    static_assert(spec.valid());
    EXPECT_EQ(spec.sample_rate, 48_kHz);
    EXPECT_EQ(spec.num_channels, 2u);
}

TEST(StreamSpec, ConvertsBetweenFramesAndSamples)
{
    constexpr StreamSpec spec{.sample_rate = 44.1_kHz, .num_channels = 2};

    EXPECT_EQ(spec.samples_per(Frames{128}), 256u);
    EXPECT_EQ(spec.frames_per(256), Frames{128});
    EXPECT_EQ(spec.frames_per(spec.samples_per(Frames{7})), Frames{7});
}

TEST(StreamSpec, SizesABufferFromMilliseconds)
{
    constexpr StreamSpec spec{.sample_rate = 48_kHz, .num_channels = 2};

    EXPECT_EQ(spec.samples_per(1_s), 96000u);
    EXPECT_EQ(spec.samples_per(250_ms), 24000u);
}

TEST(StreamSpec, RejectsEmptyStreams)
{
    EXPECT_FALSE((StreamSpec{.sample_rate = 0_Hz, .num_channels = 2}).valid());
    EXPECT_FALSE((StreamSpec{.sample_rate = 48_kHz, .num_channels = 0}).valid());
}

TEST(StreamSpec, ComparesMemberwise)
{
    constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};
    constexpr StreamSpec mono{.sample_rate = 48_kHz, .num_channels = 1};

    EXPECT_EQ(stereo, (StreamSpec{}));
    EXPECT_NE(stereo, mono);
}

} // namespace
