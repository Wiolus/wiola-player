/**
 * @file
 * @brief Implementation of command-line option parsing.
 */

#include "cli.hpp"

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
