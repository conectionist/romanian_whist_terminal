# Romanian Whist — Terminal Client

A terminal UI for [Romanian Whist](https://en.wikipedia.org/wiki/Romanian_Whist), built on
top of the [Romanian Whist engine](https://github.com/conectionist/romanian_whist_engine).

All game logic lives in the engine, which is pulled in as a git submodule — **engine 5.x**,
which owns the game loop. This client does not drive a game; it registers as an
`IGameObserver`, calls `run()`, and draws what it is told. This repo contains only the
terminal interface:

| File | Role |
|---|---|
| `src/main.cpp` | Entry point and argument handling |
| `src/AppOptions.{h,cpp}` | Flags, environment and terminal capability detection |
| `src/Terminal.{h,cpp}` | Escape sequences, alternate screen, width measurement |
| `src/CardFormat.{h,cpp}` | Suit glyphs, colours, and the order a hand is displayed in |
| `src/HandLayout.{h,cpp}` | Fitting a hand across the available width |
| `src/GameView.{h,cpp}` | The snapshot of game state a frame is drawn from |
| `src/Renderer.{h,cpp}` | Composes each frame |
| `src/Pacer.{h,cpp}` | Animation delays and pauses |
| `src/Input.{h,cpp}` | Reading and validating what the player types |
| `src/SetupWizard.{h,cpp}` | Asking what kind of game to play |
| `src/UiContext.h` | Gives the human's move provider access to the screen |
| `src/ConsoleMoveProvider.{h,cpp}` | Draws the human's bidding and card screens and reads their choice |
| `src/TerminalRomanianWhist.{h,cpp}` | The `IGameObserver` the engine reports to — every frame is drawn from a callback |

---

## Requirements

- A C++20 compiler
- CMake 3.21 or newer

## Getting the source

The engine lives in a submodule, so clone recursively:

```bash
git clone --recurse-submodules git@github.com:conectionist/romanian_whist_terminal.git
cd romanian_whist_terminal
```

Already cloned without `--recurse-submodules`? Fetch the engine with:

```bash
git submodule update --init --recursive
```

If you skip this, CMake stops with a clear error telling you to run that command.

## Building

```bash
cmake --preset default
cmake --build build
```

The executable is written to `build/terminal_romanian_whist-<version>` (an unversioned
`build/terminal_romanian_whist` copy is also created for convenience). For an optimised
build use `--preset release` (output goes to `build-release/`).

## Running

```bash
./build/terminal_romanian_whist
```

Run `--help` for the full list of options. In short:

| Option | Effect |
|---|---|
| `--demo` | Skip setup and watch four bots play |
| `--seed=<n>` | Replay the same deal and the same bot choices |
| `--delay=<ms>` | Time between bot plays (default 700) |
| `--no-animate` | Play at full speed, never pausing |
| `--auto` | Advance without waiting for Enter |
| `--no-color` | Never emit colour sequences |
| `--ascii` | Use letters for suits instead of glyphs |
| `--no-clear` | Scroll output instead of redrawing in place |
| `--no-alt-screen` | Draw on the main screen, overwriting scrollback |
| `--version` | Print the app version and the linked engine version |
| `--help`, `-h` | Print the full option list and exit |

While a hand is playing out, pressing <kbd>Enter</kbd> skips ahead to the end of the
round.

### Setting up a game

Without `--demo`, a short wizard asks whether you are playing or watching, how many bots and
how each should play, and three questions about the shape of the game:

- **Which shape of game?** — `1-8-1` counts up from one card and back down; `8-1-8` does the
  reverse.
- **End with forehead and hidden hands?** — appends one Forehead round and one Hidden round to
  the end of the game.
- **Play every one-card round forehead?** — makes every one-card round in the schedule a
  Forehead round, not just the appended one.

A table needs 2 to 6 players, so the bot count on offer depends on your first answer: watching
means at least 2 bots, playing means at least 1.

### Forehead and Hidden rounds

Both are one-card rounds that change what you can see while bidding, and the client draws them
differently:

- **Forehead** — your own card is hidden from you, and every *other* seat's card is shown, by
  name. You bid on what everyone else is holding.
- **Hidden** — nobody's card is shown, including your own. You bid on nothing.

Play itself is unchanged: there is one card in hand, so there is no decision to make. Only the
bidding screen differs, and `ConsoleMoveProvider` prompts blind in both.

Who may see which hand is the engine's rule, not this client's — `GameView` asks
`GameEngine::canSeeHand()` per seat rather than deciding from who is human, so this client and
any other cannot disagree about it.

A spectated game (`--demo`, or a setup with no human seat) shows no hands at all. "A spectator
sees everything" would be a client policy, and this client does not implement one: a spectator
is not a seat, so there is no viewer to ask `canSeeHand()` about.

### Terminal support

Colour and screen control are used only when they will work. Colour is dropped when
output is redirected, when `TERM` is `dumb`, or when `NO_COLOR` is set (its presence is
what counts, not its value). Suit glyphs need a UTF-8 locale, and on Windows a console
that accepts virtual terminal sequences; without either, suits fall back to `H D S C` and
the box drawing to ASCII. `WHIST_DELAY_MS` and `WHIST_ASCII` set `--delay` and `--ascii`
from the environment. Explicit flags win over all of it.

Redirected output also never waits for input, so piping a script of moves in runs at full
speed and writes a clean, escape-free transcript.

## Updating the engine

The engine is released with tags, so pin one rather than tracking the tip:

```bash
cd libs/RomanianWhistEngine
git fetch --tags
git checkout v5.0.0
cd ../..
git add libs/RomanianWhistEngine
git commit -m "Update engine to 5.0.0"
```

`git pull origin master` also works. Check the engine's `CHANGELOG.md` before crossing a major
version — this client requires **engine 5.0.0 or newer** (it asks for the Reckoner presets by
their 5.0.0 names, seats `CyborgStrategy`, and reads `Standing::place` and `getWinners()`), and
4.0.0 broke most of the 3.x API.

What changed in this client, and when, is in [CHANGELOG.md](CHANGELOG.md).

---

## Development notes

`--demo` sets up a fixed four-bot game, which needs no input and is the quickest way to
see a change. Combined with `--no-animate` a whole game finishes in well under a second.

Add `--seed=<n>` and it prints the *same* transcript every run, which is what makes a
rendering change reviewable as a diff:

```bash
./build/terminal_romanian_whist --demo --no-animate --seed=1 > before.txt
# ... make a change, rebuild ...
./build/terminal_romanian_whist --demo --no-animate --seed=1 | diff before.txt -
```

The seed reaches both halves of the randomness: it feeds the engine's deal through
`GameSetup::shuffleSeed`, and, offset per seat, every bot that draws — each
`RandomCardStrategy`, and each Reckoner's deal sampler. Seeding only one of the two would still
wander. The rest, the Cyborg included, draw nothing: given the same deal, they play the same
game.

### Drawing

`GameView` is the only thing the renderer reads. `refreshFromEngine` fills it from the
engine and is the sole place that touches the engine's state accessors, so drawing code
cannot advance the game by accident.

Each frame is built as one string and written in a single call. Redraws home the cursor
and erase forwards rather than clearing the screen, because a full clear flashes visibly
and there is one redraw per card played.

Column widths are measured with `Terminal::visibleWidth`, which skips escape sequences and
counts codepoints rather than bytes. Build a cell as plain text and pad it *before*
painting it — padding a painted string counts the escape bytes as columns.

`IMoveProvider` hands an implementation no route back to the game, so `ConsoleMoveProvider`
is given a `UiContext` when it is constructed. That reference is why `TerminalRomanianWhist`
declares its engine member **last**: the engine owns the players, a player owns the human's
`ConsoleMoveProvider`, and members are destroyed in reverse declaration order — so declaring
the engine last is what makes the provider die before the `UiContext` it points at.

### Input handling

`Input` re-prompts on malformed or out-of-range input. End-of-file is treated as a request
to quit — pressing <kbd>Ctrl</kbd>+<kbd>D</kbd>, or piping input that runs out, ends the
game with a message and a non-zero exit status rather than playing the rest of the hand
automatically. Unwinding rather than exiting on the spot is deliberate: it lets `Terminal`
restore the screen on the way out.

## VSCode

Build with <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd>, or use the **run** task to build
and launch. Debugging uses the "Build and debug terminal_romanian_whist" launch configuration.
IntelliSense reads `build/compile_commands.json`, so configure once before expecting
completions to resolve.
