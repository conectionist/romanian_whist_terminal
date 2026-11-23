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
    Ducking,
    Cyborg,
    ReckonerAlpha,
    ReckonerBeta,
    ReckonerGamma,
    ReckonerDelta,
    Reckoner
};

// Every option the wizard offers, in menu order, so the menu and the factory
// cannot drift apart.
inline constexpr AiLevel AiLevels[] = { AiLevel::Random,
                                        AiLevel::FirstCard,
                                        AiLevel::LowRisk,
                                        AiLevel::Ducking,
                                        AiLevel::Cyborg,
                                        AiLevel::ReckonerAlpha,
                                        AiLevel::ReckonerBeta,
                                        AiLevel::ReckonerGamma,
                                        AiLevel::ReckonerDelta };

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
// Every answer is bounded, so the engine accepts the setup it returns.
class SetupWizard
{
private:
    const Renderer& renderer;
    const Terminal& terminal;

    // Longest name that still leaves room for the " (you)" suffix in the seat
    // table's name column.
    static constexpr unsigned int MaximumNameLength = 10;

    // The engine refuses more than six players by throwing, which would end the
    // program before the game starts.
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
