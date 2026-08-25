# Romanian Whist — Terminal Client

A terminal UI for [Romanian Whist](https://en.wikipedia.org/wiki/Romanian_Whist), built on
top of the [Romanian Whist engine](https://github.com/conectionist/RomanianWhist).

All game logic lives in the engine, which is pulled in as a git submodule. This repo
contains only the terminal interface:

| File | Role |
|---|---|
| `src/main.cpp` | Entry point |
| `src/TerminalRomanianWhist.{h,cpp}` | Game loop, scoreboard and results rendering |
| `src/ConsoleMoveProvider.{h,cpp}` | Reads a human player's bets and card choices from stdin |

---

## Requirements

- A C++17 compiler
- CMake 3.14 or newer

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

The executable is written to `build/whist_terminal`. For an optimised build use
`--preset release` (output goes to `build-release/`).

## Running

```bash
./build/whist_terminal
```

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

`TerminalRomanianWhist::startGame` currently calls `initializeTest()`, which sets up a
fixed 4-player AI-only game — useful for testing because it needs no input. Switch the
call to `initialize()` for the interactive setup that prompts for player count, names and
game structure.

### Input handling

`ConsoleMoveProvider` re-prompts on malformed or out-of-range input. End-of-file is
treated as a request to quit — pressing <kbd>Ctrl</kbd>+<kbd>D</kbd>, or piping input
that runs out, ends the game with a message and a non-zero exit status rather than
playing the rest of the hand automatically.

## VSCode

Build with <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd>, or use the **run** task to build
and launch. Debugging uses the "Build and debug whist_terminal" launch configuration.
IntelliSense reads `build/compile_commands.json`, so configure once before expecting
completions to resolve.
