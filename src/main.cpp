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

#include <cli/cli.hpp>

#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

void print_usage(std::string_view program)
{
    std::cout << "Usage: " << program << " [options] [file...]\n"
              << "\n"
              << "Options:\n"
              << "  -h, --help       Show this help and exit\n"
              << "  -v, --version    Show version and exit\n";
}

} // namespace

int main(int argc, char** argv)
{
    const std::span raw(argv, static_cast<std::size_t>(argc));
    const std::string_view program = raw.empty() ? "wiola-player" : raw[0];

    std::vector<std::string_view> args;
    for (std::size_t i = 1; i < raw.size(); ++i) {
        args.emplace_back(raw[i]);
    }

    switch (wiola::parse_args(args)) {
    case wiola::Action::Help:
        print_usage(program);
        return 0;
    case wiola::Action::Version:
        std::cout << "wiola-player " << WIOLA_VERSION << '\n';
        return 0;
    case wiola::Action::Run:
        break;
    }

    std::cout << "wiola-player " << WIOLA_VERSION << " — nothing to play yet.\n";
    return 0;
}
