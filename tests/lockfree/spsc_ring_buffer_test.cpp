/**
 * @file
 * @brief Unit tests for the single-producer / single-consumer ring buffer.
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

#include <lockfree/spsc_ring_buffer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <optional>

namespace {

using wiola::hw::hardware_destructive_interference_size;

TEST(SPSCRingBuffer, KeepsProducerAndConsumerIndicesApart)
{
    EXPECT_GE(alignof(wiola::lockfree::SPSCRingBuffer<float>),
        hardware_destructive_interference_size);
    EXPECT_GE(sizeof(wiola::lockfree::SPSCRingBuffer<float>),
        2 * hardware_destructive_interference_size);
}

TEST(SPSCRingBuffer, RoundsCapacityUpToPowerOfTwo)
{
    EXPECT_EQ(wiola::lockfree::SPSCRingBuffer<float>{5}.capacity(), 8u);
    EXPECT_EQ(wiola::lockfree::SPSCRingBuffer<float>{8}.capacity(), 8u);
    EXPECT_EQ(wiola::lockfree::SPSCRingBuffer<float>{1}.capacity(), 2u);
}

TEST(SPSCRingBuffer, PopsWhatWasPushed)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array src{1.0F, 2.0F, 3.0F};
    std::array<float, 3> dst{};

    EXPECT_EQ(buffer.push(src), 3u);
    EXPECT_EQ(buffer.pop(dst), 3u);
    EXPECT_EQ(dst, src);
    EXPECT_TRUE(buffer.empty_approx());
}

TEST(SPSCRingBuffer, PushIsShortWhenFull)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array src{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};

    EXPECT_EQ(buffer.push(src), 4u);
    EXPECT_EQ(buffer.push(src), 0u);
}

TEST(SPSCRingBuffer, PopIsEmptyWhenDrained)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    std::array<float, 2> dst{};

    EXPECT_EQ(buffer.pop(dst), 0u);
    EXPECT_FALSE(buffer.try_pop().has_value());
}

TEST(SPSCRingBuffer, WrapsAroundTheEndOfStorage)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array first{1.0F, 2.0F, 3.0F};
    const std::array second{4.0F, 5.0F, 6.0F};
    std::array<float, 3> dst{};

    EXPECT_EQ(buffer.push(first), 3u);
    EXPECT_EQ(buffer.pop(dst), 3u);
    EXPECT_EQ(buffer.push(second), 3u);
    EXPECT_EQ(buffer.pop(dst), 3u);
    EXPECT_EQ(dst, second);
}

TEST(SPSCRingBuffer, ClearDiscardsWhatWasNotRead)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};
    const std::array written{1, 2, 3, 4};

    EXPECT_EQ(buffer.push(written), written.size());
    EXPECT_EQ(buffer.size_approx(), written.size());

    buffer.clear();

    EXPECT_EQ(buffer.size_approx(), 0u);
    EXPECT_TRUE(buffer.empty_approx());

    // The buffer is usable again, and gives back only what came after the clear.
    const std::array again{5, 6};
    EXPECT_EQ(buffer.push(again), again.size());

    std::array<int, 4> read{};
    EXPECT_EQ(buffer.pop(read), again.size());
    EXPECT_EQ(read[0], 5);
    EXPECT_EQ(read[1], 6);
}

TEST(SPSCRingBuffer, TakesAndGivesBackOneValue)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};

    EXPECT_TRUE(buffer.try_push(1.5F));
    EXPECT_EQ(buffer.size_approx(), 1u);

    const std::optional<float> value{buffer.try_pop()};

    ASSERT_TRUE(value.has_value());
    EXPECT_FLOAT_EQ(*value, 1.5F);
    EXPECT_TRUE(buffer.empty_approx());
}

TEST(SPSCRingBuffer, RefusesOneMoreThanFits)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{2};

    EXPECT_TRUE(buffer.try_push(1.0F));
    EXPECT_TRUE(buffer.try_push(2.0F));
    EXPECT_FALSE(buffer.try_push(3.0F));
    EXPECT_EQ(buffer.size_approx(), 2u);
}

/// Any element type reaches the same answer, and takes the same path to it.
TEST(SPSCRingBuffer, IsShortWhenFullWhateverItHolds)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{4};
    const std::array src{1, 2, 3, 4, 5, 6};

    EXPECT_EQ(buffer.push(src), 4u);
    EXPECT_EQ(buffer.push(src), 0u);
}

TEST(SPSCRingBuffer, OffersEverySlotToAWriter)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const auto region = buffer.acquire_write();

    EXPECT_EQ(region.size(), 4u);
    EXPECT_EQ(region.first.size(), 4u);
    EXPECT_TRUE(region.second.empty());
}

TEST(SPSCRingBuffer, CommitsOnlyWhatAWriterUsed)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const auto region = buffer.acquire_write();

    region.first[0] = 1.0F;
    region.first[1] = 2.0F;
    buffer.commit_write(2);

    EXPECT_EQ(buffer.size_approx(), 2u);

    std::array<float, 2> dst{};

    EXPECT_EQ(buffer.pop(dst), 2u);
    EXPECT_FLOAT_EQ(dst[0], 1.0F);
    EXPECT_FLOAT_EQ(dst[1], 2.0F);
}

/// Storage runs out before the free slots do, so what is offered arrives in two pieces.
TEST(SPSCRingBuffer, SplitsAWrittenRegionAcrossTheWrap)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array filler{1.0F, 2.0F, 3.0F};
    std::array<float, 3> drained{};

    ASSERT_EQ(buffer.push(filler), 3u);
    ASSERT_EQ(buffer.pop(drained), 3u);

    const auto region = buffer.acquire_write();

    ASSERT_EQ(region.size(), 4u);
    EXPECT_EQ(region.first.size(), 1u);
    EXPECT_EQ(region.second.size(), 3u);

    region.first[0] = 4.0F;
    region.second[0] = 5.0F;
    region.second[1] = 6.0F;
    region.second[2] = 7.0F;
    buffer.commit_write(region.size());

    std::array<float, 4> dst{};

    EXPECT_EQ(buffer.pop(dst), 4u);
    EXPECT_EQ(dst, (std::array{4.0F, 5.0F, 6.0F, 7.0F}));
}

TEST(SPSCRingBuffer, OffersWhatIsReadyToAReader)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array src{1.0F, 2.0F, 3.0F};

    ASSERT_EQ(buffer.push(src), 3u);

    const auto region = buffer.acquire_read();

    ASSERT_EQ(region.size(), 3u);
    EXPECT_EQ(region.first.size(), 3u);
    EXPECT_TRUE(region.second.empty());
    EXPECT_FLOAT_EQ(region.first[0], 1.0F);
    EXPECT_FLOAT_EQ(region.first[2], 3.0F);
}

TEST(SPSCRingBuffer, CommitsOnlyWhatAReaderTook)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array src{1.0F, 2.0F, 3.0F};

    ASSERT_EQ(buffer.push(src), 3u);

    const auto region = buffer.acquire_read();

    ASSERT_EQ(region.size(), 3u);
    buffer.commit_read(1);

    EXPECT_EQ(buffer.size_approx(), 2u);

    std::array<float, 2> dst{};

    EXPECT_EQ(buffer.pop(dst), 2u);
    EXPECT_FLOAT_EQ(dst[0], 2.0F);
}

TEST(SPSCRingBuffer, SplitsAReadRegionAcrossTheWrap)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array filler{1.0F, 2.0F, 3.0F};
    const std::array written{4.0F, 5.0F, 6.0F, 7.0F};
    std::array<float, 3> drained{};

    ASSERT_EQ(buffer.push(filler), 3u);
    ASSERT_EQ(buffer.pop(drained), 3u);
    ASSERT_EQ(buffer.push(written), 4u);

    const auto region = buffer.acquire_read();

    ASSERT_EQ(region.size(), 4u);
    EXPECT_EQ(region.first.size(), 1u);
    EXPECT_EQ(region.second.size(), 3u);
    EXPECT_FLOAT_EQ(region.first[0], 4.0F);
    EXPECT_FLOAT_EQ(region.second[0], 5.0F);
    EXPECT_FLOAT_EQ(region.second[2], 7.0F);

    buffer.commit_read(region.size());

    EXPECT_TRUE(buffer.empty_approx());
}

TEST(SPSCRingBuffer, LeavesTheBufferAloneWhenNothingIsCommitted)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};

    static_cast<void>(buffer.acquire_write());
    buffer.commit_write(0);

    EXPECT_TRUE(buffer.empty_approx());

    ASSERT_TRUE(buffer.try_push(1.0F));

    static_cast<void>(buffer.acquire_read());
    buffer.commit_read(0);

    EXPECT_EQ(buffer.size_approx(), 1u);
}

/// A reader that arrives before a writer is offered nothing, in one empty piece.
TEST(SPSCRingBuffer, OffersNothingToAReaderOfAnEmptyBuffer)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const auto region = buffer.acquire_read();

    EXPECT_EQ(region.size(), 0u);
    EXPECT_TRUE(region.first.empty());
    EXPECT_TRUE(region.second.empty());
}

TEST(SPSCRingBuffer, OffersNothingToAWriterOfAFullBuffer)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{2};
    const std::array src{1.0F, 2.0F};

    ASSERT_EQ(buffer.push(src), 2u);

    const auto region = buffer.acquire_write();

    EXPECT_EQ(region.size(), 0u);
    EXPECT_TRUE(region.first.empty());
    EXPECT_TRUE(region.second.empty());
}

} // namespace
