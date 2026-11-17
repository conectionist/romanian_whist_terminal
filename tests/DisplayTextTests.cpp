#include <catch2/catch_test_macros.hpp>

#include "DisplayText.h"

#include <string>
#include <string_view>

using namespace std::string_view_literals;

using DisplayText::Encoding;

// Every case is written as bytes, so the tests do not depend on the compiler
// reading the source as UTF-8. A literal is split where the next character is a
// hex digit, which would otherwise run on into the \x escape before it.

TEST_CASE("Printable UTF-8, inside and outside ASCII, is printable", "[display-text]")
{
    const std::string_view cases[] = {
        "Ana"sv,
        " ~"sv,                            // the ends of printable ASCII
        "\xC8\x98" "tefan"sv,              // a Romanian letter, two bytes
        "\xC4\x9B"sv,                      // a letter whose second byte is 0x9B
        "\xC2\xA0"sv,                      // U+00A0, just past the C1 controls
        "\xC2\xA9" "Ana"sv,                // U+00A9: lead byte C2, as the C1 controls have
        "\xE2\x99\xA0"sv,                  // a suit glyph, three bytes
        "\xF0\x9F\x82\xA1"sv,              // a playing card, four bytes
        "\xF4\x8F\xBF\xBF"sv,              // U+10FFFF, the last code point
        ""sv,
    };

    for(const std::string_view text : cases)
    {
        CAPTURE(DisplayText::escaped(text, Encoding::Utf8));
        CHECK(DisplayText::isPrintable(text, Encoding::Utf8));

        // Compared as std::string: Catch2 can print a string_view only when it
        // was built as C++17 or later, and its macOS and Windows builds are not.
        CHECK(DisplayText::escaped(text, Encoding::Utf8) == std::string(text));
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
        CAPTURE(DisplayText::escaped(text, Encoding::Utf8));
        CHECK_FALSE(DisplayText::isPrintable(text, Encoding::Utf8));
        CHECK_FALSE(DisplayText::isPrintable(text, Encoding::EightBit));
    }
}

TEST_CASE("Malformed UTF-8 is refused", "[display-text]")
{
    const std::string_view cases[] = {
        "\x9B"sv,                          // a raw 8-bit CSI
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
        "\xAA" "tefan"sv,                  // a Latin-2 letter, which is not UTF-8
    };

    for(const std::string_view text : cases)
    {
        CAPTURE(DisplayText::escaped(text, Encoding::Utf8));
        CHECK_FALSE(DisplayText::isPrintable(text, Encoding::Utf8));
    }
}

TEST_CASE("An 8-bit terminal is protected from every byte from 0x80 to 0x9F", "[display-text]")
{
    // Such a terminal reads each of these bytes as a C1 control, so one inside a
    // well-formed UTF-8 letter is refused just the same.
    const std::string_view refused[] = {
        "\x80"sv,
        "\x9B" "2J"sv,
        "\x9F"sv,
        "\xC4\x9B" "2J"sv,                 // a UTF-8 letter carrying a CSI
        "\xC4\x9D" "0;x" "\xC4\x9C"sv,     // and one carrying an OSC and its ST
    };

    for(const std::string_view text : refused)
    {
        CAPTURE(DisplayText::escaped(text, Encoding::EightBit));
        CHECK_FALSE(DisplayText::isPrintable(text, Encoding::EightBit));
    }

    // The rest of the high half is letters in Latin-1 and Latin-2.
    const std::string_view accepted[] = {
        "\xA0"sv,
        "\xAA" "tefan"sv,                  // a Latin-2 letter
        "\xC8\xA9"sv,                      // È and ©, which also happen to be valid UTF-8
        "\xFF"sv,
    };

    for(const std::string_view text : accepted)
    {
        CAPTURE(DisplayText::escaped(text, Encoding::EightBit));
        CHECK(DisplayText::isPrintable(text, Encoding::EightBit));
    }
}

TEST_CASE("Escaping writes out every byte that would not print", "[display-text]")
{
    CHECK(DisplayText::escaped("a\x1B" "[2Jb"sv, Encoding::Utf8) == "a\\x1B[2Jb");
    CHECK(DisplayText::escaped("\0"sv, Encoding::Utf8) == "\\x00");
    CHECK(DisplayText::escaped("\xC2\x9B"sv, Encoding::Utf8) == "\\xC2\\x9B");
    CHECK(DisplayText::escaped("\x9B" "2J"sv, Encoding::Utf8) == "\\x9B2J");
    CHECK(DisplayText::escaped("Ana\xC2"sv, Encoding::Utf8) == "Ana\\xC2");
    CHECK(DisplayText::escaped("\xC8\x98" "tefan\x07"sv, Encoding::Utf8) == "\xC8\x98" "tefan\\x07");

    CHECK(DisplayText::escaped("\xC4\x9B" "2J"sv, Encoding::EightBit) == "\xC4" "\\x9B2J");
    CHECK(DisplayText::escaped("\xAA" "tefan"sv, Encoding::EightBit) == "\xAA" "tefan");
}

TEST_CASE("Escaped text is always printable", "[display-text]")
{
    // Every single byte, and every pair, so no lead byte or range edge is left out.
    for(const Encoding encoding : { Encoding::Utf8, Encoding::EightBit })
    {
        for(int first = 0 ; first < 256 ; first++)
        {
            const std::string one(1, static_cast<char>(first));
            REQUIRE(DisplayText::isPrintable(DisplayText::escaped(one, encoding), encoding));

            for(int second = 0 ; second < 256 ; second++)
            {
                const std::string two = one + static_cast<char>(second);
                REQUIRE(DisplayText::isPrintable(DisplayText::escaped(two, encoding), encoding));
            }
        }
    }
}

TEST_CASE("Characters are counted, not bytes", "[display-text]")
{
    // Ș, t, e, f, ă, n, i, c, ă: nine characters in twelve bytes of UTF-8.
    const std::string_view name = "\xC8\x98" "tef" "\xC4\x83" "nic" "\xC4\x83"sv;

    CHECK(DisplayText::characterCount(name, Encoding::Utf8) == 9);
    CHECK(DisplayText::characterCount("Ana"sv, Encoding::Utf8) == 3);
    CHECK(DisplayText::characterCount("\xF0\x9F\x82\xA1"sv, Encoding::Utf8) == 1);
    CHECK(DisplayText::characterCount(""sv, Encoding::Utf8) == 0);

    // An 8-bit encoding spends one byte on each character.
    CHECK(DisplayText::characterCount("\xAA" "tefan"sv, Encoding::EightBit) == 6);
}
