#include "Pacer.h"

#include "Input.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <thread>

#ifdef _WIN32
    // See the note in Terminal.cpp: any Windows header that drags in windows.h
    // would define min as a macro and break the std::min below.
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <conio.h>
#else
    #include <poll.h>
    #include <unistd.h>
#endif

namespace
{

// Bots play faster when nobody is waiting on their own turn.
constexpr std::chrono::milliseconds SpectatorBeat{250};

// Discards the line the player pressed Enter on, so it does not go on to
// satisfy the next prompt as well.
//
// Read through std::cin rather than the file descriptor: every other read in
// the program goes through cin, and draining bytes behind its back would leave
// its buffer inconsistent with the stream. The terminal is in line mode, so the
// poll that got us here means a complete line is waiting and this cannot block.
void discardPendingLine()
{
#ifdef _WIN32
    while(_kbhit())
        _getch();
#else
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
#endif
}

} // namespace

Pacer::Pacer(const AppOptions& _options, const Terminal& _terminal)
    : options(_options),
      terminal(_terminal),
      humanIsPlaying(false),
      skipRestOfRound(false)
{}

void Pacer::setHumanIsPlaying(bool _humanIsPlaying)
{
    humanIsPlaying = _humanIsPlaying;
}

void Pacer::resetSkip()
{
    skipRestOfRound = false;
}

bool Pacer::sleepOrSkip(std::chrono::milliseconds duration)
{
    if(duration.count() <= 0)
        return false;

    if(!terminal.interactive())
        return false;

#ifdef _WIN32
    // No portable way to wait on the console handle and a timeout together
    // without a message loop, so Windows sleeps and checks afterwards. It
    // costs one beat of latency on a skip; it never garbles output.
    std::this_thread::sleep_for(duration);

    if(_kbhit())
    {
        discardPendingLine();
        return true;
    }

    return false;
#else
    pollfd input{};
    input.fd = STDIN_FILENO;
    input.events = POLLIN;

    const int ready = poll(&input, 1, static_cast<int>(duration.count()));

    if(ready > 0)
    {
        discardPendingLine();
        return true;
    }

    return false;
#endif
}

void Pacer::beat()
{
    // Note that --auto does not silence the beat: it means "do not wait for me",
    // not "do not animate". --no-animate sets the delay to zero, which does.
    if(skipRestOfRound)
        return;

    // Nobody is waiting on their own turn in a spectator game, so it can move
    // along faster. An explicit --delay still wins if it asks for less.
    const std::chrono::milliseconds delay = humanIsPlaying
        ? options.playDelay
        : std::min(options.playDelay, SpectatorBeat);

    if(sleepOrSkip(delay))
        skipRestOfRound = true;
}

void Pacer::trickPause()
{
    if(skipRestOfRound)
        return;

    if(!terminal.interactive())
        return;

    if(options.autoAdvance)
    {
        if(sleepOrSkip(options.trickDwell))
            skipRestOfRound = true;
        return;
    }

    Input::waitForEnter(true);
}

void Pacer::roundPause()
{
    // A skip runs to the end of the round and stops here, which is the point:
    // the scoreboard is what it was skipping ahead to.
    if(!terminal.interactive())
        return;

    if(options.autoAdvance)
    {
        sleepOrSkip(options.trickDwell);
        return;
    }

    Input::waitForEnter(true);
}
