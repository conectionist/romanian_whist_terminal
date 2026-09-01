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

namespace
{
// What the game loop used to accumulate in the client, one push_back per card.
// The engine holds the trick now, and every entry carries the seat that played
// it - so this reads the table off rather than reconstructing it.
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

    // The seat the renderer walks the table from. It MUST be whoever led this
    // trick, and getTrickLeaderSeat() is not that from the moment the trick is
    // won - it has already moved on to the winner, which is precisely when the
    // trick-end screen is drawn. The trick's own first entry is the honest
    // answer, and falls back to the leader only while nothing has been played.
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
        seat.isHuman = view.humanSeat >= 0 && static_cast<unsigned int>(view.humanSeat) == i;

        // Bidding runs from whoever opened the round, wrapping round the
        // table. The engine works that out now - reimplementing the modular
        // arithmetic here is exactly the sort of turn-order rule that has no
        // business living in a client.
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

    if(view.humanSeat < 0)
        return;

    std::vector<romanian_whist::Card> ordered =
        players.at(static_cast<std::size_t>(view.humanSeat)).getHand();

    CardFormat::sortForDisplay(ordered, trump);

    for(const romanian_whist::Card& card : ordered)
    {
        HandEntry entry;
        entry.card = card;
        entry.legal = true;
        view.hand.push_back(entry);
    }
}
