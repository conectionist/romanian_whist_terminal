# Code quality review

A review of the terminal client as of 2.5.1 (commit `d77cc1b`, October 2026): what is good, what
gets in the way, and what to change first. The engine submodule is reviewed in its own repository.

**Verdict: 6 out of 10.** On a scale where 1 is AI slop and 10 is a codebase others should copy.
The code itself is close to a 7: small, well organised, careful with the terminal, and lightly
commented. What pulls it down is the tests. There are few of them, none cover the game flow or the
screen, and reading the code turned up a bug that a test would have caught.

## How this was measured

- Read `src/` (about 3,400 lines), `tests/` (about 760 lines), both CMake files and the CI workflow.
- Built the `default` preset with GCC 13 and `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`:
  no warnings anywhere.
- Ran the tests: all 28 passed.
- Ran `--demo --no-animate --seed=42` twice with output piped to a file: both runs printed the same
  19,040 lines, and `--seed=43` printed a different game.
- Counted lines of code against lines of comment and a few patterns (sentinel values, `default:`
  in switches, history in comments).

## Strengths

### The structure is clear

- **The screen is drawn from a snapshot.** `GameView` is plain data, `refreshFromEngine()` is the one
  place that maps engine state onto it, and `Renderer` reads only the view, never the engine. Drawing
  code cannot move the game on by accident.
- **One class owns the terminal.** All escape sequences live in `Terminal`, so `--no-color` and
  piped output are honoured in one place rather than by every caller.
- **The game's flow is the engine's.** `TerminalRomanianWhist` is an `IGameObserver`, and each
  callback draws a frame and paces the game. `GameEngine` is declared last, so the human's move
  provider, which refers to the UI, is destroyed before the UI it points at.

### It is careful with the terminal

- The alternate screen is restored by a destructor, so an exception still leaves a usable shell.
- Ctrl-C runs a handler that uses only async-signal-safe calls (`write`, `_exit`) to restore the
  screen before exiting (`Terminal.cpp`).
- A frame is built as one string and written in one call, so it does not tear. The cursor is moved
  home rather than the screen cleared, so it does not flicker.
- On Windows it turns on escape-sequence processing and UTF-8, and falls back to plain output if
  either fails.

### Input recovers from mistakes

`Input.cpp` asks again after malformed input, stops cleanly when input is closed (the exception
unwinds to `main()`, restoring the screen on the way), and always reads the whole line, so a stray
Enter cannot answer the next question.

### The options are well thought out

`AppOptions` starts from what a capable terminal supports and steps down for a pipe, `NO_COLOR`,
`TERM=dumb` and a non-UTF-8 locale, then applies the flags, which always win. No other code decides
for itself whether to use colour, suit glyphs or screen control. `--seed` seeds the deal and every
bot that draws at random, so a game can be replayed exactly.

### The comments are about right

About 2 comment lines per 10 lines of code, and they explain reasons rather than restating code.
Project history comes up only 3 times.

## Weaknesses

### 1. The tests are thin

| Area | Tests |
|---|---|
| Command-line options (`AppOptions`) | 9 |
| Hand layout | 6 |
| Input | 5 |
| Card formatting | 4 |
| Winner announcement (`Renderer`) | 4 |
| Game flow, `ConsoleMoveProvider`, `refreshFromEngine()`, the frame, `Pacer`, `SetupWizard` | 0 |

28 test cases in about 760 lines, for about 3,400 lines of source. `Renderer.cpp` is 614 lines, and
only its winner text is tested. The engine has 188 tests and the desktop client about 705.

### 2. A hint is wrong when you must play a trump

`ConsoleMoveProvider.cpp:116`:

```cpp
if(leadSuit && choices.size() < hand.size())
    ui.view.hint = "you must follow " + ui.cardFormat.suitGlyph(*leadSuit);
```

Fewer legal cards than cards in hand happens in two cases: you hold the suit led, or you do not hold
it and must play a trump. In the second case the hint still says "you must follow ♥", which is
exactly what you cannot do. It should say that you must play a trump.

### 3. Some comments are out of date

`Input.h:38` and `Input.cpp:122` say duplicate names are rejected because the engine stores bets and
results by name. The engine has stored them by seat since 4.0. Rejecting duplicate names is still
right (two players with one name would be confusing to read), but the reason given is not.

A few comments also describe the past: "the bid used to be asked for blind"
(`ConsoleMoveProvider.cpp:48`), "Everything that used to be loop() and playCurrentRoundTricks()"
(`TerminalRomanianWhist.cpp:158`).

### 4. Warnings are not switched on

Neither `CMakeLists.txt` nor CI turns on any warnings for the client's own code. The build is clean
today with every warning on, but only by care: nothing would stop a new warning reaching `master`.
The engine has `WHIST_ENABLE_WARNINGS` and `WHIST_WARNINGS_AS_ERRORS` for this.

### 5. Older C++ habits

