# Omabook

A small, local EPUB reader for Omarchy. Omabook follows the active Omarchy
theme while you read and remembers your place. It is a desktop application,
not a Quickshell plugin.

## What it does

- Opens DRM-free, reflowable EPUB 2 and EPUB 3 books from disk.
- Shows a centered reading column, table of contents, and book search. The
  empty view has only an enabled folder control at the bottom; tiny footer
  icons keep the header empty, following Omawrite's visual approach.
- Restores the last chapter and reading position.
- Follows Omarchy's active palette and desktop text scale without restarting.
- Opens books in place. No library, upload, account, or network service is used.

Publisher CSS, fixed-layout EPUBs, DRM, PDF, and annotations are not supported
in this first release. The lightweight Qt renderer intentionally prioritizes
readable prose over exact publisher layout.

## Build

On Omarchy, install the build dependencies if they are missing:

```bash
omarchy pkg add base-devel qt6-base qt6-declarative libzip xdg-desktop-portal
```

Then build and run:

```bash
qmake6 omabook.pro -o Makefile
make -j"$(nproc)"
./omabook /path/to/book.epub
```

To install just for your user, run:

```bash
./packaging/install-local.sh
```

This installs the binary in `~/.local/bin`, the desktop entry in
`~/.local/share/applications`, and the icon in `~/.local/share/icons`. Remove
those three installed files to uninstall. It leaves reading state in
`~/.local/share/Dimas Mufid/omabook` for the reader to decide whether to keep.
Open **Omabook** from the Omarchy app launcher, or run `omabook` in a terminal.

## Controls

| Key | Action |
|---|---|
| `Ctrl+O` | Open EPUB |
| `Ctrl+T` | Contents |
| `Ctrl+F` | Search |
| `Alt+Left` / `Alt+Right` | Previous / next chapter |
| `PageUp` / `PageDown` | Scroll |
| `Ctrl++` / `Ctrl+-` / `Ctrl+0` | Adjust / reset reading size |
| `F11` | Full screen |

## Verify

```bash
./tests/run.sh
```

The tests generate original EPUB 2 and EPUB 3 fixtures and exercise the
container, package, navigation, text, links, and unsafe-path handling. The
[specifications](ai/specs/README.md) describe the visual and technical
decisions, limits, and manual checks.

## Credits and license

Omabook is MIT licensed. Its `SystemTheme` implementation is adapted from
[Omawrite](https://github.com/omacom/omawrite), Copyright 2026 David
Heinemeier Hansson, used under the MIT license. See [LICENSE](LICENSE).
