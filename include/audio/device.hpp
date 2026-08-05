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
 * What the output is doing.
 *
 * This is observed rather than remembered: whether a device is still running is the audio stack's
 * answer, not ours, and it can change without anyone here asking for it. A device that fails
 * under us therefore shows up as `stopped` on the next look.
 */
enum class DeviceState {
    closed,
    stopped,
    running,
};

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

    /// Starts the callback, opening the output the first time. False when no device is
    /// available. Starting an already running device does nothing.
    [[nodiscard]] bool start() noexcept;

    /// Halts the callback, keeping the output open so that starting again is immediate. The
    /// output itself is only given back when the device is destroyed.
    void stop() noexcept;

    /// What the output is doing, asked of the audio stack each time.
    [[nodiscard]] DeviceState state() const noexcept;

    /// Whether the callback is being called. Shorthand for `state() == DeviceState::running`.
    [[nodiscard]] bool running() const noexcept;

    /// Frames handed to the output since the last reset. This is what has been heard; a decoder's
    /// own position runs ahead of it by whatever the buffer is holding.
    [[nodiscard]] std::size_t frames_played() const noexcept;

    /// Sets that count back to zero. Only legal while the device is stopped.
    void reset_frames_played() noexcept;

    /// Callbacks that found the buffer short and had to emit silence.
    [[nodiscard]] std::size_t num_underruns() const noexcept;

    [[nodiscard]] StreamSpec spec() const noexcept;

private:
    struct Backend;

    void render(std::span<float> output) noexcept;

    StreamSpec spec_{};
    lockfree::SPSCRingBuffer<float>* buffer_;
    std::atomic<std::size_t> num_underruns_{0};
    std::atomic<std::size_t> frames_played_{0};
    std::unique_ptr<Backend> backend_;
};

} // namespace wiola::audio
