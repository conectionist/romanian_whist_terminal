#ifndef DISPLAY_TEXT_H
#define DISPLAY_TEXT_H

#include <string>
#include <string_view>

// Checks text from outside the program, such as a typed name or a flag, before
// it is printed: a terminal obeys a control character rather than showing it,
// so an escape sequence could move the cursor or retitle the window.
namespace DisplayText
{

// True for well-formed UTF-8 with no C0, DEL or C1 control. Malformed UTF-8 is
// refused because a lenient decoder can read an overlong form as a control,
// and a stray lead byte throws out the column count.
bool isPrintable(std::string_view text);

// The text with every byte that is not part of a printable character written
// as \xNN, for echoing text that has to be shown whatever it holds.
std::string escaped(std::string_view text);

} // namespace DisplayText

#endif
