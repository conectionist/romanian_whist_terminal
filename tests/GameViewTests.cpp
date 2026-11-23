#include <catch2/catch_test_macros.hpp>

#include "CardStringMaker.h"

#include "GameView.h"

#include <romanian_whist/AiMoveProvider.h>
#include <romanian_whist/GameEngine.h>
#include <romanian_whist/IGameObserver.h>
#include <romanian_whist/strategies/DuckingStrategy.h>
#include <romanian_whist/strategies/FirstCardStrategy.h>
#include <romanian_whist/strategies/LowRiskStrategy.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace romanian_whist;

// refreshFromEngine() checked against the engine it reads, at every callback
// of a whole game. These bots don't draw at random and the deck shuffle is the
// same on every platform, so the game is the same everywhere this runs.
namespace
{
const std::vector<std::string> Names{ "Ana", "Bogdan", "Cristi", "Dan" };

GameSetup fourBots()
{
    GameSetup setup;
    setup.seats.push_back(SeatSetup{ Names[0], std::make_unique<AiMoveProvider>(std::make_unique<FirstCardStrategy>()) });
    setup.seats.push_back(SeatSetup{ Names[1], std::make_unique<AiMoveProvider>(std::make_unique<LowRiskStrategy>()) });
    setup.seats.push_back(SeatSetup{ Names[2], std::make_unique<AiMoveProvider>(std::make_unique<DuckingStrategy>()) });
    setup.seats.push_back(SeatSetup{ Names[3], std::make_unique<AiMoveProvider>(std::make_unique<LowRiskStrategy>()) });
    setup.shuffleSeed = 7;
    return setup;
}

// CHECK rather than REQUIRE: these run inside the engine's game loop, and a
// failure should be reported without throwing through it.
class ViewChecker : public IGameObserver
{
public:
    GameView view;

    unsigned int roundsOfType[3] = { 0, 0, 0 };
    unsigned int cardsChecked = 0;

    explicit ViewChecker(std::optional<unsigned int> humanSeat)
    {
        view.humanSeat = humanSeat ? static_cast<int>(*humanSeat) : -1;
    }

    void onRoundStarted(const GameEngine& engine) override
    {
        refreshFromEngine(view, engine);
        roundsOfType[static_cast<int>(engine.getCurrentRoundType())]++;

        CHECK(view.roundNumber == engine.getCurrentRoundIndex() + 1);
        CHECK(view.roundCount == engine.getRoundCount());
        CHECK(view.roundType == engine.getCurrentRoundType());
        CHECK(view.betsPlaced == 0);

        const std::optional<Card> trump = engine.getCurrentTrumpCard();
        CHECK(view.hasTrump == trump.has_value());
        if(trump)
            CHECK(view.trump == *trump);

        REQUIRE(view.seats.size() == Names.size());
        for(std::size_t i = 0 ; i < Names.size() ; i++)
        {
            CHECK(view.seats[i].name == Names[i]);
            CHECK(view.seats[i].cardsLeft == engine.getCurrentRoundTrickCount());
        }
    }

    void onBetRequested(const GameEngine& engine, Seat) override
    {
        refreshFromEngine(view, engine);
        checkVisibleHands(engine);
    }

    void onBetPlaced(const GameEngine& engine, Seat seat, unsigned int bet) override
    {
        refreshFromEngine(view, engine);

        CHECK(view.seats[seat.index].hasBet);
        CHECK(view.seats[seat.index].bet == bet);

        unsigned int placed = 0;
        unsigned int sum = 0;
        for(unsigned int i = 0 ; i < Names.size() ; i++)
        {
            if(const std::optional<unsigned int> placedBet = engine.getBet(Seat{i}))
            {
                placed++;
                sum += *placedBet;
            }
        }

        CHECK(view.betsPlaced == placed);
        CHECK(view.betSum == sum);
    }

    void onCardPlayed(const GameEngine& engine, Seat seat, const Card& card) override
    {
        refreshFromEngine(view, engine);
        cardsChecked++;

        REQUIRE_FALSE(view.table.empty());
        CHECK(view.table.back().seatIndex == seat.index);
        CHECK(view.table.back().card == card);

        CHECK(view.leadSeat == view.table.front().seatIndex);
        CHECK(view.hasLeadSuit);
        CHECK(view.leadSuit == view.table.front().card.suit);

        unsigned int winning = 0;
        for(const PlayedCardView& played : view.table)
        {
            if(played.isWinning)
            {
                winning++;
                CHECK(played.seatIndex == engine.getCurrentTrickLeader()->index);
            }
        }
        CHECK(winning == 1);

        CHECK(view.seats[seat.index].cardsLeft == engine.getPlayers().at(seat.index).getHand().size());
    }

    void onRoundScored(const GameEngine& engine) override
    {
        refreshFromEngine(view, engine);

        for(unsigned int i = 0 ; i < Names.size() ; i++)
        {
            CHECK(view.seats[i].tricksWon == engine.getTricksWon(Seat{i}));
            CHECK(view.seats[i].roundScore == engine.getRoundScore(Seat{i}));
            CHECK(view.seats[i].totalScore == engine.getTotalScore(Seat{i}));
        }
    }

private:
    // Normal rounds show the human their own hand only; Forehead rounds show
    // every other hand but not their own; Hidden rounds show none. A spectated
    // game shows no hand at all.
    void checkVisibleHands(const GameEngine& engine) const
    {
        const RoundType type = engine.getCurrentRoundType();

        if(view.humanSeat < 0)
        {
            CHECK(view.hand.empty());
            for(const SeatView& seat : view.seats)
                CHECK(seat.hand.empty());
            return;
        }

        const auto human = static_cast<std::size_t>(view.humanSeat);

        if(type == RoundType::Normal)
            CHECK(view.hand.size() == view.seats[human].cardsLeft);
        else
            CHECK(view.hand.empty());

        for(std::size_t i = 0 ; i < view.seats.size() ; i++)
        {
            if(i == human)
                CHECK(view.seats[i].hand.empty());
            else if(type == RoundType::Forehead)
                CHECK(view.seats[i].hand.size() == view.seats[i].cardsLeft);
            else
                CHECK(view.seats[i].hand.empty());
        }
    }
};

void playWith(ViewChecker& checker)
{
    GameEngine engine;
    engine.addObserver(&checker);
    engine.start(fourBots());
    engine.run();

    // Every kind of round was reached, so every branch above was exercised.
    CHECK(checker.roundsOfType[static_cast<int>(RoundType::Normal)] > 0);
    CHECK(checker.roundsOfType[static_cast<int>(RoundType::Forehead)] > 0);
    CHECK(checker.roundsOfType[static_cast<int>(RoundType::Hidden)] > 0);
    CHECK(checker.cardsChecked > 0);

    // The view was last refreshed when the final round was scored, before its
    // score was added to the totals, so the two together are the final score.
    for(const Standing& standing : engine.getStandings())
    {
        const SeatView& seat = checker.view.seats[standing.seat.index];
        CHECK(seat.totalScore + seat.roundScore == standing.score);
    }
}
} // namespace

TEST_CASE("the view matches the engine throughout a spectated game", "[game-view]")
{
    ViewChecker checker(std::nullopt);
    playWith(checker);

    for(const SeatView& seat : checker.view.seats)
        CHECK_FALSE(seat.isHuman);
}

TEST_CASE("the view shows a seated player only the hands the round allows", "[game-view]")
{
    ViewChecker checker(0u);
    playWith(checker);

    CHECK(checker.view.seats[0].isHuman);
    CHECK_FALSE(checker.view.seats[1].isHuman);
}
