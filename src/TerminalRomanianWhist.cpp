#include "TerminalRomanianWhist.h"

#include <cstdint>
#include <optional>
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
// `seed` is unset unless --seed was given. Only RandomCardStrategy reads it -
// the other three are deterministic already - but it is offset per seat so that
// two random bots in the same game do not draw the identical stream.
std::unique_ptr<IStrategy> makeStrategy(AiLevel level,
                                        const std::optional<std::uint32_t>& seed,
                                        std::size_t seatIndex)
{
    switch(level)
    {
        case AiLevel::FirstCard: return std::make_unique<FirstCardStrategy>();
        case AiLevel::LowRisk:   return std::make_unique<LowRiskStrategy>();
        case AiLevel::Ducking:   return std::make_unique<DuckingStrategy>();
        case AiLevel::Random:    break;
    }

    if(seed)
        return std::make_unique<RandomCardStrategy>(*seed + static_cast<std::uint32_t>(seatIndex));

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

GameSetup TerminalRomanianWhist::buildSetup(const GameConfig& config)
{
    GameSetup setup;

    if(!config.spectator)
    {
        // Seated first, so seat 0 is the human whenever there is one.
        setup.seats.push_back(SeatSetup{ config.humanName,
                                         std::make_unique<ConsoleMoveProvider>(ui) });
        view.humanSeat = 0;
    }

    for(std::size_t i = 0 ; i < config.aiNames.size() ; i++)
    {
        setup.seats.push_back(
            SeatSetup{ config.aiNames[i],
                       std::make_unique<AiMoveProvider>(
                           makeStrategy(config.aiLevels[i], options.seed, i)) });
    }

    setup.structure = config.structure;
    setup.endWithForeheadAndHidden = config.endWithForeheadAndHidden;
    setup.all1GamesAreForehead = config.all1GamesAreForehead;
    setup.shuffleSeed = options.seed;

    return setup;
}

void TerminalRomanianWhist::startGame()
{
    SetupWizard wizard(renderer, terminal);

    GameSetup setup = buildSetup(options.demo ? SetupWizard::demoConfig() : wizard.run());

    game.addObserver(this);
    pacer.setHumanIsPlaying(view.humanSeat >= 0);
    startTime = std::chrono::steady_clock::now();

    // Seats the table, lays out the schedule, builds the deck and starts the
    // game - and fires onGameStarted() on the way out, which is why the observer
    // is registered above it.
    game.start(std::move(setup));

    // Everything that used to be loop() and playCurrentRoundTricks(). The
    // callbacks below are called from inside here, on this thread.
    game.run();

    // Off the alternate screen first, so the result is still there after the
    // program exits.
    terminal.leaveAltScreen();
    renderer.drawGameOver(game.getStandings(), game.getWinners());
}

bool TerminalRomanianWhist::isHumanSeat(Seat seat) const
{
    return view.humanSeat >= 0 && static_cast<unsigned int>(view.humanSeat) == seat.index;
}

void TerminalRomanianWhist::refresh(const GameEngine& engine)
{
    refreshFromEngine(view, engine);

    const auto elapsed = std::chrono::steady_clock::now() - startTime;
    view.elapsedMinutes = static_cast<unsigned int>(
        std::chrono::duration_cast<std::chrono::minutes>(elapsed).count());
}

void TerminalRomanianWhist::onRoundStarted(const GameEngine& engine)
{
    view.phase = Phase::Betting;
    view.message.clear();
    view.table.clear();
    view.activeSeat = -1;

    refresh(engine);
}

void TerminalRomanianWhist::onBetRequested(const GameEngine& engine, Seat seat)
{
    view.activeSeat = static_cast<int>(seat.index);
    refresh(engine);

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
    refresh(engine);
    renderer.drawFrame(view);
    pacer.beat();
}

void TerminalRomanianWhist::onBettingComplete(const GameEngine& engine)
{
    // Hold on the completed bid table before the first card goes down. Without
    // this the last bid is only ever seen when an AI made it, and the human
    // never gets to read what the table has committed to.
    view.activeSeat = -1;
    refresh(engine);
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

    refresh(engine);
}

void TerminalRomanianWhist::onCardRequested(const GameEngine& engine, Seat seat)
{
    view.activeSeat = static_cast<int>(seat.index);

    refresh(engine);

    if(isHumanSeat(seat))
        return;

    view.hint = engine.getPlayers().at(seat.index).getName() + " is thinking...";
    renderer.drawFrame(view);
    pacer.beat();
}

void TerminalRomanianWhist::onCardPlayed(const GameEngine& engine, Seat seat, const Card&)
{
    view.hint.clear();
    refresh(engine);
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
    refresh(engine);

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

    refresh(engine);
    renderer.drawFrame(view, "Press Enter to deal the next round");
    pacer.roundPause();
}
