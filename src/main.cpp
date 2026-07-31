/**
 * @file
 * @brief Program entry point.
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

#include <audio/device.hpp>
#include <audio/stream_spec.hpp>
#include <audio/tone.hpp>
#include <cli/cli.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;
using namespace wiola::units::literals;

/// Feeds a sine into the ring buffer from this thread while the device drains it from its own.
int play_tone(const wiola::Options& options)
{
    const wiola::audio::StreamSpec spec{};

    // A quarter second of slack between the producer and the callback.
    wiola::lockfree::SPSCRingBuffer<float> buffer{spec.samples_per(250_ms)};
    wiola::audio::Device device{spec, buffer};
    wiola::audio::SineSource source{spec, options.tone};

    // Prime the buffer so the first callbacks find frames waiting for them.
    std::array<float, 1024> chunk{};

    while (buffer.size_approx() + chunk.size() <= buffer.capacity()) {
        source.render(chunk);
        buffer.push(chunk);
    }

    if (!device.start()) {
        std::cerr << "wiola-player: no playback device available\n";
        return 1;
    }

    std::cout << "wiola-player " << WIOLA_VERSION << " — " << options.tone.get<wiola::units::Hz>()
              << " Hz for " << options.duration.get<wiola::units::Sec>() << " s\n";

    const auto deadline = std::chrono::steady_clock::now() + options.duration.chrono();

    while (std::chrono::steady_clock::now() < deadline) {
        source.render(chunk);

        for (std::size_t num_written = 0; num_written < chunk.size();) {
            num_written += buffer.push(std::span{chunk}.subspan(num_written));

            if (num_written < chunk.size())
                std::this_thread::sleep_for(2ms);
        }
    }

    device.stop();

    if (device.num_underruns() > 0)
        std::cout << "underruns: " << device.num_underruns() << '\n';

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    const std::span raw(argv, static_cast<std::size_t>(argc));

    std::vector<std::string_view> args;
    for (std::size_t i = 1; i < raw.size(); ++i)
        args.emplace_back(raw[i]);

    wiola::Options options;

    if (const std::optional<int> exit_code = wiola::run_cli(args, options))
        return *exit_code;

    if (options.tone > wiola::units::Frequency{})
        return play_tone(options);

    std::cout << "wiola-player " << WIOLA_VERSION << " — nothing to play yet.\n";
    return 0;
}
