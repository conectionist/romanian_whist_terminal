#include "Renderer.h"

#include <algorithm>

using romanian_whist::RoundType;

namespace
{

// The seat table's columns. They add up to 60, leaving the rest of an 80-column
// window for the right-hand captions on the section rules.
constexpr unsigned int IndentWidth = 2;
constexpr unsigned int MarkerWidth = 2;
constexpr unsigned int BidOrderWidth = 4;
constexpr unsigned int GapWidth = 2;
constexpr unsigned int NameWidth = 16;
constexpr unsigned int BetWidth = 5;
constexpr unsigned int WonWidth = 6;
constexpr unsigned int RoundWidth = 8;
constexpr unsigned int TotalWidth = 7;

// Below this the full frame does not fit and the layout sheds its optional
// parts rather than scrolling.
constexpr unsigned int MinimumFullHeight = 26;

constexpr unsigned int RuleWidth = 78;

std::string signedScore(int score)
{
    return score > 0 ? "+" + std::to_string(score) : std::to_string(score);
}

} // namespace

Renderer::Renderer(const Terminal& _terminal, const CardFormat& _cardFormat)
    : terminal(_terminal),
      cardFormat(_cardFormat)
{}

bool Renderer::compact() const
{
    return terminal.height() < MinimumFullHeight;
}

std::string Renderer::pad(std::string_view text, unsigned int width, bool rightAlign)
{
    const unsigned int visible = Terminal::visibleWidth(text);

    if(visible >= width)
        return std::string(text);

    const std::string spaces(width - visible, ' ');

    return rightAlign ? spaces + std::string(text) : std::string(text) + spaces;
}

std::string Renderer::horizontalLine(unsigned int length) const
{
    const std::string unit = terminal.unicodeEnabled() ? "─" : "-";

    std::string line;
    line.reserve(unit.size() * length);

    for(unsigned int i = 0 ; i < length ; i++)
        line += unit;

    return line;
}

std::string Renderer::boxCorner(bool top, bool left) const
{
    if(!terminal.unicodeEnabled())
        return "+";

    if(top)
        return left ? "┌" : "┐";

    return left ? "└" : "┘";
}

std::string Renderer::roundTypeLabel(RoundType type) const
{
    switch(type)
    {
        case RoundType::Forehead: return "Forehead";
        case RoundType::Hidden:   return "Hidden";
        case RoundType::Normal:   break;
    }

    return "Normal";
}

void Renderer::appendTrimmed(std::string& out, const std::string& line) const
{
    // Cells are padded to align the next column, which leaves the last one on a
    // row padded against nothing. Harmless on screen, but it shows up as stray
    // whitespace when the output is redirected to a file.
    const std::size_t end = line.find_last_not_of(' ');

    out += end == std::string::npos ? std::string() : line.substr(0, end + 1);
    terminal.endLine(out);
}

void Renderer::appendBlank(std::string& out) const
{
    if(compact())
        return;

    terminal.endLine(out);
}

void Renderer::appendRule(std::string& out,
                          std::string_view caption,
                          std::string_view rightCaption) const
{
    std::string head = "  " + terminal.paint(caption, Terminal::Color::Cyan, true) + " ";

    std::string tail;
    if(!rightCaption.empty())
        tail = " " + terminal.dim(rightCaption) + " " + horizontalLine(3);

    const unsigned int used = Terminal::visibleWidth(head) + Terminal::visibleWidth(tail);
    const unsigned int fill = used < RuleWidth ? RuleWidth - used : 0;

    out += head;
    out += terminal.dim(horizontalLine(fill));
    out += tail;
    terminal.endLine(out);
}

void Renderer::appendHeader(std::string& out, const GameView& view) const
{
    const unsigned int inner = RuleWidth;

    out += boxCorner(true, true) + horizontalLine(inner) + boxCorner(true, false);
    terminal.endLine(out);

    std::string title = terminal.paint("ROMANIAN WHIST", Terminal::Color::Yellow, true);

    std::string details = "Round " + std::to_string(view.roundNumber) +
                          "/" + std::to_string(view.roundCount);

    std::string body = " " + title + "     " + details +
                       "     Joc de " + std::to_string(view.trickCount) +
                       "     " + roundTypeLabel(view.roundType);

    if(view.hasTrump)
        body += "     Trump  " + cardFormat.card(view.trump);
    else
        body += "     No trump";

    const std::string vertical = terminal.unicodeEnabled() ? "│" : "|";

    out += vertical + pad(body, inner) + vertical;
    terminal.endLine(out);

    out += boxCorner(false, true) + horizontalLine(inner) + boxCorner(false, false);
    terminal.endLine(out);
}

