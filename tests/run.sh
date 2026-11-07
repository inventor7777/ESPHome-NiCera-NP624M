#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp "${TMPDIR:-/tmp}/nicera-pir-test.XXXXXX")
trap 'rm -f "$test_binary"' EXIT
clang++ -std=c++17 -fsanitize=address,undefined -I tests/stubs -I . \
  tests/protocol_test.cpp components/nicera_np624m/nicera_np624m.cpp -o "$test_binary"
"$test_binary"
echo 'NiCera configuration, readback, motion acknowledgment and retry checks passed'
