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
#include <cstddef>
#include <numbers>
#include <span>

namespace {

using wiola::audio::SineSource;
using namespace wiola::units;
using wiola::audio::StreamSpec;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};
constexpr StreamSpec mono{.sample_rate = 44100_Hz, .num_channels = 1};
constexpr StreamSpec surround{.sample_rate = 48_kHz, .num_channels = 6};

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

TEST(SineSource, ReportsTheSpecItWasGiven)
{
    EXPECT_EQ(SineSource(stereo, 440_Hz).spec(), stereo);
    EXPECT_EQ(SineSource(mono, 440_Hz).spec(), mono);
}

TEST(SineSource, ReturnsTheNumberOfSamplesWritten)
{
    SineSource source{stereo, 440_Hz};
    std::array<float, 512> block{};
    std::array<float, 5> partial{};

    EXPECT_EQ(source.render(block), block.size());
    EXPECT_EQ(source.render(partial), 4U);
}

TEST(SineSource, WritesNothingWhenThereIsNoWholeFrame)
{
    SineSource source{stereo, 440_Hz};
    std::array<float, 1> block{42.0F};

    EXPECT_EQ(source.render(std::span<float>{}), 0U);
    EXPECT_EQ(source.render(block), 0U);
    EXPECT_FLOAT_EQ(block[0], 42.0F);
}

/// Held back frames are not skipped: what a short span refused is what the next call renders.
TEST(SineSource, HoldsPhaseWhenNothingIsWritten)
{
    SineSource stalled{stereo, 440_Hz};
    SineSource plain{stereo, 440_Hz};

    std::array<float, 1> too_short{};
    std::array<float, 64> from_stalled{};
    std::array<float, 64> from_plain{};

    stalled.render(too_short);
    stalled.render(from_stalled);
    plain.render(from_plain);

    EXPECT_EQ(from_stalled, from_plain);
}

TEST(SineSource, FillsEveryChannelOfAnyLayout)
{
    SineSource single{mono, 440_Hz};
    SineSource many{surround, 440_Hz};

    std::array<float, 64> single_block{};
    std::array<float, 64> many_block{};

    EXPECT_EQ(single.render(single_block), single_block.size());
    EXPECT_EQ(many.render(many_block), 60U);

    for (std::size_t i = 0; i + surround.num_channels <= many_block.size();
        i += surround.num_channels) {
        for (std::size_t channel = 1; channel < surround.num_channels; ++channel)
            EXPECT_FLOAT_EQ(many_block[i + channel], many_block[i]);
    }
}

TEST(SineSource, StartsAtZeroPhase)
{
    SineSource source{stereo, 440_Hz};
    std::array<float, 2> block{};

    source.render(block);

    EXPECT_FLOAT_EQ(block[0], 0.0F);
}

/// Wrapping the phase keeps the tone on the analytic sine rather than only near it.
TEST(SineSource, FollowsTheAnalyticSineAcrossWraps)
{
    constexpr auto frequency = 997_Hz;
    constexpr float amplitude{0.75F};
    SineSource source{mono, frequency, amplitude};

    std::array<float, 8192> block{};
    source.render(block);

    const double step{2.0 * std::numbers::pi * (frequency / mono.sample_rate)};

    for (std::size_t i = 0; i < block.size(); ++i) {
        const auto expected =
            static_cast<float>(amplitude * std::sin(step * static_cast<double>(i)));
        EXPECT_NEAR(block[i], expected, 1e-4F) << "at sample " << i;
    }
}

TEST(SineSource, IsSilentAtZeroAmplitude)
{
    SineSource source{stereo, 440_Hz, 0.0F};
    std::array<float, 128> block{};
    block.fill(42.0F);

    source.render(block);

    for (const float sample : block)
        EXPECT_FLOAT_EQ(sample, 0.0F);
}

TEST(SineSource, DefaultsToAQuietAmplitude)
{
    SineSource source{stereo, 440_Hz};
    std::array<float, 4096> block{};

    source.render(block);

    const auto [min, max] = std::ranges::minmax_element(block);
    EXPECT_LE(*max, 0.2F);
    EXPECT_GE(*min, -0.2F);
    EXPECT_GT(*max, 0.19F);
}

} // namespace
