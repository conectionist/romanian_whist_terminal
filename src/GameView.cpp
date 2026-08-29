#include "GameView.h"

#include "CardFormat.h"

#include <optional>

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

        // Disengaged until this seat has bid, which is what tells a genuine
        // bid of zero from no bid at all.
        const std::optional<unsigned int> bet = round.getBet(romanian_whist::Seat{i});

        seat.hasBet = bet.has_value();
        seat.bet = bet.value_or(0);
        seat.tricksWon = round.getTricksWon(romanian_whist::Seat{i});

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

    // Rebuild the human's hand every frame rather than leaving whatever was
    // last drawn on screen: cards leave the hand as they are played, and a hand
    // held over from the previous round is worse than none at all.
    //
    // Unnumbered here. The move provider re-lays it with numbers when it is
    // actually the human's turn to choose a card.
    view.hand.clear();
    view.legalCount = 0;

    if(view.humanSeat < 0)
        return;

    std::vector<const romanian_whist::Card*> ordered(
        players.at(static_cast<std::size_t>(view.humanSeat)).getHand().begin(),
        players.at(static_cast<std::size_t>(view.humanSeat)).getHand().end());

    CardFormat::sortForDisplay(ordered, trump);

    for(const romanian_whist::Card* card : ordered)
    {
        HandEntry entry;
        entry.card = *card;
        entry.legal = true;
        view.hand.push_back(entry);
    }
}
