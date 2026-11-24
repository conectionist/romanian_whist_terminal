#include "AppOptions.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#ifdef _WIN32
    // windows.h defines min and max as macros unless asked not to, which turns
    // any later std::min( into std::( and fails to compile.
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
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

constexpr long MaximumDelayMilliseconds = 10000;

constexpr const char* DelayPrefix = "--delay=";
constexpr const char* SeedPrefix = "--seed=";
constexpr const char* ReckonerPresetPrefix = "--reckoner-preset=";

bool startsWith(const char* argument, const char* prefix)
{
    return std::strncmp(argument, prefix, std::strlen(prefix)) == 0;
}

std::string invalidValueError(const char* flag,
                              const char* value,
                              const char* expected,
                              DisplayText::Encoding encoding)
{
    return std::string("Invalid value for ") + flag + ": '" + DisplayText::escaped(value, encoding) +
           "'\nExpected " + expected + ".";
}

// strtol and strtoull skip leading spaces and accept a sign, so without this
// " +7" would be read as 7 and "-0" as 0.
bool startsWithDigit(const char* text)
{
    return std::isdigit(static_cast<unsigned char>(*text)) != 0;
}

bool parseDelayValue(const char* text, long& valueOut)
{
    if(!startsWithDigit(text))
        return false;

    // A value too big for long comes back as LONG_MAX, which the cap below
    // brings down like any other.
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);

    if(*end != '\0')
        return false;

    // Anything past a few seconds a card is indistinguishable from a hang, and
    // the cap keeps the doubling for the trick dwell - and the conversion to
    // poll's int timeout - well clear of overflowing.
    valueOut = std::min(parsed, MaximumDelayMilliseconds);
    return true;
}

// A value out of range is refused, not capped: a seed the caller did not ask
// for would replay the wrong game without a word.
bool parseSeedValue(const char* text, std::uint32_t& valueOut)
{
    if(!startsWithDigit(text))
        return false;

    errno = 0;

    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text, &end, 10);

    if(*end != '\0' || errno == ERANGE)
        return false;

    if(parsed > std::numeric_limits<std::uint32_t>::max())
        return false;

    valueOut = static_cast<std::uint32_t>(parsed);
    return true;
}

const char* usageForBareFlag(const char* argument)
{
    if(std::strcmp(argument, "--delay") == 0)
        return "--delay=<ms>";
    if(std::strcmp(argument, "--seed") == 0)
        return "--seed=<n>";
    if(std::strcmp(argument, "--reckoner-preset") == 0)
        return "--reckoner-preset=<p>";
    return nullptr;
}

bool parseReckonerPresetValue(const char* text, romanian_whist::reckoner::ReckonerPreset& valueOut)
{
    std::string lowered(text);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if(lowered == "alpha")
        valueOut = romanian_whist::reckoner::ReckonerPreset::Alpha;
    else if(lowered == "beta")
        valueOut = romanian_whist::reckoner::ReckonerPreset::Beta;
    else if(lowered == "gamma")
        valueOut = romanian_whist::reckoner::ReckonerPreset::Gamma;
    else if(lowered == "delta")
        valueOut = romanian_whist::reckoner::ReckonerPreset::Delta;
    else
        return false;

    return true;
}

} // namespace

CommandLine AppOptions::fromCommandLine(int argc, char** argv)
{
    CommandLine result;
    AppOptions& options = result.options;

    bool helpRequested = false;
    bool versionRequested = false;

    const bool stdoutIsTerminal = isTerminal(true);
    const bool stdinIsTerminal = isTerminal(false);
    const bool dumb = environmentSaysDumbTerminal();

    options.interactive = stdoutIsTerminal && stdinIsTerminal;
    options.color = stdoutIsTerminal && !dumb && !environmentVariableSet("NO_COLOR");
    options.screenControl = stdoutIsTerminal && !dumb;
    options.altScreen = options.screenControl;

    // --ascii only changes which glyphs are drawn: the terminal still reads
    // whatever the locale says it does.
    options.encoding = localeSupportsUnicode() ? DisplayText::Encoding::Utf8
                                               : DisplayText::Encoding::EightBit;
    options.unicode = options.encoding == DisplayText::Encoding::Utf8 &&
                      !environmentVariableSet("WHIST_ASCII");

    // Redirected output is almost always a log or a test fixture: nothing there
    // should hang waiting for a keypress that will never come.
    options.autoAdvance = !options.interactive;

    long delayOverride = -1;
    if(const char* delayText = std::getenv("WHIST_DELAY_MS"))
    {
        long parsed = 0;
        if(parseDelayValue(delayText, parsed))
            delayOverride = parsed;
    }

    for(int i = 1 ; i < argc ; i++)
    {
        const char* argument = argv[i];

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
        else if(startsWith(argument, DelayPrefix))
        {
            const char* value = argument + std::strlen(DelayPrefix);
            long parsedDelay = 0;
            if(!parseDelayValue(value, parsedDelay))
            {
                result.action = CommandLine::Action::Error;
                result.error = invalidValueError("--delay", value, "a whole number of milliseconds, 0 or more",
                                                 options.encoding);
                return result;
            }
            delayOverride = parsedDelay;
        }
        else if(startsWith(argument, SeedPrefix))
        {
            const char* value = argument + std::strlen(SeedPrefix);
            std::uint32_t parsedSeed = 0;
            if(!parseSeedValue(value, parsedSeed))
            {
                result.action = CommandLine::Action::Error;
                result.error = invalidValueError("--seed", value, "a whole number from 0 to 4294967295",
                                                 options.encoding);
                return result;
            }
            options.seed = parsedSeed;
        }
        else if(startsWith(argument, ReckonerPresetPrefix))
        {
            const char* value = argument + std::strlen(ReckonerPresetPrefix);
            if(!parseReckonerPresetValue(value, options.reckonerPreset))
            {
                result.action = CommandLine::Action::Error;
                result.error = invalidValueError("--reckoner-preset", value,
                                                 "'alpha', 'beta', 'gamma', or 'delta'", options.encoding);
                return result;
            }
        }
        else if(const char* usage = usageForBareFlag(argument))
        {
            result.action = CommandLine::Action::Error;
            result.error = std::string(argument) + " needs a value: " + usage;
            return result;
        }
        else
        {
            result.action = CommandLine::Action::Error;
            result.error = "Unrecognised option: " + DisplayText::escaped(argument, options.encoding) +
                           "\nRun with --help to see the available options.";
            return result;
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

    // A bad flag has already returned an error, even after --help. Of the two
    // that print and exit, --help wins.
    if(helpRequested)
        result.action = CommandLine::Action::Help;
    else if(versionRequested)
        result.action = CommandLine::Action::Version;

    return result;
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
        "  --reckoner-preset=<p> Preset for Reckoner AI: alpha, beta, gamma, delta (default: gamma)\n"
        "  --seed=<n>        Replay the same deal and the same bot choices\n"
        "  -h, --help        Show this help\n"
        "  -v, --version     Show version information\n"
        "\n"
        "Colour is disabled automatically when output is redirected, when TERM is\n"
        "dumb, or when NO_COLOR is set. Suit glyphs need a UTF-8 locale.\n"
        "WHIST_DELAY_MS and WHIST_ASCII set --delay and --ascii from the environment.\n"
        "\n"
        "While a hand is playing out, press Enter to skip ahead to the end of the round.\n";
}
