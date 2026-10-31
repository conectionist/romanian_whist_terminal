#include "Input.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{

// At end-of-file operator>> fails while constructing its sentry and returns
// without writing to its target, so the target must be initialised first.
//
// Malformed input is recoverable: clear the failure and discard the offending
// line. End of input is not - no further input can ever arrive, so re-prompting
// would spin forever. It propagates as an exception for main() to report.
void failOnEndOfInput()
{
    if(std::cin.eof())
        throw std::runtime_error("Input closed while awaiting a choice.");
}

// Every read takes its whole line, Enter included, so nothing is left behind for
// the next one. A leftover Enter would answer the next "Press Enter" on its own.
//
// This cannot be tidied up later by checking what is waiting: on libstdc++,
// std::cin is synced with stdio by default and in_avail() is then always 0.
void discardRestOfLine()
{
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string trim(const std::string& text)
{
    const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };

    const auto begin = std::find_if(text.begin(), text.end(), notSpace);
    const auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();

    return begin < end ? std::string(begin, end) : std::string();
}

// Names are printed to the terminal as they are, so a control character in one
// would be obeyed rather than shown: an escape sequence can move the cursor,
// retitle the window or write to the clipboard.
bool containsControlCharacter(const std::string& text)
{
    for(std::size_t i = 0 ; i < text.size() ; i++)
    {
        const unsigned char byte = static_cast<unsigned char>(text[i]);

        if(byte < 0x20 || byte == 0x7F)
            return true;

        // The C1 controls, U+0080 to U+009F, which UTF-8 encodes as C2 80 to C2 9F.
        if(byte == 0xC2 && i + 1 < text.size())
        {
            const unsigned char next = static_cast<unsigned char>(text[i + 1]);
            if(next >= 0x80 && next <= 0x9F)
                return true;
        }
    }

    return false;
}

} // namespace

bool Input::equalIgnoringCase(const std::string& left, const std::string& right)
{
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(),
                      [](unsigned char a, unsigned char b)
                      {
                          return std::tolower(a) == std::tolower(b);
                      });
}

int Input::readChoice(int lowest, int highest, const Redraw& redraw)
{
    int value = lowest;

    while(true)
    {
        if(std::cin >> value)
        {
            discardRestOfLine();

            if(value >= lowest && value <= highest)
                return value;

            redraw("Enter a number between " + std::to_string(lowest) +
                   " and " + std::to_string(highest) + ".");
        }
        else
        {
            failOnEndOfInput();

            std::cin.clear();
            discardRestOfLine();

            redraw("That is not a number. Enter one between " + std::to_string(lowest) +
                   " and " + std::to_string(highest) + ".");
        }
    }
}

std::string Input::readLine(const Redraw& redraw)
{
    std::string line;

    while(true)
    {
        if(!std::getline(std::cin, line))
        {
            failOnEndOfInput();

            std::cin.clear();
            redraw("Could not read that. Try again.");
            continue;
        }

        const std::string trimmed = trim(line);
        if(!trimmed.empty())
            return trimmed;

        redraw("That cannot be empty.");
    }
}

std::string Input::readName(unsigned int maxLength,
                            const std::vector<std::string>& taken,
                            const Redraw& redraw)
{
    while(true)
    {
        const std::string name = readLine(redraw);

        if(containsControlCharacter(name))
        {
            redraw("Names cannot contain control characters.");
            continue;
        }

        if(name.size() > maxLength)
        {
            redraw("Keep it to " + std::to_string(maxLength) + " characters or fewer.");
            continue;
        }

        // Bets and results are stored per name, so two players sharing one would
        // share a score. Reject rather than silently corrupt the round.
        const bool duplicate = std::any_of(taken.begin(), taken.end(),
            [&name](const std::string& other) { return Input::equalIgnoringCase(other, name); });

        if(duplicate)
        {
            redraw("Someone is already called that. Pick another name.");
            continue;
        }

        return name;
    }
}

void Input::waitForEnter(bool interactive)
{
    if(!interactive)
        return;

    std::string discarded;
    if(!std::getline(std::cin, discarded))
        failOnEndOfInput();
}
