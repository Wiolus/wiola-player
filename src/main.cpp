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
#include <codec/open.hpp>
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

/// Feeds a source into the ring buffer from this thread while the device drains it from its own.
/// Stops at `limit` when there is one, otherwise when the source runs out and the buffer empties.
template<typename Source>
int play(Source& source, wiola::audio::StreamSpec spec, std::optional<wiola::units::Time> limit)
{
    // A quarter second of slack between the producer and the callback.
    wiola::lockfree::SPSCRingBuffer<float> buffer{spec.samples_per(250_ms)};
    wiola::audio::Device device{spec, buffer};

    // Prime the buffer so the first callbacks find frames waiting for them.
    std::array<float, 1024> chunk{};

    while (buffer.size_approx() + chunk.size() <= buffer.capacity()) {
        const std::size_t num_rendered{source.render(chunk)};

        if (num_rendered == 0)
            break;

        buffer.push(std::span{chunk}.first(num_rendered));
    }

    if (!device.start()) {
        std::cerr << "wiola-player: no playback device available\n";
        return 1;
    }

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

    if (device.num_underruns() > 0)
        std::cout << "underruns: " << device.num_underruns() << '\n';

    return 0;
}

int play_file(const wiola::Options& options)
{
    const std::unique_ptr<wiola::codec::Decoder> reader{wiola::codec::open_file(options.file)};

    if (!reader) {
        std::cerr << "wiola-player: cannot read " << options.file << '\n';
        return 1;
    }

    const wiola::audio::StreamSpec spec{reader->spec()};

    std::cout << "wiola-player " << WIOLA_VERSION << " — " << options.file.filename().string()
              << ", " << spec.sample_rate.get<wiola::units::Hz>() << " Hz, " << spec.num_channels
              << " ch\n";

    return play(*reader, spec, std::nullopt);
}

int play_tone(const wiola::Options& options)
{
    const wiola::audio::StreamSpec spec{};
    wiola::audio::SineSource source{spec, options.tone};

    std::cout << "wiola-player " << WIOLA_VERSION << " — " << options.tone.get<wiola::units::Hz>()
              << " Hz for " << options.duration.get<wiola::units::Sec>() << " s\n";

    return play(source, spec, options.duration);
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

    if (!options.file.empty())
        return play_file(options);

    if (options.tone > wiola::units::Frequency{})
        return play_tone(options);

    std::cout << "wiola-player " << WIOLA_VERSION << " — nothing to play yet.\n";
    return 0;
}
