#ifndef TERMINAL_ROMANIAN_WHIST_H
#define TERMINAL_ROMANIAN_WHIST_H

#include <romanian_whist/GameEngine.h>

#include "AppOptions.h"
#include "CardFormat.h"
#include "GameView.h"
#include "Pacer.h"
#include "Renderer.h"
#include "SetupWizard.h"
#include "Terminal.h"
#include "UiContext.h"

#include <romanian_whist/IGameObserver.h>
#include <romanian_whist/Trick.h>

#include <chrono>

// The engine owns the loop now, so this is no longer a driver - it is what the
// engine reports to. Every method below the constructor is a callback, and the
// order they fire in is IGameObserver's contract, not this class's.
class TerminalRomanianWhist : public romanian_whist::IGameObserver
{
private:
    AppOptions& options;
    Terminal terminal;
    CardFormat cardFormat;
    Renderer renderer;
    Pacer pacer;
    GameView view;
    UiContext ui;

    // Set once, when startGame() calls game.start(). Wall-clock rather than the
    // engine's own notion of time, because the engine has none - a bidding
    // pause is real time the player experiences either way.
    std::chrono::steady_clock::time_point startTime;

    // Declared last on purpose. The engine owns the players, a player owns the
    // human's ConsoleMoveProvider, and that provider holds a reference to `ui`
    // above. Members are destroyed in reverse declaration order, so this keeps
    // the provider dying before the context it points at. Do not move it.
    romanian_whist::GameEngine game;

public:
    explicit TerminalRomanianWhist(AppOptions& _options);

    void startGame();

    // ---- IGameObserver ----
    // Drawing happens here, on the game thread, and blocking in one of these is
    // how this client paces itself and waits for Enter. See IGameObserver.h for
    // what the engine guarantees is readable inside each.
    void onRoundStarted(const romanian_whist::GameEngine& engine) override;
    void onBetRequested(const romanian_whist::GameEngine& engine, romanian_whist::Seat seat) override;
    void onBetPlaced(const romanian_whist::GameEngine& engine,
                     romanian_whist::Seat seat,
                     unsigned int bet) override;
    void onBettingComplete(const romanian_whist::GameEngine& engine) override;
    void onTrickStarted(const romanian_whist::GameEngine& engine,
                        unsigned int trickNumber,
                        romanian_whist::Seat leader) override;
    void onCardRequested(const romanian_whist::GameEngine& engine, romanian_whist::Seat seat) override;
    void onCardPlayed(const romanian_whist::GameEngine& engine,
                      romanian_whist::Seat seat,
                      const romanian_whist::Card& card) override;
    void onTrickWon(const romanian_whist::GameEngine& engine,
                    romanian_whist::Seat winner,
                    unsigned int trickNumber) override;
    void onRoundScored(const romanian_whist::GameEngine& engine) override;

private:
    // Builds the engine's setup from the wizard's answers. It touches the
    // engine only through start(), so the seats do not exist until then - but
    // it is still what decides view.humanSeat, which nothing else sets.
    romanian_whist::GameSetup buildSetup(const GameConfig& config);

    bool isHumanSeat(romanian_whist::Seat seat) const;

    // refreshFromEngine() plus the one thing about the view it doesn't set:
    // elapsed minutes since startTime. Every callback below calls this instead
    // of refreshFromEngine() directly, so the clock can never fall behind on a
    // frame that forgot to update it.
    void refresh(const romanian_whist::GameEngine& engine);
};

#endif
