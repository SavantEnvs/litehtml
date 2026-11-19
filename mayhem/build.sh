#!/usr/bin/env bash
# mayhem/build.sh — litehtml: sanitized static lib + 7 libFuzzer harnesses (+ -standalone reproducers),
# then a normal-flags build of upstream's grid/math regression tests and the mayhem layout oracle.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export CC CXX
cd "$SRC"
# litehtml's string_id is an open enum by design: string_id.cpp _id() hands out
# static_cast<string_id>(array.size()-1) for every unknown tag/attribute/property name, so UBSan's
# `enum` check fires on every document with a custom name (html_tag.cpp select(), etc.). Relax only that
# check; ASan and the rest of UBSan stay halting. Kept out when sanitizers are switched off.
[ -z "$SANITIZER_FLAGS" ] || SANITIZER_FLAGS="$SANITIZER_FLAGS -fno-sanitize=enum"

# 1) the library (litehtml + bundled gumbo), instrumented. build/ is gitignored upstream.
FZ="$SRC/build/mayhem-fuzz"
cmake -S . -B "$FZ" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLITEHTML_ENABLE_LINT=OFF -DBUILD_SHARED_LIBS=OFF -DLITEHTML_BUILD_CONTAINER_CAIRO=ON \
      -DCMAKE_C_FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link -O1 $DEBUG_FLAGS" \
      -DCMAKE_CXX_FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link -O1 $DEBUG_FLAGS"
cmake --build "$FZ" -j"$MAYHEM_JOBS" --target litehtml gumbo litehtml_container_cairo
LIBS=("$FZ/liblitehtml.a" "$FZ/src/gumbo/libgumbo.a")
INC=(-I"$SRC/include" -I"$SRC/include/litehtml" -I"$SRC/mayhem/harnesses")

# 2) harnesses: fuzzer + standalone reproducer, LSan off
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$FZ/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FZ/standalone_main.o"
for h in fuzz_render fuzz_css fuzz_events fuzz_selector fuzz_encodings fuzz_url; do
  $CXX -std=c++17 $SANITIZER_FLAGS $DEBUG_FLAGS -O1 "${INC[@]}" -c "$SRC/mayhem/harnesses/$h.cpp" -o "$FZ/$h.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$FZ/$h.o" "$FZ/lsan_off.o" "${LIBS[@]}" -o "$SRC/$h"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$FZ/$h.o" "$FZ/standalone_main.o" "$FZ/lsan_off.o" "${LIBS[@]}" -o "$SRC/$h-standalone"
done

# 2b) fuzz_render_cairo: upstream's cairo container (containers/cairo, built instrumented above) on a
#     cairo image surface; libcairo itself comes from apt (libcairo2-dev, installed by the Dockerfile).
CAIRO_CFLAGS=$(pkg-config --cflags cairo); CAIRO_LIBS=$(pkg-config --libs cairo)
$CXX -std=c++17 $SANITIZER_FLAGS $DEBUG_FLAGS -O1 "${INC[@]}" -I"$SRC/containers/cairo" $CAIRO_CFLAGS \
     -c "$SRC/mayhem/harnesses/fuzz_render_cairo.cpp" -o "$FZ/fuzz_render_cairo.o"
CLIBS=("$FZ/containers/cairo/liblitehtml_container_cairo.a" "${LIBS[@]}")
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$FZ/fuzz_render_cairo.o" "$FZ/lsan_off.o" "${CLIBS[@]}" $CAIRO_LIBS -o "$SRC/fuzz_render_cairo"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$FZ/fuzz_render_cairo.o" "$FZ/standalone_main.o" "$FZ/lsan_off.o" "${CLIBS[@]}" $CAIRO_LIBS -o "$SRC/fuzz_render_cairo-standalone"

# 3) tests with the project's normal flags: upstream regression tests + layout oracle
TB="$SRC/build/mayhem-test"
cmake -S . -B "$TB" -G Ninja -DCMAKE_BUILD_TYPE=Release -DLITEHTML_ENABLE_LINT=OFF -DBUILD_SHARED_LIBS=OFF \
      -DLITEHTML_BUILD_GRID_TESTS=ON -DLITEHTML_BUILD_MATH_TESTS=ON \
      -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build "$TB" -j"$MAYHEM_JOBS" --target litehtml gumbo litehtml-grid-tests litehtml-math-tests
$CXX -std=c++17 -O2 $COVERAGE_FLAGS "${INC[@]}" "$SRC/mayhem/harnesses/layout_oracle.cpp" \
     "$TB/liblitehtml.a" "$TB/src/gumbo/libgumbo.a" -o "$TB/layout_oracle"
