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
    // legally be played, and returns what each number maps to.
    //
    // Writes the layout to view.hand and view.legalCount as well as returning
    // its choices, so bidding calls it just to put the hand on screen.
    //
    // Three orderings meet here and none of them coincide: the hand as the
    // engine holds it, the display order, and the legal subset the numbering
    // runs over. Carrying positions rather than cards is what keeps the last of
    // those translatable back to the first.
    std::vector<std::size_t> showHand(const std::vector<romanian_whist::Card>& hand,
                                      const std::vector<romanian_whist::Card>& legalCards,
                                      const std::optional<romanian_whist::Card>& trump) const;

    void redraw(const std::string& prompt, const std::string& error) const;

public:
    explicit ConsoleMoveProvider(UiContext& _ui);
    ~ConsoleMoveProvider() override = default;

    unsigned int makeBet(const romanian_whist::BetContext& context) override;

    std::optional<std::size_t> playCard(const romanian_whist::PlayContext& context) override;

    // The line shown under the hand while a card is chosen. Static so a test can
    // check it: the frame it appears in goes straight to stdout.
    static std::string playHint(const std::vector<romanian_whist::Card>& hand,
                                std::size_t legalCount,
                                const std::optional<romanian_whist::Card>& trump,
                                const std::optional<romanian_whist::Suit>& leadSuit,
                                const CardFormat& cardFormat);
};

#endif
