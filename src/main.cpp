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

#include <audio/stream_spec.hpp>
#include <cli/cli.hpp>
#include <codec/open.hpp>
#include <engine/player.hpp>
#include <utils/units.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {

int play_file(const wiola::Options& options)
{
    const std::unique_ptr<wiola::codec::Decoder> reader{wiola::codec::open_file(options.file)};

    if (!reader) {
        std::cerr << "wiola-player: cannot read " << options.file << '\n';
        return EXIT_FAILURE;
    }

    const wiola::audio::StreamSpec spec{reader->spec()};

    std::cout << "wiola-player " << WIOLA_VERSION << " — " << options.file.filename().string()
              << ", " << spec.sample_rate.get<wiola::units::Hz>() << " Hz, " << spec.num_channels
              << " ch\n";

    wiola::engine::Player player{*reader};

    if (!player.start()) {
        std::cerr << "wiola-player: no playback device available\n";
        return EXIT_FAILURE;
    }

    player.wait();

    if (player.num_underruns() > 0)
        std::cout << "underruns: " << player.num_underruns() << '\n';

    return EXIT_SUCCESS;
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

    std::cout << "wiola-player " << WIOLA_VERSION << " — nothing to play yet.\n";

    return EXIT_SUCCESS;
}
