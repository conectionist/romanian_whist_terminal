#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include <romanian_whist/Card.h>
#include <romanian_whist/GameEngine.h>
#include <romanian_whist/RoundType.h>
#include <romanian_whist/Seat.h>

#include <optional>
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

    // Empty unless the viewer can see this seat's hand, and always empty for the
    // human's own: theirs goes in GameView::hand, so ConsoleMoveProvider has one
    // place to write its numbering.
    std::vector<HandEntry> hand;
};

struct PlayedCardView
{
    unsigned int seatIndex = 0;
    romanian_whist::Card card{};
    bool isWinning = false;
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

    // Empty when nobody is seated there: spectator games have no human, and
    // between turns nobody is active.
    std::optional<romanian_whist::Seat> humanSeat;
    std::optional<romanian_whist::Seat> activeSeat;
    unsigned int leadSeat = 0;

    bool hasLeadSuit = false;
    romanian_whist::Suit leadSuit{};
    std::vector<PlayedCardView> table;

    std::vector<HandEntry> hand;
    unsigned int legalCount = 0;

    std::string message;
    std::string hint;
    std::string error;

    // Minutes since the game started. The engine has no notion of wall-clock
    // time, so this is left to the caller to set - same as activeSeat, message
    // and hint above.
    unsigned int elapsedMinutes = 0;

    unsigned int betsPlaced = 0;
    unsigned int betSum = 0;

    const SeatView* humanSeatView() const;
};

// Refills everything readable straight from the engine: seats, scores, bets,
// streaks, hand sizes, round and trick counts, trump and round type, and the
// cards on the table.
//
// Reads the engine, and view.humanSeat to decide which hands to fill in. The
// caller sets the rest: the phase, the trick number, the active seat, the
// human's numbered hand, any message or hint, and elapsed wall-clock time.
void refreshFromEngine(GameView& view, const romanian_whist::GameEngine& game);

#endif