void Renderer::appendSeatTable(std::string& out, const GameView& view) const
{
    const bool showStreak = !compact();

    std::string header = std::string(IndentWidth + MarkerWidth, ' ') +
                         pad("Bid#", BidOrderWidth) +
                         std::string(GapWidth, ' ') +
                         pad("Player", NameWidth) +
                         pad("Bid", BetWidth, true) +
                         pad("Won", WonWidth, true) +
                         pad("Round", RoundWidth, true) +
                         pad("Total", TotalWidth, true);

    if(showStreak)
        header += std::string(GapWidth, ' ') + "Streak";

    out += terminal.dim(header);
    terminal.endLine(out);

    for(const SeatView& seat : view.seats)
    {
        const bool active = view.activeSeat >= 0 &&
                            static_cast<unsigned int>(view.activeSeat) == seat.seatIndex;

        std::string name = seat.name;
        if(seat.isHuman)
            name += " (you)";

        // The score a round is worth is only known once it has been scored, so
        // an in-progress round shows a dash rather than a misleading zero.
        const bool roundScored = view.phase == Phase::RoundEnd || view.phase == Phase::GameOver;

        std::string row = std::string(IndentWidth, ' ') +
                          pad(active ? ">" : "", MarkerWidth) +
                          pad(std::to_string(seat.bidOrder), BidOrderWidth) +
                          std::string(GapWidth, ' ') +
                          pad(name, NameWidth) +
                          pad(seat.hasBet ? std::to_string(seat.bet) : "-", BetWidth, true) +
                          pad(std::to_string(seat.tricksWon), WonWidth, true) +
                          pad(roundScored ? signedScore(seat.roundScore) : "-", RoundWidth, true) +
                          pad(std::to_string(seat.totalScore), TotalWidth, true);

        if(showStreak)
        {
            std::string streak;
            if(seat.winStreak > 0)
                streak = "W" + std::to_string(seat.winStreak);
            else if(seat.lossStreak > 0)
                streak = "L" + std::to_string(seat.lossStreak);

            // Last column, so neither padded nor written when empty: trailing
            // spaces would only show up as stray whitespace in redirected output.
            if(!streak.empty())
                row += std::string(GapWidth, ' ') + streak;
        }

        // Colour repeats what the markers already say, so a colourless terminal
        // loses nothing.
        if(active)
            out += terminal.paint(row, Terminal::Color::Green, true);
        else if(seat.isHuman)
            out += terminal.paint(row, Terminal::Color::Default, true);
        else
            out += row;

        terminal.endLine(out);
    }
}

void Renderer::appendTrick(std::string& out, const GameView& view) const
{
    std::string rightCaption;
    if(view.hasLeadSuit)
        rightCaption = "lead " + cardFormat.suitGlyph(view.leadSuit);

    appendRule(out,
               "TRICK " + std::to_string(view.trickNumber) + " of " + std::to_string(view.trickCount),
               rightCaption);
    appendBlank(out);

    // Seats appear in the order they played, so the table reads the way it was
    // dealt out rather than by seat number.
    for(const PlayedCardView& played : view.table)
    {
        const SeatView& seat = view.seats[played.seatIndex];

        std::string row = std::string(IndentWidth + MarkerWidth + 2, ' ') +
                          pad(seat.name, NameWidth) +
                          pad(cardFormat.card(played.card), 6);

        if(played.isWinning)
            row += terminal.paint(view.table.size() == view.seats.size() ? "wins" : "winning",
                                  Terminal::Color::Green, true);

        appendTrimmed(out, row);
    }

    // A placeholder for whoever is on turn keeps the panel from jumping about
    // as cards land in it.
    if(view.activeSeat >= 0 && view.table.size() < view.seats.size())
    {
        const SeatView& seat = view.seats[static_cast<std::size_t>(view.activeSeat)];

        std::string row = std::string(IndentWidth, ' ') +
                          pad(">", MarkerWidth) + "  " +
                          pad(seat.name, NameWidth) +
                          terminal.dim("--");

        out += terminal.paint(row, Terminal::Color::Green, true);
        terminal.endLine(out);
    }
}

void Renderer::appendHand(std::string& out, const GameView& view) const
{
    if(view.hand.empty())
        return;

    std::string rightCaption;
    if(!view.hint.empty())
        rightCaption = view.hint;
    else
        rightCaption = std::to_string(view.hand.size()) +
                       (view.hand.size() == 1 ? " card" : " cards");

    appendRule(out, "YOUR HAND", rightCaption);
    appendBlank(out);

    // One list, one numbering. Printing the hand and the legal plays as two
    // separate lists made the player match up two sets of numbers; here an
    // unplayable card keeps its place in the grid but has no number to type.
    bool anyIllegal = false;
    std::size_t index = 0;

    while(index < view.hand.size())
    {
        const romanian_whist::Suit suit = view.hand[index].card.suit;

        std::string row = std::string(IndentWidth + 2, ' ');
        row += terminal.paint(cardFormat.suitGlyph(suit),
                              CardFormat::isRed(suit) ? Terminal::Color::Red : Terminal::Color::Default,
                              true);
        row += "  ";

        while(index < view.hand.size() && view.hand[index].card.suit == suit)
        {
            const HandEntry& entry = view.hand[index];

            std::string cell;
            if(entry.choice > 0)
                cell = terminal.paint("[" + std::to_string(entry.choice) + "]",
                                      Terminal::Color::Cyan, true) +
                       " " + cardFormat.card(entry.card);
            else if(entry.legal)
                cell = "    " + cardFormat.card(entry.card);
            else
            {
                anyIllegal = true;
                cell = terminal.dim(" .  ") + cardFormat.dimCard(entry.card);
            }

            row += pad(cell, 10);
            index++;
        }

        appendTrimmed(out, row);
    }

    // Colour already says this; the legend is for terminals without it.
    if(anyIllegal && !terminal.colorEnabled())
    {
        out += "  " + terminal.dim(". = cannot be played on this trick");
        terminal.endLine(out);
    }
}

