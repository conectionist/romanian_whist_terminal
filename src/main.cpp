#include <cstring>
#include <iostream>
#include <stdexcept>

#include <romanian_whist/Version.h>

#include "TerminalRomanianWhist.h"
#include "Version.h"

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0)
        {
            std::cout << "terminal_romanian_whist " << whist_terminal::VersionString
                       << " (romanian_whist engine " << romanian_whist::VersionString << ")"
                       << std::endl;
            return 0;
        }
    }

    TerminalRomanianWhist whist;

    try
    {
        whist.startGame();
    }
    catch(const std::exception& e)
    {
        // Reaches here when stdin closes mid-game (Ctrl-D, or piped input
        // running out). Unwinding beats exiting from inside the input code.
        std::cerr << "\n" << e.what() << " Exiting." << std::endl;
        return 1;
    }

    return 0;
}
