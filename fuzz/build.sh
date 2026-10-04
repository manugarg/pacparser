#!/usr/bin/env bash
# Build the libFuzzer harness with AddressSanitizer.
#
# Needs a clang that ships libFuzzer. Apple's stock clang does not, so on macOS
# point CC at Homebrew LLVM: CC=/opt/homebrew/opt/llvm/bin/clang ./fuzz/build.sh
# Honours $CC/$CFLAGS/$LIB_FUZZING_ENGINE so OSS-Fuzz can call it unchanged.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
SRC="$ROOT/src"
OUT="${OUT:-$HERE}"
mkdir -p "$OUT"

CC="${CC:-clang}"
SANITIZER_FLAGS="${SANITIZER_FLAGS:--fsanitize=address,fuzzer-no-link}"
CFLAGS="${CFLAGS:--g -O1 $SANITIZER_FLAGS}"
# OSS-Fuzz passes the engine via $LIB_FUZZING_ENGINE; standalone uses -fsanitize=fuzzer.
ENGINE="${LIB_FUZZING_ENGINE:--fsanitize=fuzzer}"

echo "[*] CC=$CC"
echo "[*] building QuickJS"
$CC $CFLAGS -fPIC -c "$SRC/quickjs/quickjs.c" -o "$OUT/quickjs.o"

echo "[*] building pacparser"
$CC $CFLAGS -I"$SRC/quickjs" -DVERSION='"fuzz"' -c "$SRC/pacparser.c" -o "$OUT/pacparser.o"

echo "[*] linking fuzz_pac"
$CC $CFLAGS $ENGINE -I"$SRC" "$HERE/fuzz_pac.c" "$OUT/pacparser.o" "$OUT/quickjs.o" -o "$OUT/fuzz_pac"

echo "[+] built $OUT/fuzz_pac"
