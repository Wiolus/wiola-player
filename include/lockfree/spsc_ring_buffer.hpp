/**
 * @file
 * @brief Lock-free single-producer / single-consumer ring buffer.
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

#pragma once

#include <core/cache_line.hpp>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>

namespace wiola::lockfree {

/// Elements must be copyable byte-wise: the buffer never runs constructors or destructors.
template<typename T>
concept RingElement = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

/**
 * Bounded ring buffer for exactly one producer thread and one consumer thread.
 *
 * Every operation is wait-free: no locks, no allocation, no unbounded retry loop, so the
 * consumer side is safe to call from an audio callback. Capacity is rounded up to a power
 * of two. Indices are free-running and only masked on access, which keeps "full" and
 * "empty" distinguishable without wasting a slot.
 */
template<RingElement T>
class SPSCRingBuffer {
    static_assert(std::atomic<std::size_t>::is_always_lock_free,
        "platform lacks lock-free size_t atomics");

public:
    /// Non-owning view of a contiguous run of slots; `second` is non-empty only across a wrap.
    template<typename U>
    struct Region {
        std::span<U> first;
        std::span<U> second;

        [[nodiscard]] std::size_t size() const noexcept;
    };

    using WriteRegion = Region<T>;
    using ReadRegion = Region<const T>;

    explicit SPSCRingBuffer(std::size_t minimum_capacity);

    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer(SPSCRingBuffer&&) = delete;
    SPSCRingBuffer& operator=(SPSCRingBuffer&&) = delete;
    ~SPSCRingBuffer() = default;

    [[nodiscard]] std::size_t capacity() const noexcept;

    /// Producer side. Returns the number of elements accepted, which may be short of `src`.
    std::size_t push(std::span<const T> src) noexcept;

    /// Consumer side. Returns the number of elements written into `dst`.
    std::size_t pop(std::span<T> dst) noexcept;

    [[nodiscard]] bool try_push(const T& value) noexcept;
    [[nodiscard]] std::optional<T> try_pop() noexcept;

    /// Zero-copy producer handshake: fill the returned spans, then commit what you used.
    [[nodiscard]] WriteRegion acquire_write() noexcept;
    void commit_write(std::size_t num_elements) noexcept;

    /// Zero-copy consumer handshake, mirroring acquire_write().
    [[nodiscard]] ReadRegion acquire_read() noexcept;
    void commit_read(std::size_t num_elements) noexcept;

    /// Observers. Stale the instant they return; for metering and sizing, not for control flow.
    [[nodiscard]] std::size_t size_approx() const noexcept;
    [[nodiscard]] bool empty_approx() const noexcept;

private:
    void copy_in(std::size_t w, std::span<const T> src) noexcept;
    void copy_out(std::size_t r, std::span<T> dst) noexcept;

    const std::size_t mask_;
    const std::unique_ptr<T[]> storage_;

    // Each index shares a cache line only with the cache owned by the same thread, so the
    // producer and consumer never invalidate each other's line on their own bookkeeping.
    alignas(hw::hardware_destructive_interference_size) std::atomic<std::size_t> write_{0};
    std::size_t producer_read_cache_{0};

    alignas(hw::hardware_destructive_interference_size) std::atomic<std::size_t> read_{0};
    std::size_t consumer_write_cache_{0};
};

template<RingElement T>
template<typename U>
std::size_t SPSCRingBuffer<T>::Region<U>::size() const noexcept
{
    return first.size() + second.size();
}

template<RingElement T>
SPSCRingBuffer<T>::SPSCRingBuffer(std::size_t minimum_capacity)
    : mask_{std::bit_ceil(std::max<std::size_t>(minimum_capacity, 2)) - 1}
    , storage_{std::make_unique_for_overwrite<T[]>(mask_ + 1)}
{
}

template<RingElement T>
std::size_t SPSCRingBuffer<T>::capacity() const noexcept
{
    return mask_ + 1;
}

