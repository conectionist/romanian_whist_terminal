# Security review

A review of the terminal client and the engine it links, as of 2.5.1 (commit `85b6cfa`, engine
`9ddc379`, October 2026). It answers one question: what risks do you take by running this program?
The fixes it led to ship in 2.5.2.

**Verdict: safe to run.** The game cannot read or write your files, cannot reach the network and
cannot start other programs. Testing found no memory errors. The realistic risk is in how the
release binaries are built rather than in the game. Of the six findings below, four are fixed and
two are left open, with reasons.

## How this was checked

- Read all of `src/` and `tests/`, both CMake files, the CI workflow and the `.vscode` settings. In
  the engine, read the build files and the checks it makes on what a client hands it.
- Listed every function the Linux release binary imports from the C and C++ runtimes.
- Built the client and the engine with AddressSanitizer, UndefinedBehaviorSanitizer and
  `-D_GLIBCXX_ASSERTIONS` (GCC 13) and ran both test suites: the client's 42 tests and the
  engine's 188 all pass.
- Fed that build 400 random input streams through the setup questions and the game. Most lines
  were a single digit, mixed with garbage, out-of-range and overflowing numbers, escape sequences,
  invalid UTF-8, NUL bytes and 5,000-character lines, and about half the streams ended mid-line.
  Half the runs had a UTF-8 locale and half had none.
  306 of them played a whole game and 399 scored at least one round. None produced a sanitizer
  report, a crash or a hang.
- Tried hostile values for each environment variable the program reads (negative, overflowing,
  non-numeric, empty) and for each flag. Each was clamped, ignored or rejected with a message.
- Searched the full history of both repositories for credentials. There are none.
- Had the fixes reviewed twice, each time by a code review that saw neither these notes nor the
  other review. The first found that the name check missed raw 8-bit control bytes and malformed
  UTF-8, and that the Catch2 pin could be bypassed. The second found that a valid UTF-8 letter can
  still carry a byte an 8-bit terminal obeys, that the strict check refused accented names in
  Latin-1 and Latin-2 locales, and that switching the Windows console's input to UTF-8 was
  unreliable. All of these are fixed below.

## What the program can do

The Linux release binary's whole reach outside its own memory is:

| Call | What for |
|---|---|
| `getenv` | `NO_COLOR`, `TERM`, `LC_ALL`, `LC_CTYPE`, `LANG`, `WHIST_ASCII`, `WHIST_DELAY_MS`, `COLUMNS`, `LINES` |
| `isatty`, `ioctl(TIOCGWINSZ)` | Whether it is in a terminal, and how big the terminal is |
| `read` on stdin, through the C++ runtime | Your answers, and the Enter that ends a pause |
| `poll` on stdin | Letting Enter skip a wait |
| `fwrite`, `write` to stdout | Drawing the screen |
| `signal(SIGINT)`, `_exit` | Restoring the screen on Ctrl-C |
| `std::random_device` | Seeding the shuffle |

It opens no files of its own (the C++ runtime may read `/dev/urandom` to seed the shuffle), so it
saves nothing and reads nothing from your disk. It has no network code, so
there is no telemetry and no update check. It needs no administrator rights. On Windows it also
turns on escape-sequence processing for its console and switches the console's output to UTF-8.

The Linux release build is a position-independent executable with full RELRO, a non-executable
stack and stack protectors, which is what Ubuntu's GCC gives by default. The Windows and macOS
binaries were not inspected.

Every number you type is range-checked before it is used, and the engine checks again: it refuses
a table of fewer than 2 or more than 6 seats, an empty or duplicate name, and a card index outside
the hand, and stops with an error rather than reading past the end of anything.

## Findings

### 1. Catch2 was fetched by tag (fixed)

`tests/CMakeLists.txt` fetched Catch2 by its tag, `v3.16.0`, and a tag can be moved to point at
other code. CI builds and runs the tests in the same job that then uploads the release binary, so
code slipped into Catch2 could have changed that binary before it was uploaded.

Catch2 is now pinned to `317ac1ed4c0bb6e6b91eafc817e05c488feffcb3`, the commit `v3.16.0` points
to. Moving to a newer Catch2 now means changing that commit by hand. The pin is declared in the
top-level `CMakeLists.txt`, before the engine is added, because CMake keeps the first declaration
of a name: with `WHIST_BUILD_TESTS=ON`, the engine's declaration would otherwise win.

The engine's own `tests/CMakeLists.txt` still fetches Catch2 by tag. That does not reach this
client's binaries, because the engine's tests are not built when it is a submodule here, but it is
worth the same fix in the engine's repository for its own CI.

### 2. The release action was pinned by tag (fixed)

`softprops/action-gh-release` is a third-party action that runs with a token that can write to the
repository. It was referenced as `@v3`, a tag its owner can move. It is now pinned to
`efb35369e0ad2afab669f228072c1b0d510eae64`, which is v3.0.3 and what `v3` pointed to at the time.

GitHub's own actions (`checkout`, `upload-artifact`, `download-artifact`) stay on major-version
tags. GitHub already runs the workflow, so trusting its actions adds little. Pinned commits do not
update themselves, so Dependabot (`.github/dependabot.yml`) opens a pull request each month that
bumps them along with their version comments. It cannot read CMake, so the Catch2 commit is still
updated by hand.

