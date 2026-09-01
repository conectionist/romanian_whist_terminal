#ifndef HAND_LAYOUT_H
#define HAND_LAYOUT_H

#include "GameView.h"

#include <romanian_whist/Card.h>

#include <cstddef>
#include <optional>
#include <vector>

// How a hand is presented, and the way back from what the player typed to what
// the engine holds.
//
// Three orderings meet here and no two of them coincide:
//
//   1. the hand as the engine holds it - deal order, and what an index into it
//      must refer to;
//   2. display order - trump suit first, then a fixed alternating-colour order,
//      high rank first within a suit;
//   3. the numbered legal subset, which is what the player actually chooses a
//      number from.
//
// Carrying positions rather than cards is what keeps the third translatable
// back to the first. Getting it wrong means the player types 2 and plays a
// different card, which no rule in the engine would object to.
struct HandLayout
{
    // Every card in the hand, in display order, whether it can be played or
    // not: an illegal card is still shown, just greyed and unnumbered.
    std::vector<HandEntry> entries;

    // Hand positions of the numbered cards, in the order they were numbered -
    // so choices[n - 1] is the index the engine wants when the player types n.
    // Empty when nothing is being chosen.
    std::vector<std::size_t> choices;
};

// `legalCards` empty means nothing is being chosen - the bidding screen just
// shows the hand - so every card reads as legal and none is numbered. That is
// the case GameView uses; ConsoleMoveProvider passes a real legal set when it
// is asking for a card.
//
// `trump` is empty in 8-card rounds, which have no trump.
HandLayout layOutHand(const std::vector<romanian_whist::Card>& hand,
                      const std::vector<romanian_whist::Card>& legalCards,
                      const std::optional<romanian_whist::Card>& trump);

#endif
