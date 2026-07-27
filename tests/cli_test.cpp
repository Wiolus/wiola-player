/**
 * @file
 * @brief Unit tests for command-line option parsing.
 */

#include "cli.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace {

using wiola::Action;
using wiola::parse_args;

TEST(ParseArgs, NoArgumentsRuns) { EXPECT_EQ(parse_args({ }), Action::Run); }

TEST(ParseArgs, RecognisesHelp)
{
    const std::array<std::string_view, 1> short_form { "-h" };
    const std::array<std::string_view, 1> long_form { "--help" };

    EXPECT_EQ(parse_args(short_form), Action::Help);
    EXPECT_EQ(parse_args(long_form), Action::Help);
}

TEST(ParseArgs, RecognisesVersion)
{
    const std::array<std::string_view, 1> short_form { "-v" };
    const std::array<std::string_view, 1> long_form { "--version" };

    EXPECT_EQ(parse_args(short_form), Action::Version);
    EXPECT_EQ(parse_args(long_form), Action::Version);
}

TEST(ParseArgs, IgnoresUnknownArguments)
{
    const std::array<std::string_view, 2> args { "song.mp3", "--unknown" };

    EXPECT_EQ(parse_args(args), Action::Run);
}

TEST(ParseArgs, FirstRecognisedOptionWins)
{
    const std::array<std::string_view, 2> args { "--version", "--help" };

    EXPECT_EQ(parse_args(args), Action::Version);
}

} // namespace
