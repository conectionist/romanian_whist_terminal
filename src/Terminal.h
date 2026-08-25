#ifndef TERMINAL_H
#define TERMINAL_H

#include "AppOptions.h"

#include <string>
#include <string_view>

// Everything that writes to the screen goes through here, so the escape
// sequences live in exactly one place and honouring --no-color or a piped stdout
// is not something each caller has to remember.
//
// Constructing one switches to the alternate screen (when allowed) and the
// destructor puts the terminal back, so an exception unwinding out of the game
// still leaves a usable shell.
class Terminal
{
private:
    // Non-const because a console that turns out not to support escape
    // sequences has to be able to switch the capabilities back off, and every
    // other component reads them from here afterwards.
    AppOptions& options;
    bool altScreenActive;

public:
    enum class Color
    {
        Default,
        Red,
        Green,
        Yellow,
        Blue,
        Cyan,
        Grey,
        Inverse
    };

    explicit Terminal(AppOptions& _options);
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    bool colorEnabled() const;
    bool unicodeEnabled() const;
    bool screenControlEnabled() const;
    bool interactive() const;

    // Clamped to 80: the layout is designed for it and never widens.
    unsigned int width() const;
    unsigned int height() const;

    void enterAltScreen();
    void leaveAltScreen();

    // A frame is built as one string and written in a single call - painting it
    // piecemeal makes the redraw visibly tear.
    //
    // beginFrame homes the cursor rather than clearing the screen: a full clear
    // flashes on every redraw, and there is one redraw per card played.
    void beginFrame(std::string& out) const;
    void endFrame(std::string& out) const;
    void write(std::string_view frame) const;

    // Terminates a line inside a frame, erasing whatever the previous frame left
    // to the right of it.
    void endLine(std::string& out) const;

    std::string paint(std::string_view text, Color color, bool bold = false) const;
    std::string dim(std::string_view text) const;

    // Columns the text occupies once printed: colour sequences count for
    // nothing and a multi-byte glyph counts once, so padding computed from
    // std::string::size() would be wrong on both counts.
    static unsigned int visibleWidth(std::string_view text);
};

#endif
