#include "GameView.h"

using romanian_whist::GameEngine;
using romanian_whist::Round;

const SeatView* GameView::humanSeatView() const
{
    if(humanSeat < 0 || static_cast<std::size_t>(humanSeat) >= seats.size())
        return nullptr;

    return &seats[static_cast<std::size_t>(humanSeat)];
}

void refreshFromEngine(GameView& view, const GameEngine& game, unsigned int openingSeat)
{
    const Round& round = game.getCurrentRound();
    const auto& players = game.getPlayers();
    const unsigned int playerCount = game.getPlayerCount();

    view.roundNumber = game.getCurrentRoundIndex() + 1;
    view.roundCount = game.getRoundCount();
    view.trickCount = game.getCurrentRoundTrickCount();
    view.roundType = game.getCurrentRoundType();

    const romanian_whist::Card* trump = game.getCurrentTrumpCard();
    view.hasTrump = trump != nullptr;
    if(trump != nullptr)
        view.trump = *trump;

    view.seats.resize(playerCount);
    view.betsPlaced = 0;
    view.betSum = 0;

    for(unsigned int i = 0 ; i < playerCount ; i++)
    {
        const romanian_whist::Player& player = players.at(i);
        SeatView& seat = view.seats[i];

        seat.name = player.getName();
        seat.seatIndex = i;
        seat.isHuman = view.humanSeat >= 0 && static_cast<unsigned int>(view.humanSeat) == i;

        // Bidding runs from whoever opened the round, wrapping round the table.
        seat.bidOrder = ((i + playerCount - openingSeat) % playerCount) + 1;

        // getBet returns 0 both for a bid of zero and for no bid at all, so the
        // display has to ask which it is before showing a number.
        seat.hasBet = round.hasBet(seat.name);
        seat.bet = seat.hasBet ? round.getBet(seat.name) : 0;
        seat.tricksWon = round.getActual(seat.name);

        if(seat.hasBet)
        {
            view.betsPlaced++;
            view.betSum += seat.bet;
        }

        seat.cardsLeft = static_cast<unsigned int>(player.getHand().size());
        seat.roundScore = player.getCurrentRoundScore();
        seat.totalScore = player.getTotalScore();
        seat.winStreak = player.getConsecutiveWins();
        seat.lossStreak = player.getConsecutiveLosses();
    }
}
