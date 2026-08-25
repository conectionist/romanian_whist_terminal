#ifndef CONSOLE_MOVE_PROVIDER_H
#define CONSOLE_MOVE_PROVIDER_H

#include <romanian_whist/IMoveProvider.h>

#include <vector>

class ConsoleMoveProvider : public romanian_whist::IMoveProvider
{
public:
    ConsoleMoveProvider() = default;
    virtual ~ConsoleMoveProvider() = default;

    virtual unsigned int makeBet(const std::vector<romanian_whist::Card*>& hand,
                                 romanian_whist::Card* trump,
                                 bool isFirstPlayer) override;

    virtual romanian_whist::Card* playCard(const std::vector<romanian_whist::Card*>& hand,
                                           romanian_whist::Card* trump,
                                           const romanian_whist::Suit* leadSuit) override;
};

#endif