- **`-1` means "no seat".** `GameView::humanSeat` and `activeSeat` are `int`s set to `-1`
  (`GameView.h:96`), checked with `>= 0` and cast to `unsigned int` around the code: 13 places.
  `std::optional<Seat>` would say it outright.
- **`default:` in switches over enums** (`TerminalRomanianWhist.cpp:35`, `:49`, `:56`, `:79`). A new
  `AiLevel` or `ReckonerPreset` silently takes the default branch instead of being named by
  `-Wswitch`. The engine avoids `default:` for exactly this reason.
- **Results through output parameters.** `AppOptions::fromCommandLine()` returns a `bool` and fills
  three output parameters (`helpRequested`, `versionRequested`, `errorOut`).
- **No `std::ranges`** and no `[[nodiscard]]`.

### 6. Code copied between the clients

The card display order (`CardFormat::inDisplayOrder()`, `suitPosition()`) and the hand layout
(`HandLayout`) exist here and, again, in the desktop client, whose `src/core/HandLayout.h` says it
was ported from this one. A change to the display order in one client will not reach the other.

### 7. Smaller points

- Includes in `TerminalRomanianWhist.cpp` are unordered, with engine and project headers mixed and a
  double blank line.
- `ConsoleMoveProvider` has a private `layOutHand()` that hides the free function of the same name,
  so the code calls `::layOutHand()` to reach it.
- `virtual` is repeated on functions marked `override` (`ConsoleMoveProvider.h`).
- `.vscode/` is committed, and there is no `.clang-format`.

## Suggestions, most important first

### 1. Record a seeded game as a golden test

The demo is already deterministic: `--demo --no-animate --seed=42` with output piped prints the same
19,040 lines every time. Commit that output as `tests/golden/demo-seed-42.txt` and add a CTest test
that runs the client the same way and compares the two. One test then covers the observer callbacks,
`refreshFromEngine()`, the renderer and the pacer.

An engine update that changes how the bots play will change the file. That is the point: the diff
shows what changed, and the file is regenerated on purpose, in the same PR as the submodule bump.

Then add unit tests where the logic is:

- `ConsoleMoveProvider`'s hints and prompts, with a fake input stream, including the trump case
  above.
- `refreshFromEngine()` against a seeded engine at known points of a game.
- `Renderer::drawFrame()` on a hand-built `GameView`, for the compact layout and for each phase.

### 2. Fix the trump hint and the stale comments

Tell the two cases apart by whether the player holds the suit led:

```cpp
const bool holdsLeadSuit = leadSuit &&
    std::ranges::any_of(hand, [&](const Card& card) { return card.suit == *leadSuit; });

if(holdsLeadSuit && choices.size() < hand.size())
    ui.view.hint = "you must follow " + ui.cardFormat.suitGlyph(*leadSuit);
else if(leadSuit && choices.size() < hand.size())
    ui.view.hint = "you must play a trump";
else if(!leadSuit)
    ui.view.hint = "you lead";
else
    ui.view.hint.clear();
```

Then correct the reason given in `Input.h` and `Input.cpp`, and remove the comments about what the
code used to do.

### 3. Turn warnings on, and make CI fail on them

Copy the engine's two options: `WHIST_TERMINAL_ENABLE_WARNINGS` (on when top-level) adds
`-Wall -Wextra -Wpedantic -Wshadow` or `/W4` to `terminal_core`, the executable and the tests, and
`WHIST_TERMINAL_WARNINGS_AS_ERRORS` adds `-Werror` or `/WX`. CI configures with the second one on.

### 4. Replace `-1` with `std::optional`, and drop `default:` from enum switches

```cpp
std::optional<romanian_whist::Seat> humanSeat;
std::optional<romanian_whist::Seat> activeSeat;

bool isHumanSeat(romanian_whist::Seat seat) const
{
    return view.humanSeat == seat;
}
```

`Seat` already has `operator==`, and comparing an `optional` with a value is false when it is
empty, so the `>= 0` checks and the casts go. In `getReckonerKnobs()` and `isReckoner()`, list every
enumerator and remove `default:`, so the compiler names any level a future change forgets.

### 5. Share the display order with the desktop client

The display order and the hand layout are the same rules in both clients. Move them into the
engine, for example as `romanian_whist/display/HandLayout.h`, so the clients cannot drift apart.
They are presentation rather than rules, but they depend only on engine types, and both clients
already depend on the engine.

### 6. Smaller tidying

- Return a result from `AppOptions::fromCommandLine()` instead of filling output parameters.
- Order the includes, rename the private `layOutHand()` (for example `showHand()`), and drop the
  repeated `virtual`.
- Add a `.clang-format` for the style the code already uses, check it in CI, and remove `.vscode/`.
- Add a `CLAUDE.md` with the same comment rule as the engine and the desktop client.

## What would make it an 8

Suggestions 1 to 3. The golden test alone covers most of what is untested today, and turning
warnings on keeps the build as clean as it is now.
