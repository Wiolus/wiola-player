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
#include <gui/main_window.hpp>

#include <QApplication>

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
    const std::span raw(argv, static_cast<std::size_t>(argc));

    std::vector<std::string_view> args;
    for (std::size_t i = 1; i < raw.size(); ++i)
        args.emplace_back(raw[i]);

    // Only help and version are answered here
    if (const std::optional<int> exit_code = wiola::run_cli(args))
        return *exit_code;

    const QApplication app{argc, argv};
    wiola::gui::MainWindow window;

    window.show();

    return QApplication::exec();
}
