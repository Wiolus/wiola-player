/**
 * @file
 * @brief Drives a decoder through a device until it ends.
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

#include <engine/play.hpp>

#include <audio/device.hpp>
#include <audio/stream_spec.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <array>
#include <chrono>
#include <span>
#include <thread>

namespace wiola::engine {

namespace {

using namespace std::chrono_literals;
using namespace units::literals;

/// How long to wait when the buffer is full, or when it is draining at the end.
constexpr auto poll_interval{2ms};

} // namespace

std::optional<std::size_t> play(codec::Decoder& source)
{
    const audio::StreamSpec spec{source.spec()};

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

    for (std::size_t num_rendered{source.render(chunk)}; num_rendered > 0;
        num_rendered = source.render(chunk)) {
        for (std::size_t num_written = 0; num_written < num_rendered;) {
            num_written +=
                buffer.push(std::span{chunk}.subspan(num_written, num_rendered - num_written));

            if (num_written < num_rendered)
                std::this_thread::sleep_for(poll_interval);
        }
    }

    // The source is finished, but what is already buffered has not been heard yet.
    while (buffer.size_approx() > 0)
        std::this_thread::sleep_for(poll_interval);

    device.stop();

    return device.num_underruns();
}

} // namespace wiola::engine
