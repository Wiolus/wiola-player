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
#include <atomic>
#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <stop_token>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;
using wiola::hw::hardware_destructive_interference_size;

/// How long a threaded test waits for the other side before giving up, so a buffer that stops
/// carrying anything fails rather than hangs.
constexpr std::chrono::seconds patience{30};

/// Small, so that a run of any length wraps many times.
constexpr std::size_t threaded_capacity{64};

constexpr int num_threaded_elements{200000};

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

TEST(SPSCRingBuffer, DiscardsWhatWasNotRead)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};
    const std::array written{1, 2, 3, 4};

    EXPECT_EQ(buffer.push(written), written.size());

    buffer.mark_discard();

    std::array<int, 4> read{};
    EXPECT_EQ(buffer.pop(read), 0u);

    // The buffer is usable again, and gives back only what came after the mark.
    const std::array again{5, 6};
    EXPECT_EQ(buffer.push(again), again.size());
    EXPECT_EQ(buffer.pop(read), again.size());
    EXPECT_EQ(read[0], 5);
    EXPECT_EQ(read[1], 6);
}

/// What the producer writes after marking is kept: the mark is a place, not a switch.
TEST(SPSCRingBuffer, KeepsWhatWasWrittenAfterAMark)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};
    const std::array stale{1, 2, 3, 4};
    const std::array fresh{5, 6};

    EXPECT_EQ(buffer.push(stale), stale.size());
    buffer.mark_discard();
    EXPECT_EQ(buffer.push(fresh), fresh.size());

    std::array<int, 6> read{};
    EXPECT_EQ(buffer.pop(read), fresh.size());
    EXPECT_EQ(read[0], 5);
    EXPECT_EQ(read[1], 6);
}

/// A consumer that was away for several of them is left at the last one, not walked through each.
TEST(SPSCRingBuffer, AppliesOnlyTheLatestOfSeveralMarks)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};

    EXPECT_EQ(buffer.push(std::array{1, 2}), 2u);
    buffer.mark_discard();
    EXPECT_EQ(buffer.push(std::array{3, 4}), 2u);
    buffer.mark_discard();
    EXPECT_EQ(buffer.push(std::array{5}), 1u);

    std::array<int, 8> read{};
    EXPECT_EQ(buffer.pop(read), 1u);
    EXPECT_EQ(read[0], 5);
}

TEST(SPSCRingBuffer, MarksNothingOnAnEmptyBuffer)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};

    buffer.mark_discard();
    buffer.mark_discard();

    EXPECT_EQ(buffer.push(std::array{1, 2}), 2u);

    std::array<int, 4> read{};
    EXPECT_EQ(buffer.pop(read), 2u);
    EXPECT_EQ(read[0], 1);
}

/// The point of marking rather than clearing: the space belongs to the consumer until it has
/// stepped over it, so a producer that marks while nothing is reading gains nothing to write in.
TEST(SPSCRingBuffer, FreesTheSpaceOnlyOnceTheConsumerHasRead)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{4};
    const std::array full{1, 2, 3, 4};

    EXPECT_EQ(buffer.push(full), full.size());

    buffer.mark_discard();

    EXPECT_EQ(buffer.push(std::array{5}), 0u);
    EXPECT_EQ(buffer.size_approx(), full.size());

    std::array<int, 4> read{};
    EXPECT_EQ(buffer.pop(read), 0u);

    EXPECT_TRUE(buffer.empty_approx());
    EXPECT_EQ(buffer.push(std::array{5}), 1u);
    EXPECT_EQ(buffer.pop(read), 1u);
    EXPECT_EQ(read[0], 5);
}

/// Indices run free and are only masked on access, so a mark has to survive the wrap.
TEST(SPSCRingBuffer, DiscardsAcrossTheWrap)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{4};
    std::array<int, 4> read{};

    for (int round = 0; round < 3; ++round) {
        EXPECT_EQ(buffer.push(std::array{1, 2, 3}), 3u);
        EXPECT_EQ(buffer.pop(read), 3u);
    }

    EXPECT_EQ(buffer.push(std::array{7, 8}), 2u);
    buffer.mark_discard();
    EXPECT_EQ(buffer.pop(read), 0u);

    EXPECT_EQ(buffer.push(std::array{9}), 1u);
    EXPECT_EQ(buffer.pop(read), 1u);
    EXPECT_EQ(read[0], 9);
}

