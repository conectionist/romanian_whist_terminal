#ifndef RENDERER_H
#define RENDERER_H

#include "CardFormat.h"
#include "GameView.h"
#include "Terminal.h"

#include <romanian_whist/GameEngine.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Turns a GameView into the screen.
//
// Each draw builds the entire frame as one string and writes it in a single
// call, so a redraw never tears. Nothing here reads or touches the game.
class Renderer
{
private:
    const Terminal& terminal;
    const CardFormat& cardFormat;

public:
    Renderer(const Terminal& _terminal, const CardFormat& _cardFormat);

    void drawFrame(const GameView& view, std::string_view prompt = {}) const;

    // A framed question with numbered options, for the setup wizard.
    void drawQuestion(std::string_view title,
                      const std::vector<std::string>& options,
                      std::string_view prompt,
                      std::string_view error = {}) const;

    // Drawn after leaving the alternate screen so the result survives in
    // scrollback once the program exits.
    //
    // `winners` comes from GameEngine::getWinners() rather than being picked
    // out of `standings` here: the engine decides who won a drawn game, and a
    // client that decides it for itself is how this came to announce one winner
    // of a three-way draw in the first place.
    void drawGameOver(const std::vector<romanian_whist::Standing>& standings,
                      const std::vector<romanian_whist::Standing>& winners) const;

    // The closing line: "Ana wins with 47 points." for one winner, "Ana and
    // Bogdan tie for first with 47 points." for a draw. Public and static
    // because it is the one part of the result screen that is pure text with no
    // Terminal behind it, and so the one part a test can hold still.
    static std::string winnerAnnouncement(const std::vector<romanian_whist::Standing>& winners);

private:
    void appendHeader(std::string& out, const GameView& view) const;
    void appendSeatTable(std::string& out, const GameView& view) const;
    void appendTrick(std::string& out, const GameView& view) const;
    void appendHand(std::string& out, const GameView& view) const;
    void appendOtherHands(std::string& out, const GameView& view) const;

    // Shared body of appendHand/appendOtherHands: one hand, laid out and
    // captioned. `hand`'s own entries say whether a card is numbered or
    // greyed out, so this needs nothing more to draw either the human's own
    // turn or another seat's read-only Forehead-round hand.
    void appendHandBlock(std::string& out,
                         const std::vector<HandEntry>& hand,
                         std::string_view caption,
                         std::string_view rightCaption) const;
    void appendFooter(std::string& out, const GameView& view, std::string_view prompt) const;

    void appendRule(std::string& out,
                    std::string_view caption,
                    std::string_view rightCaption = {}) const;
    void appendBlank(std::string& out) const;
    void appendTrimmed(std::string& out, const std::string& line) const;

    // Cells are built as plain text and padded before any colour is applied:
    // padding a painted string counts the escape bytes as columns and the
    // whole table shifts.
    static std::string pad(std::string_view text, unsigned int width, bool rightAlign = false);

    // Same rule as pad(): call on plain text, colour the result afterward.
    static std::string padCenter(std::string_view text, unsigned int width);

    // Drops the streak column and the spacer lines when the window is too short
    // for the full layout.
    bool compact() const;

    std::string roundTypeLabel(romanian_whist::RoundType type) const;
    std::string boxCorner(bool top, bool left) const;

    // The header table's row divider meets the box sides at a T, not a corner.
    std::string boxJunction(bool left) const;
    std::string horizontalLine(unsigned int length) const;
};

#endif
