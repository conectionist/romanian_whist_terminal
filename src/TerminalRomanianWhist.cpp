#include "TerminalRomanianWhist.h"

#include <stdexcept>


#include <romanian_whist/Trick.h>
#include <romanian_whist/Player.h>
#include "ConsoleMoveProvider.h"
#include "Input.h"
#include <romanian_whist/AiMoveProvider.h>
#include <romanian_whist/strategies/FirstCardStrategy.h>
#include <romanian_whist/strategies/RandomCardStrategy.h>

using namespace romanian_whist;

TerminalRomanianWhist::TerminalRomanianWhist(AppOptions& _options) : options(_options),
                                                                     terminal(options),
                                                                     cardFormat(terminal),
                                                                     renderer(terminal, cardFormat),
                                                                     pacer(options, terminal),
                                                                     ui{options, terminal, renderer, cardFormat, view}
{}

unsigned int TerminalRomanianWhist::seatOf(PlayerList::const_iterator player) const
{
    // By address, not by name: two players may share a name, and list nodes do
    // not move.
    const auto& players = game.getPlayers();

    for(unsigned int i = 0 ; i < players.size() ; i++)
    {
        if(&players.at(i) == &*player)
            return i;
    }

    return 0;
}

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
        std::unique_ptr<IStrategy> strategy = config.aiLevels[i] == 0
            ? std::unique_ptr<IStrategy>(std::make_unique<RandomCardStrategy>())
            : std::unique_ptr<IStrategy>(std::make_unique<FirstCardStrategy>());

        game.addPlayer(config.aiNames[i], std::make_unique<AiMoveProvider>(std::move(strategy)));
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

        openingSeat = seatOf(game.getCurrentRound().getOpeningPlayer());

        view.phase = Phase::Betting;
        view.message.clear();
        view.table.clear();

        auto currentPlayer = game.getFirstPlayerOfTheRound();

        for(unsigned int i = 0 ; i < game.getPlayerCount() ; i++)
        {
            view.activeSeat = static_cast<int>(seatOf(currentPlayer));
            refreshFromEngine(view, game, openingSeat);

            const bool isHuman = view.activeSeat == view.humanSeat;
            if(!isHuman)
                renderer.drawFrame(view);

            const unsigned int bet = currentPlayer->getBet(game.getCurrentTrumpCard(), i == 0);
            game.placeBet(currentPlayer, bet);

            if(!isHuman)
            {
                refreshFromEngine(view, game, openingSeat);
                renderer.drawFrame(view);
                pacer.beat();
            }

            currentPlayer = game.getNextPlayer(currentPlayer);
        }

        // Seed every seat's result. setResult assigns rather than accumulates,
        // so the per-trick updates that follow keep it live - but a player who
        // wins nothing would otherwise never be written at all.
        currentPlayer = game.getFirstPlayerOfTheRound();
        for(unsigned int i = 0 ; i < game.getPlayerCount() ; i++)
        {
            game.setResult(currentPlayer, 0);
            currentPlayer = game.getNextPlayer(currentPlayer);
        }

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

    std::vector<unsigned int> tricksWon(playerCount, 0);

    pacer.resetSkip();

    for(unsigned int trickIndex = 0 ; trickIndex < trickCount ; trickIndex++)
    {
        Trick trick;

        // Whoever leads this trick: the round's opener for the first, then each
        // trick's winner.
        const auto leader = game.getFirstPlayerOfTheRound();
        auto currentPlayer = leader;

        view.phase = Phase::Playing;
        view.trickNumber = trickIndex + 1;
        view.leadSeat = seatOf(leader);
        view.table.clear();
        view.hasLeadSuit = false;
        view.message.clear();

        for(unsigned int i = 0 ; i < playerCount ; i++)
        {
            view.activeSeat = static_cast<int>(seatOf(currentPlayer));

            const bool isHuman = view.activeSeat == view.humanSeat;

            refreshFromEngine(view, game, openingSeat);

            // The human's own frame is drawn by their move provider, complete
            // with the prompt; drawing one here first would only flicker.
            if(!isHuman)
            {
                view.hint = currentPlayer->getName() + " is thinking...";
                renderer.drawFrame(view);
                pacer.beat();
            }

            Card* trump = game.getCurrentTrumpCard();
            const Suit* leadSuit = trick.hasLeadSuit() ? &trick.getLeadSuit() : nullptr;
            Card* playedCard = currentPlayer->playCard(trump, leadSuit);

            // Contractually possible: a move provider returns nullptr when it
            // has no legal play. It should not be reachable, since a player
            // holding cards always has at least one legal one - so say so
            // loudly rather than dereferencing null or playing on regardless.
            if(playedCard == nullptr)
                throw std::runtime_error(currentPlayer->getName() + " had no legal card to play.");

            if(!trick.hasLeadSuit())
                trick.setLeadSuit(playedCard->suit);

            trick.addPlayedCard(playedCard);

            view.hasLeadSuit = true;
            view.leadSuit = trick.getLeadSuit();
            view.table.push_back({ static_cast<unsigned int>(view.activeSeat), *playedCard, false });

            markCurrentlyWinning(trick, leader);

            view.hint.clear();
            refreshFromEngine(view, game, openingSeat);
            renderer.drawFrame(view);

            // The human's own card needs no pause: the time they spent choosing
            // it already served as one, and the next bot's beat gives the card a
            // moment on screen before another lands on top of it.
            if(!isHuman)
                pacer.beat();

            currentPlayer = game.getNextPlayer(currentPlayer);
        }

        const auto winner = game.determineTrickWinner(trick, leader);
        trick.setWinner(winner);
        game.addTrickToCurrentRound(trick);
        game.setFirstPlayerOfTheRound(winner);

        const unsigned int winnerSeat = seatOf(winner);
        tricksWon[winnerSeat]++;

        // Pushed back every trick rather than once at the end of the round, so
        // the "won" column reads the engine's own numbers as they happen.
        game.setResult(winner, tricksWon[winnerSeat]);

        view.phase = Phase::TrickEnd;
        view.activeSeat = -1;
        view.message = winner->getName() + " wins the trick.";
        refreshFromEngine(view, game, openingSeat);
        const bool moreTricks = trickIndex + 1 < trickCount;
        renderer.drawFrame(view, moreTricks ? "Press Enter for the next trick"
                                            : "Press Enter to see the round result");
        pacer.trickPause();
    }
}

void TerminalRomanianWhist::markCurrentlyWinning(const Trick& trick, PlayerList::iterator leader)
{
    // Asking the engine rather than reimplementing the ranking here means the
    // highlight can never disagree with the winner it eventually declares. It
    // copes with a partly played trick: it ranks whatever has been played.
    const auto leadingPlayer = game.determineTrickWinner(trick, leader);
    const unsigned int leadingSeat = seatOf(leadingPlayer);

    for(PlayedCardView& played : view.table)
        played.isWinning = played.seatIndex == leadingSeat;
}
