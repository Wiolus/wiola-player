/**
 * @file
 * @brief Unit tests for command-line option parsing.
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

#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace {

using wiola::run_cli;

/// CLI11 prints help, version, and usage errors itself; keep that out of the test log.
std::optional<int> run_quietly(std::span<const std::string_view> args)
{
    wiola::Options options;

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    const std::optional<int> exit_code = run_cli(args, options);

    testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    return exit_code;
}

TEST(RunCli, NoArgumentsStartsThePlayer)
{
    EXPECT_FALSE(run_quietly({}).has_value());
}

TEST(RunCli, HandlesHelp)
{
    const std::array<std::string_view, 1> short_form{"-h"};
    const std::array<std::string_view, 1> long_form{"--help"};

    EXPECT_EQ(run_quietly(short_form), 0);
    EXPECT_EQ(run_quietly(long_form), 0);
}

TEST(RunCli, HandlesVersion)
{
    const std::array<std::string_view, 1> short_form{"-v"};
    const std::array<std::string_view, 1> long_form{"--version"};

    EXPECT_EQ(run_quietly(short_form), 0);
    EXPECT_EQ(run_quietly(long_form), 0);
}

TEST(RunCli, IgnoresUnknownArguments)
{
    const std::array<std::string_view, 2> args{"song.mp3", "--unknown"};

    EXPECT_FALSE(run_quietly(args).has_value());
}

TEST(RunCli, PrintsHelpText)
{
    const std::array<std::string_view, 1> args{"--help"};
    wiola::Options options;

    testing::internal::CaptureStdout();
    EXPECT_EQ(run_cli(args, options), 0);
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("--version"), std::string::npos);
    EXPECT_NE(output.find("wiola-player"), std::string::npos);
}

} // namespace
