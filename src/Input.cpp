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

std::string trim(const std::string& text)
{
    const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };

    const auto begin = std::find_if(text.begin(), text.end(), notSpace);
    const auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();

    return begin < end ? std::string(begin, end) : std::string();
}

bool equalIgnoringCase(const std::string& left, const std::string& right)
{
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(),
                      [](unsigned char a, unsigned char b)
                      {
                          return std::tolower(a) == std::tolower(b);
                      });
}

} // namespace

int Input::readChoice(int lowest, int highest, const Redraw& redraw)
{
    int value = lowest;

    while(true)
    {
        if(std::cin >> value)
        {
            if(value >= lowest && value <= highest)
                return value;

            redraw("Enter a number between " + std::to_string(lowest) +
                   " and " + std::to_string(highest) + ".");
        }
        else
        {
            failOnEndOfInput();

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

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
        // A preceding formatted read leaves its newline behind; skip it rather
        // than returning an empty line for it.
        if(std::cin.peek() == '\n')
            std::cin.ignore();

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

        if(name.size() > maxLength)
        {
            redraw("Keep it to " + std::to_string(maxLength) + " characters or fewer.");
            continue;
        }

        // Bets and results are stored per name, so two players sharing one would
        // share a score. Reject rather than silently corrupt the round.
        const bool duplicate = std::any_of(taken.begin(), taken.end(),
            [&name](const std::string& other) { return equalIgnoringCase(other, name); });

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

    if(std::cin.peek() == '\n')
        std::cin.ignore();

    std::string discarded;
    if(!std::getline(std::cin, discarded))
        failOnEndOfInput();
}
