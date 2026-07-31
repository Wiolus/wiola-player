/**
 * @file
 * @brief Playback device fed by a single-producer ring buffer.
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

#include <audio/stream_spec.hpp>
#include <lockfree/spsc_ring_buffer.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <span>

namespace wiola::audio {

/**
 * Default system output.
 *
 * The device callback runs on a real-time thread owned by the backend: it only pops from the
 * ring buffer and never allocates, locks, or blocks. Whoever constructs the device owns the
 * buffer and is the single producer.
 */
class Device {
public:
    Device(StreamSpec spec, lockfree::SPSCRingBuffer<float>& buffer);

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;
    ~Device();

    /// Opens the default output and starts the callback. False when no device is available.
    [[nodiscard]] bool start() noexcept;
    void stop() noexcept;

    /// Callbacks that found the buffer short and had to emit silence.
    [[nodiscard]] std::size_t num_underruns() const noexcept;
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] StreamSpec spec() const noexcept;

private:
    struct Backend;

    void render(std::span<float> output) noexcept;

    StreamSpec spec_{};
    lockfree::SPSCRingBuffer<float>* buffer_;
    std::atomic<std::size_t> num_underruns_{0};
    std::unique_ptr<Backend> backend_;
};

} // namespace wiola::audio
