/**
 * @file
 * @brief Unit tests for the source a device is pointed at.
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

#include <audio/relay.hpp>

#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace {

using namespace wiola::units::literals;
using wiola::audio::Relay;
using wiola::audio::Source;
using wiola::audio::StreamSpec;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

/// A source of one repeated value, which says how much of a span it is willing to fill.
class Constant final : public Source {
public:
    Constant(float value, std::size_t num_samples) noexcept
        : value_{value}
        , num_samples_{num_samples}
    {
    }

    std::size_t render(std::span<float> interleaved) override
    {
        const std::size_t num_written{std::min(num_samples_, interleaved.size())};

        std::ranges::fill(interleaved.first(num_written), value_);

        return num_written;
    }

    [[nodiscard]] StreamSpec spec() const noexcept override { return stereo; }

private:
    float value_;
    std::size_t num_samples_;
};

} // namespace

TEST(Relay, ReportsTheSpecItWasBuiltFor)
{
    const Relay relay{stereo};

    EXPECT_EQ(relay.spec(), stereo);
}

/// A device asking a relay with no track is asking for what is not there.
TEST(Relay, GivesNothingBeforeItIsPointedAtAnything)
{
    Relay relay{stereo};
    std::array<float, 4> block{1.0F, 1.0F, 1.0F, 1.0F};

    EXPECT_FALSE(relay.pointed());
    EXPECT_EQ(relay.render(block), 0u);

    // A short answer is the device's to fill, so what was in the block is left alone.
    EXPECT_EQ(block[0], 1.0F);
}

TEST(Relay, PlaysWhatItIsPointedAt)
{
    Relay relay{stereo};
    Constant source{0.5F, 4};
    std::array<float, 4> block{};

    relay.point_at(source);

    EXPECT_TRUE(relay.pointed());
    EXPECT_EQ(relay.render(block), 4u);
    EXPECT_EQ(block[0], 0.5F);
    EXPECT_EQ(block[3], 0.5F);
}

/// The point of it: the track changes and the device does not.
TEST(Relay, PlaysWhateverItWasPointedAtLast)
{
    Relay relay{stereo};
    Constant first{0.25F, 4};
    Constant next{0.75F, 4};
    std::array<float, 4> block{};

    relay.point_at(first);
    ASSERT_EQ(relay.render(block), 4u);
    ASSERT_EQ(block[0], 0.25F);

    relay.point_at(next);

    EXPECT_EQ(relay.render(block), 4u);
    EXPECT_EQ(block[0], 0.75F);
}

TEST(Relay, GivesNothingOnceItIsPointedAtNothing)
{
    Relay relay{stereo};
    Constant source{0.5F, 4};
    std::array<float, 4> block{};

    relay.point_at(source);
    ASSERT_EQ(relay.render(block), 4u);

    relay.point_at_nothing();

    EXPECT_FALSE(relay.pointed());
    EXPECT_EQ(relay.render(block), 0u);
}

/// A track running out is a short answer, and must read as one rather than as a full block of
/// silence: what is left of the block belongs to whoever asked.
TEST(Relay, PassesOnAShortAnswer)
{
    Relay relay{stereo};
    Constant nearly_spent{0.5F, 2};
    std::array<float, 4> block{};

    relay.point_at(nearly_spent);

    EXPECT_EQ(relay.render(block), 2u);
    EXPECT_EQ(block[1], 0.5F);
    EXPECT_EQ(block[2], 0.0F);
}
