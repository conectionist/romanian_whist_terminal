# Changelog

The version lives in `CMakeLists.txt` and is printed by `--version`, alongside the version of
the engine this build is linked against.

## 2.1.0

**A drawn game no longer has one winner.** The result screen read the first row of the standings
and announced it, so a game two seats finished level on said one of them had won — and numbered the
other second. Romanian Whist has no tie-breaker, so both of those were wrong.

Requires **engine 4.1.0**, which is what decides this now: `Standing::place` ranks the table with
ties sharing a place, and `GameEngine::getWinners()` returns every seat on the top score. This
client renders that answer rather than working one out for itself.

### Fixed

- **Tied players share a place.** Places come from `Standing::place`, so a table finishing 47, 47,
  30, 30 reads 1, 1, 3, 3 rather than 1, 2, 3, 4. A tie for first consumes second, so the row below
  it is third.
- **Every winner is highlighted**, not just the top row.
- **The closing line names all of them**: "Ana and Bogdan tie for first with 47 points.", or
  "Ana, Bogdan and Dan tie for first…" for three. A single winner still reads "Ana wins with 47
  points."

### Added

- `tests/RendererTests.cpp` — the announcement wording, which is the part of the result screen that
  is pure text and so the part a test can hold still.

## 2.0.0

**The client stopped driving the game.** In 1.x `TerminalRomanianWhist` owned the game loop —
it shuffled, dealt, walked the table collecting bids, resolved each trick, tracked who had won
what, and told the engine when to score. Roughly 190 lines of it, most of them game rules living
in a UI.

Engine 4.0.0 moved that loop into `GameEngine::run()`. This client is now an `IGameObserver`: it
registers, calls `run()`, and draws each frame from a callback. `loop()`,
`playCurrentRoundTricks()`, `markCurrentlyWinning()`, `seatOf()`, the `openingSeat` member and
the shadow `tricksWon` vector are all gone, and nothing in this repo decides a rule any more.

That is an internal change — no flag was removed and no prompt reworded — but it is the whole
program, so it gets the major.

### Added

- **`--seed=<n>`** — replays the same deal and the same bot choices. `--demo --no-animate
  --seed=N` prints an identical transcript every run, which is what makes a rendering change
  reviewable as a `diff`. It feeds the engine's deal and, offset per seat, each bot's
  `RandomCardStrategy`.
- **Forehead and Hidden rounds are drawn.** Your own card is hidden while you bid; in a Forehead
  round every other seat's hand is shown by name, in a Hidden round nothing is. Who may see what
  is asked of `GameEngine::canSeeHand()` per seat rather than decided here, so this client and any
  other cannot disagree about the rule.

### Changed

- **Scores differ from 1.x.** Forehead and Hidden rounds were in the schedule but played as
  ordinary rounds; they are now bid blind. This is the specified rules arriving, not a change of
  behaviour for its own sake.
- Requires **engine 4.x**. The submodule is pinned, so this only matters if you move the pin.

### Removed

- **A spectated game can no longer be set up with a single bot.** The "How many bots?" menu now
  starts at 2 when you are watching rather than playing, so the options are renumbered on that
  one prompt. A one-bot spectated game is a one-player game, which the engine rejects outright
  (2–6 seats), so this removes a setup that had stopped being playable rather than a working one.

## 1.4.1 and earlier

See the git history and the release tags.
