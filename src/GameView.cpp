#include "GameView.h"

#include "HandLayout.h"

#include "CardFormat.h"

#include <optional>

using romanian_whist::GameEngine;
using romanian_whist::Round;

const SeatView* GameView::humanSeatView() const
{
    if(!humanSeat || humanSeat->index >= seats.size())
        return nullptr;

    return &seats[humanSeat->index];
}

namespace
{
// Seats come from the trick itself, so the client never has to work out turn
// order to know who played what.
void rebuildTable(GameView& view, const GameEngine& game)
{
    const romanian_whist::Trick& trick = game.getCurrentTrick();
    const std::vector<romanian_whist::PlayedCard>& played = trick.getPlayedCards();

    view.table.clear();

    for(const romanian_whist::PlayedCard& entry : played)
        view.table.push_back({ entry.seat.index, entry.card, false });

    // Who is winning, asked of the engine rather than reimplemented here, so
    // the highlight can never disagree with the winner it eventually declares.
    // It copes with a partly played trick, which is the whole point: this is
    // asked once per card, not once per trick.
    if(const std::optional<romanian_whist::Seat> leading = game.getCurrentTrickLeader())
    {
        for(PlayedCardView& entry : view.table)
            entry.isWinning = entry.seatIndex == leading->index;
    }

    view.hasLeadSuit = trick.hasLeadSuit();

    if(view.hasLeadSuit)
        view.leadSuit = trick.getLeadSuit();

    // Where the renderer starts the table: the trick's first card, since
    // getTrickLeaderSeat() moves on to the winner as the trick-end screen is
    // drawn. Until a card is played, the leader is the right seat.
    view.leadSeat = played.empty() ? game.getTrickLeaderSeat().index
                                   : played.front().seat.index;
}
}

void refreshFromEngine(GameView& view, const GameEngine& game)
{
    const auto& players = game.getPlayers();
    const unsigned int playerCount = game.getPlayerCount();

    view.roundNumber = game.getCurrentRoundIndex() + 1;
    view.roundCount = game.getRoundCount();
    view.trickCount = game.getCurrentRoundTrickCount();
    view.roundType = game.getCurrentRoundType();

    const std::optional<romanian_whist::Card> trump = game.getCurrentTrumpCard();
    view.hasTrump = trump.has_value();
    if(trump)
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
        seat.isHuman = view.humanSeat == romanian_whist::Seat{i};

        // Bidding runs from whoever opened the round, wrapping round the table.
        // That is a turn-order rule, so it is asked of the engine rather than
        // worked out here.
        seat.bidOrder = game.getBiddingOrder(romanian_whist::Seat{i});

        // Disengaged until this seat has bid, which is what tells a genuine
        // bid of zero from no bid at all.
        const std::optional<unsigned int> bet = game.getBet(romanian_whist::Seat{i});

        seat.hasBet = bet.has_value();
        seat.bet = bet.value_or(0);
        seat.tricksWon = game.getTricksWon(romanian_whist::Seat{i});

        if(seat.hasBet)
        {
            view.betsPlaced++;
            view.betSum += seat.bet;
        }

        seat.cardsLeft = static_cast<unsigned int>(player.getHand().size());
        seat.roundScore = game.getRoundScore(romanian_whist::Seat{i});
        seat.totalScore = game.getTotalScore(romanian_whist::Seat{i});
        seat.winStreak = player.getConsecutiveWins();
        seat.lossStreak = player.getConsecutiveLosses();

        // Cleared every frame, because which hands the human can see changes
        // from round to round. A spectator sees none: showing them every hand
        // is a policy the client has not taken up.
        seat.hand.clear();

        if(view.humanSeat && !seat.isHuman)
        {
            if(game.canSeeHand(*view.humanSeat, romanian_whist::Seat{i}))
                seat.hand = layOutHand(player.getHand(), {}, trump).entries;
        }
    }

    rebuildTable(view, game);

    // Rebuild the human's hand every frame rather than leaving whatever was
    // last drawn on screen: cards leave the hand as they are played, and a hand
    // held over from the previous round is worse than none at all.
    //
    // Unnumbered here. The move provider re-lays it with numbers when it is
    // actually the human's turn to choose a card.
    view.hand.clear();
    view.legalCount = 0;

    if(!view.humanSeat)
        return;

    const romanian_whist::Seat humanSeat = *view.humanSeat;

    // Forehead: the human cannot see their own hand while it is being bid on.
    // Hidden: nobody can. Both leave this cleared, same as spectator games.
    if(!game.canSeeHand(humanSeat, humanSeat))
        return;

    // The same layout the move provider draws, asked for with no legal set -
    // which is what says "nothing is being chosen here", so every card reads as
    // legal and none is numbered. legalCount stays 0 for the same reason.
    view.hand = layOutHand(players.at(humanSeat.index).getHand(),
                           {},
                           trump).entries;
}
