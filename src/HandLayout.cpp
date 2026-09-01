#include "HandLayout.h"

#include "CardFormat.h"

#include <algorithm>

using romanian_whist::Card;

HandLayout layOutHand(const std::vector<Card>& hand,
                      const std::vector<Card>& legalCards,
                      const std::optional<Card>& trump)
{
    // Positions rather than cards, sorted into display order. The extra
    // indirection is the whole point: sorting the cards themselves would lose
    // the only thing that can be turned back into an index for the engine.
    std::vector<std::size_t> ordered(hand.size());
    for(std::size_t i = 0 ; i < hand.size() ; i++)
        ordered[i] = i;

    std::sort(ordered.begin(), ordered.end(),
        [&hand, &trump](std::size_t left, std::size_t right)
        { return CardFormat::inDisplayOrder(hand[left], hand[right], trump); });

    HandLayout layout;
    layout.entries.reserve(hand.size());

    // No candidates means nothing is being chosen, so no card is greyed out and
    // none gets a number.
    const bool choosing = !legalCards.empty();

    for(const std::size_t position : ordered)
    {
        const Card& card = hand[position];

        // By value: a deck holds no duplicates, so equality is identity here.
        const bool legal = !choosing ||
                           std::find(legalCards.begin(), legalCards.end(), card) != legalCards.end();

        HandEntry entry;
        entry.card = card;
        entry.legal = legal;

        if(choosing && legal)
        {
            layout.choices.push_back(position);
            entry.choice = static_cast<unsigned int>(layout.choices.size());
        }

        layout.entries.push_back(entry);
    }

    return layout;
}