/// A region already handed out stays valid; the mark is applied when the next one is asked for.
TEST(SPSCRingBuffer, LeavesAReadRegionAlreadyHandedOutAlone)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{8};

    EXPECT_EQ(buffer.push(std::array{1, 2, 3, 4}), 4u);

    const auto region = buffer.acquire_read();
    ASSERT_EQ(region.size(), 4u);

    buffer.mark_discard();

    EXPECT_EQ(region.first[0], 1);
    buffer.commit_read(2);

    EXPECT_EQ(buffer.acquire_read().size(), 0u);
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

/// What the buffer is for: one thread writing while another reads, with every element arriving
/// once and in the order it was written.
TEST(SPSCRingBuffer, CarriesEveryElementBetweenTwoThreads)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{threaded_capacity};

    const std::jthread producer{[&buffer](const std::stop_token& stop) {
        for (int value = 0; value < num_threaded_elements && !stop.stop_requested();) {
            if (buffer.try_push(value))
                ++value;
            else
                std::this_thread::yield();
        }
    }};

    const auto deadline = std::chrono::steady_clock::now() + patience;

    for (int expected = 0; expected < num_threaded_elements;) {
        const std::optional<int> value{buffer.try_pop()};

        if (!value.has_value()) {
            ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "stalled at " << expected;
            std::this_thread::yield();
            continue;
        }

        ASSERT_EQ(*value, expected);
        ++expected;
    }
}

/// The same, through the regions rather than element by element.
TEST(SPSCRingBuffer, CarriesEveryElementBetweenTwoThreadsInRegions)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{threaded_capacity};

    const std::jthread producer{[&buffer](const std::stop_token& stop) {
        int value{0};

        while (value < num_threaded_elements && !stop.stop_requested()) {
            const auto region = buffer.acquire_write();

            if (region.size() == 0) {
                std::this_thread::yield();
                continue;
            }

            std::size_t written{0};

            for (std::span<int> part : {region.first, region.second})
                for (int& slot : part)
                    if (value < num_threaded_elements) {
                        slot = value++;
                        ++written;
                    }

            buffer.commit_write(written);
        }
    }};

    const auto deadline = std::chrono::steady_clock::now() + patience;
    int expected{0};

    while (expected < num_threaded_elements) {
        const auto region = buffer.acquire_read();

        if (region.size() == 0) {
            ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "stalled at " << expected;
            std::this_thread::yield();
            continue;
        }

        for (std::span<const int> part : {region.first, region.second})
            for (const int value : part)
                ASSERT_EQ(value, expected++);

        buffer.commit_read(region.size());
    }
}

} // namespace

/// A discard is the one thing that crosses between the two sides, so it is the one thing worth
/// hammering: the consumer must never be handed a value the producer had already marked stale.
TEST(SPSCRingBuffer, NeverCarriesWhatWasMarkedStale)
{
    wiola::lockfree::SPSCRingBuffer<int> buffer{threaded_capacity};
    std::atomic<int> marked_through{-1};

    const std::jthread producer{[&buffer, &marked_through](const std::stop_token& stop) {
        for (int value = 0; value < num_threaded_elements && !stop.stop_requested();) {
            if (!buffer.try_push(value)) {
                std::this_thread::yield();
                continue;
            }

            // Every so often, everything written so far is declared stale. The bar is published
            // after the mark, so a consumer that reads the bar has certainly seen the mark.
            if (value % 1000 == 999) {
                buffer.mark_discard();
                marked_through.store(value, std::memory_order_release);
            }

            ++value;
        }
    }};

    const auto deadline = std::chrono::steady_clock::now() + patience;
    int num_read{0};
    int num_skipped{0};
    int last{-1};

    while (num_read < num_threaded_elements / 2) {
        const int bar{marked_through.load(std::memory_order_acquire)};
        const std::optional<int> value{buffer.try_pop()};

        if (!value.has_value()) {
            ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "stalled at " << num_read;
            std::this_thread::yield();
            continue;
        }

        // Nothing comes back twice, nothing comes back out of order, and nothing comes back
        // from before a mark this read has already stepped over.
        ASSERT_GT(*value, last);
        ASSERT_GT(*value, bar) << "read " << *value << " after everything through " << bar
                               << " was marked stale";

        if (*value != last + 1)
            ++num_skipped;

        last = *value;
        ++num_read;
    }

    // Otherwise the run said nothing: every mark landed on an empty buffer and discarded nothing.
    EXPECT_GT(num_skipped, 0) << "no mark ever discarded anything";
}
