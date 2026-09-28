# Omabook specifications

Status: Implemented for v1  
Last updated: 2026-09-28

Omabook is a small, local EPUB reader for Omarchy. It is a standalone desktop
application, not a Quickshell plugin. The directory name under `plugins/` is a
workspace convention; installation will use an app binary and desktop entry.

## Documents

| Document | Purpose |
|---|---|
| [Visual and interaction design](visual-design.md) | Screens, layout, typography, theme behavior, keyboard use, and states |
| [Technical design](technical-design.md) | EPUB pipeline, Qt architecture, storage, security, delivery, and verification |

## Decisions for v1

1. Open local, DRM-free, reflowable EPUB 2 and EPUB 3 books first. PDF, fixed
   layout EPUB, DRM, cloud sync, accounts, stores, and annotations are later
   work.
2. Use a Qt Quick UI with a C++ backend, following Omawrite's application
   pattern. Use a separate process and window, rather than running a reader
   inside the Omarchy shell.
3. Follow the active Omarchy palette live from
   `~/.local/state/omarchy/current/theme/colors.toml` and follow Omarchy's
   desktop text scale. Reading font size remains a per-app preference.
4. Show one book at a time. Open it in place through the footer folder icon,
   file manager, or command line. No library, recents, upload, or copied books.
5. Use a small native rich-text renderer for the first implementation and
   verify it against real EPUBs.
6. Keep all book contents and reading state on the local computer. No network
   requests are part of v1.

## Reference material

- [Omawrite source](https://github.com/omacom/omawrite): Qt Quick/C++ app,
  narrow writing column, restrained chrome, portal file picker, text scaling.
- [Omawrite theme loader](https://github.com/omacom/omawrite/blob/master/src/backend.cpp):
  reads the active `colors.toml`, watches its parent directories, and re-arms
  watches after changes.
- [Omarchy theme contract](https://github.com/omacom/omarchy/blob/quattro/docs/theming.md):
  active theme staging path and live theme changes.
- [EPUB 3.3](https://www.w3.org/TR/epub-33/): container, package, navigation,
  and content format.
- [Qt supported HTML subset](https://doc.qt.io/qt-6/richtext-html-subset.html):
  the rendering boundary for the lightweight v1 approach.

## Specification change rule

When implementation reveals a different EPUB or Qt behavior, record the
observed book fixture and update the affected decision and acceptance criterion
before expanding the feature set.