### 3. CI jobs had the default token permissions (fixed)

With no `permissions` block, the build jobs got whatever the repository's Actions settings give by
default, which on older repositories is read and write. They only need to read. The workflow now
sets `contents: read` for every job, and the release job keeps the `contents: write` it already
asked for. Pull requests from forks were read-only all along.

### 4. Outside text could carry terminal escape sequences (fixed)

A player name was printed exactly as typed, and so was a bad flag in its error message. Text holding
an escape sequence is obeyed by the terminal rather than shown: it can recolour the screen, move the
cursor, change the window title and, in terminals that allow it, write to the clipboard.

The risk was low. Only the person at the keyboard types names, or whoever wrote a file piped into
the game, and names are capped at 10 characters. Only whoever runs the program chooses its flags.

`src/DisplayText.cpp` now checks such text against what the terminal reads, which the locale says:

- **In a UTF-8 locale**, a name must be well-formed UTF-8 with no control character (C0, DEL or
  C1). Malformed UTF-8 is refused as well, because a lenient decoder can read an overlong form as a
  control, and a stray lead byte made the seat table's columns drift.
- **In any other locale**, the terminal reads one byte per character and obeys every byte from 0x80
  to 0x9F as a control, even one inside what would be a UTF-8 letter. Those bytes are refused, and
  the rest of the high half, the Latin-1 and Latin-2 letters, is allowed.

A name that fails is asked for again, and the 10-character limit counts characters rather than
bytes. A bad flag is echoed with each byte that would not print written out as `\xNN`.

With no locale set at all, as is common over SSH, the game assumes a non-UTF-8 terminal, as it
already did for the suit glyphs. A name with a letter such as Ș, whose UTF-8 form contains one of
those bytes, is then refused even if the terminal does read UTF-8. That is the safe side of a guess
the program cannot make reliably.

On Windows the check always takes UTF-8, because the game switches the console's output to it. The
console's input stays on its own code page, so an accented letter typed there arrives as a byte
that is not UTF-8, and the name is refused. In 2.5.1 such a name was accepted and then drawn as the
wrong characters. Reading it properly means converting from the console's code page, which is left
for later because it could not be tried on Windows here.

### 5. Release binaries cannot be verified (open)

Releases come with no checksums, signatures or build provenance, and the macOS and Windows binaries
are unsigned, so Gatekeeper and SmartScreen warn and you have to click through. A downloaded binary
is as trustworthy as the GitHub account that publishes it, and no more.

The cheapest step is GitHub's `actions/attest-build-provenance` in the release job, run on
`artifacts/*` before the release is created. It needs `id-token: write` and `attestations: write`
on that job, and is free for public repositories. Anyone could then check a download with
`gh attestation verify <file> -R conectionist/romanian_whist_terminal`. Signing for macOS and
Windows needs paid certificates. Building from source avoids the question.

This was left open because the release job only runs on master, so the change cannot be tried
before it is merged.

### 6. Opening the folder in VS Code runs CMake (open, by design)

`.vscode/settings.json` sets `cmake.configureOnOpen`, so the CMake Tools extension configures the
project as soon as the folder is opened and trusted. That runs the CMake scripts and downloads
Catch2. VS Code asks whether you trust the folder first, and the scripts do nothing but build. It is
noted here so it does not come as a surprise.

## Not concerns

- **The shuffle is not cryptographic.** It is `std::mt19937` seeded with 32 bits from
  `std::random_device`, and `--seed` replays any game. That is right for a card game and wrong for
  anything with money on it.
- **The Windows console keeps UTF-8 output** and escape-sequence processing after the game exits.
  This is cosmetic.
- **Names are checked for what the terminal obeys, not for how they look.** A name can still hold
  a right-to-left override, characters drawn two columns wide or not at all, or nothing but
  invisible spaces, and two names can look alike. In a non-UTF-8 locale, a Latin-1 letter from À
  up also puts that seat's row a column or more out. Each of these only garbles the screen of
  whoever typed the names, and scores are kept per seat, so look-alike names never share one.
- **One enormous line on stdin is read whole into memory.** Only the person running the program
  can send one.
- **`COLUMNS` and `LINES` are read with `atoi`.** A value out of range is undefined in theory. In
  practice a bad value only changes the layout, and never causes a crash.
- **Only Ctrl-C is caught.** If the game is killed any other way, with `kill` say, the terminal can
  stay on the alternate screen; `reset` fixes it. The terminal is never put into raw mode, so typing
  keeps working.

## Building from source

Cloning fetches this repository and the engine submodule, which is pinned by commit. Configuring
runs the CMake scripts of both and, unless tests are switched off, downloads and builds Catch2 at
the pinned commit. Nothing else is downloaded. To skip Catch2:

```bash
cmake --preset release -DWHIST_TERMINAL_BUILD_TESTS=OFF
```

## Limits of this review

- Everything was run on Linux. The Windows and macOS code paths were read, not run.
- The repository's settings were not visible: branch protection, who can push, and the Actions
  defaults.
- The engine was reviewed for what this client feeds it, not as a whole. Its AI code was exercised
  by the fuzzing and its test suite but not read line by line.
