# Omabook visual and interaction design

Status: Implemented for v1  
Last updated: 2026-09-28

## Direction

Omabook is a quiet, single-book reader modeled on Omawrite's restraint. The
reference is the Omawrite screenshot supplied by the user and its source:
one centered column, generous empty space, no header or toolbar, and small
icon controls along the bottom edge. All visible actions live in that footer.
There is no library screen, bookshelf, recent-books list, or `More` menu.

The window title identifies the book for the desktop window manager; no book
title is repeated in a top bar. No paper texture, card grid, page-turn
animation, or app-specific color theme picker.

## Reading surface

```
┌───────────────────────────────────────────────────────────────────┐
│                                                                   │
│                 Chapter heading                                   │
│                 Book text in a narrow, centered column.           │
│                 Images fit inside that same column.               │
│                                                                   │
│                                                                   │
│  [folder] [contents] [search] [−] [+] [fullscreen]  chapter    34% │
└───────────────────────────────────────────────────────────────────┘
```

The reading column is at most 680 logical pixels wide, with at least 24 px
side margins. It begins about 58 px below the window top and scrolls vertically
within the current chapter. The window uses the active Omarchy `background`
color, with no distinct page card. Default reading text is 20 px generic
serif, 155% line height, and semantic headings, emphasis, lists, quotes, and
links. Images preserve aspect ratio and fit a narrow window. Desktop text
scale multiplies the default size; the footer `−` and `+` controls adjust the
reader's font size between 14 and 32 px at scale 1.

The empty window has one quiet line near the top of the same column: `Open an
EPUB to read.` The folder icon remains available at the bottom left. Loading
shows a small `Opening…` label without replacing a currently open book.

## Footer

The footer has no colored strip or divider. It is roughly 34 px high and
mirrors Omawrite's 16 px outline icons, 28 px hit targets, muted color, and
subtle opacity. From left to right:

| Control | Action |
|---|---|
| Folder | Open a local EPUB through the XDG portal file picker |
| Contents | Toggle the current book's table of contents |
| Search | Search the current book |
| Minus / plus | Decrease / increase reading size |
| Full screen | Toggle full screen |
| Chapter label | Passive current-chapter context, elided when space is tight |
| Percentage at far right | Approximate book progress, based on source text |

Controls that require a book are dimmed until one is open. All icons have
tooltips and accessible names. The footer remains visible during scrolling.
There is no dropdown containing additional actions.

## Contents and search

Contents and search are small, explicit panels above their footer controls.
Only one panel can be open at a time. Contents lists the book's navigation
entries and highlights the current chapter; clicking one opens it. Search has
one field and a bounded list of chapter-context results. `Enter` opens the
selected result, arrow controls or keyboard navigation move between results,
and `Escape` dismisses the panel and returns focus to reading. Neither panel
creates a persistent sidebar or changes the window layout.

The EPUB's own chapter headings remain part of the reading document; Omabook
does not add a second title above them. Cover-only spine items show the cover
image, and the contents panel labels them `Cover` where appropriate.

## Theme and accessibility

Read the active Omarchy `colors.toml` values `background`, `foreground`,
`accent`, `selection`, `muted`, and `mode`. Update an open book live when the
theme changes without losing chapter or scroll position. Built-in light/dark
fallbacks keep the app readable if the palette is absent. Text and controls
follow desktop text scale; focus indicators, tooltips, accessible names, and
keyboard-only use remain available. The app does not alter Omarchy's theme.

## Keyboard path

| Key | Action |
|---|---|
| `Ctrl+O` | Open EPUB |
| `Ctrl+T` | Contents |
| `Ctrl+F` | Search |
| `Alt+Left` / `Alt+Right` | Previous / next chapter |
| `PageUp` / `PageDown`, `Space` / `Shift+Space` | Scroll |
| `Ctrl+-` / `Ctrl++` | Reading size |
| `F11` | Full screen |
| `Escape` | Dismiss panel, then leave full screen |

## Errors and boundaries

Malformed, encrypted, fixed-layout, or oversized books get a short message
above the footer. A failed open leaves the current book intact. Missing images
use a restrained placeholder and leave surrounding text readable. External
web links require confirmation. Books are opened in place, never uploaded or
copied by the app.

## Visual acceptance

1. The header area remains empty in the no-book and reading states. Every
   visible action is in the footer, and there is no `More` button.
2. The folder icon opens an EPUB; an opened chapter renders in a centered,
   readable column at wide and narrow window sizes.
3. Light and dark Omarchy palette changes update the same text in place.
4. Contents, search, font adjustment, and full screen work by mouse and
   keyboard; footer controls remain discernible at 150% desktop text scale.
