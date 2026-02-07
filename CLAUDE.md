# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Pacparser is a C library (with Python bindings) for parsing proxy auto-config (PAC) files. It embeds QuickJS JavaScript engine to evaluate PAC scripts and implements the standard PAC helper functions (e.g., `dnsDomainIs`, `isInNet`, `myIpAddress`). Licensed under LGPL.

## Build Commands

All build commands run from the repo root using `make -C src`.

```bash
# Build C library and pactester CLI (also runs tests)
make -C src

# Build without internet-dependent tests
NO_INTERNET=1 make -C src

# Build Python module (also runs Python tests)
make -C src pymod

# Install C library and pactester
sudo make -C src install

# Install Python module
sudo make -C src install-pymod

# Clean all build artifacts
make -C src clean

# Windows build (requires MinGW/MSYS2)
make -C src -f Makefile.win32
```

## Testing

Tests run automatically as part of the build (`make -C src` runs `testpactester` target).

```bash
# Run C tests manually
../tests/runtests.sh                    # from src/
NO_INTERNET=1 ../tests/runtests.sh      # skip DNS-dependent tests

# Run Python tests manually
python ../tests/runtests.py             # from src/
NO_INTERNET=1 python ../tests/runtests.py
```

Test data is in `tests/testdata` with format: `<pactester params> | <expected result>`. Tests use `tests/proxy.pac` as the PAC file. Set `DEBUG=1` for verbose test output.

## Architecture

### Build Flow
1. QuickJS engine compiles to `src/quickjs/libquickjs.a`
2. `pacparser.c` compiles against QuickJS headers to `pacparser.o`
3. Shared library (`libpacparser.so.1` / `.dylib` / `.dll`) links `pacparser.o` + `libquickjs.a`
4. `pactester` CLI statically links against `libpacparser.a`
5. Python C extension (`_pacparser`) wraps `pacparser.o` + `libquickjs.a` via setuptools

### Key Source Files

- **`src/pacparser.c`** — Core library. Initializes QuickJS context, evaluates PAC scripts, implements DNS helper functions (`dns_resolve`, `my_ip`). All public API functions live here.
- **`src/pacparser.h`** — Public C API (9 functions: `init`, `parse_pac_file`, `parse_pac_string`, `find_proxy`, `just_find_proxy`, `cleanup`, `setmyip`, `set_error_printer`, `version`).
- **`src/pac_utils.h`** — PAC standard JavaScript functions embedded as a C string. This is Mozilla's PAC utility implementation defining `dnsDomainIs()`, `isInNet()`, `shExpMatch()`, etc.
- **`src/pactester.c`** — CLI tool wrapping the library. Usage: `pactester -p <pacfile> -u <url> [-c client_ip] [-f urlslist]`.
- **`src/pymod/pacparser/__init__.py`** — Python API wrapper. Adds host auto-extraction from URLs and the `URLError` exception.
- **`src/pymod/pacparser_py.c`** — Python C extension binding `_pacparser` methods to the C library.
- **`src/pymod/setup.py`** — Python build config. Version derived from git tags.

### Platform Handling
The `src/Makefile` detects OS via `uname` and adjusts shared library naming, linking flags, and compiler flags for Linux, macOS, and FreeBSD. Windows uses `src/Makefile.win32` with MinGW.