template<RingElement T>
std::size_t SPSCRingBuffer<T>::push(std::span<const T> src) noexcept
{
    const std::size_t w = write_.load(std::memory_order_relaxed);
    std::size_t num_free_slots = capacity() - (w - producer_read_cache_);

    if (num_free_slots < src.size()) [[unlikely]] {
        producer_read_cache_ = read_.load(std::memory_order_acquire);
        num_free_slots = capacity() - (w - producer_read_cache_);
    }

    const std::size_t n = std::min(num_free_slots, src.size());
    copy_in(w, src.first(n));
    write_.store(w + n, std::memory_order_release);
    return n;
}

template<RingElement T>
std::size_t SPSCRingBuffer<T>::pop(std::span<T> dst) noexcept
{
    const std::size_t r = read_.load(std::memory_order_relaxed);
    std::size_t ready = consumer_write_cache_ - r;

    if (ready < dst.size()) [[unlikely]] {
        consumer_write_cache_ = write_.load(std::memory_order_acquire);
        ready = consumer_write_cache_ - r;
    }

    const std::size_t n = std::min(ready, dst.size());
    copy_out(r, dst.first(n));
    read_.store(r + n, std::memory_order_release);
    return n;
}

template<RingElement T>
bool SPSCRingBuffer<T>::try_push(const T& value) noexcept
{
    return push({&value, 1}) == 1;
}

template<RingElement T>
std::optional<T> SPSCRingBuffer<T>::try_pop() noexcept
{
    T value;

    if (pop({&value, 1}) == 0) {
        return std::nullopt;
    }

    return value;
}

template<RingElement T>
auto SPSCRingBuffer<T>::acquire_write() noexcept -> WriteRegion
{
    const std::size_t w = write_.load(std::memory_order_relaxed);
    producer_read_cache_ = read_.load(std::memory_order_acquire);

    const std::size_t num_free_slots = capacity() - (w - producer_read_cache_);
    const std::size_t offset = w & mask_;
    const std::size_t head = std::min(num_free_slots, capacity() - offset);

    return {
        std::span{storage_.get() + offset, head                 },
        std::span{storage_.get(),          num_free_slots - head}
    };
}

template<RingElement T>
void SPSCRingBuffer<T>::commit_write(std::size_t num_elements) noexcept
{
    write_.store(write_.load(std::memory_order_relaxed) + num_elements, std::memory_order_release);
}

template<RingElement T>
auto SPSCRingBuffer<T>::acquire_read() noexcept -> ReadRegion
{
    const std::size_t r = read_.load(std::memory_order_relaxed);
    consumer_write_cache_ = write_.load(std::memory_order_acquire);

    const std::size_t ready = consumer_write_cache_ - r;
    const std::size_t offset = r & mask_;
    const std::size_t head = std::min(ready, capacity() - offset);

    return {
        std::span<const T>{storage_.get() + offset, head        },
        std::span<const T>{storage_.get(),          ready - head}
    };
}

template<RingElement T>
void SPSCRingBuffer<T>::commit_read(std::size_t num_elements) noexcept
{
    read_.store(read_.load(std::memory_order_relaxed) + num_elements, std::memory_order_release);
}

template<RingElement T>
std::size_t SPSCRingBuffer<T>::size_approx() const noexcept
{
    return write_.load(std::memory_order_acquire) - read_.load(std::memory_order_acquire);
}

template<RingElement T>
bool SPSCRingBuffer<T>::empty_approx() const noexcept
{
    return size_approx() == 0;
}

template<RingElement T>
void SPSCRingBuffer<T>::copy_in(std::size_t w, std::span<const T> src) noexcept
{
    const std::size_t offset = w & mask_;
    const std::size_t head = std::min(src.size(), capacity() - offset);

    std::copy_n(src.data(), head, storage_.get() + offset);
    std::copy_n(src.data() + head, src.size() - head, storage_.get());
}

template<RingElement T>
void SPSCRingBuffer<T>::copy_out(std::size_t r, std::span<T> dst) noexcept
{
    const std::size_t offset = r & mask_;
    const std::size_t head = std::min(dst.size(), capacity() - offset);

    std::copy_n(storage_.get() + offset, head, dst.data());
    std::copy_n(storage_.get(), dst.size() - head, dst.data() + head);
}

} // namespace wiola::lockfree
