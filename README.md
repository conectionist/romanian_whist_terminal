# Romanian Whist — Terminal Client

A terminal UI for [Romanian Whist](https://en.wikipedia.org/wiki/Romanian_Whist), built on
top of the [Romanian Whist engine](https://github.com/conectionist/romanian_whist_engine).

All game logic lives in the engine, which is pulled in as a git submodule. This repo
contains only the terminal interface:

| File | Role |
|---|---|
| `src/main.cpp` | Entry point and argument handling |
| `src/AppOptions.{h,cpp}` | Flags, environment and terminal capability detection |
| `src/Terminal.{h,cpp}` | Escape sequences, alternate screen, width measurement |
| `src/CardFormat.{h,cpp}` | Suit glyphs, colours, and the order a hand is displayed in |
| `src/GameView.{h,cpp}` | The snapshot of game state a frame is drawn from |
| `src/Renderer.{h,cpp}` | Composes each frame |
| `src/Pacer.{h,cpp}` | Animation delays and pauses |
| `src/Input.{h,cpp}` | Reading and validating what the player types |
| `src/SetupWizard.{h,cpp}` | Asking what kind of game to play |
| `src/UiContext.h` | Gives the human's move provider access to the screen |
| `src/ConsoleMoveProvider.{h,cpp}` | Draws the human's bidding and card screens and reads their choice |
| `src/TerminalRomanianWhist.{h,cpp}` | The game loop |

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
| `--delay=<ms>` | Time between bot plays (default 700) |
| `--no-animate` | Play at full speed, never pausing |
| `--auto` | Advance without waiting for Enter |
| `--no-color` | Never emit colour sequences |
| `--ascii` | Use letters for suits instead of glyphs |
| `--no-clear` | Scroll output instead of redrawing in place |
| `--no-alt-screen` | Draw on the main screen, overwriting scrollback |
| `--version` | Print the app version and the linked engine version |

While a hand is playing out, pressing <kbd>Enter</kbd> skips ahead to the end of the
round.

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

```bash
cd libs/RomanianWhistEngine
git pull origin master
cd ../..
git add libs/RomanianWhistEngine
git commit -m "Update engine"
```

---

## Development notes

`--demo` sets up a fixed four-bot game, which needs no input and is the quickest way to
see a change. Combined with `--no-animate` a whole game finishes in well under a second.

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
declares its engine member last: members are destroyed in reverse order, so the provider
must die before the context it points at.

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
