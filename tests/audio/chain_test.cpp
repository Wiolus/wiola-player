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

using wiola::audio::Chain;

std::array<float, 4> samples()
{
    return {1.0F, -0.5F, 0.25F, 0.0F};
}

} // namespace

TEST(Chain, StartsAtFullVolume)
{
    const Chain chain;

    EXPECT_FLOAT_EQ(chain.volume(), 1.0F);
}

/// Full volume is the samples as they arrived, not something multiplied by one.
TEST(Chain, LeavesSamplesUntouchedAtFullVolume)
{
    Chain chain;
    auto buffer{samples()};

    chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, ScalesEverySample)
{
    Chain chain;
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
    Chain chain;
    auto buffer{samples()};

    chain.set_volume(0.0F);
    chain.process(buffer);

    for (const float sample : buffer)
        EXPECT_FLOAT_EQ(sample, 0.0F);
}

TEST(Chain, HoldsTheSettingAcrossCalls)
{
    Chain chain;
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
    Chain chain;
    auto buffer{samples()};

    chain.set_volume(4.0F);

    EXPECT_FLOAT_EQ(chain.volume(), 1.0F);

    chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, TakesAnythingBelowZeroAsSilence)
{
    Chain chain;

    chain.set_volume(-2.0F);

    EXPECT_FLOAT_EQ(chain.volume(), 0.0F);

    chain.set_volume(std::numeric_limits<float>::quiet_NaN());

    EXPECT_FLOAT_EQ(chain.volume(), 0.0F);
}

TEST(Chain, AcceptsNothingToDo)
{
    Chain chain;

    chain.set_volume(0.5F);
    chain.process(std::span<float>{});
}
