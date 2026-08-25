#include "ConsoleMoveProvider.h"

#include <romanian_whist/CardValidator.h>

#include <iostream>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

using romanian_whist::Card;
using romanian_whist::CardValidator;
using romanian_whist::Suit;

namespace
{

// Reads an integer in [0, maxValid] from stdin, re-prompting until one arrives.
//
// `value` must be initialised: at end-of-file operator>> fails while constructing
// its sentry, which returns without ever writing to the target. Reading an
// uninitialised int there would be undefined behaviour.
//
// Malformed input is recoverable (clear the failure, discard the offending line).
// EOF is not: no further input can ever arrive, so re-prompting would spin
// forever. It propagates as an exception for main() to report.
int readChoice(int maxValid, const std::string& prompt)
{
    int value = 0;

    while(true)
    {
        if(std::cin >> value)
        {
            if(value >= 0 && value <= maxValid)
                return value;
        }
        else
        {
            if(std::cin.eof())
                throw std::runtime_error("Input closed while awaiting a choice.");

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        std::cout << prompt;
    }
}

} // namespace

unsigned int ConsoleMoveProvider::makeBet(const std::vector<Card*>& hand, Card* trump, bool isFirstPlayer)
{
    const int maxBet = static_cast<int>(hand.size());

    std::cout << "Enter bet (0-" << maxBet << "): ";

    return static_cast<unsigned int>(
        readChoice(maxBet, "Invalid bet. Enter a number between 0 and " +
                           std::to_string(maxBet) + ": "));
}

Card* ConsoleMoveProvider::playCard(std::vector<Card*>& hand, Card* trump, const Suit* leadSuit)
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

    std::cout << "Choose a card (0-" << maxIndex << "): ";

    const int choice = readChoice(maxIndex,
                                  "Invalid choice. Enter a number between 0 and " +
                                  std::to_string(maxIndex) + ": ");

    Card* card = legalCards[static_cast<std::size_t>(choice)];
    hand.erase(std::find(hand.begin(), hand.end(), card));
    return card;
}
