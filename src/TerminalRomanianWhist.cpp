#include "TerminalRomanianWhist.h"

#include <iostream>
#include <unordered_map>

#include <romanian_whist/Trick.h>
#include <romanian_whist/Player.h>
#include "ConsoleMoveProvider.h"
#include "Input.h"
#include <romanian_whist/AiMoveProvider.h>
#include <romanian_whist/strategies/FirstCardStrategy.h>
#include <romanian_whist/strategies/RandomCardStrategy.h>

using std::cin, std::cout, std::endl;
using std::string, std::vector;
using namespace romanian_whist;

TerminalRomanianWhist::TerminalRomanianWhist(AppOptions& _options) : options(_options),
                                                                     terminal(options),
                                                                     cardFormat(terminal),
                                                                     renderer(terminal, cardFormat)
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
    if(options.demo)
        initializeTest();
    else
        initialize();

    loop();
}

void TerminalRomanianWhist::initialize()
{
    int mode;
    cout << "Choose the mode:" << endl;
    cout << "1) Participant (you vs AI bots)" << endl;
    cout << "2) Spectator (watch AI bots compete)" << endl;
    cin >> mode;
    bool isParticipant = mode == 1;

    string playerName;
    if(isParticipant)
    {
        cout << "Enter your name:" << endl;
        cin >> playerName;
    }

    int aiCount;
    cout << "Enter the number of AI players (1-5):" << endl;
    cin >> aiCount;

    vector<string> aiNames(aiCount);
    cout << "Enter the names of the " << aiCount << " AI players:" << endl;
    for(int i = 0 ; i < aiCount ; i++)
        cin >> aiNames[i];

    vector<int> aiLevels(aiCount);
    for(int i = 0 ; i < aiCount ; i++)
    {
        cout << "Choose the level for " << aiNames[i] << ":" << endl;
        cout << "0) Random" << endl;
        cout << "1) First" << endl;
        cin >> aiLevels[i];
    }

    int gameStructure;
    cout << "Choose the game structure:" << endl;
    cout << "1) 1-8-1" << endl;
    cout << "2) 8-1-8" << endl;
    cin >> gameStructure;

    int endWithForeheadAndHidden;
    cout << "Should the game end with forehead and hidden hands?" << endl;
    cout << "1) Yes" << endl;
    cout << "2) No" << endl;
    cin >> endWithForeheadAndHidden;

    int all1GamesAreForehead;
    cout << "Should all 1-games be forehead?" << endl;
    cout << "1) Of course not!" << endl;
    cout << "2) Sure!" << endl;
    cin >> all1GamesAreForehead;

    if(isParticipant)
    {
        // Seated first, so seat 0 is the human whenever there is one.
        game.addPlayer(playerName, std::make_unique<ConsoleMoveProvider>());
        view.humanSeat = 0;
    }

    for(int i = 0 ; i < aiCount ; i++)
    {
        std::unique_ptr<IStrategy> strategy = aiLevels[i] == 0
            ? std::unique_ptr<IStrategy>(std::make_unique<RandomCardStrategy>())
            : std::unique_ptr<IStrategy>(std::make_unique<FirstCardStrategy>());
        game.addPlayer(aiNames[i], std::make_unique<AiMoveProvider>(std::move(strategy)));
    }

    int playerCount = aiCount + (isParticipant ? 1 : 0);

    game.initializeScoreboard(gameStructure == 1 ? GameStructure::S_181 : GameStructure::S_818,
                              endWithForeheadAndHidden == 1,
                              all1GamesAreForehead == 1);

    game.initializeDeck(playerCount);
}

void TerminalRomanianWhist::initializeTest()
{
    int playerCount = 4;
    
    //game.addPlayer("Danutz", std::make_unique<ConsoleMoveProvider>());
    game.addPlayer("Danutz", std::make_unique<AiMoveProvider>(std::make_unique<FirstCardStrategy>()));
    game.addPlayer("Mihai", std::make_unique<AiMoveProvider>(std::make_unique<FirstCardStrategy>()));
    game.addPlayer("Aditz", std::make_unique<AiMoveProvider>(std::make_unique<RandomCardStrategy>()));
    game.addPlayer("Fane", std::make_unique<AiMoveProvider>(std::make_unique<RandomCardStrategy>()));
    
    game.initializeScoreboard(GameStructure::S_181,
                              true,
                              false);

    game.initializeDeck(playerCount);
}

void TerminalRomanianWhist::loop()
{
    game.setStatus(GameStatus::InProgress);

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

            const unsigned int bet = currentPlayer->getBet(game.getCurrentTrumpCard(), i == 0);
            game.placeBet(currentPlayer, bet);

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
        Input::waitForEnter(terminal.interactive() && !options.autoAdvance);

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
    
    // Track tricks won by each player
    std::unordered_map<std::string, unsigned int> tricksWon;
    
    // Initialize all players with 0 tricks won
    auto firstPlayer = game.getFirstPlayerOfTheRound();
    auto currentPlayer = firstPlayer;
    for(unsigned int i = 0 ; i < game.getPlayerCount() ; i++)
    {
        tricksWon[currentPlayer->getName()] = 0;
        currentPlayer = game.getNextPlayer(currentPlayer);
    }

    for(unsigned int trickIndex = 0 ; trickIndex < trickCount ; trickIndex++)
    {
        Trick trick;
        currentPlayer = game.getFirstPlayerOfTheRound();

        cout << endl << "Trick " << (trickIndex + 1) << " of " << trickCount << endl;

        for(unsigned int i = 0 ; i < game.getPlayerCount() ; i++)
        {
            Card* trump = game.getCurrentTrumpCard();
            const Suit* leadSuit = trick.getPlayedCards().empty() ? nullptr : &trick.getLeadSuit();
            Card* playedCard = currentPlayer->playCard(trump, leadSuit);

            if(trick.getPlayedCards().empty())
                trick.setLeadSuit(playedCard->suit);

            trick.addPlayedCard(playedCard);
            cout << currentPlayer->getName() << " plays " << playedCard->toString() << endl;

            currentPlayer = game.getNextPlayer(currentPlayer);
        }

        auto winner = game.determineTrickWinner(trick, game.getFirstPlayerOfTheRound());
        trick.setWinner(winner);
        game.addTrickToCurrentRound(trick);
        game.setFirstPlayerOfTheRound(winner);

        // Track tricks won
        tricksWon[winner->getName()]++;
        
        cout << winner->getName() << " wins the trick." << endl;
    }
    
    // Set the results for each player
    currentPlayer = firstPlayer;
    for(unsigned int i = 0 ; i < game.getPlayerCount() ; i++)
    {
        unsigned int wonTricks = tricksWon[currentPlayer->getName()];
        game.setResult(currentPlayer, wonTricks);
        currentPlayer = game.getNextPlayer(currentPlayer);
    }
}
