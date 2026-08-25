#ifndef PACER_H
#define PACER_H

#include "AppOptions.h"
#include "Terminal.h"

#include <functional>

// How fast the game plays out.
//
// Cards are dealt onto the table one at a time rather than all at once, because
// four cards appearing together says nothing about who played what in response
// to whom.
//
// Every wait is skippable: pressing Enter during a hand jumps to the end of the
// round. Waiting on a bare keypress would need raw mode, and a raw-mode toggle
// around every sleep is a poor trade for saving one key.
//
// When output is redirected nothing waits at all, so piping a script of moves
// in runs at full speed.
class Pacer
{
private:
    const AppOptions& options;
    const Terminal& terminal;
    bool humanIsPlaying;
    bool skipRestOfRound;

    // Returns true if the player interrupted the wait.
    bool sleepOrSkip(std::chrono::milliseconds duration);

public:
    Pacer(const AppOptions& _options, const Terminal& _terminal);

    // Whether anyone is sitting at the table, which decides whether a finished
    // trick waits for a keypress or simply dwells.
    void setHumanIsPlaying(bool _humanIsPlaying);

    // Called at the start of each round: a skip only skips the round it was
    // asked for.
    void resetSkip();

    // Between one player's card and the next.
    void beat();

    // After a trick is decided. Waits for Enter, same as roundPause - --auto
    // (or --no-animate, which implies it) dwells instead.
    void trickPause();

    // After a round is scored. Waits whenever there is anyone to wait for -
    // this is the moment worth reading, and the one a skip runs ahead to.
    void roundPause();
};

#endif
