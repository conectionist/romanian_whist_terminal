#ifndef INPUT_H
#define INPUT_H

#include <functional>
#include <string>
#include <vector>

// Reading from stdin, with the rule that malformed input is recoverable and a
// closed stream is not.
//
// Every read takes a Redraw: once the screen is being repainted in place, a
// bare "try again" line printed under the prompt would be wiped by the next
// frame. The caller repaints the whole frame with the error shown in it.
namespace Input
{

using Redraw = std::function<void(const std::string& error)>;

// Reads an integer in [lowest, highest], re-prompting until one arrives.
// Throws std::runtime_error at end of input.
int readChoice(int lowest, int highest, const Redraw& redraw);

// Reads a whole line, trimmed, re-prompting while it is empty. Takes the line
// rather than a whitespace-delimited word so that a name with a space in it
// does not leave its tail queued up for the next prompt.
std::string readLine(const Redraw& redraw);

// As readLine, but also rejects names longer than maxLength or matching one
// already in `taken` (case-insensitively).
std::string readName(unsigned int maxLength,
                     const std::vector<std::string>& taken,
                     const Redraw& redraw);

// Waits for Enter. Returns immediately if stdin is not interactive.
void waitForEnter(bool interactive);

} // namespace Input

#endif
