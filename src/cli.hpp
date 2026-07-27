/**
 * @file
 * @brief Command-line option parsing.
 */

#pragma once

#include <span>
#include <string_view>

namespace wiola {

enum class Action {
    Run,
    Help,
    Version,
};

Action parse_args(std::span<const std::string_view> args);

} // namespace wiola
