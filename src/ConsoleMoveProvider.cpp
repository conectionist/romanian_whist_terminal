#include "ConsoleMoveProvider.h"

#include "HandLayout.h"
#include "Input.h"

#include <romanian_whist/CardValidator.h>

#include <algorithm>
#include <optional>
#include <cstddef>
#include <string>

using romanian_whist::BetContext;
using romanian_whist::Card;
using romanian_whist::CardValidator;
using romanian_whist::PlayContext;
using romanian_whist::Suit;

ConsoleMoveProvider::ConsoleMoveProvider(UiContext& _ui) : ui(_ui)
{}

void ConsoleMoveProvider::redraw(const std::string& prompt, const std::string& error) const
{
    ui.view.error = error;
    ui.renderer.drawFrame(ui.view, prompt);
}

std::vector<std::size_t> ConsoleMoveProvider::layOutHand(const std::vector<Card>& hand,
                                                        const std::vector<Card>& legalCards,
                                                        const std::optional<Card>& trump) const
{
    // The layout itself is a pure function of the hand - see HandLayout.h for
    // the three orderings it reconciles. All this adds is putting the result on
    // screen, which keeps every write to the view in one place.
    HandLayout layout = ::layOutHand(hand, legalCards, trump);

    ui.view.hand = std::move(layout.entries);
    ui.view.legalCount = static_cast<unsigned int>(layout.choices.size());

    return std::move(layout.choices);
}

unsigned int ConsoleMoveProvider::makeBet(const BetContext& context)
{
    // Every card is shown, none numbered: this is the bidding screen, and the
    // player is choosing a number of tricks rather than a card. Seeing the hand
    // here at all is the point - the bid used to be asked for blind.
    layOutHand(context.hand, {}, context.trump);

    const int maxBet = static_cast<int>(context.hand.size());
    const std::string prompt = "How many tricks will you win? (0-" + std::to_string(maxBet) + "): ";

    // Say which number is barred before it is typed, not only after. Bidding
    // last is the only time this happens, and the player has no other way to
    // know the rule is about to bite.
    if(context.forbiddenBet)
        ui.view.hint = "you bid last - " + std::to_string(*context.forbiddenBet) + " is not allowed";
    else
        ui.view.hint.clear();

    redraw(prompt, "");

    const auto redrawPrompt = [this, &prompt](const std::string& error) { redraw(prompt, error); };

    // readChoice only understands a contiguous range, so the one barred value
    // is caught out here and the question asked again.
    while(true)
    {
        const int bet = Input::readChoice(0, maxBet, redrawPrompt);

        if(!context.forbiddenBet || static_cast<unsigned int>(bet) != *context.forbiddenBet)
        {
            ui.view.error.clear();
            ui.view.hint.clear();

            return static_cast<unsigned int>(bet);
        }

        redrawPrompt("Bidding " + std::to_string(bet) + " would make the bids add up to " +
                     std::to_string(maxBet) + ". Pick another number.");
    }
}

std::optional<std::size_t> ConsoleMoveProvider::playCard(const PlayContext& context)
{
    const std::vector<Card>& hand = context.hand;
    const std::optional<Card>& trump = context.trump;
    const std::optional<Suit>& leadSuit = context.leadSuit;

    CardValidator cardValidator;
    const std::vector<Card> legalCards = cardValidator.getLegalCards(hand, trump, leadSuit);

    // No legal play. Returning empty matches what the engine's own strategies
    // do; prompting would be impossible since no number could ever be valid.
    if(legalCards.empty())
        return std::nullopt;

    const std::vector<std::size_t> choices = layOutHand(hand, legalCards, trump);

    if(leadSuit && choices.size() < hand.size())
        ui.view.hint = "you must follow " + ui.cardFormat.suitGlyph(*leadSuit);
    else if(!leadSuit)
        ui.view.hint = "you lead";
    else
        ui.view.hint.clear();

    const int highest = static_cast<int>(choices.size());
    const std::string prompt = highest == 1
        ? std::string("Only one card can be played. Press 1: ")
        : "Which card? (1-" + std::to_string(highest) + "): ";

    redraw(prompt, "");

    const int choice = Input::readChoice(1, highest,
        [this, &prompt](const std::string& error) { redraw(prompt, error); });

    ui.view.error.clear();
    ui.view.hint.clear();

    // The number was an index into the numbered, legal, display-ordered list;
    // what comes back out of it is the position in the hand the engine holds.
    return choices[static_cast<std::size_t>(choice) - 1];
}
