#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "CardStringMaker.h"

#include "GameView.h"
#include "PlainTerminal.h"
#include "Renderer.h"

#include <romanian_whist/Card.h>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;
using romanian_whist::Card;
using romanian_whist::Rank;
using romanian_whist::Suit;

// Frames drawn from hand-built views. Each test checks the lines that carry its
// point rather than the whole frame, so a change to spacing elsewhere does not
// break them all.
namespace
{
constexpr unsigned int FullHeight = 30;
constexpr unsigned int CompactHeight = 24;

GameView fourSeats(Phase phase)
{
    GameView view;
    view.phase = phase;
    view.roundNumber = 5;
    view.roundCount = 26;
    view.trickCount = 3;
    view.hasTrump = true;
    view.trump = Card(Rank::Jack, Suit::Spades);

    const char* names[] = { "Ana", "Bogdan", "Cristi", "Dan" };

    for(unsigned int i = 0 ; i < 4 ; i++)
    {
        SeatView seat;
        seat.name = names[i];
        seat.seatIndex = i;
        seat.bidOrder = i + 1;
        seat.cardsLeft = 3;
        view.seats.push_back(seat);
    }

    return view;
}

std::string draw(const GameView& view, unsigned int height, std::string_view prompt = {})
{
    const PlainTerminal plain;
    const Renderer renderer(plain.terminal, plain.cardFormat, height);

    return renderer.frame(view, prompt);
}

std::vector<std::string> lines(const std::string& frame)
{
    std::vector<std::string> result;
    std::istringstream stream(frame);

    for(std::string line ; std::getline(stream, line) ; )
        result.push_back(line);

    return result;
}

// The first line containing `text`, or an empty string.
std::string lineWith(const std::string& frame, const std::string& text)
{
    for(const std::string& line : lines(frame))
    {
        if(line.find(text) != std::string::npos)
            return line;
    }

    return {};
}

// The first line after the one containing `section` that contains `text`, or
// an empty string. Names appear in both the seat table and the trick, so a
// search has to start from the right section.
std::string lineAfter(const std::string& frame, const std::string& section, const std::string& text)
{
    bool inSection = false;

    for(const std::string& line : lines(frame))
    {
        if(!inSection)
            inSection = line.find(section) != std::string::npos;
        else if(line.find(text) != std::string::npos)
            return line;
    }

    return {};
}
} // namespace

TEST_CASE("the bidding frame counts the bids and marks who is bidding", "[frame]")
{
    GameView view = fourSeats(Phase::Betting);
    view.seats[0].hasBet = true;
    view.seats[0].bet = 2;
    view.betsPlaced = 1;
    view.betSum = 2;
    view.activeSeat = romanian_whist::Seat{1};

    const std::string frame = draw(view, FullHeight);

    REQUIRE_THAT(frame, ContainsSubstring("BIDDING"));
    REQUIRE_THAT(frame, ContainsSubstring("bids placed 1 of 4"));
    REQUIRE_THAT(lineAfter(frame, "BIDDING", "Bogdan"), StartsWith("  > "));
    REQUIRE_THAT(lineAfter(frame, "BIDDING", "Ana"), StartsWith("    "));
}

