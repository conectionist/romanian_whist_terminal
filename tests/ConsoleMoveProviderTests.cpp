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
// Plain text, so suits are written as letters.
struct PlainFormat
{
    AppOptions options;
    Terminal terminal;
    CardFormat cardFormat;

    PlainFormat() : options(plainOptions()), terminal(options), cardFormat(terminal) {}

    static AppOptions plainOptions()
    {
        AppOptions plain;
        plain.color = false;
        plain.unicode = false;
        plain.screenControl = false;
        plain.altScreen = false;
        plain.interactive = false;
        return plain;
    }
};

// The legal cards come from the engine's own rules, so these tests check the
// wording against what the player is really allowed to play.
std::string hintFor(const std::vector<Card>& hand,
                    const std::optional<Card>& trump,
                    const std::optional<Suit>& leadSuit)
{
    const PlainFormat plain;
    const std::size_t legalCount = CardValidator{}.getLegalCards(hand, trump, leadSuit).size();

    return ConsoleMoveProvider::playHint(hand, legalCount, trump, leadSuit, plain.cardFormat);
}

// Turned up after the deal, so no hand below holds it.
const Card SpadesTrump(Rank::Two, Suit::Spades);
} // namespace

TEST_CASE("the first card of a trick is a lead", "[move-provider]")
{
    const std::vector<Card> hand{ Card(Rank::Ace, Suit::Hearts), Card(Rank::Three, Suit::Spades) };

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

    REQUIRE(hintFor(hand, SpadesTrump, Suit::Hearts) == "you must play a trump (S)");
}

TEST_CASE("no hint when neither rule explains the barred cards", "[move-provider]")
{
    // The engine's rules cannot bar a card here, but a rule it adds later could,
    // and naming a rule that does not apply would mislead.
    const PlainFormat plain;
    const std::vector<Card> hand{ Card(Rank::King, Suit::Clubs), Card(Rank::Ten, Suit::Diamonds) };

    SECTION("in a round without trumps")
    {
        REQUIRE(ConsoleMoveProvider::playHint(hand, 1, std::nullopt, Suit::Hearts, plain.cardFormat).empty());
    }

    SECTION("holding no trump")
    {
        REQUIRE(ConsoleMoveProvider::playHint(hand, 1, SpadesTrump, Suit::Hearts, plain.cardFormat).empty());
    }
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
