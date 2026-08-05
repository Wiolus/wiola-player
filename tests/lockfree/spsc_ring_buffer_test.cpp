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
    const std::array<float, 3> src{1.0F, 2.0F, 3.0F};
    std::array<float, 3> dst{};

    EXPECT_EQ(buffer.push(src), 3u);
    EXPECT_EQ(buffer.pop(dst), 3u);
    EXPECT_EQ(dst, src);
    EXPECT_TRUE(buffer.empty_approx());
}

TEST(SPSCRingBuffer, PushIsShortWhenFull)
{
    wiola::lockfree::SPSCRingBuffer<float> buffer{4};
    const std::array<float, 6> src{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};

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
    const std::array<float, 3> first{1.0F, 2.0F, 3.0F};
    const std::array<float, 3> second{4.0F, 5.0F, 6.0F};
    std::array<float, 3> dst{};

    EXPECT_EQ(buffer.push(first), 3u);
    EXPECT_EQ(buffer.pop(dst), 3u);
    EXPECT_EQ(buffer.push(second), 3u);
    EXPECT_EQ(buffer.pop(dst), 3u);
    EXPECT_EQ(dst, second);
}

} // namespace

TEST(SPSCRingBuffer, ClearDiscardsWhatWasNotRead)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};
    const std::array<int, 4> written{1, 2, 3, 4};

    EXPECT_EQ(buffer.push(written), written.size());
    EXPECT_EQ(buffer.size_approx(), written.size());

    buffer.clear();

    EXPECT_EQ(buffer.size_approx(), 0u);
    EXPECT_TRUE(buffer.empty_approx());

    // The buffer is usable again, and gives back only what came after the clear.
    const std::array<int, 2> again{5, 6};
    EXPECT_EQ(buffer.push(again), again.size());

    std::array<int, 4> read{};
    EXPECT_EQ(buffer.pop(read), again.size());
    EXPECT_EQ(read[0], 5);
    EXPECT_EQ(read[1], 6);
}
