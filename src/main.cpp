/**
 * @file
 * @brief Program entry point.
 */

#include "cli.hpp"

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
