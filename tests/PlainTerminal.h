#ifndef TERMINAL_PLAIN_TERMINAL_H
#define TERMINAL_PLAIN_TERMINAL_H

#include "AppOptions.h"
#include "CardFormat.h"
#include "Terminal.h"

// A terminal with colour, glyphs and screen control off, so what a test checks
// is plain text: suits are letters and nothing is written to the real screen.
struct PlainTerminal
{
    AppOptions options;
    Terminal terminal;
    CardFormat cardFormat;

    PlainTerminal() : options(plainOptions()), terminal(options), cardFormat(terminal) {}

    static AppOptions plainOptions()
    {
        AppOptions plain;
        plain.color = false;
        plain.unicode = false;
        plain.screenControl = false;
        plain.altScreen = false;
        plain.interactive = false;
        return plain;
    }
};

#endif
