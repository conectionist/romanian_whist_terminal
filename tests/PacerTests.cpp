#include <catch2/catch_test_macros.hpp>

#include "AppOptions.h"
#include "Pacer.h"
#include "PlainTerminal.h"
#include "Terminal.h"

#include <string>
#include <vector>

// Whether a pause waits for Enter also decides whether its "Press Enter" prompt
// is drawn, so each case here is a prompt that is or is not shown.
namespace
{
struct PacerFor
{
    AppOptions options;
    Terminal terminal;
    Pacer pacer;

    explicit PacerFor(const AppOptions& _options)
        : options(_options), terminal(options), pacer(options, terminal) {}
};

AppOptions withTerminal(bool interactive, bool autoAdvance)
{
    AppOptions options = PlainTerminal::plainOptions();
    options.interactive = interactive;
    options.autoAdvance = autoAdvance;
    return options;
}

// Parsed from argv, so the test sees what a flag really sets, then made
// interactive, as it would be in a terminal.
AppOptions fromFlag(std::string flag)
{
    std::string program = "terminal_romanian_whist";
    std::vector<char*> argv = { program.data(), flag.data() };

    AppOptions options = AppOptions::fromCommandLine(static_cast<int>(argv.size()), argv.data()).options;
    options.interactive = true;
    return options;
}
}

TEST_CASE("An interactive game waits for Enter after each trick and round", "[pacer]")
{
    const PacerFor subject(withTerminal(true, false));

    CHECK(subject.pacer.trickWaitsForEnter());
    CHECK(subject.pacer.roundWaitsForEnter());
}

TEST_CASE("Redirected output never waits for Enter", "[pacer]")
{
    const PacerFor subject(withTerminal(false, false));

    CHECK_FALSE(subject.pacer.trickWaitsForEnter());
    CHECK_FALSE(subject.pacer.roundWaitsForEnter());
}

TEST_CASE("The auto and no-animate flags never wait for Enter, even in a terminal", "[pacer]")
{
    for(const char* flag : { "--auto", "--no-animate" })
    {
        INFO(flag);
        const PacerFor subject(fromFlag(flag));

        CHECK_FALSE(subject.pacer.trickWaitsForEnter());
        CHECK_FALSE(subject.pacer.roundWaitsForEnter());
    }
}
