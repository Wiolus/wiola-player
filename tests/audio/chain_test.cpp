/**
 * @file
 * @brief Tests for what is done to samples on their way to the output.
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

#include <audio/dsp/chain.hpp>

#include <audio/dsp/tuning.hpp>
#include <fakes/tone.hpp>
#include <fixtures/shaping.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace {

using wiola::audio::BandLayout;
using wiola::audio::Chain;
using wiola::audio::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

auto samples()
{
    return std::array{1.0F, -0.5F, 0.25F, 0.0F};
}

} // namespace

TEST(Chain, StartsAtFullVolume)
{
    const wiola::testing::Shaping shaping{stereo};

    EXPECT_FLOAT_EQ(shaping.volume.gain(), 1.0F);
}

/// Full volume is the samples as they arrived, not something multiplied by one.
TEST(Chain, LeavesSamplesUntouchedAtFullVolume)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, ScalesEverySample)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.volume.set_gain(0.5F);
    shaping.chain.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 0.5F);
    EXPECT_FLOAT_EQ(buffer[1], -0.25F);
    EXPECT_FLOAT_EQ(buffer[2], 0.125F);
    EXPECT_FLOAT_EQ(buffer[3], 0.0F);
}

TEST(Chain, SilencesAtZero)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.volume.set_gain(0.0F);
    shaping.chain.process(buffer);

    for (const float sample : buffer)
        EXPECT_FLOAT_EQ(sample, 0.0F);
}

TEST(Chain, HoldsTheSettingAcrossCalls)
{
    wiola::testing::Shaping shaping{stereo};
    auto first{samples()};
    auto second{samples()};

    shaping.volume.set_gain(0.25F);
    shaping.chain.process(first);
    shaping.chain.process(second);

    EXPECT_EQ(first, second);
    EXPECT_FLOAT_EQ(shaping.volume.gain(), 0.25F);
}

/// A listener may ask for more than arrived, but only so much more.
TEST(Chain, RefusesMoreThanTheMostItMayLift)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.volume.set_gain(4.0F);

    EXPECT_FLOAT_EQ(shaping.volume.gain(), wiola::audio::tuning::max_volume_boost);
}

/// Past what an output takes there is nothing to hear, so what is lifted past it is flattened.
TEST(Chain, FlattensWhatALiftPutPastFullScale)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.volume.set_gain(1.4F);
    shaping.chain.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 1.0F);
    EXPECT_FLOAT_EQ(buffer[1], -0.7F);
    EXPECT_FLOAT_EQ(buffer[2], 0.35F);
}

TEST(Chain, TakesAnythingBelowZeroAsSilence)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.volume.set_gain(-2.0F);

    EXPECT_FLOAT_EQ(shaping.volume.gain(), 0.0F);

    shaping.volume.set_gain(std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ(shaping.volume.gain(), 0.0F);
}

TEST(Chain, AcceptsNothingToDo)
{
    wiola::testing::Shaping shaping{stereo};
    std::array block{1.0F, 1.0F};

    shaping.volume.set_gain(0.5F);
    shaping.chain.process(std::span<float>{});

    // Nothing to do is not something to recover from: what comes after is shaped as it would
    // have been.
    shaping.chain.process(block);

    EXPECT_FLOAT_EQ(block[0], 0.5F);
    EXPECT_FLOAT_EQ(block[1], 0.5F);
}

TEST(Chain, LaysOutOneBandPerOctave)
{
    const wiola::testing::Shaping shaping{stereo};

    ASSERT_EQ(shaping.equalizer.num_bands(), 10u);
    EXPECT_EQ(shaping.equalizer.band_center(0), 31.25_Hz);
    EXPECT_EQ(shaping.equalizer.band_center(9), 16_kHz);

    for (std::size_t i = 1; i < shaping.equalizer.num_bands(); ++i)
        EXPECT_DOUBLE_EQ(shaping.equalizer.band_center(i) / shaping.equalizer.band_center(i - 1),
            2.0);
}

/// A band that close to half the sample rate cannot be given its width, so it is not offered.
TEST(Chain, DropsBandsTheFormatCannotCarry)
{
    wiola::testing::Shaping shaping{
        StreamSpec{.sample_rate = 22050_Hz, .num_channels = 2}
    };

    EXPECT_EQ(shaping.equalizer.num_bands(), 9u);
    EXPECT_EQ(shaping.equalizer.band_center(8), 8_kHz);
}

TEST(Chain, LeavesEveryBandWellBelowNyquist)
{
    for (const Frequency rate : {8_kHz, 22050_Hz, 44100_Hz, 48_kHz, 96_kHz}) {
        wiola::testing::Shaping shaping{
            StreamSpec{.sample_rate = rate, .num_channels = 2}
        };

        for (std::size_t i = 0; i < shaping.equalizer.num_bands(); ++i)
            EXPECT_LT(shaping.equalizer.band_center(i), rate / 2.0)
                << "at " << rate.to<Hz>() << " Hz";
    }
}

TEST(Chain, OffersNoBandsForAFormatItCannotUse)
{
    const wiola::testing::Shaping shaping{
        StreamSpec{.sample_rate = Frequency{}, .num_channels = 2}
    };

    EXPECT_EQ(shaping.equalizer.num_bands(), 0u);
}

/// The row of octave bands is a default, not the only layout a chain can be built with.
TEST(Chain, TakesTheLayoutItIsGiven)
{
    const BandLayout thirds{.first = 20_Hz, .count = 31, .ratio = 1.2599210498948732};
    const wiola::testing::Shaping shaping{stereo, thirds};

    EXPECT_EQ(shaping.equalizer.band_center(0), 20_Hz);
    EXPECT_NEAR(shaping.equalizer.band_center(3).to<Hz>(), 40.0, 1e-9);

    // Three of these to an octave, so the last of the 31 is ten octaves up, which is more than
    // 48 kHz can carry: the cap answers the same way whatever the layout asks for.
    EXPECT_NEAR(shaping.equalizer.band_center(30).to<Hz>(), 20480.0, 1e-6);
    EXPECT_EQ(shaping.equalizer.num_bands(), 30u);
}

TEST(Chain, TakesANewFormatWithoutForgettingTheSetting)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.volume.set_gain(0.5F);
    shaping.chain.configure(StreamSpec{.sample_rate = 22050_Hz, .num_channels = 1});

    EXPECT_FLOAT_EQ(shaping.volume.gain(), 0.5F);
    EXPECT_EQ(shaping.equalizer.num_bands(), 9u);

    shaping.chain.configure(stereo);

    EXPECT_EQ(shaping.equalizer.num_bands(), 10u);
}

namespace {

/// Level of `chain`'s output for a sine at `tone` in a stream of `spec`, in decibels against the
/// sine's own level.
double response_db(Chain& chain, StreamSpec spec, Frequency tone)
{
    constexpr std::size_t num_blocks{40};
    constexpr std::size_t num_frames{512};

    wiola::testing::SineSource source{spec, tone, 0.5F};
    std::vector<float> block(spec.samples_per(wiola::audio::Frames{num_frames}));

    double energy{0.0};
    double reference{0.0};

    for (std::size_t i = 0; i < num_blocks; ++i) {
        source.render(block);

        double plain{0.0};

        for (const float sample : block)
            plain += static_cast<double>(sample) * sample;

        chain.process(block);

        double shaped{0.0};

        for (const float sample : block)
            shaped += static_cast<double>(sample) * sample;

        // The first blocks are the filters filling up, and say nothing about the response.
        if (i >= num_blocks / 2) {
            reference += plain;
            energy += shaped;
        }
    }

    return 10.0 * std::log10(energy / reference);
}

} // namespace

TEST(Chain, StartsWithEveryBandFlat)
{
    const wiola::testing::Shaping shaping{stereo};

    for (std::size_t i = 0; i < shaping.equalizer.num_bands(); ++i)
        EXPECT_FLOAT_EQ(shaping.equalizer.band_gain(i), 0.0F);
}

/// A lifted band comes out where it was: the room it needed was taken from everything else.
TEST(Chain, LiftsABandAboveTheRest)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(5, 6.0F);

    const double lifted{response_db(shaping.chain, stereo, shaping.equalizer.band_center(5))};
    const double rest{response_db(shaping.chain, stereo, shaping.equalizer.band_center(0))};

    EXPECT_NEAR(lifted, 0.0, 0.2);
    EXPECT_NEAR(lifted - rest, 6.0, 0.3);
}

TEST(Chain, CutsWhatABandIsSetTo)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(5, -6.0F);

    EXPECT_NEAR(response_db(shaping.chain, stereo, shaping.equalizer.band_center(5)), -6.0, 0.2);
}

/// A band reaches its neighbors, but by much less than it lifts its own center.
TEST(Chain, LeavesTheNextBandMostlyAlone)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(5, 6.0F);

    const double lifted{response_db(shaping.chain, stereo, shaping.equalizer.band_center(5))};
    const double neighbor{response_db(shaping.chain, stereo, shaping.equalizer.band_center(6))};
    const double rest{response_db(shaping.chain, stereo, shaping.equalizer.band_center(0))};

    EXPECT_GT(neighbor, rest);
    EXPECT_LT(neighbor, lifted - 3.0);
}

TEST(Chain, LeavesSamplesUntouchedWhileFlat)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.equalizer.set_band_gain(3, 6.0F);
    shaping.equalizer.set_band_gain(3, 0.0F);
    shaping.chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, RefusesMoreThanABandMayDo)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(2, 40.0F);

    EXPECT_FLOAT_EQ(shaping.equalizer.band_gain(2), 12.0F);

    shaping.equalizer.set_band_gain(2, -40.0F);

    EXPECT_FLOAT_EQ(shaping.equalizer.band_gain(2), -12.0F);
}

/// A gain outlives the format that could not carry its band.
TEST(Chain, RemembersGainsOfBandsAFormatDrops)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(9, 6.0F);
    shaping.chain.configure(StreamSpec{.sample_rate = 22050_Hz, .num_channels = 2});

    ASSERT_EQ(shaping.equalizer.num_bands(), 9u);
    EXPECT_FLOAT_EQ(shaping.equalizer.band_gain(9), 6.0F);

    shaping.chain.configure(stereo);

    const double restored{response_db(shaping.chain, stereo, shaping.equalizer.band_center(9))};
    const double rest{response_db(shaping.chain, stereo, shaping.equalizer.band_center(0))};

    EXPECT_NEAR(restored - rest, 6.0, 0.5);
}

TEST(Chain, StartsWithTheEqualizerOnAndNoPreamp)
{
    const wiola::testing::Shaping shaping{stereo};

    EXPECT_TRUE(shaping.equalizer.enabled());
    EXPECT_FLOAT_EQ(shaping.equalizer.preamp(), 0.0F);
}

/// The cut is what a boost needs, so a lifted band comes out where it was and everything else
/// comes down around it.
TEST(Chain, TakesTheRoomABoostNeeds)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(5, 6.0F);
    // The cut is worked out where the samples are shaped.
    std::array<float, 2> block{};
    shaping.chain.process(block);

    EXPECT_FLOAT_EQ(shaping.equalizer.preamp(), -6.0F);
    EXPECT_NEAR(response_db(shaping.chain, stereo, shaping.equalizer.band_center(5).hz() * 1_Hz),
        0.0, 0.5);
}

/// Only a lift needs room: cutting a band asks for none.
TEST(Chain, TakesNoRoomForACut)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(5, -6.0F);
    std::array<float, 2> block{};
    shaping.chain.process(block);

    EXPECT_FLOAT_EQ(shaping.equalizer.preamp(), 0.0F);
}

/// The room is for the largest lift, not for their sum.
TEST(Chain, TakesTheRoomTheLargestBoostNeeds)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.equalizer.set_band_gain(2, 3.0F);
    shaping.equalizer.set_band_gain(5, 9.0F);
    std::array<float, 2> block{};
    shaping.chain.process(block);

    EXPECT_FLOAT_EQ(shaping.equalizer.preamp(), -9.0F);
}

TEST(Chain, RunsNeitherBandsNorPreampWhileTurnedOff)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.equalizer.set_band_gain(5, 6.0F);
    shaping.equalizer.set_enabled(false);
    shaping.chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, KeepsTheVolumeWhileTurnedOff)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.equalizer.set_band_gain(5, 12.0F);
    shaping.equalizer.set_enabled(false);
    shaping.volume.set_gain(0.5F);
    shaping.chain.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 0.5F);
    EXPECT_FLOAT_EQ(buffer[1], -0.25F);
}

/// A boosted band can ask for more than an output takes, and is not allowed to hand it over.
TEST(Chain, KeepsBoostedSamplesInRange)
{
    wiola::testing::Shaping shaping{stereo};
    wiola::testing::SineSource source{stereo, 1_kHz, 0.95F};
    std::vector<float> block(stereo.samples_per(wiola::audio::Frames{512}));

    shaping.equalizer.set_band_gain(5, 12.0F);

    for (int i = 0; i < 20; ++i) {
        source.render(block);
        shaping.chain.process(block);

        for (const float sample : block)
            EXPECT_LE(std::abs(sample), 1.0F);
    }
}
