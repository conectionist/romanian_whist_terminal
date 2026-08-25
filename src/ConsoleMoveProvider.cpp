#include "ConsoleMoveProvider.h"

#include "Input.h"

#include <romanian_whist/CardValidator.h>

#include <algorithm>
#include <string>

using romanian_whist::BetContext;
using romanian_whist::Card;
using romanian_whist::CardValidator;
using romanian_whist::Suit;

ConsoleMoveProvider::ConsoleMoveProvider(UiContext& _ui) : ui(_ui)
{}

void ConsoleMoveProvider::redraw(const std::string& prompt, const std::string& error) const
{
    ui.view.error = error;
    ui.renderer.drawFrame(ui.view, prompt);
}

std::vector<Card*> ConsoleMoveProvider::layOutHand(const std::vector<Card*>& hand,
                                                   const std::vector<Card*>& legalCards,
                                                   Card* trump) const
{
    std::vector<const Card*> ordered(hand.begin(), hand.end());
    CardFormat::sortForDisplay(ordered, trump);

    std::vector<Card*> choices;

    ui.view.hand.clear();
    ui.view.hand.reserve(ordered.size());

    // No candidates means nothing is being chosen - the bidding screen just
    // shows the hand - so no card is greyed out and none gets a number.
    const bool choosing = !legalCards.empty();

    for(const Card* card : ordered)
    {
        // Compare by pointer: identity is what the engine keys on when it
        // removes the played card from the hand.
        const bool legal = !choosing ||
                           std::find(legalCards.begin(), legalCards.end(), card) != legalCards.end();

        HandEntry entry;
        entry.card = *card;
        entry.legal = legal;

        if(choosing && legal)
        {
            choices.push_back(const_cast<Card*>(card));
            entry.choice = static_cast<unsigned int>(choices.size());
        }

        ui.view.hand.push_back(entry);
    }

    ui.view.legalCount = static_cast<unsigned int>(choices.size());

    return choices;
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

Card* ConsoleMoveProvider::playCard(const std::vector<Card*>& hand, Card* trump, const Suit* leadSuit)
{
    CardValidator cardValidator;
    const std::vector<Card*> legalCards = cardValidator.getLegalCards(hand, trump, leadSuit);

    // No legal play. Returning nullptr matches what the engine's own strategies
    // do; prompting would be impossible since no number could ever be valid.
    if(legalCards.empty())
        return nullptr;

    const std::vector<Card*> choices = layOutHand(hand, legalCards, trump);

    if(leadSuit != nullptr && choices.size() < hand.size())
        ui.view.hint = "you must follow " + ui.cardFormat.suitGlyph(*leadSuit);
    else if(leadSuit == nullptr)
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

    return choices[static_cast<std::size_t>(choice) - 1];
}
