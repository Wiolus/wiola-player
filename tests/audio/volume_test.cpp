/**
 * @file
 * @brief Tests for how loud the output is.
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

#include <audio/dsp/volume.hpp>

#include <audio/dsp/tuning.hpp>
#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <limits>
#include <span>

namespace {

using wiola::audio::Volume;
using wiola::pcm::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

auto samples()
{
    return std::array{1.0F, -0.5F, 0.25F, 0.0F};
}

} // namespace

TEST(Volume, StartsAtFullVolume)
{
    const Volume volume;

    EXPECT_FLOAT_EQ(volume.gain(), 1.0F);
}

/// Full volume is the samples as they arrived, not something multiplied by one.
TEST(Volume, LeavesSamplesUntouchedAtFullVolume)
{
    Volume volume;
    auto buffer{samples()};

    volume.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Volume, ScalesEverySample)
{
    Volume volume;
    auto buffer{samples()};

    volume.set_gain(0.5F);
    volume.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 0.5F);
    EXPECT_FLOAT_EQ(buffer[1], -0.25F);
    EXPECT_FLOAT_EQ(buffer[2], 0.125F);
    EXPECT_FLOAT_EQ(buffer[3], 0.0F);
}

TEST(Volume, SilencesAtZero)
{
    Volume volume;
    auto buffer{samples()};

    volume.set_gain(0.0F);
    volume.process(buffer);

    for (const float sample : buffer)
        EXPECT_FLOAT_EQ(sample, 0.0F);
}

TEST(Volume, HoldsTheSettingAcrossCalls)
{
    Volume volume;
    auto first{samples()};
    auto second{samples()};

    volume.set_gain(0.25F);
    volume.process(first);
    volume.process(second);

    EXPECT_EQ(first, second);
    EXPECT_FLOAT_EQ(volume.gain(), 0.25F);
}

/// A listener may ask for more than arrived, but only so much more.
TEST(Volume, RefusesMoreThanTheMostItMayLift)
{
    Volume volume;

    volume.set_gain(4.0F);

    EXPECT_FLOAT_EQ(volume.gain(), wiola::audio::tuning::max_volume_boost);
}

TEST(Volume, TakesAnythingBelowZeroAsSilence)
{
    Volume volume;

    volume.set_gain(-2.0F);

    EXPECT_FLOAT_EQ(volume.gain(), 0.0F);

    volume.set_gain(std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ(volume.gain(), 0.0F);
}

/// Holding a sample to what an output takes is the chain's, not this one's: a step that lifts
/// hands over what it lifted, and the ceiling is kept once at the end.
TEST(Volume, LiftsPastFullScaleWithoutHoldingIt)
{
    Volume volume;
    auto buffer{samples()};

    volume.set_gain(1.4F);
    volume.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 1.4F);
    EXPECT_FLOAT_EQ(buffer[1], -0.7F);
    EXPECT_FLOAT_EQ(buffer[2], 0.35F);
}

/// Nothing here follows the stream, so being told about one changes nothing.
TEST(Volume, IsUnchangedByTheFormat)
{
    Volume volume;
    auto buffer{samples()};

    volume.set_gain(0.5F);
    volume.configure(stereo);
    volume.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 0.5F);
}

TEST(Volume, AcceptsNothingToDo)
{
    Volume volume;
    std::array block{1.0F, 1.0F};

    volume.set_gain(0.5F);
    volume.process(std::span<float>{});

    // Nothing to do is not something to recover from: what comes after is shaped as it would
    // have been.
    volume.process(block);

    EXPECT_FLOAT_EQ(block[0], 0.5F);
    EXPECT_FLOAT_EQ(block[1], 0.5F);
}
