# CCH / CMS1 preservation engine

An unofficial, portable C rewrite of the Xiangqi engine from *Chinese Chess
Master III: Jiangzu* (象棋大師 III／將族), associated with Yu Hsi-Shun / 虞希舜
and T-Time / 光譜資訊. The goal is to preserve this particular engine's behavior,
not compete with the strongest modern engines.

No original executable, opening book, artwork, music,
manual, memory image, or disassembler database is distributed here.

Recovered numeric evaluation tables **are** included. This was a
disassembly-informed rewrite, not a separated clean-room implementation.
Read [NOTICE.md](NOTICE.md) for attribution and license scope.

## Build

Requires a C11 compiler, CMake 3.16+, and POSIX threads. macOS and Linux are
supported by the build workflow; native Windows support is not claimed.

```sh
git clone https://github.com/xusheng6/cch-cms1.git
cd cch-cms1
cmake -S port -B port/build -DCMAKE_BUILD_TYPE=Release
cmake --build port/build --parallel
(cd port/build && ctest --output-on-failure)
```

No original game files or Python dependencies are needed to build or play.
Select the absolute path to `port/build/cch` as a custom engine in a Xiangqi GUI.
It supports both UCI and UCCI, including positions, game clocks, increments,
fixed-depth search, and analysis PV output.

For a terminal smoke test, start `port/build/cch` and enter:

```text
uci
isready
position startpos
go depth 3
```

Enter `quit` to exit. The `Profile` option exposes 36 recovered opponents;
`Unlimited` leaves search control to the GUI. Explicit depth/clock arguments
override profile defaults. The command `profiles` describes the recovered
settings. Profile clocks refer to game time, not time per move.

The separate `port/build/cch-modern` executable is an experimental variant with
a larger hashed cache. It is **not** the fidelity target. The project provides
an engine, not a recreation of the original DOS graphical game.

## Validation

The preservation build matched the original's best move and reported score
on 408,708 research cases, predominantly depths 2–4, with a smaller depth-5–7
sample. This is not proof of universal equivalence or identical search trees.
See [the validation summary](VALIDATION.md).

This repository includes the C regression suite and a **100-case smoke
subset**, not the entire research corpus. With Python 3.10+:

```sh
python3 tools/test_cms1_oracle.py
python3 tools/compare_cms1_oracle.py port/tests/oracles/audit-100.jsonl port/build/cch --jobs 4
```

These tests need no emulator, network connection, opening book, or third-party
Python packages. The optional original-book regression can be enabled with
`-DCCH_ORIGINAL_BOOK=/path/to/OPENING.LIB` during CMake configuration; that file
must be separately obtained.

## License

MIT terms apply to the contributors' licensable work, subject to the
third-party provenance described in [NOTICE.md](NOTICE.md).
