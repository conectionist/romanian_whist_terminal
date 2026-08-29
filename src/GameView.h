#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include <romanian_whist/Card.h>
#include <romanian_whist/GameEngine.h>
#include <romanian_whist/RoundType.h>

#include <string>
#include <vector>

// What the screen shows, as plain data.
//
// The renderer reads only this, never the engine, which keeps drawing code
// unable to advance the game by accident and means refreshFromEngine is the one
// place that has to know how engine state maps onto the display.
//
// Cards are held by value: a snapshot outlives nothing and borrows nothing.

enum class Phase
{
    Setup,
    Betting,
    Playing,
    TrickEnd,
    RoundEnd,
    GameOver
};

struct SeatView
{
    std::string name;
    unsigned int seatIndex = 0;

    // Position in this round's bidding, 1 for whoever opened it.
    unsigned int bidOrder = 0;

    bool isHuman = false;
    bool hasBet = false;
    unsigned int bet = 0;
    unsigned int tricksWon = 0;
    unsigned int cardsLeft = 0;
    int roundScore = 0;
    int totalScore = 0;
    unsigned int winStreak = 0;
    unsigned int lossStreak = 0;
};

struct PlayedCardView
{
    unsigned int seatIndex = 0;
    romanian_whist::Card card{};
    bool isWinning = false;
};

struct HandEntry
{
    romanian_whist::Card card{};

    // Whether this card could be played on the current trick. False greys it
    // out. Every card is "legal" on the bidding screen, where no card is being
    // chosen at all.
    bool legal = false;

    // The number the player types to choose this card. Zero means no number is
    // offered - either the card cannot be played, or nothing is being chosen.
    // Numbering runs over displayed order, so what is shown and what is typed
    // cannot drift apart.
    unsigned int choice = 0;
};

struct GameView
{
    Phase phase = Phase::Setup;

    unsigned int roundNumber = 0;
    unsigned int roundCount = 0;
    unsigned int trickCount = 0;
    unsigned int trickNumber = 0;
    romanian_whist::RoundType roundType = romanian_whist::RoundType::Normal;

    bool hasTrump = false;
    romanian_whist::Card trump{};

    std::vector<SeatView> seats;

    // -1 when nobody is seated there: spectator games have no human, and
    // between turns nobody is active.
    int humanSeat = -1;
    int activeSeat = -1;
    unsigned int leadSeat = 0;

    bool hasLeadSuit = false;
    romanian_whist::Suit leadSuit{};
    std::vector<PlayedCardView> table;

    std::vector<HandEntry> hand;
    unsigned int legalCount = 0;

    std::string message;
    std::string hint;
    std::string error;

    unsigned int betsPlaced = 0;
    unsigned int betSum = 0;

    const SeatView* humanSeatView() const;
};

// Refills everything readable straight from the engine: seats, scores, bets,
// streaks, hand sizes, round and trick counts, trump and round type.
//
// Leaves the caller to set what the engine does not model - which seat is
// active, what is on the table, the human's hand layout, and any message.
// `openingSeat` is the round's opening bidder, which the caller caches. The
// round knows it too (Round::getOpenerSeat()), so this parameter is redundant
// and goes when Phase 2 rebuilds the table from the engine's in-flight trick.
void refreshFromEngine(GameView& view,
                       const romanian_whist::GameEngine& game,
                       unsigned int openingSeat);

#endif
