#include <iostream>
#include <stdexcept>

#include "TerminalRomanianWhist.h"

int main()
{
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
