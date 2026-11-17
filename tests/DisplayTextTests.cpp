#include <catch2/catch_test_macros.hpp>

#include "DisplayText.h"

#include <string>
#include <string_view>

using namespace std::string_view_literals;

// Every case is written as bytes, so the tests do not depend on the compiler
// reading the source as UTF-8. A literal is split where the next character is a
// hex digit, which would otherwise run on into the \x escape before it.

TEST_CASE("Printable text, inside and outside ASCII, is printable", "[display-text]")
{
    const std::string_view cases[] = {
        "Ana"sv,
        " ~"sv,                            // the ends of printable ASCII
        "\xC8\x98" "tefan"sv,              // a Romanian letter, two bytes
        "\xC2\xA0"sv,                      // U+00A0, just past the C1 controls
        "\xC2\xA9" "Ana"sv,                // U+00A9: lead byte C2, as the C1 controls have
        "\xE2\x99\xA0"sv,                  // a suit glyph, three bytes
        "\xF0\x9F\x82\xA1"sv,              // a playing card, four bytes
        "\xF4\x8F\xBF\xBF"sv,              // U+10FFFF, the last code point
        ""sv,
    };

    for(const std::string_view text : cases)
    {
        CAPTURE(DisplayText::escaped(text));
        CHECK(DisplayText::isPrintable(text));

        // Compared as std::string: Catch2 can print a string_view only when it
        // was built as C++17 or later, and its macOS and Windows builds are not.
        CHECK(DisplayText::escaped(text) == std::string(text));
    }
}

TEST_CASE("Control characters are refused at both ends of each range", "[display-text]")
{
    const std::string_view cases[] = {
        "\0"sv,
        "\x1F"sv,
        "\x1B" "[2J"sv,
        "An\ta"sv,
        "\x7F"sv,
        "\xC2\x80"sv,                      // U+0080, the first C1 control
        "\xC2\x9B"sv,                      // U+009B, the single-character CSI
        "\xC2\x9F"sv,                      // U+009F, the last C1 control
    };

    for(const std::string_view text : cases)
    {
        CAPTURE(DisplayText::escaped(text));
        CHECK_FALSE(DisplayText::isPrintable(text));
    }
}

TEST_CASE("Malformed UTF-8 is refused", "[display-text]")
{
    const std::string_view cases[] = {
        "\x9B"sv,                          // a raw 8-bit CSI, as a non-UTF-8 terminal reads it
        "\x9D" "0;title"sv,                // a raw 8-bit OSC
        "\x80"sv,                          // a continuation byte with no lead
        "Ana\xC2"sv,                       // a lead byte at the very end
        "\xE2\x82"sv,                      // a three-byte sequence cut short
        "\xC2" "A"sv,                      // a lead byte followed by ASCII
        "\xC0\x9B"sv,                      // U+001B written overlong in two bytes
        "\xC1\xBF"sv,
        "\xE0\x82\x9B"sv,                  // U+009B written overlong in three bytes
        "\xF0\x80\x82\x9B"sv,              // and in four
        "\xED\xA0\x80"sv,                  // a UTF-16 surrogate
        "\xF4\x90\x80\x80"sv,              // U+110000, past the last code point
        "\xF5\x80\x80\x80"sv,
        "\xFF"sv,
    };

    for(const std::string_view text : cases)
    {
        CAPTURE(DisplayText::escaped(text));
        CHECK_FALSE(DisplayText::isPrintable(text));
    }
}

TEST_CASE("Escaping writes out every byte that would not print", "[display-text]")
{
    CHECK(DisplayText::escaped("a\x1B" "[2Jb"sv) == "a\\x1B[2Jb");
    CHECK(DisplayText::escaped("\0"sv) == "\\x00");
    CHECK(DisplayText::escaped("\xC2\x9B"sv) == "\\xC2\\x9B");
    CHECK(DisplayText::escaped("\x9B" "2J"sv) == "\\x9B2J");
    CHECK(DisplayText::escaped("Ana\xC2"sv) == "Ana\\xC2");
    CHECK(DisplayText::escaped("\xC8\x98" "tefan\x07"sv) == "\xC8\x98" "tefan\\x07");
}

TEST_CASE("Escaped text is always printable", "[display-text]")
{
    // Every single byte, and every pair, so no lead byte or range edge is left out.
    for(int first = 0 ; first < 256 ; first++)
    {
        const std::string one(1, static_cast<char>(first));
        REQUIRE(DisplayText::isPrintable(DisplayText::escaped(one)));

        for(int second = 0 ; second < 256 ; second++)
        {
            const std::string two = one + static_cast<char>(second);
            REQUIRE(DisplayText::isPrintable(DisplayText::escaped(two)));
        }
    }
}
