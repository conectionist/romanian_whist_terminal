# Changelog

The version lives in `CMakeLists.txt` and is printed by `--version`, alongside the version of
the engine this build is linked against.

## 2.4.0

**A new bot: the Cyborg.** Engine 4.3.0 added `CyborgStrategy`, a rule-based player that remembers
the whole round — every card played, every bid, every void a seat has shown — and plays to hit its
bid. It is now on the setup menu, in the quick game and in the demo. By the engine's tournament
tests it sits between the two strategies it was built to be measured against: at four players it
scores about 14 points a game more than LowRisk and about 15 fewer than the Reckoner's Medium
preset.

Requires **engine 4.3.0**, the first release with the Cyborg.

There is no new flag. The Cyborg has one strength and draws no random numbers, so there is no
preset to pick and nothing for `--seed` to hand it — a seeded game replays it exactly as it is.
Its `CyborgKnobs` stay unexposed: the engine documents them as development tuning, not a
difficulty dial.

### Added

- **"Cyborg" on the "How should … play?" menu**, as option 5, between "Ducking tricks" and the
  Reckoner presets, which move down one to 6–9.

### Changed

- **The quick game seats the Reckoner, the Cyborg and LowRisk.** It was the Reckoner, LowRisk and
  Ducking.
- **`--demo` seats the Cyborg in Ducking's place**: Reckoner Hard, LowRisk, Cyborg and Reckoner
  Medium, which puts the Cyborg beside the two strategies the engine measures it against. A
  `--demo --seed=N` transcript saved before this release will not match one made after it.
- **Ducking is in neither default line-up now.** It is still on the menu.
- **A bot level the client has no seat for now stops the game at setup**, rather than quietly
  seating a random-card bot under its name. The menu's labels and its levels are also checked
  against each other at compile time, so the two cannot drift apart.

## 2.3.0

**The Reckoner joins the table.** Engine 4.2.0's `ReckonerStrategy` is a bot that remembers the
round and, from its Medium preset up, samples the hands it cannot see to price each card. It comes
in four presets — Easy, Medium, Hard and Brutal — and the setup menu offers all four.

Needs the engine's Reckoner, which was released in **engine 4.2.0**. The 2.3.0 release itself was
built against the untagged engine commit that merged it; the pin moved to the 4.2.0 tag
afterwards, with no change to this client.

### Added

- **Four Reckoner entries on the "How should … play?" menu** — Reckoner (Easy), (Medium), (Hard)
  and (Brutal) — after "Ducking tricks".
- **`--reckoner-preset=<easy|medium|hard|brutal>`** picks the preset for the quick game's
  Reckoner, which has no menu to ask. The default is `hard`. A seat chosen from the menu keeps the
  preset it was given there.
- **`--seed` reaches the Reckoner too.** Its deal sampler draws random numbers, so a seeded game
  seeds each Reckoner as well, offset per seat as `RandomCardStrategy` is.

### Changed

- **The quick game seats the Reckoner, LowRisk and Ducking.** It was LowRisk, Ducking and
  FirstCard.
- **`--demo` seats Reckoner Hard, LowRisk, Ducking and Reckoner Medium.** It was LowRisk,
  Ducking, FirstCard and Random.

## 2.2.0

**The header is a table, and it keeps time.** The one-line header above the seat table is now two
centred rows, captions over values, for the round, the cards dealt, the game type, the trump and
the time elapsed. The section rules below it are narrowed to the seat table's width rather than
stretching to 78 columns.

Requires **engine 4.1.0**, as 2.1.0 did.

### Added

- **Elapsed time**, in whole minutes since the game started. It is wall-clock time kept by this
  client, since the engine has no clock of its own — a pause over a bid counts, because the player
  sat through it.

### Changed

- **The header is redesigned** as a two-row table, as above.

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
