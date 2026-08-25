#include "Terminal.h"

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
    #include <io.h>
    #include <windows.h>
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif

namespace
{

constexpr unsigned int LayoutWidth = 80;
constexpr unsigned int FallbackHeight = 24;

const char* colorCode(Terminal::Color color)
{
    switch(color)
    {
        case Terminal::Color::Red:     return "\x1b[31m";
        case Terminal::Color::Green:   return "\x1b[32m";
        case Terminal::Color::Yellow:  return "\x1b[33m";
        case Terminal::Color::Blue:    return "\x1b[34m";
        case Terminal::Color::Cyan:    return "\x1b[36m";
        case Terminal::Color::Grey:    return "\x1b[90m";
        case Terminal::Color::Inverse: return "\x1b[7m";
        case Terminal::Color::Default: break;
    }

    return "";
}

// Ctrl-C kills the process outright, so ~Terminal never runs and the shell is
// left on the alternate screen with an invisible cursor. Put the terminal back
// first, then die the way an uncaught SIGINT would.
//
// Only async-signal-safe calls belong here: write(2) and _exit(2), no
// iostreams, no allocation. It runs whether or not the alternate screen was
// entered, since the sequences are harmless when it was not.
extern "C" void restoreTerminalOnInterrupt(int)
{
    const char restore[] = "\x1b[?25h\x1b[0m\x1b[?1049l";

#ifdef _WIN32
    _write(_fileno(stdout), restore, sizeof(restore) - 1);
    _exit(130);
#else
    const ssize_t written = write(STDOUT_FILENO, restore, sizeof(restore) - 1);
    (void)written;
    _exit(130);
#endif
}

// Without this a Windows console prints the escape sequences literally. Called
// once; a failure means the caller must fall back to plain output.
bool enableWindowsVirtualTerminal()
{
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if(handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD mode = 0;
    if(!GetConsoleMode(handle, &mode))
        return false;

    if(!SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return false;

    SetConsoleOutputCP(CP_UTF8);
    return true;
#else
    return true;
#endif
}

} // namespace

Terminal::Terminal(AppOptions& _options) : options(_options),
                                           altScreenActive(false)
{
    // A console that cannot process escape sequences must not be sent any, and
    // one still on its default code page cannot print the suit glyphs. This is
    // only reachable on Windows; elsewhere it always succeeds.
    //
    // Called unconditionally: it is also what selects the UTF-8 code page, so
    // skipping it when colour and screen control are both off would leave the
    // glyphs to come out as mojibake.
    if(!enableWindowsVirtualTerminal())
    {
        options.color = false;
        options.unicode = false;
        options.screenControl = false;
        options.altScreen = false;
    }

    if(options.screenControl)
        std::signal(SIGINT, restoreTerminalOnInterrupt);

    enterAltScreen();
}

Terminal::~Terminal()
{
    if(options.screenControl)
        std::signal(SIGINT, SIG_DFL);

    leaveAltScreen();

    if(options.screenControl)
        write("\x1b[?25h\x1b[0m");
}

bool Terminal::colorEnabled() const
{
    return options.color;
}

bool Terminal::unicodeEnabled() const
{
    return options.unicode;
}

bool Terminal::screenControlEnabled() const
{
    return options.screenControl;
}

bool Terminal::interactive() const
{
    return options.interactive;
}

unsigned int Terminal::width() const
{
    unsigned int detected = 0;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        detected = static_cast<unsigned int>(info.srWindow.Right - info.srWindow.Left + 1);
#else
    winsize size{};
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        detected = size.ws_col;
#endif

    if(detected == 0)
    {
        if(const char* columns = std::getenv("COLUMNS"))
            detected = static_cast<unsigned int>(std::atoi(columns));
    }

    if(detected == 0)
        return LayoutWidth;

    return std::min(detected, LayoutWidth);
}

unsigned int Terminal::height() const
{
    unsigned int detected = 0;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        detected = static_cast<unsigned int>(info.srWindow.Bottom - info.srWindow.Top + 1);
#else
    winsize size{};
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_row > 0)
        detected = size.ws_row;
#endif

    if(detected == 0)
    {
        if(const char* lines = std::getenv("LINES"))
            detected = static_cast<unsigned int>(std::atoi(lines));
    }

    return detected == 0 ? FallbackHeight : detected;
}

void Terminal::enterAltScreen()
{
    if(altScreenActive || !options.altScreen)
        return;

    write("\x1b[?1049h\x1b[?25l\x1b[H\x1b[J");
    altScreenActive = true;
}

void Terminal::leaveAltScreen()
{
    if(!altScreenActive)
        return;

    write("\x1b[?25h\x1b[?1049l");
    altScreenActive = false;
}

void Terminal::beginFrame(std::string& out) const
{
    if(options.screenControl)
        out += "\x1b[H";
    else
        out += "\n";
}

void Terminal::endFrame(std::string& out) const
{
    if(options.screenControl)
        out += "\x1b[J";
}

void Terminal::endLine(std::string& out) const
{
    if(options.screenControl)
        out += "\x1b[K";

    out += "\n";
}

void Terminal::write(std::string_view frame) const
{
    std::fwrite(frame.data(), 1, frame.size(), stdout);
    std::fflush(stdout);
}

std::string Terminal::paint(std::string_view text, Color color, bool bold) const
{
    if(!options.color || (color == Color::Default && !bold))
        return std::string(text);

    std::string result;
    if(bold)
        result += "\x1b[1m";

    result += colorCode(color);
    result += text;
    result += "\x1b[0m";

    return result;
}

std::string Terminal::dim(std::string_view text) const
{
    if(!options.color)
        return std::string(text);

    return "\x1b[2m" + std::string(text) + "\x1b[0m";
}

unsigned int Terminal::visibleWidth(std::string_view text)
{
    unsigned int columns = 0;

    for(std::size_t i = 0 ; i < text.size() ; )
    {
        if(text[i] == '\x1b')
        {
            // Skip the whole CSI sequence: ESC [ ... <final byte in @-~>.
            i++;
            if(i < text.size() && text[i] == '[')
            {
                i++;
                while(i < text.size() && (text[i] < '@' || text[i] > '~'))
                    i++;
            }
            if(i < text.size())
                i++;
            continue;
        }

        // Count one column per codepoint by skipping UTF-8 continuation bytes.
        // Every glyph this interface prints is single-width; suit symbols are
        // emitted without a variation selector precisely so they stay that way.
        const unsigned char byte = static_cast<unsigned char>(text[i]);
        if(byte < 0x80)
            i += 1;
        else if((byte & 0xE0) == 0xC0)
            i += 2;
        else if((byte & 0xF0) == 0xE0)
            i += 3;
        else if((byte & 0xF8) == 0xF0)
            i += 4;
        else
            i += 1;

        columns++;
    }

    return columns;
}
