/**
 * @file
 * @brief Unit tests for the sine tone generator.
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
#include <audio/tone.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

using wiola::audio::SineSource;
using namespace wiola::units;
using wiola::audio::StreamSpec;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

TEST(SineSource, StaysWithinAmplitude)
{
    SineSource source{stereo, 440_Hz, 0.5F};
    std::array<float, 4096> block{};

    source.render(block);

    const auto [min, max] = std::ranges::minmax_element(block);
    EXPECT_LE(*max, 0.5F);
    EXPECT_GE(*min, -0.5F);
    EXPECT_GT(*max, 0.4F);
}

TEST(SineSource, WritesTheSameSignalToEveryChannel)
{
    SineSource source{stereo, 1_kHz};
    std::array<float, 512> block{};

    source.render(block);

    for (std::size_t i = 0; i < block.size(); i += stereo.num_channels) {
        EXPECT_FLOAT_EQ(block[i], block[i + 1]);
    }
}

TEST(SineSource, KeepsPhaseAcrossCalls)
{
    SineSource continuous{stereo, 480_Hz};
    SineSource restarted{stereo, 480_Hz};

    std::array<float, 256> first_half{};
    std::array<float, 256> second_half{};
    std::array<float, 512> whole{};

    continuous.render(first_half);
    continuous.render(second_half);
    restarted.render(whole);

    EXPECT_TRUE(std::equal(first_half.begin(), first_half.end(), whole.begin()));
    EXPECT_TRUE(std::equal(second_half.begin(), second_half.end(), whole.begin() + 256));
}

TEST(SineSource, ProducesTheRequestedFrequency)
{
    constexpr auto frequency = 500_Hz;
    SineSource source{stereo, frequency};

    // One second of frames, so zero crossings should be two per cycle.
    std::array<float, 2 * 48000> block{};
    source.render(block);

    std::size_t num_crossings = 0;
    for (std::size_t i = stereo.num_channels; i < block.size(); i += stereo.num_channels) {
        if ((block[i - stereo.num_channels] < 0.0F) != (block[i] < 0.0F))
            ++num_crossings;
    }

    EXPECT_NEAR(static_cast<double>(num_crossings), 2.0 * frequency.hz(), 2.0);
}

TEST(SineSource, LeavesPartialFramesUntouched)
{
    SineSource source{stereo, 440_Hz};
    std::array<float, 5> block{};
    block.fill(42.0F);

    source.render(block);

    EXPECT_NE(block[0], 42.0F);
    EXPECT_FLOAT_EQ(block[4], 42.0F);
}

} // namespace
