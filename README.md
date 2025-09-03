# Hoover Chess Utilities

Hoover chess utilities is a small set of command-line tools for
building the [TCEC games database](https://github.com/TCEC-Chess/tcecgames/) and providing
database query functionality to TCEC_hoover_bot in [TCEC Twitch chat](https://www.twitch.tv/tcec_chess_tv).

## Documentation

Documentation of Hoover Chess Utilities is available at:
https://skiminki.github.io/hoover-chess-utils/hoover-chess-utils/index.html (work in progress).

More detailed documentation of the PGN reader lib is available at:
https://skiminki.github.io/hoover-chess-utils/pgn-reader/index.html (work in progress).


## Building

### Prerequisites

**General**
- Development tool chain for GCC or CLANG with C++23 support. CLANG-19 or newer is required for test coverage builds.
- CMake 3.28 or newer
- GNU Flex
- `xxd` utility (from vim)
- `doxygen` and `graphviz` for building the docs
- Python for various scripts

**Ubuntu 24.10**
```
apt-get install \
    clang cmake doxygen flex g++ gcc graphviz libclang-rt-dev \
    libfl-dev lld llvm make python3 xxd
```

### Build scripts

**Release build**

```
./build-release.sh generic  # generic build
./build-release.sh native   # native build, likely significantly faster
```

**Debug/test build**

```
./build-run-tests.sh generic   # builds and runs the test suite for debug generic build
./build-run-tests.sh native    # builds and runs the test suite for debug native build
```

**Test coverage build**

```
./build-run-coverage.sh generic  # builds and runs the test suite for debug generic build with coverage enabled, asserts disabled
./build-run-coverage.sh native   # builds and runs the test suite for debug native build with coverage enabled, asserts disabled
```
The current test coverage target is 100% coverage for functions, code lines, code regions, and branches as reported by `llvm-cov`.

**Documentation build**

```
./build-docs.sh       # builds API documentation and full documentation using Doxygen
```

## Depelopment

- `scripts/check-source-preambles.py` --- Runs a quick check on source files. At the moment, checks that the license preamble is present, and for C++ headers,
  checks that the header inclusion guardian is correctly formed.
- `scripts/run-ethereal-perft-suite.py` --- Runs a Perft-based test suite from the [Ethereal](https://github.com/AndyGrant/Ethereal/) chess engine.
