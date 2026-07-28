/**
 * @file
 * @brief Unit tests for the cache line size constant.
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

#include <core/cache_line.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace {

using wiola::hw::hardware_destructive_interference_size;

TEST(CacheLine, IsAPowerOfTwo)
{
    EXPECT_TRUE(std::has_single_bit(hardware_destructive_interference_size));
}

TEST(CacheLine, HoldsAnIndexAndStaysWithinKnownLineSizes)
{
    EXPECT_GE(hardware_destructive_interference_size, sizeof(std::atomic<std::size_t>));
    EXPECT_LE(hardware_destructive_interference_size, 1024u);
}

TEST(CacheLine, SeparatesTwoAdjacentAtomics)
{
    struct Pair {
        alignas(hardware_destructive_interference_size) std::atomic<std::size_t> left;
        alignas(hardware_destructive_interference_size) std::atomic<std::size_t> right;
    };

    const Pair pair{};
    const auto left = reinterpret_cast<std::uintptr_t>(&pair.left);
    const auto right = reinterpret_cast<std::uintptr_t>(&pair.right);

    EXPECT_GE(right - left, hardware_destructive_interference_size);
    EXPECT_NE(left / hardware_destructive_interference_size,
        right / hardware_destructive_interference_size);
}

} // namespace
