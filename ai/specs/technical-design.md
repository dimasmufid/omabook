# Omabook technical design

Status: Implemented for v1  
Last updated: 2026-09-28

## Application boundary

Omabook is one Qt 6 desktop process. Qt Quick/QML owns the single-window
reading surface and bottom controls; a C++17 backend owns EPUB parsing, book
state, search, and Omarchy theme integration. It does not run inside Quickshell
and has no Omarchy plugin manifest. A `.desktop` entry registers the EPUB MIME
type and passes a selected file to `omabook %f`. `Ctrl+O` and the footer folder
button use the XDG portal file picker. Opening a book never moves or copies it.

The implementation follows Omawrite's Qt Quick application pattern, narrow
content column, tiny footer icons, desktop text-scale integration, and active
theme watcher. Omawrite's `SystemTheme` source is adapted under MIT with
attribution. The build uses qmake, which is installed on this Omarchy machine
and is also used by Omawrite.

```
Main.qml + FooterIconButton.qml
    ├── ReaderController: current book, search, progress, font preference
    └── OmarchyTheme + SystemTheme: palette and desktop text scale
             ReaderController ── EpubLoader: ZIP, OPF, spine, nav, XHTML
```

## EPUB pipeline

The first release accepts local, DRM-free, reflowable EPUB 2 and EPUB 3 files.
The loader works in a QtConcurrent worker so a large book does not freeze the
window. An error leaves the already open book intact.

1. Open the ZIP with `libzip`. Enforce a 100 MiB archive, 3,000 entries,
   16 MiB per entry, and 128 MiB total uncompressed data. Reject encrypted
   entries, duplicate or unsafe paths, and archive paths that escape the ZIP.
2. Read `META-INF/container.xml` to locate the OPF. Parse metadata, manifest,
   and spine in declared order with Qt XML. Reject detected fixed-layout
   books; no network resolution or filesystem extraction is allowed.
3. Parse EPUB 3 `nav` or EPUB 2 NCX for contents labels, falling back to
   headings and `Section N`. Keep the cover as a spine item when present.
4. Parse XHTML chapters into an allowlisted rich HTML subset: headings,
   paragraphs, emphasis, lists, quotes, tables, links, and local raster
   images. Drop scripts, forms, embedded web frames, external media, and
   publisher CSS. A cover SVG wrapper may reference one local raster image;
   the SVG itself is never executed. Image bytes are bounded at 12 MiB and
   dimensions at 12,000 pixels per side; displayed width is capped at 500 px.
5. Render one current chapter at a time in a read-only Qt Quick `TextEdit`
   using Qt rich text. Parsed chapters and plain-text search content remain
   in memory while the book is open, bounded by the EPUB limits above. This
   avoids a second browser engine; it does mean elaborate EPUB CSS and
   interactive EPUB content are outside v1.

The first implementation parses all chapters in the worker so full-book
search and progress are ready as soon as the book opens. If measured memory
on real books becomes excessive, the loader/controller boundary allows
lazy chapter rendering and a bounded cache without redesigning the UI.

## Reading state

A locator is `{spine item ID, character offset, nearby text quote}`. It is
captured from the top visible line after scrolling settles, on chapter change,
and on close. A versioned JSON state file under
`QStandardPaths::AppLocalDataLocation` is written through `QSaveFile`; an
invalid JSON file is backed aside. Locators are keyed by canonical local path
and an archive fingerprint. On reopening a changed file, try the saved quote
in the matching chapter; otherwise start that chapter and explain the
adjustment. Window resize and font changes recompute pixels from the character
offset. Progress is character based across spine chapters, not a page number.

Only reading size uses `QSettings`. The app does not maintain a recent-books
list, library path, scanned directories, a copy of the EPUB, or extracted
chapter files. Startup without a file argument shows the empty reader. Opening
an EPUB from a file manager or shell immediately loads that one book.

## Omarchy theme

Read `~/.local/state/omarchy/current/theme/colors.toml` and expose
`background`, `foreground`, `accent`, `selection`, `muted`, and `mode` to QML.
Watch the parent directories and palette file with `QFileSystemWatcher`, then
re-arm watches after Omarchy atomically replaces the theme. Coalesce change
bursts. Use built-in light/dark colors when the active palette is absent or
invalid. The app does not write Omarchy configuration. Omawrite's
`SystemTheme` portal listener supplies desktop text scaling.

## Security and failure boundaries

- Treat EPUBs as untrusted local data. Never execute scripts or fetch linked
  resources. Only app-generated, bounded image data enters Qt rich text.
- Resolve every internal path within the ZIP. External `http`/`https` links
  require a confirmation dialog before opening the system browser; `file:`,
  `javascript:`, and source `data:` URLs are ignored.
- Reject XML entity declarations before DOM parsing. Cap render recursion.
- Keep previous book visible when a newly opened book is invalid, oversized,
  encrypted, or unsupported. Show a brief error above the footer.
- Diagnostics must not log book text, search terms, or extracted image data.

## Repository and install shape

```text
plugins/omabook/
  omabook.pro
  src/{main,readercontroller,epubloader,omarchytheme,systemtheme}.*
  src/qml/{Main,FooterIconButton}.qml
  packaging/{omabook.desktop,omabook.svg,install-local.sh,PKGBUILD}
  tests/{make_fixtures.py,loader_test.cpp,run.sh}
  ai/specs/{README,visual-design,technical-design}.md
  README.md  LICENSE
```

The user-local installer builds the binary and places it in `~/.local/bin`,
with a desktop entry and icon under `~/.local/share`. The PKGBUILD is an
optional system package path. No background daemon or elevated service is
installed. Build dependencies are Qt 6 base/declarative, libzip, GCC, make,
and qmake; runtime also needs an XDG desktop portal for the file picker.

## Verification and completion criteria

- Loader fixtures cover EPUB 2, EPUB 3, spine order, navigation, internal
  links, text, and path traversal rejection.
- A six-book public-domain corpus opens with readable chapters, including
  prose and cover images. Long chapters, links, lists, and quotes remain
  legible in the bounded rich-text renderer.
- The empty reader shows no prompt. Its folder action is the only enabled
  control; the header stays empty and no dropdown or library UI appears.
- Chapter changes, search, font adjustment, and restored reading position
  work in a keyboard-only pass.
- A light and a dark Omarchy palette can be swapped while the same chapter
  stays open, with text still legible and scroll position unchanged.
- The installed desktop entry validates, launches, and opens an EPUB on this
  Omarchy machine. The standalone source is pushed to the Omabook GitHub repo.

## Sources

- [Omawrite source](https://github.com/omacom/omawrite)
- [Omawrite theme watcher](https://github.com/omacom/omawrite/blob/master/src/backend.cpp)
- [EPUB 3.3](https://www.w3.org/TR/epub-33/)
- [Qt supported HTML subset](https://doc.qt.io/qt-6/richtext-html-subset.html)
