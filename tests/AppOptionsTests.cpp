#include <catch2/catch_test_macros.hpp>

#include "AppOptions.h"

#include <string>
#include <vector>

// AppOptions::fromCommandLine is already a public static taking argc/argv, so
// this needs no scaffolding at all - and argv is the level worth testing at,
// since that is what a user actually types.
//
// One trap when adding to this file: a TEST_CASE name must not START with "--".
// ctest runs each case by passing its name as a single argument, and Catch2
// parses a leading double dash as one of its own options - so such a test fails
// with "Unrecognised token" under ctest while passing when run by hand. Name
// them "The seed flag ..." rather than "--seed ...".

namespace
{
struct ParseResult
{
    bool ok = false;
    AppOptions options;
    bool helpRequested = false;
    bool versionRequested = false;
    std::string error;
};

// argv[0] is the program name, which the parser skips, so every case here gets
// one prepended for it.
ParseResult parse(std::vector<std::string> arguments)
{
    arguments.insert(arguments.begin(), "terminal_romanian_whist");

    std::vector<char*> argv;
    for(std::string& argument : arguments)
        argv.push_back(argument.data());

    ParseResult result;
    result.ok = AppOptions::fromCommandLine(static_cast<int>(argv.size()),
                                            argv.data(),
                                            result.options,
                                            result.helpRequested,
                                            result.versionRequested,
                                            result.error);

    return result;
}
}

TEST_CASE("No arguments leaves the defaults alone", "[app-options]")
{
    const ParseResult result = parse({});

    REQUIRE(result.ok);
    REQUIRE_FALSE(result.helpRequested);
    REQUIRE_FALSE(result.versionRequested);
    REQUIRE_FALSE(result.options.seed.has_value());
    REQUIRE_FALSE(result.options.demo);
}

TEST_CASE("The seed flag accepts a value and makes the game replayable", "[app-options]")
{
    SECTION("an ordinary value")
    {
        const ParseResult result = parse({ "--seed=42" });

        REQUIRE(result.ok);
        REQUIRE(result.options.seed == 42u);
    }

    SECTION("zero is a seed like any other, not an absent one")
    {
        const ParseResult result = parse({ "--seed=0" });

        REQUIRE(result.ok);
        REQUIRE(result.options.seed == 0u);
    }

    SECTION("the largest value that fits")
    {
        const ParseResult result = parse({ "--seed=4294967295" });

        REQUIRE(result.ok);
        REQUIRE(result.options.seed == 4294967295u);
    }
}

TEST_CASE("A malformed --seed is rejected rather than replaying the wrong game", "[app-options]")
{
    // The whole point of the flag is reproducing one exact game, so a value the
    // parser cannot honour has to be an error - silently seeding something else
    // would be the one failure the flag exists to rule out.
    SECTION("a negative value")
    {
        // strtoull wraps a leading sign rather than rejecting it, so without an
        // explicit check this parses as 2^64-1 and fails a range test with a
        // confusing message instead of being read as the typo it is.
        REQUIRE_FALSE(parse({ "--seed=-1" }).ok);
    }

    SECTION("an explicit plus")
    {
        REQUIRE_FALSE(parse({ "--seed=+1" }).ok);
    }

    SECTION("past the top of the range")
    {
        REQUIRE_FALSE(parse({ "--seed=4294967296" }).ok);
    }

    SECTION("no value at all")
    {
        REQUIRE_FALSE(parse({ "--seed=" }).ok);
    }

    SECTION("not a number")
    {
        REQUIRE_FALSE(parse({ "--seed=abc" }).ok);
    }

    SECTION("trailing rubbish after a good number")
    {
        REQUIRE_FALSE(parse({ "--seed=42x" }).ok);
    }
}

TEST_CASE("Display and pacing flags are recognised", "[app-options]")
{
    const ParseResult result = parse({ "--no-color", "--ascii", "--no-clear",
                                       "--no-alt-screen", "--auto", "--demo" });

    REQUIRE(result.ok);
    REQUIRE_FALSE(result.options.color);
    REQUIRE_FALSE(result.options.unicode);
    REQUIRE_FALSE(result.options.screenControl);
    REQUIRE_FALSE(result.options.altScreen);
    REQUIRE(result.options.autoAdvance);
    REQUIRE(result.options.demo);
}

TEST_CASE("Help and version are reported rather than acted on", "[app-options]")
{
    SECTION("--help")
    {
        const ParseResult result = parse({ "--help" });

        REQUIRE(result.ok);
        REQUIRE(result.helpRequested);
    }

    SECTION("-h")
    {
        REQUIRE(parse({ "-h" }).helpRequested);
    }

    SECTION("--version")
    {
        const ParseResult result = parse({ "--version" });

        REQUIRE(result.ok);
        REQUIRE(result.versionRequested);
    }

    SECTION("-v")
    {
        REQUIRE(parse({ "-v" }).versionRequested);
    }
}

TEST_CASE("An unrecognised argument is an error with something to say", "[app-options]")
{
    const ParseResult result = parse({ "--not-a-flag" });

    REQUIRE_FALSE(result.ok);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("The reckoner preset defaults to Hard when unspecified", "[app-options]")
{
    const ParseResult result = parse({});

    REQUIRE(result.ok);
    REQUIRE(result.options.reckonerPreset == romanian_whist::reckoner::ReckonerPreset::Hard);
}

TEST_CASE("The reckoner preset flag selects the requested difficulty", "[app-options]")
{
    using romanian_whist::reckoner::ReckonerPreset;

    SECTION("easy")
    {
        const ParseResult result = parse({ "--reckoner-preset=easy" });

        REQUIRE(result.ok);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Easy);
    }

    SECTION("medium")
    {
        const ParseResult result = parse({ "--reckoner-preset=medium" });

        REQUIRE(result.ok);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Medium);
    }

    SECTION("hard")
    {
        const ParseResult result = parse({ "--reckoner-preset=hard" });

        REQUIRE(result.ok);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Hard);
    }

    SECTION("brutal")
    {
        const ParseResult result = parse({ "--reckoner-preset=brutal" });

        REQUIRE(result.ok);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Brutal);
    }

    SECTION("case-insensitive")
    {
        const ParseResult result = parse({ "--reckoner-preset=HARD" });

        REQUIRE(result.ok);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Hard);
    }
}

TEST_CASE("A malformed --reckoner-preset is rejected", "[app-options]")
{
    SECTION("unknown value")
    {
        const ParseResult result = parse({ "--reckoner-preset=unknown" });

        REQUIRE_FALSE(result.ok);
        REQUIRE_FALSE(result.error.empty());
    }

    SECTION("empty value")
    {
        const ParseResult result = parse({ "--reckoner-preset=" });

        REQUIRE_FALSE(result.ok);
        REQUIRE_FALSE(result.error.empty());
    }
}
