/**
 * @file
 * @brief Unit tests for the ring buffer source.
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

#include <audio/buffer_source.hpp>
#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using wiola::audio::BufferSource;
using namespace wiola::units;
using wiola::audio::Source;
using wiola::audio::StreamSpec;
using wiola::lockfree::SPSCRingBuffer;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

auto counted(std::size_t count)
{
    std::vector<float> samples(count);

    for (std::size_t i = 0; i < count; ++i)
        samples[i] = static_cast<float>(i);

    return samples;
}

TEST(BufferSource, ReportsTheSpecItWasGiven)
{
    SPSCRingBuffer<float> buffer{64};
    BufferSource source{stereo, buffer.consumer()};

    EXPECT_EQ(source.spec(), stereo);
}

TEST(BufferSource, GivesNothingFromAnEmptyBuffer)
{
    SPSCRingBuffer<float> buffer{64};
    BufferSource source{stereo, buffer.consumer()};

    std::array<float, 8> block{};
    block.fill(42.0F);

    EXPECT_EQ(source.render(block), 0U);
    EXPECT_FLOAT_EQ(block[0], 42.0F);
}

TEST(BufferSource, GivesWhatWasPushedInOrder)
{
    SPSCRingBuffer<float> buffer{64};
    auto producer{buffer.producer()};
    BufferSource source{stereo, buffer.consumer()};

    const auto pushed{counted(8)};
    ASSERT_EQ(producer.push(pushed), pushed.size());

    std::array<float, 8> block{};

    EXPECT_EQ(source.render(block), block.size());

    for (std::size_t i = 0; i < block.size(); ++i)
        EXPECT_FLOAT_EQ(block[i], static_cast<float>(i));
}

/// Short is how a source says the writer has not kept up, which is what a device counts as an
/// underrun.
TEST(BufferSource, IsShortWhenTheBufferHoldsLess)
{
    SPSCRingBuffer<float> buffer{64};
    auto producer{buffer.producer()};
    BufferSource source{stereo, buffer.consumer()};

    const auto pushed{counted(4)};
    ASSERT_EQ(producer.push(pushed), pushed.size());

    std::array<float, 8> block{};
    block.fill(42.0F);

    EXPECT_EQ(source.render(block), 4U);
    EXPECT_FLOAT_EQ(block[3], 3.0F);
    EXPECT_FLOAT_EQ(block[4], 42.0F);
}

TEST(BufferSource, TakesOnlyWhatWasAskedFor)
{
    SPSCRingBuffer<float> buffer{64};
    auto producer{buffer.producer()};
    BufferSource source{stereo, buffer.consumer()};

    const auto pushed{counted(16)};
    ASSERT_EQ(producer.push(pushed), pushed.size());

    std::array<float, 4> block{};

    EXPECT_EQ(source.render(block), block.size());
    EXPECT_EQ(buffer.size_approx(), 12U);
}

TEST(BufferSource, AnswersAsASource)
{
    SPSCRingBuffer<float> buffer{64};
    auto producer{buffer.producer()};
    BufferSource buffered{stereo, buffer.consumer()};
    Source& source{buffered};

    const auto pushed{counted(4)};
    ASSERT_EQ(producer.push(pushed), pushed.size());

    std::array<float, 4> block{};

    EXPECT_EQ(source.render(block), block.size());
    EXPECT_EQ(source.spec(), stereo);
}

TEST(BufferSource, AcceptsNothingToFill)
{
    SPSCRingBuffer<float> buffer{64};
    auto producer{buffer.producer()};
    BufferSource source{stereo, buffer.consumer()};

    const auto pushed{counted(4)};
    ASSERT_EQ(producer.push(pushed), pushed.size());

    EXPECT_EQ(source.render(std::span<float>{}), 0U);
    EXPECT_EQ(buffer.size_approx(), pushed.size());
}

} // namespace
