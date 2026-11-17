#ifndef DISPLAY_TEXT_H
#define DISPLAY_TEXT_H

#include <cstddef>
#include <string>
#include <string_view>

// Checks text from outside the program, such as a typed name or a flag, before
// it is printed: a terminal obeys a control character rather than showing it,
// so an escape sequence could move the cursor or retitle the window.
namespace DisplayText
{

// How the terminal reads the bytes it is sent, which decides which bytes it
// obeys as controls.
enum class Encoding
{
    Utf8,

    // Latin-1 and the like. Every byte from 0x80 to 0x9F is a C1 control here,
    // even one that would be part of a well-formed UTF-8 character.
    EightBit
};

// True if every character is one the terminal will show rather than obey. In
// UTF-8, malformed text is refused too: a lenient decoder can read an overlong
// form as a control, and a stray lead byte throws out the column count.
bool isPrintable(std::string_view text, Encoding encoding);

// The text with every byte that isPrintable would refuse written as \xNN, for
// echoing text that has to be shown whatever it holds.
std::string escaped(std::string_view text, Encoding encoding);

// Characters rather than bytes, so a limit on a name's length does not count an
// accented letter twice. Assumes text that isPrintable accepts.
std::size_t characterCount(std::string_view text, Encoding encoding);

} // namespace DisplayText

#endif
