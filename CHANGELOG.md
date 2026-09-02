# Changelog

The version lives in `CMakeLists.txt` and is printed by `--version`, alongside the version of
the engine this build is linked against.

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
