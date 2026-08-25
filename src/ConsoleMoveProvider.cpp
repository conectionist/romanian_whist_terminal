#include "ConsoleMoveProvider.h"

#include "Input.h"

#include <romanian_whist/CardValidator.h>

#include <iostream>
#include <string>

using romanian_whist::Card;
using romanian_whist::CardValidator;
using romanian_whist::Suit;

namespace
{

// Until the provider draws whole frames, a re-prompt just prints the reason
// followed by the prompt again.
Input::Redraw reprompt(const std::string& prompt)
{
    return [prompt](const std::string& error)
    {
        std::cout << error << " " << prompt;
    };
}

} // namespace

unsigned int ConsoleMoveProvider::makeBet(const std::vector<Card*>& hand, Card* trump, bool isFirstPlayer)
{
    (void)trump;
    (void)isFirstPlayer;

    const int maxBet = static_cast<int>(hand.size());
    const std::string prompt = "Enter bet (0-" + std::to_string(maxBet) + "): ";

    std::cout << prompt;

    return static_cast<unsigned int>(Input::readChoice(0, maxBet, reprompt(prompt)));
}

Card* ConsoleMoveProvider::playCard(const std::vector<Card*>& hand, Card* trump, const Suit* leadSuit)
{
    CardValidator cardValidator;
    std::vector<Card*> legalCards = cardValidator.getLegalCards(hand, trump, leadSuit);

    // No legal play. Returning nullptr matches what the engine's own strategies
    // do; prompting would be impossible since no index could ever be valid.
    if(legalCards.empty())
        return nullptr;

    std::cout << "Your hand:" << std::endl;
    for(Card* card : hand)
        std::cout << "  " << card->toString() << std::endl;

    std::cout << "Legal plays:" << std::endl;
    for(std::size_t i = 0 ; i < legalCards.size() ; i++)
        std::cout << i << ") " << legalCards[i]->toString() << std::endl;

    const int maxIndex = static_cast<int>(legalCards.size()) - 1;
    const std::string prompt = "Choose a card (0-" + std::to_string(maxIndex) + "): ";

    std::cout << prompt;

    const int choice = Input::readChoice(0, maxIndex, reprompt(prompt));

    return legalCards[static_cast<std::size_t>(choice)];
}
