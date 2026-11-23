#ifndef APP_OPTIONS_H
#define APP_OPTIONS_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <romanian_whist/strategies/reckoner/ReckonerKnobs.h>

#include "DisplayText.h"

struct CommandLine;

// How the interface is allowed to draw itself, and how fast it plays.
//
// Every field starts at what a capable interactive terminal supports and is
// then knocked down by the environment (not a TTY, NO_COLOR set, TERM=dumb, a
// non-UTF-8 locale) and finally by explicit flags, which always win. The rest
// of the program never checks isatty or getenv itself - it asks here.
struct AppOptions
{
    bool color = true;          // ANSI colour sequences
    bool unicode = true;        // suit glyphs and box drawing
    bool screenControl = true;  // redraw in place instead of scrolling
    bool altScreen = true;      // draw on the alternate screen, keeping scrollback
    bool interactive = true;    // stdin and stdout are both terminals
    bool autoAdvance = false;   // never wait for Enter
    bool demo = false;          // skip setup: four bots, 1-8-1

    // What the locale says the terminal reads, which decides the bytes it would
    // obey as controls in text that comes from outside.
    DisplayText::Encoding encoding = DisplayText::Encoding::Utf8;

    std::chrono::milliseconds playDelay{700};   // between one bot's play and the next
    std::chrono::milliseconds trickDwell{1200}; // how long a finished trick sits on screen

    // Set to replay the same game. It seeds the deal and every bot that draws
    // randomly, which together are the only sources of chance in a spectated
    // game - so `--demo --no-animate --seed=N` prints the same transcript every
    // time, and two builds can be diffed against each other. Unset means both
    // seed themselves from std::random_device.
    std::optional<std::uint32_t> seed;

    // Preset for the Reckoner AI strategy (default: Gamma).
    romanian_whist::reckoner::ReckonerPreset reckonerPreset =
        romanian_whist::reckoner::ReckonerPreset::Gamma;

    static CommandLine fromCommandLine(int argc, char** argv);

    static std::string helpText();
};

// `options` is complete unless the action is Error: parsing stops at the bad
// flag, before the delay flags are applied.
struct CommandLine
{
    enum class Action
    {
        Run,
        Help,
        Version,
        Error
    };

    Action action = Action::Run;
    AppOptions options;
    std::string error;
};

#endif
