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
 * along with Wiola.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cli/cli.hpp>

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace {

using wiola::Action;
using wiola::parse_args;

TEST(ParseArgs, NoArgumentsRuns)
{
    EXPECT_EQ(parse_args({}), Action::Run);
}

TEST(ParseArgs, RecognisesHelp)
{
    const std::array<std::string_view, 1> short_form{"-h"};
    const std::array<std::string_view, 1> long_form{"--help"};

    EXPECT_EQ(parse_args(short_form), Action::Help);
    EXPECT_EQ(parse_args(long_form), Action::Help);
}

TEST(ParseArgs, RecognisesVersion)
{
    const std::array<std::string_view, 1> short_form{"-v"};
    const std::array<std::string_view, 1> long_form{"--version"};

    EXPECT_EQ(parse_args(short_form), Action::Version);
    EXPECT_EQ(parse_args(long_form), Action::Version);
}

TEST(ParseArgs, IgnoresUnknownArguments)
{
    const std::array<std::string_view, 2> args{"song.mp3", "--unknown"};

    EXPECT_EQ(parse_args(args), Action::Run);
}

TEST(ParseArgs, FirstRecognisedOptionWins)
{
    const std::array<std::string_view, 2> args{"--version", "--help"};

    EXPECT_EQ(parse_args(args), Action::Version);
}

} // namespace
