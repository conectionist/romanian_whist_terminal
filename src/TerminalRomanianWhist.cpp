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

void TerminalRomanianWhist::startGame()
{
    SetupWizard wizard(renderer, terminal);

    applySetup(options.demo ? SetupWizard::demoConfig() : wizard.run());

    loop();
}

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

void TerminalRomanianWhist::loop()
{
    game.setStatus(GameStatus::InProgress);

    pacer.setHumanIsPlaying(view.humanSeat >= 0);

    while(game.isInProgress())
    {
        game.shuffleDeck();
        game.dealCards();

        openingSeat = game.getCurrentRound().getOpenerSeat().index;

        view.phase = Phase::Betting;
        view.message.clear();
        view.table.clear();

        Seat currentSeat = game.getRoundLeaderSeat();

        for(unsigned int i = 0 ; i < game.getPlayerCount() ; i++)
        {
            view.activeSeat = static_cast<int>(currentSeat.index);
            refreshFromEngine(view, game, openingSeat);

            const bool isHuman = view.activeSeat == view.humanSeat;
            if(!isHuman)
                renderer.drawFrame(view);

            // Asked fresh each time round: it only names a bid once everyone
            // but the last player has bid, and only while a bid could still
            // make the round add up exactly.
            const unsigned int bet = game.getPlayer(currentSeat).getBet(game.getCurrentTrumpCard(),
                                                                        i == 0,
                                                                        game.getForbiddenBet());
            game.placeBet(currentSeat, bet);

            if(!isHuman)
            {
                refreshFromEngine(view, game, openingSeat);
                renderer.drawFrame(view);
                pacer.beat();
            }

            currentSeat = game.getNextSeat(currentSeat);
        }

        // Hold on the completed bid table before the first card goes down.
        // Without this the last bid is only ever seen when an AI made it, and
        // the human never gets to read what the table has committed to.
        view.activeSeat = -1;
        refreshFromEngine(view, game, openingSeat);
        renderer.drawFrame(view);
        pacer.beat();

        playCurrentRoundTricks();

        // Score the round but hold off committing, so the scoreboard can show
        // what the round was worth alongside the total it folds into.
        game.calculateScores();

        view.phase = Phase::RoundEnd;
        view.activeSeat = -1;
        view.message.clear();
        refreshFromEngine(view, game, openingSeat);
        renderer.drawFrame(view, "Press Enter to deal the next round");
        pacer.roundPause();

        game.commitRoundScores();
        game.completeCurrentRound();
    }

    // Off the alternate screen first, so the result is still there after the
    // program exits.
    terminal.leaveAltScreen();
    renderer.drawGameOver(game.getPlayerScores());
}

void TerminalRomanianWhist::playCurrentRoundTricks()
{
    const unsigned int trickCount = game.getCurrentRoundTrickCount();
    const unsigned int playerCount = game.getPlayerCount();

    pacer.resetSkip();

    for(unsigned int trickIndex = 0 ; trickIndex < trickCount ; trickIndex++)
    {
        Trick trick;

        // Whoever leads this trick: the round's opener for the first, then each
        // trick's winner.
        const Seat leader = game.getRoundLeaderSeat();
        Seat currentSeat = leader;

        view.phase = Phase::Playing;
        view.trickNumber = trickIndex + 1;
        view.leadSeat = leader.index;
        view.table.clear();
        view.hasLeadSuit = false;
        view.message.clear();

        for(unsigned int i = 0 ; i < playerCount ; i++)
        {
            view.activeSeat = static_cast<int>(currentSeat.index);

            const bool isHuman = view.activeSeat == view.humanSeat;

            refreshFromEngine(view, game, openingSeat);

            // The human's own frame is drawn by their move provider, complete
            // with the prompt; drawing one here first would only flicker.
            if(!isHuman)
            {
                view.hint = game.getPlayer(currentSeat).getName() + " is thinking...";
                renderer.drawFrame(view);
                pacer.beat();
            }

            Card* trump = game.getCurrentTrumpCard();
            const Suit* leadSuit = trick.hasLeadSuit() ? &trick.getLeadSuit() : nullptr;

            // The trick so far, plus what this player owes on their bid, so a
            // strategy can tell whether a card would actually win and whether
            // it wants it to.
            const Round& round = game.getCurrentRound();

            Card* playedCard = game.getPlayer(currentSeat).playCard(trump,
                                                       leadSuit,
                                                       trick.cardsInPlayOrder(),
                                                       round.getBet(currentSeat).value_or(0),
                                                       round.getTricksWon(currentSeat));

            // Contractually possible: a move provider returns nullptr when it
            // has no legal play. It should not be reachable, since a player
            // holding cards always has at least one legal one - so say so
            // loudly rather than dereferencing null or playing on regardless.
            if(playedCard == nullptr)
                throw std::runtime_error(game.getPlayer(currentSeat).getName() + " had no legal card to play.");

            if(!trick.hasLeadSuit())
                trick.setLeadSuit(playedCard->suit);

            trick.addPlayedCard(currentSeat, playedCard);

            view.hasLeadSuit = true;
            view.leadSuit = trick.getLeadSuit();
            view.table.push_back({ static_cast<unsigned int>(view.activeSeat), *playedCard, false });

            markCurrentlyWinning(trick);

            view.hint.clear();
            refreshFromEngine(view, game, openingSeat);
            renderer.drawFrame(view);

            // The human's own card needs no pause: the time they spent choosing
            // it already served as one, and the next bot's beat gives the card a
            // moment on screen before another lands on top of it.
            if(!isHuman)
                pacer.beat();

            currentSeat = game.getNextSeat(currentSeat);
        }

        const Seat winner = game.determineTrickWinner(trick);
        trick.setWinner(winner);

        // Added before the trick-end screen is drawn, so the "won" column
        // counts this trick: the round derives every seat's total from the
        // winners of the tricks it holds.
        game.addTrickToCurrentRound(trick);
        game.setRoundLeaderSeat(winner);

        view.phase = Phase::TrickEnd;
        view.activeSeat = -1;
        view.message = game.getPlayers().at(winner.index).getName() + " wins the trick.";
        refreshFromEngine(view, game, openingSeat);
        const bool moreTricks = trickIndex + 1 < trickCount;
        renderer.drawFrame(view, moreTricks ? "Press Enter for the next trick"
                                            : "Press Enter to see the round result");
        pacer.trickPause();
    }
}

void TerminalRomanianWhist::markCurrentlyWinning(const Trick& trick)
{
    // Asking the engine rather than reimplementing the ranking here means the
    // highlight can never disagree with the winner it eventually declares. It
    // copes with a partly played trick: it ranks whatever has been played.
    const Seat leading = game.determineTrickWinner(trick);

    for(PlayedCardView& played : view.table)
        played.isWinning = played.seatIndex == leading.index;
}
