#ifndef APP_OPTIONS_H
#define APP_OPTIONS_H

#include <chrono>
#include <string>

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

    std::chrono::milliseconds playDelay{700};   // between one bot's play and the next
    std::chrono::milliseconds trickDwell{1200}; // how long a finished trick sits on screen

    // Returns false and fills errorOut on a malformed flag. `helpRequested` and
    // `versionRequested` are handled by the caller, which prints and exits.
    static bool fromCommandLine(int argc,
                                char** argv,
                                AppOptions& optionsOut,
                                bool& helpRequested,
                                bool& versionRequested,
                                std::string& errorOut);

    static std::string helpText();
};

#endif
