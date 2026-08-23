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

#include <audio/chain.hpp>

#include <gtest/gtest.h>

#include <array>
#include <limits>
#include <span>

namespace {

using wiola::audio::BandLayout;
using wiola::audio::Chain;
using wiola::audio::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

std::array<float, 4> samples()
{
    return {1.0F, -0.5F, 0.25F, 0.0F};
}

} // namespace

TEST(Chain, StartsAtFullVolume)
{
    const Chain chain{stereo};

    EXPECT_FLOAT_EQ(chain.volume(), 1.0F);
}

/// Full volume is the samples as they arrived, not something multiplied by one.
TEST(Chain, LeavesSamplesUntouchedAtFullVolume)
{
    Chain chain{stereo};
    auto buffer{samples()};

    chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, ScalesEverySample)
{
    Chain chain{stereo};
    auto buffer{samples()};

    chain.set_volume(0.5F);
    chain.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 0.5F);
    EXPECT_FLOAT_EQ(buffer[1], -0.25F);
    EXPECT_FLOAT_EQ(buffer[2], 0.125F);
    EXPECT_FLOAT_EQ(buffer[3], 0.0F);
}

TEST(Chain, SilencesAtZero)
{
    Chain chain{stereo};
    auto buffer{samples()};

    chain.set_volume(0.0F);
    chain.process(buffer);

    for (const float sample : buffer)
        EXPECT_FLOAT_EQ(sample, 0.0F);
}

TEST(Chain, HoldsTheSettingAcrossCalls)
{
    Chain chain{stereo};
    auto first{samples()};
    auto second{samples()};

    chain.set_volume(0.25F);
    chain.process(first);
    chain.process(second);

    EXPECT_EQ(first, second);
    EXPECT_FLOAT_EQ(chain.volume(), 0.25F);
}

TEST(Chain, RefusesToAmplify)
{
    Chain chain{stereo};
    auto buffer{samples()};

    chain.set_volume(4.0F);

    EXPECT_FLOAT_EQ(chain.volume(), 1.0F);

    chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, TakesAnythingBelowZeroAsSilence)
{
    Chain chain{stereo};

    chain.set_volume(-2.0F);

    EXPECT_FLOAT_EQ(chain.volume(), 0.0F);

    chain.set_volume(std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ(chain.volume(), 0.0F);
}

TEST(Chain, AcceptsNothingToDo)
{
    Chain chain{stereo};

    chain.set_volume(0.5F);
    chain.process(std::span<float>{});
}

TEST(Chain, KeepsTheFormatItWasGiven)
{
    const Chain chain{stereo};

    EXPECT_EQ(chain.spec(), stereo);
}

TEST(Chain, LaysOutOneBandPerOctave)
{
    const Chain chain{stereo};

    ASSERT_EQ(chain.num_bands(), 10u);
    EXPECT_EQ(chain.band_center(0), 31.25_Hz);
    EXPECT_EQ(chain.band_center(9), 16_kHz);

    for (std::size_t i = 1; i < chain.num_bands(); ++i)
        EXPECT_DOUBLE_EQ(chain.band_center(i) / chain.band_center(i - 1), 2.0);
}

/// A band that close to half the sample rate cannot be given its width, so it is not offered.
TEST(Chain, DropsBandsTheFormatCannotCarry)
{
    const Chain chain{
        StreamSpec{.sample_rate = 22050_Hz, .num_channels = 2}
    };

    EXPECT_EQ(chain.num_bands(), 9u);
    EXPECT_EQ(chain.band_center(8), 8_kHz);
}

TEST(Chain, LeavesEveryBandWellBelowNyquist)
{
    for (const Frequency rate : {8_kHz, 22050_Hz, 44100_Hz, 48_kHz, 96_kHz}) {
        const Chain chain{
            StreamSpec{.sample_rate = rate, .num_channels = 2}
        };

        for (std::size_t i = 0; i < chain.num_bands(); ++i)
            EXPECT_LT(chain.band_center(i), rate / 2.0) << "at " << rate.to<Hz>() << " Hz";
    }
}

TEST(Chain, OffersNoBandsForAFormatItCannotUse)
{
    const Chain chain{
        StreamSpec{.sample_rate = Frequency{}, .num_channels = 2}
    };

    EXPECT_EQ(chain.num_bands(), 0u);
}

/// The row of octave bands is a default, not the only layout a chain can be built with.
TEST(Chain, TakesTheLayoutItIsGiven)
{
    const BandLayout thirds{.first = 20_Hz, .count = 31, .ratio = 1.2599210498948732};
    const Chain chain{stereo, thirds};

    EXPECT_EQ(chain.band_center(0), 20_Hz);
    EXPECT_NEAR(chain.band_center(3).to<Hz>(), 40.0, 1e-9);

    // Three of these to an octave, so the last of the 31 is ten octaves up, which is more than
    // 48 kHz can carry: the cap answers the same way whatever the layout asks for.
    EXPECT_NEAR(chain.band_center(30).to<Hz>(), 20480.0, 1e-6);
    EXPECT_EQ(chain.num_bands(), 30u);
}

TEST(Chain, TakesANewFormatWithoutForgettingTheSetting)
{
    Chain chain{stereo};

    chain.set_volume(0.5F);
    chain.configure(StreamSpec{.sample_rate = 22050_Hz, .num_channels = 1});

    EXPECT_FLOAT_EQ(chain.volume(), 0.5F);
    EXPECT_EQ(chain.spec().num_channels, 1u);
    EXPECT_EQ(chain.num_bands(), 9u);

    chain.configure(stereo);

    EXPECT_EQ(chain.num_bands(), 10u);
}
