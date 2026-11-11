# Fuzzing pacparser

A [libFuzzer](https://llvm.org/docs/LibFuzzer.html) harness for the PAC parsing
path. It drives `pacparser_parse_pac_string()` followed by
`pacparser_find_proxy()`, exercising pacparser's own C layer and the PAC helper
functions (`myIpAddress`, `dnsResolve`, `isInNet`, `shExpMatch`, ...) on top of
the embedded QuickJS engine.

## Input format

Each input is split on NUL bytes into up to three fields:

```
<pac script> \0 <url> \0 <host>
```

Missing `url`/`host` fall back to fixed defaults, so a bare PAC script is a
valid input too. The `fuzz/corpus/` directory contains a few readable seeds.

## Build

Needs a clang that ships libFuzzer (upstream LLVM clang or the OSS-Fuzz
toolchain). Apple's stock clang does not include it — install LLVM via Homebrew
on macOS:

```sh
CC=/opt/homebrew/opt/llvm/bin/clang ./fuzz/build.sh   # macOS (Homebrew LLVM)
./fuzz/build.sh                                        # Linux with clang
```

## Run

```sh
./fuzz/fuzz_pac fuzz/corpus            # replay/extend the seed corpus
./fuzz/fuzz_pac -max_total_time=60 fuzz/corpus
```

Set `ASAN_OPTIONS=detect_leaks=0` if you only want to chase memory-corruption
bugs and not leaks.

## OSS-Fuzz

`build.sh` honours `$CC`, `$CFLAGS`, and `$LIB_FUZZING_ENGINE`, so it can be
called directly from an OSS-Fuzz `build.sh` with no changes.
