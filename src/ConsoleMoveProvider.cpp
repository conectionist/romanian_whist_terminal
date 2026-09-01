#include "ConsoleMoveProvider.h"

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
    // Positions rather than cards, sorted into display order. The extra
    // indirection is the price of the engine wanting a hand index back: sorting
    // the cards themselves would lose the only thing that can be translated
    // into one.
    std::vector<std::size_t> ordered(hand.size());
    for(std::size_t i = 0 ; i < hand.size() ; i++)
        ordered[i] = i;

    std::sort(ordered.begin(), ordered.end(),
        [&hand, &trump](std::size_t left, std::size_t right)
        { return CardFormat::inDisplayOrder(hand[left], hand[right], trump); });

    std::vector<std::size_t> choices;

    ui.view.hand.clear();
    ui.view.hand.reserve(ordered.size());

    // No candidates means nothing is being chosen - the bidding screen just
    // shows the hand - so no card is greyed out and none gets a number.
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
            choices.push_back(position);
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
