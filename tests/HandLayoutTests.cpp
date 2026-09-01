#include <catch2/catch_test_macros.hpp>

#include "CardStringMaker.h"

#include "HandLayout.h"

#include <cstddef>
#include <optional>
#include <vector>

using romanian_whist::Card;
using romanian_whist::Rank;
using romanian_whist::Suit;

// This function reconciles three orderings and no two of them coincide: the
// hand as the engine holds it, the order the cards are drawn in, and the
// numbered legal subset the player types a number from. Get the mapping wrong
// and the player types 2 and plays a different card - which breaks no rule the
// engine could object to, so nothing downstream would notice.
//
// Until this file, the only thing that had ever checked it was a one-off
// analysis of a rendered transcript, which cannot run in CI.

namespace
{
// Deliberately NOT in display order: spades sort first, then hearts, then
// clubs, then diamonds, and rank runs high to low inside a suit. So the display
// order of this hand is A-spades, Q-spades, K-hearts, 7-diamonds - hand
// positions 1, 3, 2, 0, which share no prefix with deal order.
const Card DiamondsSeven(Rank::Seven, Suit::Diamonds);   // hand position 0
const Card SpadesAce(Rank::Ace, Suit::Spades);           // hand position 1
const Card HeartsKing(Rank::King, Suit::Hearts);         // hand position 2
const Card SpadesQueen(Rank::Queen, Suit::Spades);       // hand position 3

const std::vector<Card> Hand{ DiamondsSeven, SpadesAce, HeartsKing, SpadesQueen };

// The invariant the whole function exists to hold: whatever number is shown
// against a card, following it through `choices` lands on that same card in the
// hand the engine holds. Checked over every numbered entry rather than at one
// position, so an error anywhere in the mapping shows up.
void requireNumbersLeadBackToTheRightCards(const HandLayout& layout, const std::vector<Card>& hand)
{
    unsigned int seen = 0;

    for(const HandEntry& entry : layout.entries)
    {
        if(entry.choice == 0)
            continue;

        seen++;

        // Numbering runs 1..n in display order, with no gaps.
        REQUIRE(entry.choice == seen);
        REQUIRE(entry.choice <= layout.choices.size());

        const std::size_t position = layout.choices[entry.choice - 1];

        REQUIRE(position < hand.size());
        REQUIRE(hand[position] == entry.card);
    }

    REQUIRE(seen == layout.choices.size());
}
}

TEST_CASE("Cards are laid out in display order, not the order they were dealt", "[hand-layout]")
{
    const HandLayout layout = layOutHand(Hand, Hand, std::nullopt);

    const std::vector<Card> expected{ SpadesAce, SpadesQueen, HeartsKing, DiamondsSeven };

    REQUIRE(layout.entries.size() == expected.size());

    for(std::size_t i = 0 ; i < expected.size() ; i++)
        REQUIRE(layout.entries[i].card == expected[i]);

    // And the positions it hands back are into the hand, which is in a
    // different order than what was just drawn.
    REQUIRE(layout.choices == std::vector<std::size_t>{ 1, 3, 2, 0 });

    requireNumbersLeadBackToTheRightCards(layout, Hand);
}

TEST_CASE("Trump leads the display order", "[hand-layout]")
{
    // Diamonds are trump, so the seven that sorted last now sorts first, and
    // every position it hands back shifts with it.
    const Card trump(Rank::Two, Suit::Diamonds);

    const HandLayout layout = layOutHand(Hand, Hand, trump);

    REQUIRE(layout.entries.front().card == DiamondsSeven);
    REQUIRE(layout.choices == std::vector<std::size_t>{ 0, 1, 3, 2 });

    requireNumbersLeadBackToTheRightCards(layout, Hand);
}

TEST_CASE("Only legal cards are numbered, and the rest are still shown", "[hand-layout]")
{
    // Following spades: the two spades are legal, the other two are not.
    const std::vector<Card> legal{ SpadesAce, SpadesQueen };

    const HandLayout layout = layOutHand(Hand, legal, std::nullopt);

    // Every card appears, whether it can be played or not - an illegal card is
    // greyed, not hidden.
    REQUIRE(layout.entries.size() == Hand.size());

    REQUIRE(layout.entries[0].legal);
    REQUIRE(layout.entries[1].legal);
    REQUIRE_FALSE(layout.entries[2].legal);
    REQUIRE_FALSE(layout.entries[3].legal);

    REQUIRE(layout.entries[2].choice == 0);
    REQUIRE(layout.entries[3].choice == 0);

    // Two numbers offered, and they point at the two spades in the HAND, whose
    // positions are 1 and 3 - not 0 and 1 as the display would suggest.
    REQUIRE(layout.choices == std::vector<std::size_t>{ 1, 3 });

    requireNumbersLeadBackToTheRightCards(layout, Hand);
}

TEST_CASE("A single legal card is still numbered from one", "[hand-layout]")
{
    const std::vector<Card> legal{ HeartsKing };

    const HandLayout layout = layOutHand(Hand, legal, std::nullopt);

    REQUIRE(layout.choices == std::vector<std::size_t>{ 2 });
    REQUIRE(layout.entries[2].choice == 1);

    requireNumbersLeadBackToTheRightCards(layout, Hand);
}

TEST_CASE("An empty legal set means nothing is being chosen", "[hand-layout]")
{
    // The bidding screen: the hand is shown so the player can judge a bid, but
    // no card is being picked, so nothing is greyed and nothing is numbered.
    // This is also exactly what GameView asks for every frame.
    const HandLayout layout = layOutHand(Hand, {}, std::nullopt);

    REQUIRE(layout.entries.size() == Hand.size());
    REQUIRE(layout.choices.empty());

    for(const HandEntry& entry : layout.entries)
    {
        REQUIRE(entry.legal);
        REQUIRE(entry.choice == 0);
    }
}

TEST_CASE("An empty hand lays out to nothing at all", "[hand-layout]")
{
    const HandLayout layout = layOutHand({}, {}, std::nullopt);

    REQUIRE(layout.entries.empty());
    REQUIRE(layout.choices.empty());
}
