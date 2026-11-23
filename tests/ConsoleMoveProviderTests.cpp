#include <catch2/catch_test_macros.hpp>

#include "CardStringMaker.h"

#include "AppOptions.h"
#include "CardFormat.h"
#include "ConsoleMoveProvider.h"
#include "Terminal.h"

#include <romanian_whist/CardValidator.h>

#include <optional>
#include <string>
#include <vector>

using romanian_whist::Card;
using romanian_whist::CardValidator;
using romanian_whist::Rank;
using romanian_whist::Suit;

namespace
{
// The legal cards come from the engine's own rules, so these tests check the
// wording against what the player is really allowed to play.
std::string hintFor(const std::vector<Card>& hand,
                    const std::optional<Card>& trump,
                    const std::optional<Suit>& leadSuit)
{
    // Plain text, so the suit led is written as a letter.
    AppOptions options;
    options.color = false;
    options.unicode = false;
    options.screenControl = false;
    options.altScreen = false;
    options.interactive = false;

    const Terminal terminal(options);
    const CardFormat cardFormat(terminal);

    const std::size_t legalCount = CardValidator{}.getLegalCards(hand, trump, leadSuit).size();

    return ConsoleMoveProvider::playHint(hand, legalCount, leadSuit, cardFormat);
}

const Card SpadesTrump(Rank::Two, Suit::Spades);
} // namespace

TEST_CASE("the first card of a trick is a lead", "[move-provider]")
{
    const std::vector<Card> hand{ Card(Rank::Ace, Suit::Hearts), Card(Rank::Two, Suit::Spades) };

    REQUIRE(hintFor(hand, SpadesTrump, std::nullopt) == "you lead");
}

TEST_CASE("holding the suit led, you must follow it", "[move-provider]")
{
    const std::vector<Card> hand{ Card(Rank::Ace, Suit::Hearts),
                                  Card(Rank::Three, Suit::Spades),
                                  Card(Rank::King, Suit::Clubs) };

    REQUIRE(hintFor(hand, SpadesTrump, Suit::Hearts) == "you must follow H");
}

TEST_CASE("without the suit led, holding a trump, you must play a trump", "[move-provider]")
{
    const std::vector<Card> hand{ Card(Rank::Three, Suit::Spades),
                                  Card(Rank::King, Suit::Clubs),
                                  Card(Rank::Ten, Suit::Diamonds) };

    REQUIRE(hintFor(hand, SpadesTrump, Suit::Hearts) == "you must play a trump");
}

TEST_CASE("no hint when every card can be played", "[move-provider]")
{
    SECTION("without the suit led or a trump")
    {
        const std::vector<Card> hand{ Card(Rank::King, Suit::Clubs), Card(Rank::Ten, Suit::Diamonds) };

        REQUIRE(hintFor(hand, SpadesTrump, Suit::Hearts).empty());
    }

    SECTION("in a round without trumps")
    {
        const std::vector<Card> hand{ Card(Rank::King, Suit::Clubs), Card(Rank::Ten, Suit::Diamonds) };

        REQUIRE(hintFor(hand, std::nullopt, Suit::Hearts).empty());
    }

    SECTION("holding nothing but the suit led")
    {
        const std::vector<Card> hand{ Card(Rank::Ace, Suit::Hearts), Card(Rank::Four, Suit::Hearts) };

        REQUIRE(hintFor(hand, SpadesTrump, Suit::Hearts).empty());
    }
}

TEST_CASE("when trumps are led, holding one is following suit", "[move-provider]")
{
    const std::vector<Card> hand{ Card(Rank::Three, Suit::Spades), Card(Rank::King, Suit::Clubs) };

    REQUIRE(hintFor(hand, SpadesTrump, Suit::Spades) == "you must follow S");
}
