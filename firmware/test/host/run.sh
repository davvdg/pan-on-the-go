#!/usr/bin/env bash
# Banc de test hôte du PadScanner : compile et exécute la logique de détection
# sur ta machine, sans toolchain ESP32. Quelques secondes.
set -euo pipefail
cd "$(dirname "$0")"

OUT="${TMPDIR:-/tmp}/pan-test-scanner"

g++ -std=c++17 -O1 -Wall -Wextra \
    -I stubs -I ../../include -I ../../src \
    -D PAD_COUNT="${PAD_COUNT:-29}" \
    -D PADMAP_CHROMATIC \
    -D TRANSPORT_SERIAL \
    test_scanner.cpp ../../src/PadScanner.cpp \
    -o "$OUT"

"$OUT"
