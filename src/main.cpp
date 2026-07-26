#include <iostream>
#include <span>
#include <string_view>

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
    const std::span args(argv, static_cast<std::size_t>(argc));
    const std::string_view program = argc > 0 ? args[0] : "wiola-player";

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg = args[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(program);
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            std::cout << "wiola-player " << WIOLA_VERSION << '\n';
            return 0;
        }
    }

    std::cout << "wiola-player " << WIOLA_VERSION << " — nothing to play yet.\n";
    return 0;
}
