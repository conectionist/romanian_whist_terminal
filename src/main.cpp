#include <iostream>
#include <stdexcept>
#include <string>

#include <romanian_whist/Version.h>

#include "AppOptions.h"
#include "TerminalRomanianWhist.h"
#include "Version.h"

int main(int argc, char** argv)
{
    AppOptions options;
    bool helpRequested = false;
    bool versionRequested = false;
    std::string error;

    if(!AppOptions::fromCommandLine(argc, argv, options, helpRequested, versionRequested, error))
    {
        std::cerr << error << std::endl;
        return 2;
    }

    if(helpRequested)
    {
        std::cout << AppOptions::helpText();
        return 0;
    }

    if(versionRequested)
    {
        std::cout << "terminal_romanian_whist " << whist_terminal::VersionString
                  << " (romanian_whist engine " << romanian_whist::VersionString << ")"
                  << std::endl;
        return 0;
    }

    try
    {
        TerminalRomanianWhist whist(options);
        whist.startGame();
    }
    catch(const std::exception& e)
    {
        // Reaches here when stdin closes mid-game (Ctrl-D, or piped input
        // running out). Unwinding beats exiting from inside the input code:
        // it lets Terminal's destructor restore the screen on the way out.
        std::cerr << "\n" << e.what() << " Exiting." << std::endl;
        return 1;
    }

    return 0;
}
