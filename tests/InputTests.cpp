#include <catch2/catch_test_macros.hpp>

#include "Input.h"

#include <iostream>
#include <streambuf>
#include <string>

// These tests swap std::cin's buffer for one fed from a string.
//
// The buffer is deliberately unbuffered: it hands out one character at a time
// and never reports how many are waiting, so in_avail() is always 0. That is
// how std::cin behaves on libstdc++ while it is synced with stdio, which is the
// default. A std::stringbuf would report what is waiting and hide bugs that
// only show at a real terminal.
namespace
{
class UnbufferedSource : public std::streambuf
{
public:
    explicit UnbufferedSource(std::string _text)
        : text(std::move(_text))
    {}

protected:
    int_type underflow() override
    {
        return position < text.size() ? traits_type::to_int_type(text[position]) : traits_type::eof();
    }

    int_type uflow() override
    {
        return position < text.size() ? traits_type::to_int_type(text[position++]) : traits_type::eof();
    }

private:
    std::string text;
    std::size_t position = 0;
};

// Points std::cin at the given text for as long as it lives.
class FakeInput
{
public:
    explicit FakeInput(const std::string& text)
        : source(text),
          previous(std::cin.rdbuf(&source))
    {
        std::cin.clear();
    }

    ~FakeInput()
    {
        std::cin.rdbuf(previous);
        std::cin.clear();
    }

    FakeInput(const FakeInput&) = delete;
    FakeInput& operator=(const FakeInput&) = delete;

private:
    UnbufferedSource source;
    std::streambuf* previous;
};

std::string restOfInput()
{
    std::string rest;
    std::string line;
    while(std::getline(std::cin, line))
        rest += line + "\n";
    return rest;
}

const Input::Redraw ignoreErrors = [](const std::string&) {};
} // namespace

TEST_CASE("Waiting for Enter after a choice waits for a new Enter", "[input]")
{
    // The player picks a card, then presses Enter once at the trick's pause.
    // The pause must take that Enter, not the one that ended the choice.
    FakeInput input("2\n\nnext\n");

    REQUIRE(Input::readChoice(1, 3, ignoreErrors) == 2);
    Input::waitForEnter(true);

    CHECK(restOfInput() == "next\n");
}

TEST_CASE("A choice takes its whole line", "[input]")
{
    FakeInput input("2 and some words\nnext\n");

    REQUIRE(Input::readChoice(1, 3, ignoreErrors) == 2);

    CHECK(restOfInput() == "next\n");
}

TEST_CASE("A choice out of range is asked for again on the next line", "[input]")
{
    FakeInput input("9\n3\nnext\n");
    std::string error;

    REQUIRE(Input::readChoice(1, 3, [&error](const std::string& e) { error = e; }) == 3);

    CHECK(error == "Enter a number between 1 and 3.");
    CHECK(restOfInput() == "next\n");
}

TEST_CASE("A line after a choice is read whole", "[input]")
{
    FakeInput input("1\nAlex\n");

    REQUIRE(Input::readChoice(1, 3, ignoreErrors) == 1);

    CHECK(Input::readLine(ignoreErrors) == "Alex");
}

TEST_CASE("Waiting for Enter does nothing when input is not interactive", "[input]")
{
    FakeInput input("\nnext\n");

    Input::waitForEnter(false);

    CHECK(restOfInput() == "\nnext\n");
}

TEST_CASE("A name with an escape sequence in it is asked for again", "[input]")
{
    FakeInput input("\x1b]0;title\x07\nAna\n");
    std::string error;

    CHECK(Input::readName(10, {}, [&error](const std::string& e) { error = e; }) == "Ana");
    CHECK(error == "Names cannot contain control characters.");
}

TEST_CASE("Every kind of control character in a name is refused", "[input]")
{
    // A tab, DEL, and U+009B, the one-character form of the escape that starts
    // a colour or cursor sequence.
    FakeInput input("An\ta\n" "An\x7F" "a\n" "\xC2\x9B" "31m\n" "Ana\n");
    int refusals = 0;

    CHECK(Input::readName(10, {}, [&refusals](const std::string&) { refusals++; }) == "Ana");
    CHECK(refusals == 3);
}

TEST_CASE("Letters outside ASCII are allowed in a name", "[input]")
{
    // Written as bytes, so the test does not depend on the compiler reading the
    // source as UTF-8. The second name starts with C2, the lead byte the C1
    // controls share with ordinary characters such as the copyright sign.
    FakeInput input("\xC8\x98" "tefan\n" "\xC2\xA9" "Ana\n");

    CHECK(Input::readName(10, {}, ignoreErrors) == "\xC8\x98" "tefan");
    CHECK(Input::readName(10, {}, ignoreErrors) == "\xC2\xA9" "Ana");
}

TEST_CASE("A name typed with a Windows line ending is accepted", "[input]")
{
    FakeInput input("Ana\r\n");

    CHECK(Input::readName(10, {}, ignoreErrors) == "Ana");
}
