#include <catch2/catch_test_macros.hpp>

#include "CardStringMaker.h"

#include "CardFormat.h"

#include <optional>

using romanian_whist::Card;
using romanian_whist::Rank;
using romanian_whist::Suit;

// inDisplayOrder is the comparator the whole hand layout is sorted by, and it
// is handed to std::sort, which is not stable. That is only safe because no two
// cards of one hand can compare equivalent - the last case here is what pins
// that, and it is the reason display order is reproducible at all.

namespace
{
const std::optional<Card> NoTrump;
}

TEST_CASE("Suits are drawn in a fixed alternating-colour order", "[card-format]")
{
    // Spades, hearts, clubs, diamonds: neighbouring groups differ in colour, so
    // they do not blur together on screen.
    const Card spades(Rank::Two, Suit::Spades);
    const Card hearts(Rank::Two, Suit::Hearts);
    const Card clubs(Rank::Two, Suit::Clubs);
    const Card diamonds(Rank::Two, Suit::Diamonds);

    REQUIRE(CardFormat::inDisplayOrder(spades, hearts, NoTrump));
    REQUIRE(CardFormat::inDisplayOrder(hearts, clubs, NoTrump));
    REQUIRE(CardFormat::inDisplayOrder(clubs, diamonds, NoTrump));

    // And the ordering is genuinely an ordering, not just true one way round.
    REQUIRE_FALSE(CardFormat::inDisplayOrder(hearts, spades, NoTrump));
    REQUIRE_FALSE(CardFormat::inDisplayOrder(diamonds, clubs, NoTrump));
}

TEST_CASE("Trump sorts ahead of every other suit", "[card-format]")
{
    const Card trump(Rank::Two, Suit::Diamonds);

    const Card diamondsTwo(Rank::Two, Suit::Diamonds);
    const Card spadesAce(Rank::Ace, Suit::Spades);

    // Diamonds sort last of all four without trump, and first with it - even
    // the lowest diamond against the highest spade.
    REQUIRE_FALSE(CardFormat::inDisplayOrder(diamondsTwo, spadesAce, NoTrump));
    REQUIRE(CardFormat::inDisplayOrder(diamondsTwo, spadesAce, trump));
}

TEST_CASE("Rank runs high to low within a suit", "[card-format]")
{
    const Card ace(Rank::Ace, Suit::Hearts);
    const Card king(Rank::King, Suit::Hearts);
    const Card two(Rank::Two, Suit::Hearts);

    REQUIRE(CardFormat::inDisplayOrder(ace, king, NoTrump));
    REQUIRE(CardFormat::inDisplayOrder(king, two, NoTrump));
    REQUIRE_FALSE(CardFormat::inDisplayOrder(king, ace, NoTrump));
}

TEST_CASE("No two cards of one hand ever compare equivalent", "[card-format]")
{
    // The property that lets an unstable sort still produce one fixed display
    // order. Equivalence would need the same suit position and the same rank -
    // and suit position is injective over suits, so that means the same card.
    // A hand holds no duplicates, so it cannot happen.
    const Card heartsKing(Rank::King, Suit::Hearts);
    const Card spadesKing(Rank::King, Suit::Spades);

    // Same rank, different suits: one of the two directions must hold.
    REQUIRE(CardFormat::inDisplayOrder(spadesKing, heartsKing, NoTrump) !=
            CardFormat::inDisplayOrder(heartsKing, spadesKing, NoTrump));

    // A card against itself is the one equivalence there is, and it is
    // irreflexive as a strict weak ordering requires.
    REQUIRE_FALSE(CardFormat::inDisplayOrder(heartsKing, heartsKing, NoTrump));
}
