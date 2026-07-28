/**
 * @file
 * @brief Implementation of command-line option parsing.
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

namespace wiola {

Action parse_args(std::span<const std::string_view> args)
{
    for (const std::string_view arg : args) {
        if (arg == "-h" || arg == "--help") {
            return Action::Help;
        }
        if (arg == "-v" || arg == "--version") {
            return Action::Version;
        }
    }

    return Action::Run;
}

} // namespace wiola
