#include "AppOptions.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
    #include <io.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace
{

bool isTerminal(bool standardOutput)
{
#ifdef _WIN32
    return _isatty(_fileno(standardOutput ? stdout : stdin)) != 0;
#else
    return isatty(standardOutput ? STDOUT_FILENO : STDIN_FILENO) != 0;
#endif
}

// NO_COLOR is honoured on presence, whatever the value - that is what the
// convention specifies, so NO_COLOR=0 still disables colour.
bool environmentVariableSet(const char* name)
{
    return std::getenv(name) != nullptr;
}

bool environmentSaysDumbTerminal()
{
    const char* term = std::getenv("TERM");
    return term != nullptr && std::strcmp(term, "dumb") == 0;
}

// A UTF-8 locale is the only way to know the suit glyphs will land as single
// columns rather than replacement boxes. On Windows the console code page is
// what matters, and Terminal sets it at startup.
bool localeSupportsUnicode()
{
#ifdef _WIN32
    return true;
#else
    for(const char* name : {"LC_ALL", "LC_CTYPE", "LANG"})
    {
        const char* value = std::getenv(name);
        if(value == nullptr || *value == '\0')
            continue;

        std::string lowered(value);
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return lowered.find("utf-8") != std::string::npos ||
               lowered.find("utf8") != std::string::npos;
    }

    return false;
#endif
}

// Returns false if the argument is not "--flag=<non-negative integer>".
bool parseDelayFlag(const char* argument, const char* prefix, long& valueOut)
{
    const std::size_t prefixLength = std::strlen(prefix);
    if(std::strncmp(argument, prefix, prefixLength) != 0)
        return false;

    const char* text = argument + prefixLength;
    if(*text == '\0')
        return false;

    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);

    if(end == text || *end != '\0' || parsed < 0)
        return false;

    valueOut = parsed;
    return true;
}

} // namespace

bool AppOptions::fromCommandLine(int argc,
                                 char** argv,
                                 AppOptions& optionsOut,
                                 bool& helpRequested,
                                 bool& versionRequested,
                                 std::string& errorOut)
{
    AppOptions options;

    helpRequested = false;
    versionRequested = false;

    const bool stdoutIsTerminal = isTerminal(true);
    const bool stdinIsTerminal = isTerminal(false);
    const bool dumb = environmentSaysDumbTerminal();

    options.interactive = stdoutIsTerminal && stdinIsTerminal;
    options.color = stdoutIsTerminal && !dumb && !environmentVariableSet("NO_COLOR");
    options.screenControl = stdoutIsTerminal && !dumb;
    options.altScreen = options.screenControl;
    options.unicode = localeSupportsUnicode() && !environmentVariableSet("WHIST_ASCII");

    // Redirected output is almost always a log or a test fixture: nothing there
    // should hang waiting for a keypress that will never come.
    options.autoAdvance = !options.interactive;

    long delayOverride = -1;
    if(const char* delayText = std::getenv("WHIST_DELAY_MS"))
    {
        std::string flag = std::string("--delay=") + delayText;
        long parsed = 0;
        if(parseDelayFlag(flag.c_str(), "--delay=", parsed))
            delayOverride = parsed;
    }

    for(int i = 1 ; i < argc ; i++)
    {
        const char* argument = argv[i];
        long parsedDelay = 0;

        if(std::strcmp(argument, "--help") == 0 || std::strcmp(argument, "-h") == 0)
            helpRequested = true;
        else if(std::strcmp(argument, "--version") == 0 || std::strcmp(argument, "-v") == 0)
            versionRequested = true;
        else if(std::strcmp(argument, "--no-color") == 0 || std::strcmp(argument, "--no-colour") == 0)
            options.color = false;
        else if(std::strcmp(argument, "--ascii") == 0)
            options.unicode = false;
        else if(std::strcmp(argument, "--no-clear") == 0)
        {
            options.screenControl = false;
            options.altScreen = false;
        }
        else if(std::strcmp(argument, "--no-alt-screen") == 0)
            options.altScreen = false;
        else if(std::strcmp(argument, "--auto") == 0)
            options.autoAdvance = true;
        else if(std::strcmp(argument, "--no-animate") == 0)
        {
            delayOverride = 0;
            options.autoAdvance = true;
        }
        else if(std::strcmp(argument, "--demo") == 0)
            options.demo = true;
        else if(parseDelayFlag(argument, "--delay=", parsedDelay))
            delayOverride = parsedDelay;
        else
        {
            errorOut = std::string("Unrecognised option: ") + argument +
                       "\nRun with --help to see the available options.";
            return false;
        }
    }

    if(delayOverride >= 0)
    {
        options.playDelay = std::chrono::milliseconds(delayOverride);
        // A finished trick wants longer on screen than a single card does.
        options.trickDwell = std::chrono::milliseconds(delayOverride * 2);
    }

    if(!options.screenControl)
        options.altScreen = false;

    optionsOut = options;
    return true;
}

std::string AppOptions::helpText()
{
    return
        "Romanian Whist - a terminal card game\n"
        "\n"
        "Usage: terminal_romanian_whist [options]\n"
        "\n"
        "Display:\n"
        "  --no-color        Never emit colour sequences\n"
        "  --ascii           Use letters for suits instead of glyphs\n"
        "  --no-clear        Scroll output instead of redrawing in place\n"
        "  --no-alt-screen   Draw on the main screen, overwriting scrollback\n"
        "\n"
        "Pace:\n"
        "  --delay=<ms>      Time between bot plays (default 700)\n"
        "  --no-animate      Play at full speed, never pausing\n"
        "  --auto            Advance without waiting for Enter\n"
        "\n"
        "Other:\n"
        "  --demo            Skip setup and watch four bots play\n"
        "  -h, --help        Show this help\n"
        "  -v, --version     Show version information\n"
        "\n"
        "Colour is disabled automatically when output is redirected, when TERM is\n"
        "dumb, or when NO_COLOR is set. Suit glyphs need a UTF-8 locale.\n"
        "WHIST_DELAY_MS and WHIST_ASCII set --delay and --ascii from the environment.\n"
        "\n"
        "While a hand is playing out, press Enter to skip ahead to the end of the round.\n";
}
