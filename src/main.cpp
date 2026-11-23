#include <iostream>
#include <stdexcept>
#include <string>

#include <romanian_whist/Version.h>

#include "AppOptions.h"
#include "TerminalRomanianWhist.h"
#include "Version.h"

int main(int argc, char** argv)
{
    const CommandLine commandLine = AppOptions::fromCommandLine(argc, argv);

    switch(commandLine.action)
    {
        case CommandLine::Action::Error:
            std::cerr << commandLine.error << std::endl;
            return 2;

        case CommandLine::Action::Help:
            std::cout << AppOptions::helpText();
            return 0;

        case CommandLine::Action::Version:
            std::cout << "terminal_romanian_whist " << whist_terminal::VersionString
                      << " (romanian_whist engine " << romanian_whist::VersionString << ")"
                      << std::endl;
            return 0;

        case CommandLine::Action::Run:
            break;
    }

    AppOptions options = commandLine.options;

    try
    {
        TerminalRomanianWhist whist(options);
        whist.startGame();
    }
    catch(const std::exception& e)
    {
        // Reaches here when stdin closes mid-game, or when the engine refuses the
        // setup. Unwinding, rather than exiting where it happened, lets
        // Terminal's destructor restore the screen on the way out.
        std::cerr << "\n" << e.what() << " Exiting." << std::endl;
        return 1;
    }

    return 0;
}
