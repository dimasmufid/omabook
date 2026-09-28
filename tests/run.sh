#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
fixture_dir="$(mktemp -d)"
trap 'rm -rf "$fixture_dir"' EXIT
python3 "$root/tests/make_fixtures.py" "$fixture_dir"
g++ -std=c++17 -O2 -mno-direct-extern-access "$root/tests/loader_test.cpp" "$root/src/epubloader.cpp" \
  -o "$fixture_dir/loader_test" $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Xml) -lzip
QT_QPA_PLATFORM=offscreen "$fixture_dir/loader_test" "$fixture_dir" "$@"
