#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
qmake6 omabook.pro -o Makefile
make -j"$(nproc)"
install -Dm755 omabook "$HOME/.local/bin/omabook"
install -Dm644 packaging/omabook.desktop "$HOME/.local/share/applications/omabook.desktop"
install -Dm644 packaging/omabook.svg "$HOME/.local/share/icons/hicolor/scalable/apps/omabook.svg"
if command -v update-desktop-database >/dev/null; then
  update-desktop-database "$HOME/.local/share/applications"
fi
printf 'Installed Omabook for %s\n' "$USER"
