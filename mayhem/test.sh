#!/usr/bin/env bash
# mayhem/test.sh — runs upstream's CSS grid + CSS math regression tests (self-asserting) and the
# mayhem layout oracle (golden diff of placements/selectors/decoding against mayhem/oracle/expected.txt).
# Everything is built by build.sh with normal flags; nothing is compiled here.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"
TB="$SRC/build/mayhem-test"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

passed=0; failed=0
for t in litehtml-grid-tests litehtml-math-tests; do
  if [ ! -x "$TB/$t" ]; then echo "missing $TB/$t (build.sh did not build it)" >&2; failed=$((failed+1)); continue; fi
  # upstream's tests print only on failure: require exit 0 and no output
  out=$("$TB/$t" 2>&1); rc=$?
  if [ "$rc" -eq 0 ] && [ -z "$out" ]; then echo "PASS $t"; passed=$((passed+1))
  else echo "FAIL $t (rc=$rc)"; echo "$out"; failed=$((failed+1)); fi
done
if [ -x "$TB/layout_oracle" ] && "$TB/layout_oracle" > "$TB/oracle.out" 2>&1 \
   && diff -u "$SRC/mayhem/oracle/expected.txt" "$TB/oracle.out"; then
  echo "PASS layout_oracle"; passed=$((passed+1))
else echo "FAIL layout_oracle"; failed=$((failed+1)); fi
emit_ctrf "litehtml-regression" "$passed" "$failed" 0
