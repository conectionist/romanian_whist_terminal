#ifndef TERMINAL_ROMANIAN_WHIST_H
#define TERMINAL_ROMANIAN_WHIST_H

#include <romanian_whist/GameEngine.h>

#include "AppOptions.h"
#include "Terminal.h"

class TerminalRomanianWhist
{
private:
    AppOptions& options;
    Terminal terminal;
    romanian_whist::GameEngine game;

public:
    explicit TerminalRomanianWhist(AppOptions& _options);

    void startGame();

private:
    void initialize();
    void initializeTest();
    void loop();
    void playCurrentRoundTricks();
    void displayScoreboard();
    void displayFinalResults();
};

#endif
