#ifndef CONSOLE_MOVE_PROVIDER_H
#define CONSOLE_MOVE_PROVIDER_H

#include "UiContext.h"

#include <romanian_whist/IMoveProvider.h>

#include <string>
#include <vector>

// The human's moves. Draws the screen it asks its questions on, since the
// engine calls into here at exactly the moments the player needs to see
// something before deciding.
class ConsoleMoveProvider : public romanian_whist::IMoveProvider
{
private:
    UiContext& ui;

    // Lays the hand out sorted and grouped, numbering only the cards that can
    // legally be played, and returns what each number maps to. The pointers are
    // the hand's own, which matters: the engine removes the played card by
    // identity.
    std::vector<romanian_whist::Card*> layOutHand(const std::vector<romanian_whist::Card*>& hand,
                                                  const std::vector<romanian_whist::Card*>& legalCards,
                                                  romanian_whist::Card* trump) const;

    void redraw(const std::string& prompt, const std::string& error) const;

public:
    explicit ConsoleMoveProvider(UiContext& _ui);
    virtual ~ConsoleMoveProvider() = default;

    virtual unsigned int makeBet(const std::vector<romanian_whist::Card*>& hand,
                                 romanian_whist::Card* trump,
                                 bool isFirstPlayer) override;

    virtual romanian_whist::Card* playCard(const std::vector<romanian_whist::Card*>& hand,
                                           romanian_whist::Card* trump,
                                           const romanian_whist::Suit* leadSuit) override;
};

#endif