void Renderer::appendFooter(std::string& out, const GameView& view, std::string_view prompt) const
{
    if(!view.message.empty())
    {
        appendBlank(out);
        out += "  " + terminal.paint(view.message, Terminal::Color::Yellow, true);
        terminal.endLine(out);
    }

    if(!view.error.empty())
    {
        appendBlank(out);
        out += "  " + terminal.paint(view.error, Terminal::Color::Red, true);
        terminal.endLine(out);
    }

    if(!prompt.empty())
    {
        appendBlank(out);
        out += "  " + std::string(prompt);
    }
}

void Renderer::drawFrame(const GameView& view, std::string_view prompt) const
{
    std::string frame;
    frame.reserve(4096);

    terminal.beginFrame(frame);

    appendHeader(frame, view);
    appendBlank(frame);

    if(view.phase == Phase::RoundEnd)
        appendRule(frame, "ROUND RESULT");
    else if(view.phase == Phase::Betting)
        appendRule(frame, "BIDDING",
                   "bids placed " + std::to_string(view.betsPlaced) +
                   " of " + std::to_string(view.seats.size()));
    else
        appendRule(frame, "SCORE",
                   "bid " + std::to_string(view.betSum) + " of " +
                   std::to_string(view.trickCount) +
                   (view.trickCount == 1 ? " trick" : " tricks"));

    appendBlank(frame);
    appendSeatTable(frame, view);

    if(view.phase == Phase::Playing || view.phase == Phase::TrickEnd)
    {
        appendBlank(frame);
        appendTrick(frame, view);
    }

    if(!view.hand.empty())
    {
        appendBlank(frame);
        appendHand(frame, view);
    }

    appendFooter(frame, view, prompt);

    terminal.endFrame(frame);
    terminal.write(frame);
}

void Renderer::drawQuestion(std::string_view title,
                            const std::vector<std::string>& options,
                            std::string_view prompt,
                            std::string_view error) const
{
    std::string frame;
    frame.reserve(1024);

    terminal.beginFrame(frame);

    frame += terminal.paint("  ROMANIAN WHIST", Terminal::Color::Yellow, true);
    terminal.endLine(frame);
    appendBlank(frame);

    appendRule(frame, title);
    appendBlank(frame);

    for(std::size_t i = 0 ; i < options.size() ; i++)
    {
        frame += "    " + terminal.paint(std::to_string(i + 1), Terminal::Color::Cyan, true) +
                 ") " + options[i];
        terminal.endLine(frame);
    }

    if(!error.empty())
    {
        appendBlank(frame);
        frame += "  " + terminal.paint(error, Terminal::Color::Red, true);
        terminal.endLine(frame);
    }

    appendBlank(frame);
    frame += "  " + std::string(prompt);

    terminal.endFrame(frame);
    terminal.write(frame);
}

void Renderer::drawGameOver(const std::vector<std::pair<std::string, int>>& standings) const
{
    std::string frame;

    frame += "\n";
    frame += boxCorner(true, true) + horizontalLine(RuleWidth) + boxCorner(true, false) + "\n";

    const std::string vertical = terminal.unicodeEnabled() ? "│" : "|";
    frame += vertical + pad(" " + terminal.paint("FINAL RESULTS", Terminal::Color::Yellow, true),
                            RuleWidth) + vertical + "\n";
    frame += boxCorner(false, true) + horizontalLine(RuleWidth) + boxCorner(false, false) + "\n\n";

    unsigned int place = 1;
    for(const auto& [name, score] : standings)
    {
        const std::string row = "    " + pad(std::to_string(place) + ".", 4) +
                                pad(name, NameWidth) +
                                pad(std::to_string(score), TotalWidth, true) + " points";

        frame += (place == 1 ? terminal.paint(row, Terminal::Color::Yellow, true) : row);
        frame += "\n";
        place++;
    }

    if(!standings.empty())
    {
        frame += "\n    " +
                 terminal.paint(standings[0].first + " wins with " +
                                std::to_string(standings[0].second) + " points.",
                                Terminal::Color::Green, true);
        frame += "\n";
    }

    frame += "\n";
    terminal.write(frame);
}
