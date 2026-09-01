#ifndef CONSOLE_MOVE_PROVIDER_H
#define CONSOLE_MOVE_PROVIDER_H

#include "UiContext.h"

#include <romanian_whist/IMoveProvider.h>

#include <cstddef>
#include <optional>
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
    // Fills view.hand in display order and returns the HAND POSITIONS of the
    // cards the player may actually choose, in the order they were numbered on
    // screen - so the number typed at the prompt indexes the returned vector,
    // and the value found there is what the engine wants back.
    //
    // Three orderings meet here and none of them coincide: the hand as the
    // engine holds it, the display order, and the legal subset the numbering
    // runs over. Carrying positions rather than cards is what keeps the last of
    // those translatable back to the first.
    std::vector<std::size_t> layOutHand(const std::vector<romanian_whist::Card>& hand,
                                        const std::vector<romanian_whist::Card>& legalCards,
                                        const std::optional<romanian_whist::Card>& trump) const;

    void redraw(const std::string& prompt, const std::string& error) const;

public:
    explicit ConsoleMoveProvider(UiContext& _ui);
    virtual ~ConsoleMoveProvider() = default;

    virtual unsigned int makeBet(const romanian_whist::BetContext& context) override;

    virtual std::optional<std::size_t> playCard(const romanian_whist::PlayContext& context) override;
};

#endif
