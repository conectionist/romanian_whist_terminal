#include <catch2/catch_test_macros.hpp>

#include "Renderer.h"

#include <romanian_whist/GameEngine.h>
#include <romanian_whist/Seat.h>

#include <string>
#include <vector>

using romanian_whist::Seat;
using romanian_whist::Standing;

// Renderer::winnerAnnouncement() writes the result screen's last line. With no
// tie-breaker, up to six seats can share first place, and the line has to name
// every one of them.

namespace
{
// The renderer reads only the name and score, but every field is set so each
// winner looks like one the engine could return.
Standing standing(unsigned int seat, const std::string& name, int score, unsigned int place)
{
    return Standing{ Seat{seat}, name, score, place };
}
} // namespace

TEST_CASE("one winner is announced as a win", "[renderer]")
{
    const std::vector<Standing> winners{ standing(2, "Cristi", 47, 1) };

    REQUIRE(Renderer::winnerAnnouncement(winners) == "Cristi wins with 47 points.");
}

TEST_CASE("a draw names every winner", "[renderer]")
{
    SECTION("two seats are joined with and, not a comma")
    {
        const std::vector<Standing> winners{ standing(0, "Ana", 47, 1),
                                             standing(1, "Bogdan", 47, 1) };

        REQUIRE(Renderer::winnerAnnouncement(winners) ==
                "Ana and Bogdan tie for first with 47 points.");
    }

    SECTION("three seats take commas until the last, which takes the and")
    {
        const std::vector<Standing> winners{ standing(0, "Ana", 47, 1),
                                             standing(1, "Bogdan", 47, 1),
                                             standing(3, "Dan", 47, 1) };

        REQUIRE(Renderer::winnerAnnouncement(winners) ==
                "Ana, Bogdan and Dan tie for first with 47 points.");
    }

    SECTION("a table that draws outright is still one sentence")
    {
        // Every seat level - the state the game starts in, and one a short game
        // can end in. Nothing here special-cases it, which is the point of
        // checking: the loop that joins the names has to hold at the top end
        // too.
        const std::vector<Standing> winners{ standing(0, "Ana", 0, 1),
                                             standing(1, "Bogdan", 0, 1),
                                             standing(2, "Cristi", 0, 1),
                                             standing(3, "Dan", 0, 1),
                                             standing(4, "Elena", 0, 1),
                                             standing(5, "Florin", 0, 1) };

        REQUIRE(Renderer::winnerAnnouncement(winners) ==
                "Ana, Bogdan, Cristi, Dan, Elena and Florin tie for first with 0 points.");
    }
}

TEST_CASE("a negative winning score keeps its sign", "[renderer]")
{
    // A whole table can finish below zero - missed bids score negative and
    // nothing floors the total - so the winner's score is not always positive.
    const std::vector<Standing> winners{ standing(1, "Bogdan", -12, 1) };

    REQUIRE(Renderer::winnerAnnouncement(winners) == "Bogdan wins with -12 points.");
}

TEST_CASE("no winners is no announcement", "[renderer]")
{
    // getWinners() is never empty for a started game, so this is the defensive
    // branch rather than a state the app reaches. It earns its place by saying
    // what happens instead of leaving a bare "wins with" line to be discovered.
    REQUIRE(Renderer::winnerAnnouncement({}).empty());
}
