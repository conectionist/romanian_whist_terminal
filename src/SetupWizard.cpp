#include "SetupWizard.h"

#include "Input.h"

#include <iterator>
#include <string>

using romanian_whist::GameStructure;

namespace
{

const std::vector<std::string> BotNames = { "Ada", "Bob", "Cid", "Dot", "Eli" };

// What the menu calls each entry of AiLevels, in the same order. Kept here
// rather than inline in run() so the assert below can see it: the answer
// indexes AiLevels, so a label without a level would read past its end.
constexpr const char* AiLevelLabels[] = { "Randomly",
                                          "First playable card",
                                          "Low risk",
                                          "Ducking tricks",
                                          "Cyborg",
                                          "Reckoner (Alpha)",
                                          "Reckoner (Beta)",
                                          "Reckoner (Gamma)",
                                          "Reckoner (Delta)" };

static_assert(std::size(AiLevelLabels) == std::size(AiLevels),
              "Every level on the menu needs a label, and every label a level");

} // namespace

SetupWizard::SetupWizard(const Renderer& _renderer, const Terminal& _terminal)
    : renderer(_renderer),
      terminal(_terminal)
{}

int SetupWizard::ask(const std::string& title, const std::vector<std::string>& options) const
{
    const std::string prompt = "Choose 1-" + std::to_string(options.size()) + ": ";

    renderer.drawQuestion(title, options, prompt);

    return Input::readChoice(1, static_cast<int>(options.size()),
        [this, &title, &options, &prompt](const std::string& error)
        {
            renderer.drawQuestion(title, options, prompt, error);
        });
}

std::string SetupWizard::askName(const std::string& title,
                                 const std::vector<std::string>& taken) const
{
    const std::string prompt = "Name: ";

    renderer.drawQuestion(title, {}, prompt);

    return Input::readName(MaximumNameLength, taken,
        [this, &title, &prompt](const std::string& error)
        {
            renderer.drawQuestion(title, {}, prompt, error);
        });
}

GameConfig SetupWizard::run() const
{
    // Nine questions before the first card is dealt is itself a reason not to
    // play, so the common case gets to skip them.
    const int start = ask("How do you want to start?",
                          { "Quick game - you and three bots, 1-8-1",
                            "Watch four bots play",
                            "Set the game up myself" });

    if(start == 1)
    {
        GameConfig config;
        config.humanName = askName("What should we call you?", {});

        // The quick game is the one most players will judge the bots on, so it
        // seats the two that actually think, and keeps one simple bot for
        // contrast. Ordered to match the seats they are dealt into.
        static constexpr AiLevel QuickStartLevels[] = { AiLevel::Reckoner,
                                                        AiLevel::Cyborg,
                                                        AiLevel::LowRisk };

        // Skip any stock name the player has just taken for themselves: sharing
        // a name with a bot would mean sharing its bets and its score.
        for(const std::string& name : BotNames)
        {
            if(config.aiNames.size() == 3)
                break;

            if(!Input::equalIgnoringCase(name, config.humanName))
            {
                config.aiLevels.push_back(QuickStartLevels[config.aiNames.size()]);
                config.aiNames.push_back(name);
            }
        }

        return config;
    }

    if(start == 2)
        return demoConfig();

    GameConfig config;

    config.spectator = ask("Are you playing, or watching?",
                           { "Playing", "Watching" }) == 2;

    std::vector<std::string> taken;

    if(!config.spectator)
    {
        config.humanName = askName("What should we call you?", taken);
        taken.push_back(config.humanName);
    }

    // A human takes one of the seats, so the bot count has to leave room.
    const int maximumBots = MaximumPlayers - (config.spectator ? 0 : 1);

    // A spectated game has no human seat, so it needs two bots at minimum;
    // a played one already has the human, so one bot is enough.
    const int minimumBots = config.spectator ? 2 : 1;

    std::vector<std::string> countOptions;
    for(int i = minimumBots ; i <= maximumBots ; i++)
        countOptions.push_back(std::to_string(i) + (i == 1 ? " bot" : " bots"));

    const int botCount = ask("How many bots?", countOptions) + minimumBots - 1;

    for(int i = 0 ; i < botCount ; i++)
    {
        const std::string name = askName("Name for bot " + std::to_string(i + 1) +
                                         " of " + std::to_string(botCount), taken);
        config.aiNames.push_back(name);
        taken.push_back(name);
    }

    const std::vector<std::string> levelOptions(std::begin(AiLevelLabels), std::end(AiLevelLabels));

    for(int i = 0 ; i < botCount ; i++)
    {
        const int level = ask("How should " + config.aiNames[i] + " play?", levelOptions);
        config.aiLevels.push_back(AiLevels[level - 1]);
    }

    config.structure = ask("Which shape of game?",
                           { "1-8-1 - up from one card, then back down",
                             "8-1-8 - down from eight cards, then back up" }) == 1
        ? GameStructure::S_181
        : GameStructure::S_818;

    config.endWithForeheadAndHidden =
        ask("End with forehead and hidden hands?", { "Yes", "No" }) == 1;

    config.all1GamesAreForehead =
        ask("Play every one-card round forehead?", { "No", "Yes" }) == 2;

    return config;
}

GameConfig SetupWizard::demoConfig()
{
    GameConfig config;

    config.spectator = true;
    config.aiNames = { "Danutz", "Mihai", "Aditz", "Fane" };

    // The Cyborg sits with the two strategies the engine measures it against,
    // LowRisk and the Reckoner's Beta preset, so the demo doubles as that
    // comparison.
    config.aiLevels = { AiLevel::ReckonerGamma, AiLevel::LowRisk, AiLevel::Cyborg, AiLevel::ReckonerBeta };
    config.structure = GameStructure::S_181;
    config.endWithForeheadAndHidden = true;
    config.all1GamesAreForehead = false;

    return config;
}
