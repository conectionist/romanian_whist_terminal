#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "AppOptions.h"

#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
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
using Action = CommandLine::Action;

// argv[0] is the program name, which the parser skips, so every case here gets
// one prepended for it.
CommandLine parse(std::vector<std::string> arguments)
{
    arguments.insert(arguments.begin(), "terminal_romanian_whist");

    std::vector<char*> argv;
    for(std::string& argument : arguments)
        argv.push_back(argument.data());

    return AppOptions::fromCommandLine(static_cast<int>(argv.size()), argv.data());
}
}

TEST_CASE("No arguments leaves the defaults alone", "[app-options]")
{
    const CommandLine result = parse({});

    REQUIRE(result.action == Action::Run);
    REQUIRE_FALSE(result.options.seed.has_value());
    REQUIRE_FALSE(result.options.demo);
}

TEST_CASE("The seed flag accepts a value and makes the game replayable", "[app-options]")
{
    SECTION("an ordinary value")
    {
        const CommandLine result = parse({ "--seed=42" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.seed == 42u);
    }

    SECTION("zero is a seed like any other, not an absent one")
    {
        const CommandLine result = parse({ "--seed=0" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.seed == 0u);
    }

    SECTION("the largest value that fits")
    {
        const CommandLine result = parse({ "--seed=4294967295" });

        REQUIRE(result.action == Action::Run);
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
        REQUIRE(parse({ "--seed=-1" }).action == Action::Error);
    }

    SECTION("an explicit plus")
    {
        REQUIRE(parse({ "--seed=+1" }).action == Action::Error);
    }

    SECTION("past the top of the range")
    {
        REQUIRE(parse({ "--seed=4294967296" }).action == Action::Error);
    }

    SECTION("no value at all")
    {
        REQUIRE(parse({ "--seed=" }).action == Action::Error);
    }

    SECTION("not a number")
    {
        REQUIRE(parse({ "--seed=abc" }).action == Action::Error);
    }

    SECTION("trailing rubbish after a good number")
    {
        REQUIRE(parse({ "--seed=42x" }).action == Action::Error);
    }
}

TEST_CASE("Display and pacing flags are recognised", "[app-options]")
{
    const CommandLine result = parse({ "--no-color", "--ascii", "--no-clear",
                                       "--no-alt-screen", "--auto", "--demo" });

    REQUIRE(result.action == Action::Run);
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
        const CommandLine result = parse({ "--help" });

        REQUIRE(result.action == Action::Help);
    }

    SECTION("-h")
    {
        REQUIRE(parse({ "-h" }).action == Action::Help);
    }

    SECTION("--version")
    {
        const CommandLine result = parse({ "--version" });

        REQUIRE(result.action == Action::Version);
    }

    SECTION("-v")
    {
        REQUIRE(parse({ "-v" }).action == Action::Version);
    }

    SECTION("--help wins over --version")
    {
        REQUIRE(parse({ "--version", "--help" }).action == Action::Help);
    }

    SECTION("a bad flag is an error even after --help")
    {
        REQUIRE(parse({ "--help", "--not-a-flag" }).action == Action::Error);
    }
}

TEST_CASE("An unrecognised argument is an error with something to say", "[app-options]")
{
    const CommandLine result = parse({ "--not-a-flag" });

    REQUIRE(result.action == Action::Error);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("A bad argument is echoed with its control characters written out", "[app-options]")
{
    // The error is printed to the terminal, which would obey an escape sequence
    // in it rather than show it.
    SECTION("unrecognised option")
    {
        const CommandLine result = parse({ "--x\x1B]0;title\x07" });

        REQUIRE(result.action == Action::Error);
        CHECK_THAT(result.error, Catch::Matchers::ContainsSubstring("--x\\x1B]0;title\\x07"));
        CHECK(result.error.find('\x1B') == std::string::npos);
    }

    SECTION("bad reckoner preset")
    {
        const CommandLine result = parse({ "--reckoner-preset=\x9B" "2J" });

        REQUIRE(result.action == Action::Error);
        CHECK_THAT(result.error, Catch::Matchers::ContainsSubstring("'\\x9B2J'"));
        CHECK(result.error.find('\x9B') == std::string::npos);
    }
}

TEST_CASE("The reckoner preset defaults to Gamma when unspecified", "[app-options]")
{
    const CommandLine result = parse({});

    REQUIRE(result.action == Action::Run);
    REQUIRE(result.options.reckonerPreset == romanian_whist::reckoner::ReckonerPreset::Gamma);
}

TEST_CASE("The reckoner preset flag selects the requested preset", "[app-options]")
{
    using romanian_whist::reckoner::ReckonerPreset;

    SECTION("alpha")
    {
        const CommandLine result = parse({ "--reckoner-preset=alpha" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Alpha);
    }

    SECTION("beta")
    {
        const CommandLine result = parse({ "--reckoner-preset=beta" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Beta);
    }

    SECTION("gamma")
    {
        const CommandLine result = parse({ "--reckoner-preset=gamma" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Gamma);
    }

    SECTION("delta")
    {
        const CommandLine result = parse({ "--reckoner-preset=delta" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Delta);
    }

    SECTION("case-insensitive")
    {
        const CommandLine result = parse({ "--reckoner-preset=GAMMA" });

        REQUIRE(result.action == Action::Run);
        REQUIRE(result.options.reckonerPreset == ReckonerPreset::Gamma);
    }
}

TEST_CASE("A malformed --reckoner-preset is rejected", "[app-options]")
{
    SECTION("unknown value")
    {
        const CommandLine result = parse({ "--reckoner-preset=unknown" });

        REQUIRE(result.action == Action::Error);
        REQUIRE_FALSE(result.error.empty());
    }

    SECTION("empty value")
    {
        const CommandLine result = parse({ "--reckoner-preset=" });

        REQUIRE(result.action == Action::Error);
        REQUIRE_FALSE(result.error.empty());
    }

    SECTION("a name the engine retired in 5.0.0")
    {
        const CommandLine result = parse({ "--reckoner-preset=hard" });

        REQUIRE(result.action == Action::Error);

        // The CHANGELOG promises a script still passing an old name is told the new ones.
        for(const char* name : { "alpha", "beta", "gamma", "delta" })
            REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring(name));
    }
}

#ifdef _WIN32

TEST_CASE("Windows always checks outside text as UTF-8", "[app-options]")
{
    // The game switches the console's output to UTF-8, whatever the locale says.
    CHECK(parse({}).options.encoding == DisplayText::Encoding::Utf8);
}

#else

namespace
{
// Sets or clears environment variables for as long as it lives, then puts back
// what was there, so one test's locale cannot leak into the next.
class ScopedEnvironment
{
public:
    ScopedEnvironment() = default;
    ScopedEnvironment(const ScopedEnvironment&) = delete;
    ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

    ~ScopedEnvironment()
    {
        for(auto entry = saved.rbegin() ; entry != saved.rend() ; ++entry)
            assign(entry->first.c_str(), entry->second ? entry->second->c_str() : nullptr);
    }

    void set(const char* name, const char* value)
    {
        const char* previous = std::getenv(name);
        saved.emplace_back(name, previous ? std::optional<std::string>(previous) : std::nullopt);

        assign(name, value);
    }

private:
    static void assign(const char* name, const char* value)
    {
        if(value)
            ::setenv(name, value, 1);
        else
            ::unsetenv(name);
    }

    std::vector<std::pair<std::string, std::optional<std::string>>> saved;
};

// The first of LC_ALL, LC_CTYPE and LANG that is set decides the locale, so
// clearing the other two leaves only the one being tested.
void useLocale(ScopedEnvironment& environment, const char* lcAll, const char* lang = nullptr)
{
    environment.set("LC_ALL", lcAll);
    environment.set("LC_CTYPE", nullptr);
    environment.set("LANG", lang);
    environment.set("WHIST_ASCII", nullptr);
}
}

TEST_CASE("The locale decides how outside text is checked", "[app-options]")
{
    ScopedEnvironment environment;

    SECTION("a UTF-8 locale")
    {
        useLocale(environment, "en_US.UTF-8");
        CHECK(parse({}).options.encoding == DisplayText::Encoding::Utf8);
    }

    SECTION("the C locale")
    {
        useLocale(environment, "C");
        CHECK(parse({}).options.encoding == DisplayText::Encoding::EightBit);
    }

    SECTION("a Latin-2 locale named only in LANG")
    {
        useLocale(environment, nullptr, "ro_RO.ISO-8859-2");
        CHECK(parse({}).options.encoding == DisplayText::Encoding::EightBit);
    }

    SECTION("no locale at all")
    {
        useLocale(environment, nullptr);
        CHECK(parse({}).options.encoding == DisplayText::Encoding::EightBit);
    }
}

TEST_CASE("Asking for ASCII glyphs leaves the encoding alone", "[app-options]")
{
    // The terminal still reads what the locale says, so text from outside must
    // still be checked as UTF-8.
    ScopedEnvironment environment;
    useLocale(environment, "en_US.UTF-8");

    SECTION("the ascii flag")
    {
        const CommandLine result = parse({ "--ascii" });

        CHECK_FALSE(result.options.unicode);
        CHECK(result.options.encoding == DisplayText::Encoding::Utf8);
    }

    SECTION("WHIST_ASCII")
    {
        environment.set("WHIST_ASCII", "1");
        const CommandLine result = parse({});

        CHECK_FALSE(result.options.unicode);
        CHECK(result.options.encoding == DisplayText::Encoding::Utf8);
    }
}

TEST_CASE("A bad argument is escaped for the terminal the locale names", "[app-options]")
{
    // U+011B in UTF-8, whose second byte an 8-bit terminal obeys as CSI.
    ScopedEnvironment environment;

    SECTION("8-bit")
    {
        useLocale(environment, "C");
        const CommandLine result = parse({ "--x\xC4\x9B" "2J" });

        REQUIRE(result.action == Action::Error);
        CHECK_THAT(result.error, Catch::Matchers::ContainsSubstring("--x\xC4" "\\x9B2J"));
        CHECK(result.error.find('\x9B') == std::string::npos);
    }

    SECTION("UTF-8")
    {
        useLocale(environment, "en_US.UTF-8");
        const CommandLine result = parse({ "--x\xC4\x9B" "2J" });

        REQUIRE(result.action == Action::Error);
        CHECK_THAT(result.error, Catch::Matchers::ContainsSubstring("--x\xC4\x9B" "2J"));
    }
}

#endif
