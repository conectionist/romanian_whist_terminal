#ifndef TERMINAL_ROMANIAN_WHIST_H
#define TERMINAL_ROMANIAN_WHIST_H

#include <romanian_whist/GameEngine.h>

#include "AppOptions.h"
#include "CardFormat.h"
#include "GameView.h"
#include "Renderer.h"
#include "Terminal.h"
#include "UiContext.h"

#include <romanian_whist/Trick.h>

class TerminalRomanianWhist
{
private:
    AppOptions& options;
    Terminal terminal;
    CardFormat cardFormat;
    Renderer renderer;
    GameView view;
    UiContext ui;

    // Declared last on purpose. The engine owns the players, a player owns the
    // human's ConsoleMoveProvider, and that provider holds a reference to `ui`
    // above. Members are destroyed in reverse declaration order, so this keeps
    // the provider dying before the context it points at. Do not move it.
    romanian_whist::GameEngine game;

    // Who opened this round's bidding. Cached because the engine's first player
    // moves to each trick's winner as the round plays out, so by the time
    // anything asks, it no longer answers this question.
    unsigned int openingSeat = 0;

public:
    explicit TerminalRomanianWhist(AppOptions& _options);

    void startGame();

private:
    void initialize();
    void initializeTest();
    void loop();
    void playCurrentRoundTricks();
    void markCurrentlyWinning(const romanian_whist::Trick& trick,
                              romanian_whist::PlayerList::iterator leader);
    unsigned int seatOf(romanian_whist::PlayerList::const_iterator player) const;
};

#endif
