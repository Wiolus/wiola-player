/**
 * @file
 * @brief Drives a source through a device until it ends.
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

#include <audio/device.hpp>
#include <audio/stream_spec.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <thread>

namespace wiola::engine {

/**
 * Feeds `source` into a ring buffer from the calling thread while a device drains it from its own.
 *
 * Returns the number of times the device found the buffer empty, or nothing when no device could
 * be started. Playback stops at `limit` when there is one, and otherwise when the source runs out
 * and everything already buffered has been heard.
 *
 * A source is anything answering `render(std::span<float>)` with the number of samples it wrote,
 * a short answer meaning it has ended. Blocks until playback finishes.
 */
template<typename Source>
std::optional<std::size_t> play(Source& source, audio::StreamSpec spec,
    std::optional<units::Time> limit)
{
    using namespace std::chrono_literals;
    using namespace units::literals;

    // A quarter second of slack between the producer and the callback.
    lockfree::SPSCRingBuffer<float> buffer{spec.samples_per(250_ms)};
    audio::Device device{spec, buffer};

    // Prime the buffer so the first callbacks find frames waiting for them.
    std::array<float, 1024> chunk{};

    while (buffer.size_approx() + chunk.size() <= buffer.capacity()) {
        const std::size_t num_rendered{source.render(chunk)};

        if (num_rendered == 0)
            break;

        buffer.push(std::span{chunk}.first(num_rendered));
    }

    if (!device.start())
        return std::nullopt;

    const auto deadline = std::chrono::steady_clock::now() +
        (limit ? limit->chrono() : std::chrono::duration<double>{0.0});

    while (!limit || std::chrono::steady_clock::now() < deadline) {
        const std::size_t num_rendered{source.render(chunk)};

        if (num_rendered == 0) {
            while (buffer.size_approx() > 0)
                std::this_thread::sleep_for(2ms);

            break;
        }

        for (std::size_t num_written = 0; num_written < num_rendered;) {
            num_written +=
                buffer.push(std::span{chunk}.subspan(num_written, num_rendered - num_written));

            if (num_written < num_rendered)
                std::this_thread::sleep_for(2ms);
        }
    }

    device.stop();

    return device.num_underruns();
}

} // namespace wiola::engine
