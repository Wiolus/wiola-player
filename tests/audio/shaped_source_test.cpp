/**
 * @file
 * @brief Unit tests for the shaped source.
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

#include <audio/shaped_source.hpp>
#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <fixtures/shaping.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace {

using wiola::audio::Chain;
using namespace wiola::units;
using wiola::audio::ShapedSource;
using wiola::audio::Source;
using wiola::audio::StreamSpec;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};
constexpr StreamSpec mono{.sample_rate = 44100_Hz, .num_channels = 1};

/// Fills what it gives with `value`, and gives at most `num_samples` of it.
class FixedSource final : public Source {
public:
    FixedSource(StreamSpec spec, float value, std::size_t num_samples) noexcept
        : spec_{spec}
        , value_{value}
        , num_samples_{num_samples}
    {
    }

    std::size_t render(std::span<float> interleaved) override
    {
        const std::size_t num_rendered{std::min(num_samples_, interleaved.size())};

        std::ranges::fill(interleaved.first(num_rendered), value_);

        return num_rendered;
    }

    [[nodiscard]] StreamSpec spec() const noexcept override { return spec_; }

private:
    StreamSpec spec_;
    float value_;
    std::size_t num_samples_;
};

TEST(ShapedSource, ReportsTheSpecOfWhatItWraps)
{
    FixedSource inner{mono, 1.0F, 8};
    wiola::testing::Shaping shaping{mono};
    ShapedSource source{inner, shaping.chain};

    EXPECT_EQ(source.spec(), mono);
}

TEST(ShapedSource, LeavesSamplesAloneWhileNothingIsAsked)
{
    FixedSource inner{stereo, 0.5F, 8};
    wiola::testing::Shaping shaping{stereo};
    ShapedSource source{inner, shaping.chain};

    std::array<float, 8> block{};

    EXPECT_EQ(source.render(block), block.size());

    for (const float sample : block)
        EXPECT_FLOAT_EQ(sample, 0.5F);
}

TEST(ShapedSource, AppliesTheChain)
{
    FixedSource inner{stereo, 0.8F, 8};
    wiola::testing::Shaping shaping{stereo};
    ShapedSource source{inner, shaping.chain};

    shaping.volume.set_gain(0.5F);

    std::array<float, 8> block{};
    source.render(block);

    for (const float sample : block)
        EXPECT_FLOAT_EQ(sample, 0.4F);
}

TEST(ShapedSource, GivesWhatItWasGiven)
{
    FixedSource inner{stereo, 0.5F, 4};
    wiola::testing::Shaping shaping{stereo};
    ShapedSource source{inner, shaping.chain};

    std::array<float, 8> block{};

    EXPECT_EQ(source.render(block), 4U);
}

/// Past what the inner source gave there is nothing to shape, and what is there belongs to
/// whoever asked.
TEST(ShapedSource, ShapesOnlyWhatTheInnerSourceGave)
{
    FixedSource inner{stereo, 0.8F, 4};
    wiola::testing::Shaping shaping{stereo};
    ShapedSource source{inner, shaping.chain};

    shaping.volume.set_gain(0.5F);

    std::array<float, 8> block{};
    block.fill(42.0F);

    ASSERT_EQ(source.render(block), 4U);

    EXPECT_FLOAT_EQ(block[3], 0.4F);
    EXPECT_FLOAT_EQ(block[4], 42.0F);
}

TEST(ShapedSource, TakesTheSettingAsItIsWhenAsked)
{
    FixedSource inner{stereo, 0.8F, 8};
    wiola::testing::Shaping shaping{stereo};
    ShapedSource source{inner, shaping.chain};

    std::array<float, 8> first{};
    std::array<float, 8> second{};

    source.render(first);
    shaping.volume.set_gain(0.0F);
    source.render(second);

    EXPECT_FLOAT_EQ(first[0], 0.8F);
    EXPECT_FLOAT_EQ(second[0], 0.0F);
}

TEST(ShapedSource, AnswersAsASource)
{
    FixedSource inner{stereo, 0.5F, 8};
    wiola::testing::Shaping shaping{stereo};
    ShapedSource shaped{inner, shaping.chain};
    Source& source{shaped};

    std::array<float, 8> block{};

    EXPECT_EQ(source.render(block), block.size());
    EXPECT_EQ(source.spec(), stereo);
}

} // namespace
