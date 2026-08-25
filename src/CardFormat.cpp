#include "CardFormat.h"

#include <algorithm>

using romanian_whist::Card;
using romanian_whist::Rank;
using romanian_whist::Suit;

namespace
{

// Display order when there is no trump. Alternating red and black keeps
// neighbouring groups from blurring together.
constexpr Suit SuitOrder[] = { Suit::Spades, Suit::Hearts, Suit::Clubs, Suit::Diamonds };

unsigned int suitPosition(Suit suit, const Card* trump)
{
    if(trump != nullptr && suit == trump->suit)
        return 0;

    for(unsigned int i = 0 ; i < 4 ; i++)
    {
        if(SuitOrder[i] == suit)
            return i + 1;
    }

    return 5;
}

} // namespace

CardFormat::CardFormat(const Terminal& _terminal) : terminal(_terminal)
{}

std::string CardFormat::rankLabel(Rank rank) const
{
    switch(rank)
    {
        case Rank::Two:   return "2";
        case Rank::Three: return "3";
        case Rank::Four:  return "4";
        case Rank::Five:  return "5";
        case Rank::Six:   return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine:  return "9";
        case Rank::Ten:   return "10";
        case Rank::Jack:  return "J";
        case Rank::Queen: return "Q";
        case Rank::King:  return "K";
        case Rank::Ace:   return "A";
    }

    return "?";
}

std::string CardFormat::suitGlyph(Suit suit) const
{
    if(!terminal.unicodeEnabled())
    {
        switch(suit)
        {
            case Suit::Hearts:   return "H";
            case Suit::Diamonds: return "D";
            case Suit::Spades:   return "S";
            case Suit::Clubs:    return "C";
        }

        return "?";
    }

    // No variation selector: U+FE0F would make many terminals render these
    // double-width, and every column in the layout is computed assuming one.
    switch(suit)
    {
        case Suit::Hearts:   return "♥";
        case Suit::Diamonds: return "♦";
        case Suit::Spades:   return "♠";
        case Suit::Clubs:    return "♣";
    }

    return "?";
}

std::string CardFormat::suitName(Suit suit) const
{
    switch(suit)
    {
        case Suit::Hearts:   return "Hearts";
        case Suit::Diamonds: return "Diamonds";
        case Suit::Spades:   return "Spades";
        case Suit::Clubs:    return "Clubs";
    }

    return "Unknown";
}

bool CardFormat::isRed(Suit suit)
{
    return suit == Suit::Hearts || suit == Suit::Diamonds;
}

std::string CardFormat::card(const Card& card) const
{
    const std::string text = rankLabel(card.rank) + suitGlyph(card.suit);

    return terminal.paint(text,
                          isRed(card.suit) ? Terminal::Color::Red : Terminal::Color::Default,
                          true);
}

std::string CardFormat::dimCard(const Card& card) const
{
    return terminal.dim(rankLabel(card.rank) + suitGlyph(card.suit));
}

unsigned int CardFormat::cardWidth(const Card& card)
{
    return card.rank == Rank::Ten ? 3 : 2;
}

void CardFormat::sortForDisplay(std::vector<const Card*>& cards, const Card* trump)
{
    std::sort(cards.begin(), cards.end(),
        [trump](const Card* left, const Card* right)
        {
            const unsigned int leftSuit = suitPosition(left->suit, trump);
            const unsigned int rightSuit = suitPosition(right->suit, trump);

            if(leftSuit != rightSuit)
                return leftSuit < rightSuit;

            return static_cast<int>(left->rank) > static_cast<int>(right->rank);
        });
}
