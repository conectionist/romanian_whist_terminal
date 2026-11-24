#include <catch2/catch_test_macros.hpp>

#include "TerminalRomanianWhist.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

TEST_CASE("A game without a seed seeds no bot", "[bot-seed]")
{
    CHECK_FALSE(botSeed(std::nullopt, 0).has_value());
    CHECK_FALSE(botSeed(std::nullopt, 3).has_value());
}

TEST_CASE("No bot is seeded with the seed that shuffles the deck", "[bot-seed]")
{
    constexpr std::uint32_t Largest = std::numeric_limits<std::uint32_t>::max();

    for(const std::uint32_t gameSeed : { 0u, 1u, 42u, Largest - 1, Largest })
    {
        std::set<std::uint32_t> seen = { gameSeed };

        // Six, the most players a game can have: a watched game has no human,
        // so every seat can be a bot.
        for(std::size_t bot = 0 ; bot < 6 ; bot++)
        {
            INFO("game seed " << gameSeed << ", bot " << bot);

            const std::optional<std::uint32_t> seed = botSeed(gameSeed, bot);
            REQUIRE(seed.has_value());

            // Inserting fails on a repeat, of the shuffle's seed or another bot's.
            CHECK(seen.insert(*seed).second);
        }
    }
}
