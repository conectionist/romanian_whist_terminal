#include "TerminalRomanianWhist.h"

#include <stdexcept>


#include <romanian_whist/Trick.h>
#include <romanian_whist/Player.h>
#include "ConsoleMoveProvider.h"
#include "Input.h"
#include <romanian_whist/AiMoveProvider.h>
#include <romanian_whist/strategies/DuckingStrategy.h>
#include <romanian_whist/strategies/FirstCardStrategy.h>
#include <romanian_whist/strategies/LowRiskStrategy.h>
#include <romanian_whist/strategies/RandomCardStrategy.h>

using namespace romanian_whist;

namespace
{
std::unique_ptr<IStrategy> makeStrategy(AiLevel level)
{
    switch(level)
    {
        case AiLevel::FirstCard: return std::make_unique<FirstCardStrategy>();
        case AiLevel::LowRisk:   return std::make_unique<LowRiskStrategy>();
        case AiLevel::Ducking:   return std::make_unique<DuckingStrategy>();
        case AiLevel::Random:    break;
    }

    return std::make_unique<RandomCardStrategy>();
}

} // namespace

TerminalRomanianWhist::TerminalRomanianWhist(AppOptions& _options) : options(_options),
                                                                     terminal(options),
                                                                     cardFormat(terminal),
                                                                     renderer(terminal, cardFormat),
                                                                     pacer(options, terminal),
                                                                     ui{options, terminal, renderer, cardFormat, view}
{}

void TerminalRomanianWhist::applySetup(const GameConfig& config)
{
    if(!config.spectator)
    {
        // Seated first, so seat 0 is the human whenever there is one.
        game.addPlayer(config.humanName, std::make_unique<ConsoleMoveProvider>(ui));
        view.humanSeat = 0;
    }

    for(std::size_t i = 0 ; i < config.aiNames.size() ; i++)
    {
        game.addPlayer(config.aiNames[i],
                       std::make_unique<AiMoveProvider>(makeStrategy(config.aiLevels[i])));
    }

    game.initializeScoreboard(config.structure,
                              config.endWithForeheadAndHidden,
                              config.all1GamesAreForehead);

    game.initializeDeck(game.getPlayerCount());
}

void TerminalRomanianWhist::startGame()
{
    SetupWizard wizard(renderer, terminal);

    applySetup(options.demo ? SetupWizard::demoConfig() : wizard.run());

    game.addObserver(this);
    pacer.setHumanIsPlaying(view.humanSeat >= 0);

    // Still the client's job to start the game for one more phase; Phase 3
    // folds this into GameEngine::start(). It is also what fires
    // onGameStarted(), so the observer has to be registered before it.
    game.setStatus(GameStatus::InProgress);

    // Everything that used to be loop() and playCurrentRoundTricks(). The
    // callbacks below are called from inside here, on this thread.
    game.run();

    // Off the alternate screen first, so the result is still there after the
    // program exits.
    terminal.leaveAltScreen();
    renderer.drawGameOver(game.getPlayerScores());
}

bool TerminalRomanianWhist::isHumanSeat(Seat seat) const
{
    return view.humanSeat >= 0 && static_cast<unsigned int>(view.humanSeat) == seat.index;
}

void TerminalRomanianWhist::onRoundStarted(const GameEngine& engine)
{
    view.phase = Phase::Betting;
    view.message.clear();
    view.table.clear();
    view.activeSeat = -1;

    refreshFromEngine(view, engine);
}

void TerminalRomanianWhist::onBetRequested(const GameEngine& engine, Seat seat)
{
    view.activeSeat = static_cast<int>(seat.index);
    refreshFromEngine(view, engine);

    // The human's own frame is drawn by their move provider, complete with the
    // prompt; drawing one here first would only flicker.
    if(!isHumanSeat(seat))
        renderer.drawFrame(view);
}

void TerminalRomanianWhist::onBetPlaced(const GameEngine& engine, Seat seat, unsigned int)
{
    if(isHumanSeat(seat))
        return;

    // activeSeat stays on this seat: the frame showing the bid that was just
    // made should still highlight who made it.
    refreshFromEngine(view, engine);
    renderer.drawFrame(view);
    pacer.beat();
}

void TerminalRomanianWhist::onBettingComplete(const GameEngine& engine)
{
    // Hold on the completed bid table before the first card goes down. Without
    // this the last bid is only ever seen when an AI made it, and the human
    // never gets to read what the table has committed to.
    view.activeSeat = -1;
    refreshFromEngine(view, engine);
    renderer.drawFrame(view);
    pacer.beat();

    // Deliberately here and not in onRoundStarted: a skip runs to the end of a
    // round and is only cleared once the next round's cards start going down,
    // so it deliberately carries through the bidding beats above.
    pacer.resetSkip();
}

void TerminalRomanianWhist::onTrickStarted(const GameEngine& engine, unsigned int trickNumber, Seat)
{
    view.phase = Phase::Playing;
    view.trickNumber = trickNumber;
    view.message.clear();
    view.activeSeat = -1;

    refreshFromEngine(view, engine);
}

void TerminalRomanianWhist::onCardRequested(const GameEngine& engine, Seat seat)
{
    view.activeSeat = static_cast<int>(seat.index);

    refreshFromEngine(view, engine);

    if(isHumanSeat(seat))
        return;

    view.hint = engine.getPlayers().at(seat.index).getName() + " is thinking...";
    renderer.drawFrame(view);
    pacer.beat();
}

void TerminalRomanianWhist::onCardPlayed(const GameEngine& engine, Seat seat, const Card&)
{
    view.hint.clear();
    refreshFromEngine(view, engine);
    renderer.drawFrame(view);

    // The human's own card needs no pause: the time they spent choosing it
    // already served as one, and the next bot's beat gives the card a moment on
    // screen before another lands on top of it.
    if(!isHumanSeat(seat))
        pacer.beat();
}

void TerminalRomanianWhist::onTrickWon(const GameEngine& engine, Seat winner, unsigned int trickNumber)
{
    view.phase = Phase::TrickEnd;
    view.activeSeat = -1;
    view.message = engine.getPlayers().at(winner.index).getName() + " wins the trick.";

    // The finished trick is still the engine's current one throughout this
    // callback, so refreshFromEngine() rebuilds the completed table under the
    // message rather than a blank one.
    refreshFromEngine(view, engine);

    const bool moreTricks = trickNumber < engine.getCurrentRoundTrickCount();
    renderer.drawFrame(view, moreTricks ? "Press Enter for the next trick"
                                        : "Press Enter to see the round result");
    pacer.trickPause();
}

void TerminalRomanianWhist::onRoundScored(const GameEngine& engine)
{
    // This callback and not onRoundComplete: it is the last one at which the
    // engine still names this round, so the header and the scores on screen
    // belong to the same round. The scores are calculated but not yet
    // committed, which is what lets the board show what the round was worth
    // alongside the total it folds into.
    view.phase = Phase::RoundEnd;
    view.activeSeat = -1;
    view.message.clear();

    refreshFromEngine(view, engine);
    renderer.drawFrame(view, "Press Enter to deal the next round");
    pacer.roundPause();
}
