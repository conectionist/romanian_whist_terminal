#ifndef SETUP_WIZARD_H
#define SETUP_WIZARD_H

#include "Renderer.h"
#include "Terminal.h"

#include <romanian_whist/Scoreboard.h>

#include <string>
#include <vector>

// Which strategy a bot plays with. The order is the order the setup wizard
// offers them in, so it doubles as the menu index.
enum class AiLevel
{
    Random,
    FirstCard,
    LowRisk,
    Ducking
};

// Every option the wizard offers, in menu order, so the menu and the factory
// cannot drift apart.
inline constexpr AiLevel AiLevels[] = { AiLevel::Random,
                                        AiLevel::FirstCard,
                                        AiLevel::LowRisk,
                                        AiLevel::Ducking };

struct GameConfig
{
    bool spectator = false;
    std::string humanName;
    std::vector<std::string> aiNames;

    // One per entry in aiNames, in the same order.
    std::vector<AiLevel> aiLevels;

    romanian_whist::GameStructure structure = romanian_whist::GameStructure::S_181;
    bool endWithForeheadAndHidden = true;
    bool all1GamesAreForehead = false;
};

// Asks what kind of game to play.
//
// Every answer is bounded. That is not only tidiness: the engine builds its deck
// from the player count with no range check of its own, so an out-of-range count
// reaching it is undefined behaviour, and it stores bets against player names,
// so two players sharing one silently share a score.
class SetupWizard
{
private:
    const Renderer& renderer;
    const Terminal& terminal;

    // Longest name that still leaves room for the " (you)" suffix in the seat
    // table's name column.
    static constexpr unsigned int MaximumNameLength = 10;

    // The engine deals from a deck sized by player count and cannot go above
    // this without indexing off the end of the ranks.
    static constexpr int MaximumPlayers = 6;

    int ask(const std::string& title, const std::vector<std::string>& options) const;
    std::string askName(const std::string& title,
                        const std::vector<std::string>& taken) const;

public:
    SetupWizard(const Renderer& _renderer, const Terminal& _terminal);

    GameConfig run() const;

    // Four bots, 1-8-1, no questions asked.
    static GameConfig demoConfig();
};

#endif
