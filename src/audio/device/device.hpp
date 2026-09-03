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

#include <audio/device/output.hpp>
#include <audio/dsp/source.hpp>
#include <audio/stream_spec.hpp>
#include <core/macros.hpp>

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
 * The device callback runs on a real-time thread owned by the backend and does nothing but ask
 * the source for frames, so a source that allocates, locks, blocks or throws must not be given
 * to a device.
 */
class Device final : public Output {
public:
    /// Takes what it plays, which outlives the device and decides the format the output opens in.
    explicit Device(Source& source);

    NO_COPY_SEMANTIC(Device);
    NO_MOVE_SEMANTIC(Device);
    ~Device();

    /// What the output is doing, asked of the audio stack each time.
    [[nodiscard]] DeviceState state() const noexcept;

    /// Whether the callback is being called. Shorthand for `state() == DeviceState::running`.
    [[nodiscard]] bool running() const noexcept override;

    /// Frames of the source handed to the output since the last reset. Silence filled in on an
    /// underrun is not among them; a decoder's own position runs ahead of this by whatever the
    /// buffer is holding.
    [[nodiscard]] Frames frames_played() const noexcept override;

    /// Callbacks that found the buffer short and had to emit silence.
    [[nodiscard]] std::size_t num_underruns() const noexcept;

    [[nodiscard]] StreamSpec spec() const noexcept;

private:
    /// Driving it is the control's; these are what it drives.
    [[nodiscard]] bool start() noexcept override;
    void stop() noexcept override;

    struct Backend;

    void render(std::span<float> output) noexcept;

    StreamSpec spec_{};
    Source& source_;
    std::atomic<std::size_t> num_underruns_{0};
    std::atomic<std::size_t> frames_played_{0};
    std::unique_ptr<Backend> backend_;
};

} // namespace wiola::audio