TEST_CASE("the playing frame shows the trick and numbers only the legal cards", "[frame]")
{
    GameView view = fourSeats(Phase::Playing);
    view.trickNumber = 2;
    view.leadSeat = 1;
    view.hasLeadSuit = true;
    view.leadSuit = Suit::Hearts;
    view.table = { { 1, Card(Rank::King, Suit::Hearts), true },
                   { 2, Card(Rank::Three, Suit::Hearts), false } };
    view.activeSeat = romanian_whist::Seat{3};

    view.humanSeat = romanian_whist::Seat{0};
    view.seats[0].isHuman = true;
    view.hand = { { Card(Rank::Ace, Suit::Hearts), true, 1 },
                  { Card(Rank::Five, Suit::Hearts), true, 2 },
                  { Card(Rank::Nine, Suit::Clubs), false, 0 } };
    view.legalCount = 2;
    view.hint = "you must follow H";

    const std::string frame = draw(view, FullHeight, "Which card? (1-2): ");

    SECTION("the trick, in the order it is played")
    {
        REQUIRE_THAT(lineWith(frame, "TRICK 2 of 3"), ContainsSubstring("lead H"));

        const std::string leader = lineAfter(frame, "TRICK 2 of 3", "Bogdan");
        REQUIRE_THAT(leader, ContainsSubstring("KH"));
        REQUIRE_THAT(leader, ContainsSubstring("winning"));

        REQUIRE_THAT(lineAfter(frame, "TRICK 2 of 3", "Cristi"), ContainsSubstring("3H"));

        const std::string waiting = lineAfter(frame, "TRICK 2 of 3", "Dan");
        REQUIRE_THAT(waiting, StartsWith("  > "));
        REQUIRE_THAT(waiting, ContainsSubstring("--"));
    }

    SECTION("the hand, with the hint as its caption")
    {
        REQUIRE_THAT(lineAfter(frame, "TRICK 2 of 3", "YOUR HAND"), ContainsSubstring("you must follow H"));
        REQUIRE_THAT(lineAfter(frame, "YOUR HAND", "AH"), ContainsSubstring("[1] AH"));
        REQUIRE_THAT(lineAfter(frame, "YOUR HAND", "AH"), ContainsSubstring("[2] 5H"));

        const std::string clubs = lineAfter(frame, "YOUR HAND", "9C");
        REQUIRE_THAT(clubs, ContainsSubstring(" .  9C"));
        REQUIRE_THAT(clubs, !ContainsSubstring("["));

        // Without colour, greyed-out cards need saying in words.
        REQUIRE_THAT(frame, ContainsSubstring(". = cannot be played on this trick"));
    }

    SECTION("the prompt comes last")
    {
        REQUIRE_THAT(lines(frame).back(), ContainsSubstring("Which card? (1-2): "));
    }
}

TEST_CASE("the trick-end frame names the winner", "[frame]")
{
    GameView view = fourSeats(Phase::TrickEnd);
    view.trickNumber = 1;
    view.leadSeat = 0;
    view.table = { { 0, Card(Rank::Two, Suit::Hearts), false },
                   { 1, Card(Rank::King, Suit::Hearts), true },
                   { 2, Card(Rank::Three, Suit::Hearts), false },
                   { 3, Card(Rank::Four, Suit::Hearts), false } };
    view.message = "Bogdan wins the trick.";

    const std::string frame = draw(view, FullHeight, "Press Enter for the next trick");

    REQUIRE_THAT(lineAfter(frame, "TRICK 1 of 3", "Bogdan"), ContainsSubstring("wins"));
    REQUIRE_THAT(lineAfter(frame, "TRICK 1 of 3", "Bogdan"), !ContainsSubstring("winning"));
    REQUIRE_THAT(frame, ContainsSubstring("Bogdan wins the trick."));
}

TEST_CASE("the round-end frame shows the result and no trick", "[frame]")
{
    GameView view = fourSeats(Phase::RoundEnd);
    view.seats[1].roundScore = 7;
    view.seats[1].totalScore = 31;

    const std::string frame = draw(view, FullHeight, "Press Enter to deal the next round");

    REQUIRE_THAT(frame, ContainsSubstring("ROUND RESULT"));
    REQUIRE_THAT(frame, !ContainsSubstring("TRICK"));
    REQUIRE_THAT(lineAfter(frame, "ROUND RESULT", "Bogdan"), ContainsSubstring("31"));
}

TEST_CASE("a short window drops the streak column and the spacing", "[frame]")
{
    const GameView view = fourSeats(Phase::Betting);

    const std::string full = draw(view, FullHeight);
    const std::string compact = draw(view, CompactHeight);

    REQUIRE_THAT(full, ContainsSubstring("Streak"));
    REQUIRE_THAT(compact, !ContainsSubstring("Streak"));
    REQUIRE(lines(compact).size() < lines(full).size());
}
