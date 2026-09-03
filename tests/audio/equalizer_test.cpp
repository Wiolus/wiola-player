/**
 * @file
 * @brief Tests for lift and cut, one band at a time.
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

#include <audio/dsp/equalizer.hpp>

#include <fakes/tone.hpp>
#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

using wiola::audio::BandLayout;
using wiola::audio::Equalizer;
using wiola::pcm::Frames;
using wiola::pcm::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

auto samples()
{
    return std::array{1.0F, -0.5F, 0.25F, 0.0F};
}

/// Level `equalizer` leaves a sine at `tone` with, in decibels against the sine's own level.
double response_db(Equalizer& equalizer, StreamSpec spec, Frequency tone)
{
    constexpr std::size_t num_blocks{40};
    constexpr std::size_t num_frames{512};

    wiola::testing::SineSource source{spec, tone, 0.5F};
    std::vector<float> block(spec.samples_per(Frames{num_frames}));

    double energy{0.0};
    double reference{0.0};

    for (std::size_t i = 0; i < num_blocks; ++i) {
        source.render(block);

        double plain{0.0};

        for (const float sample : block)
            plain += static_cast<double>(sample) * sample;

        equalizer.process(block);

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

/// Works the settings out where the samples are shaped, which is where the preamp is decided.
void run_once(Equalizer& equalizer)
{
    std::array<float, 2> block{};

    equalizer.process(block);
}

} // namespace

TEST(Equalizer, LaysOutOneBandPerOctave)
{
    const Equalizer equalizer{stereo};

    ASSERT_EQ(equalizer.num_bands(), 10u);
    EXPECT_EQ(equalizer.band_center(0), 31.25_Hz);
    EXPECT_EQ(equalizer.band_center(9), 16_kHz);

    for (std::size_t i = 1; i < equalizer.num_bands(); ++i)
        EXPECT_DOUBLE_EQ(equalizer.band_center(i) / equalizer.band_center(i - 1), 2.0);
}

/// A band that close to half the sample rate cannot be given its width, so it is not offered.
TEST(Equalizer, DropsBandsTheFormatCannotCarry)
{
    const Equalizer equalizer{
        StreamSpec{.sample_rate = 22050_Hz, .num_channels = 2}
    };

    EXPECT_EQ(equalizer.num_bands(), 9u);
    EXPECT_EQ(equalizer.band_center(8), 8_kHz);
}

TEST(Equalizer, LeavesEveryBandWellBelowNyquist)
{
    for (const Frequency rate : {8_kHz, 22050_Hz, 44100_Hz, 48_kHz, 96_kHz}) {
        const Equalizer equalizer{
            StreamSpec{.sample_rate = rate, .num_channels = 2}
        };

        for (std::size_t i = 0; i < equalizer.num_bands(); ++i)
            EXPECT_LT(equalizer.band_center(i), rate / 2.0) << "at " << rate.to<Hz>() << " Hz";
    }
}

TEST(Equalizer, OffersNoBandsForAFormatItCannotUse)
{
    const Equalizer equalizer{
        StreamSpec{.sample_rate = Frequency{}, .num_channels = 2}
    };

    EXPECT_EQ(equalizer.num_bands(), 0u);
}

/// The row of octave bands is a default, not the only layout it can be built with.
TEST(Equalizer, TakesTheLayoutItIsGiven)
{
    const BandLayout thirds{.first = 20_Hz, .count = 31, .ratio = 1.2599210498948732};
    const Equalizer equalizer{stereo, thirds};

    EXPECT_EQ(equalizer.band_center(0), 20_Hz);
    EXPECT_NEAR(equalizer.band_center(3).to<Hz>(), 40.0, 1e-9);

    // Three of these to an octave, so the last of the 31 is ten octaves up, which is more than
    // 48 kHz can carry: the cap answers the same way whatever the layout asks for.
    EXPECT_NEAR(equalizer.band_center(30).to<Hz>(), 20480.0, 1e-6);
    EXPECT_EQ(equalizer.num_bands(), 30u);
}

TEST(Equalizer, StartsFlatAndOn)
{
    const Equalizer equalizer{stereo};

    EXPECT_TRUE(equalizer.enabled());
    EXPECT_FLOAT_EQ(equalizer.preamp(), 0.0F);

    for (std::size_t i = 0; i < equalizer.num_bands(); ++i)
        EXPECT_FLOAT_EQ(equalizer.band_gain(i), 0.0F) << "band " << i;
}

TEST(Equalizer, LeavesSamplesUntouchedWhileFlat)
{
    Equalizer equalizer{stereo};
    auto buffer{samples()};

    equalizer.set_band_gain(3, 6.0F);
    equalizer.set_band_gain(3, 0.0F);
    equalizer.process(buffer);

    EXPECT_EQ(buffer, samples());
}

/// A lifted band comes out where it was: the room it needed was taken from everything else.
TEST(Equalizer, LiftsABandAboveTheRest)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(5, 6.0F);

    const double lifted{response_db(equalizer, stereo, equalizer.band_center(5))};
    const double rest{response_db(equalizer, stereo, equalizer.band_center(0))};

    EXPECT_NEAR(lifted, 0.0, 0.2);
    EXPECT_NEAR(lifted - rest, 6.0, 0.3);
}

TEST(Equalizer, CutsWhatABandIsSetTo)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(5, -6.0F);

    EXPECT_NEAR(response_db(equalizer, stereo, equalizer.band_center(5)), -6.0, 0.2);
}

/// A band reaches its neighbors, but by much less than it lifts its own center.
TEST(Equalizer, LeavesTheNextBandMostlyAlone)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(5, 6.0F);

    const double lifted{response_db(equalizer, stereo, equalizer.band_center(5))};
    const double neighbor{response_db(equalizer, stereo, equalizer.band_center(6))};
    const double rest{response_db(equalizer, stereo, equalizer.band_center(0))};

    EXPECT_GT(neighbor, rest);
    EXPECT_LT(neighbor, lifted - 3.0);
}

TEST(Equalizer, RefusesMoreThanABandMayDo)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(2, 40.0F);

    EXPECT_FLOAT_EQ(equalizer.band_gain(2), 12.0F);

    equalizer.set_band_gain(2, -40.0F);

    EXPECT_FLOAT_EQ(equalizer.band_gain(2), -12.0F);
}

/// A gain outlives the format that could not carry its band.
TEST(Equalizer, RemembersGainsOfBandsAFormatDrops)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(9, 6.0F);
    equalizer.configure(StreamSpec{.sample_rate = 22050_Hz, .num_channels = 2});

    ASSERT_EQ(equalizer.num_bands(), 9u);
    EXPECT_FLOAT_EQ(equalizer.band_gain(9), 6.0F);

    equalizer.configure(stereo);

    const double restored{response_db(equalizer, stereo, equalizer.band_center(9))};
    const double rest{response_db(equalizer, stereo, equalizer.band_center(0))};

    EXPECT_NEAR(restored - rest, 6.0, 0.5);
}

/// The cut is what a boost needs, so a lifted band comes out where it was and everything else
/// comes down around it.
TEST(Equalizer, TakesTheRoomABoostNeeds)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(5, 6.0F);
    run_once(equalizer);

    EXPECT_FLOAT_EQ(equalizer.preamp(), -6.0F);
    EXPECT_NEAR(response_db(equalizer, stereo, equalizer.band_center(5)), 0.0, 0.5);
}

/// Only a lift needs room: cutting a band asks for none.
TEST(Equalizer, TakesNoRoomForACut)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(5, -6.0F);
    run_once(equalizer);

    EXPECT_FLOAT_EQ(equalizer.preamp(), 0.0F);
}

/// The room is for the largest lift, not for their sum.
TEST(Equalizer, TakesTheRoomTheLargestBoostNeeds)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(2, 3.0F);
    equalizer.set_band_gain(5, 9.0F);
    run_once(equalizer);

    EXPECT_FLOAT_EQ(equalizer.preamp(), -9.0F);
}

TEST(Equalizer, RunsNeitherBandsNorPreampWhileTurnedOff)
{
    Equalizer equalizer{stereo};
    auto buffer{samples()};

    equalizer.set_band_gain(5, 6.0F);
    equalizer.set_enabled(false);
    equalizer.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Equalizer, AcceptsNothingToDo)
{
    Equalizer equalizer{stereo};

    equalizer.set_band_gain(5, 6.0F);
    equalizer.process(std::span<float>{});

    EXPECT_FLOAT_EQ(equalizer.preamp(), -6.0F);
}
