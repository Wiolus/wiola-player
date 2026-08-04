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

#include <CLI/CLI.hpp>

#include <format>
#include <memory>
#include <vector>

namespace wiola {

namespace {

/// What CLI11 writes into. Numbers become quantities once parsing succeeds.
struct RawOptions {
    std::string file;
    double tone_hz{0.0};
    double num_seconds{2.0};
};

std::unique_ptr<CLI::App> make_app(RawOptions& raw)
{
    auto app = std::make_unique<CLI::App>("Wiola media player", "wiola-player");
    app->set_help_flag("-h,--help", "Show this help and exit");
    app->set_version_flag("-v,--version", std::format("wiola-player {}", WIOLA_VERSION),
        "Show version and exit");
    CLI::Option* file{app->add_option("--file", raw.file, "Play an audio file")};
    CLI::Option* tone{
        app->add_option("--tone", raw.tone_hz, "Play a test tone at this frequency, in Hz")};

    tone->excludes(file);
    app->add_option("--seconds", raw.num_seconds, "How long to play the test tone");
    app->allow_extras();

    return app;
}

} // namespace

std::optional<int> run_cli(std::span<const std::string_view> args, Options& options)
{
    RawOptions raw;
    const auto app = make_app(raw);

    // CLI11 consumes the vector overload back to front.
    std::vector<std::string> reversed(args.rbegin(), args.rend());

    try {
        app->parse(reversed);
    } catch (const CLI::ParseError& error) {
        return app->exit(error);
    }

    options.file = raw.file;
    options.tone = units::Frequency{raw.tone_hz};
    options.duration = units::Time{raw.num_seconds};

    return std::nullopt;
}

} // namespace wiola
