#ifndef CARD_FORMAT_H
#define CARD_FORMAT_H

#include "Terminal.h"

#include <romanian_whist/Card.h>

#include <string>
#include <vector>

// The only place that knows cards are drawn with suit glyphs, that hearts and
// diamonds are red, and what order a hand reads best in. The engine deliberately
// has no opinion on any of that.
class CardFormat
{
private:
    const Terminal& terminal;

public:
    explicit CardFormat(const Terminal& _terminal);

    std::string rankLabel(romanian_whist::Rank rank) const;
    std::string suitGlyph(romanian_whist::Suit suit) const;
    std::string suitName(romanian_whist::Suit suit) const;
    static bool isRed(romanian_whist::Suit suit);

    // Rank followed by suit, coloured. Widths vary - a ten is three columns
    // wide, everything else two - so pad with cardWidth rather than assuming.
    std::string card(const romanian_whist::Card& card) const;
    std::string dimCard(const romanian_whist::Card& card) const;
    static unsigned int cardWidth(const romanian_whist::Card& card);

    // Groups by suit and orders each group high to low, so a hand can be read
    // at a glance instead of in the order it happened to be dealt. Trump leads,
    // then the suits alternate colour to keep adjacent groups distinguishable.
    static void sortForDisplay(std::vector<const romanian_whist::Card*>& cards,
                               const romanian_whist::Card* trump);
};

#endif
