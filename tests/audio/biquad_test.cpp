/**
 * @file
 * @brief Tests for one second-order section.
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

#include <audio/dsp/biquad.hpp>

#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace {

using wiola::audio::Biquad;
using wiola::audio::BiquadCoefficients;
using wiola::pcm::Frames;
using wiola::pcm::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

/// A sine at `tone`, the same in every channel, written over `block` from frame `at`.
void write_tone(std::vector<float>& block, StreamSpec spec, Frequency tone, std::size_t at)
{
    const double step{2.0 * std::numbers::pi * tone.hz() / spec.sample_rate.hz()};
    const std::size_t num_frames{block.size() / spec.num_channels};

    for (std::size_t frame = 0; frame < num_frames; ++frame) {
        const auto value =
            static_cast<float>(0.5 * std::sin(step * static_cast<double>(at + frame)));

        for (std::size_t channel = 0; channel < spec.num_channels; ++channel)
            block[(frame * spec.num_channels) + channel] = value;
    }
}

double energy_of(const std::vector<float>& block)
{
    double total{0.0};

    for (const float sample : block)
        total += static_cast<double>(sample) * sample;

    return total;
}

/// Level a section leaves a sine at `tone` with, in decibels against the sine's own level. The
/// early blocks are the section settling, so only the last one is measured.
double response_db(BiquadCoefficients coefficients, StreamSpec spec, Frequency tone)
{
    constexpr std::size_t num_blocks{40};
    constexpr std::size_t num_frames{512};

    Biquad filter;
    filter.configure(coefficients, spec.num_channels);

    std::vector<float> block(spec.samples_per(Frames{num_frames}));
    double plain{0.0};
    double shaped{0.0};

    for (std::size_t i = 0; i < num_blocks; ++i) {
        write_tone(block, spec, tone, i * num_frames);
        plain = energy_of(block);

        filter.process(block);
        shaped = energy_of(block);
    }

    return 10.0 * std::log10(shaped / plain);
}

BiquadCoefficients peaking(Frequency center, double quality, double gain_db)
{
    return BiquadCoefficients::peaking(stereo, center, quality, gain_db);
}

} // namespace

TEST(Biquad, LeavesSamplesAloneBeforeItIsConfigured)
{
    Biquad filter;
    std::array block{1.0F, -0.5F, 0.25F, 0.0F};
    const auto untouched = block;

    filter.process(block);

    EXPECT_EQ(block, untouched);
}

/// The section that multiplies by one is what a default is for.
TEST(Biquad, LeavesSamplesAloneByDefault)
{
    Biquad filter;
    filter.configure(BiquadCoefficients{}, stereo.num_channels);

    std::array block{1.0F, -0.5F, 0.25F, 0.0F};
    const auto untouched = block;

    filter.process(block);

    EXPECT_EQ(block, untouched);
}

/// Asking for no lift and no cut is asking for nothing to happen, at every frequency.
TEST(Biquad, LeavesEveryToneAloneAtNoGain)
{
    for (const Frequency tone : {100_Hz, 1_kHz, 8_kHz})
        EXPECT_NEAR(response_db(peaking(1_kHz, 1.0, 0.0), stereo, tone), 0.0, 0.01)
            << "at " << tone.hz() << " Hz";
}

TEST(Biquad, LiftsAToneAtItsCenter)
{
    EXPECT_NEAR(response_db(peaking(1_kHz, 1.0, 6.0), stereo, 1_kHz), 6.0, 0.2);
}

TEST(Biquad, CutsAToneAtItsCenter)
{
    EXPECT_NEAR(response_db(peaking(1_kHz, 1.0, -6.0), stereo, 1_kHz), -6.0, 0.2);
}

/// A band is a band: what it does falls away from its center.
TEST(Biquad, LeavesADistantToneAlone)
{
    EXPECT_NEAR(response_db(peaking(1_kHz, 1.0, 12.0), stereo, 60_Hz), 0.0, 0.5);
}

TEST(Biquad, ReachesLessFarAsQualityRises)
{
    const double wide{response_db(peaking(1_kHz, 1.0, 12.0), stereo, 2_kHz)};
    const double narrow{response_db(peaking(1_kHz, 4.0, 12.0), stereo, 2_kHz)};

    EXPECT_GT(wide, narrow);

    // Both still arrive at the gain where the band sits.
    EXPECT_NEAR(response_db(peaking(1_kHz, 4.0, 12.0), stereo, 1_kHz), 12.0, 0.3);
}

/// One pair of numbers per channel, so what is in one channel stays there.
TEST(Biquad, KeepsEachChannelApart)
{
    Biquad filter;
    filter.configure(peaking(1_kHz, 1.0, 12.0), stereo.num_channels);

    constexpr std::size_t num_frames{256};
    std::vector<float> block(stereo.samples_per(Frames{num_frames}), 0.0F);

    // The left channel alone carries anything.
    for (std::size_t frame = 0; frame < num_frames; ++frame)
        block[frame * stereo.num_channels] = 0.5F;

    filter.process(block);

    for (std::size_t frame = 0; frame < num_frames; ++frame)
        ASSERT_FLOAT_EQ(block[(frame * stereo.num_channels) + 1], 0.0F) << "frame " << frame;
}

/// A block boundary is not something the sound crosses: what is carried says so.
TEST(Biquad, CarriesWhatItHoldsAcrossCalls)
{
    constexpr std::size_t num_frames{512};

    std::vector<float> whole(stereo.samples_per(Frames{num_frames}));
    write_tone(whole, stereo, 1_kHz, 0);

    std::vector<float> halves{whole};

    Biquad one;
    one.configure(peaking(1_kHz, 1.0, 9.0), stereo.num_channels);
    one.process(whole);

    Biquad two;
    two.configure(peaking(1_kHz, 1.0, 9.0), stereo.num_channels);

    const std::size_t half{halves.size() / 2};
    two.process(std::span{halves}.first(half));
    two.process(std::span{halves}.subspan(half));

    EXPECT_EQ(whole, halves);
}

/// Forgotten rather than carried: what was held belonged to a stream that has ended.
TEST(Biquad, ForgetsWhatItHeldWhenConfiguredAgain)
{
    constexpr std::size_t num_frames{256};

    std::vector<float> first(stereo.samples_per(Frames{num_frames}));
    write_tone(first, stereo, 1_kHz, 0);

    std::vector<float> again{first};

    Biquad filter;
    filter.configure(peaking(1_kHz, 1.0, 9.0), stereo.num_channels);
    filter.process(first);

    // Run something through it, then start over: the second run must match the first.
    filter.process(again);
    filter.configure(peaking(1_kHz, 1.0, 9.0), stereo.num_channels);

    write_tone(again, stereo, 1_kHz, 0);
    filter.process(again);

    EXPECT_EQ(first, again);
}

TEST(Biquad, DoesNothingForAFormatItCannotUse)
{
    const StreamSpec nothing{.sample_rate = Frequency{}, .num_channels = 2};
    const BiquadCoefficients flat{};

    const BiquadCoefficients no_rate{BiquadCoefficients::peaking(nothing, 1_kHz, 1.0, 12.0)};
    const BiquadCoefficients no_quality{BiquadCoefficients::peaking(stereo, 1_kHz, 0.0, 12.0)};

    EXPECT_FLOAT_EQ(no_rate.b0, flat.b0);
    EXPECT_FLOAT_EQ(no_rate.b1, flat.b1);
    EXPECT_FLOAT_EQ(no_rate.b2, flat.b2);
    EXPECT_FLOAT_EQ(no_rate.a1, flat.a1);
    EXPECT_FLOAT_EQ(no_rate.a2, flat.a2);

    EXPECT_FLOAT_EQ(no_quality.b0, flat.b0);
    EXPECT_FLOAT_EQ(no_quality.a2, flat.a2);
}

/// A gain moved while a track plays keeps what is carried, so the sound changes without a click.
TEST(Biquad, KeepsWhatItCarriesWhenRetuned)
{
    constexpr std::size_t num_frames{256};

    std::vector<float> block(stereo.samples_per(Frames{num_frames}));
    write_tone(block, stereo, 1_kHz, 0);

    Biquad filter;
    filter.configure(peaking(1_kHz, 1.0, 0.0), stereo.num_channels);
    filter.process(block);

    const float last{block.back()};

    write_tone(block, stereo, 1_kHz, num_frames);
    filter.retune(peaking(1_kHz, 1.0, 6.0));
    filter.process(block);

    // Nothing jumps: the first sample after the change is not far from the last before it.
    EXPECT_LT(std::abs(block.front() - last), 1.0F);
}
