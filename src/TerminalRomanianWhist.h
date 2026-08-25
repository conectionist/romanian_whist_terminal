#ifndef TERMINAL_ROMANIAN_WHIST_H
#define TERMINAL_ROMANIAN_WHIST_H

#include <romanian_whist/GameEngine.h>

class TerminalRomanianWhist
{
private:
    romanian_whist::GameEngine game;

public:
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
