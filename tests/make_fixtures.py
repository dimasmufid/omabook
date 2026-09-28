#!/usr/bin/env python3
"""Generate small, original EPUB fixtures for loader and UI checks."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED, ZIP_STORED
import sys

out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)

container = b'''<?xml version="1.0" encoding="UTF-8"?>
<container xmlns="urn:oasis:names:tc:opendocument:xmlns:container" version="1.0">
  <rootfiles><rootfile full-path="OEBPS/book.opf" media-type="application/oebps-package+xml"/></rootfiles>
</container>'''

chapter1 = b'''<?xml version="1.0" encoding="UTF-8"?>
<html xmlns="http://www.w3.org/1999/xhtml"><head><title>First Light</title></head><body>
<h1>First Light</h1><p>The first page should feel quiet. It contains <em>emphasis</em>,
<strong>weight</strong>, and a <a href="chapter2.xhtml#second">link to the next chapter</a>.</p>
<blockquote><p>A book begins with room to breathe.</p></blockquote>
<p>Omabook reads local books with the active desktop colors.</p>
</body></html>'''
chapter2 = b'''<?xml version="1.0" encoding="UTF-8"?>
<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Second Light</title></head><body>
<h1 id="second">Second Light</h1><p>Here is a longer passage for reading position checks.</p>
<ul><li>Open a book.</li><li>Read without distractions.</li><li>Come back where you left off.</li></ul>
''' + (b'<p>Reading is a simple pleasure. The page follows the window and the theme.</p>' * 100) + b'''</body></html>'''

opf3 = b'''<?xml version="1.0" encoding="UTF-8"?>
<package xmlns="http://www.idpf.org/2007/opf" version="3.0" unique-identifier="uid">
<metadata xmlns:dc="http://purl.org/dc/elements/1.1/"><dc:identifier id="uid">urn:uuid:omabook-fixture</dc:identifier>
<dc:title>Omabook Sample</dc:title><dc:creator>Dimas Mufid</dc:creator></metadata>
<manifest><item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>
<item id="one" href="chapter1.xhtml" media-type="application/xhtml+xml"/>
<item id="two" href="chapter2.xhtml" media-type="application/xhtml+xml"/></manifest>
<spine><itemref idref="one"/><itemref idref="two"/></spine></package>'''

opf2 = opf3.replace(b'version="3.0"', b'version="2.0"').replace(
    b'<item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>',
    b'<item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/>')
nav = b'''<html xmlns="http://www.w3.org/1999/xhtml"><body><nav><ol>
<li><a href="chapter1.xhtml">First Light</a></li><li><a href="chapter2.xhtml">Second Light</a></li>
</ol></nav></body></html>'''
ncx = b'''<ncx xmlns="http://www.daisy.org/z3986/2005/ncx/"><navMap>
<navPoint><navLabel><text>First Light</text></navLabel><content src="chapter1.xhtml"/></navPoint>
<navPoint><navLabel><text>Second Light</text></navLabel><content src="chapter2.xhtml"/></navPoint>
</navMap></ncx>'''

for version in (2, 3):
    with ZipFile(out / f"sample-epub{version}.epub", "w") as z:
        z.writestr("mimetype", "application/epub+zip", compress_type=ZIP_STORED)
        z.writestr("META-INF/container.xml", container, compress_type=ZIP_DEFLATED)
        z.writestr("OEBPS/book.opf", opf2 if version == 2 else opf3)
        z.writestr("OEBPS/chapter1.xhtml", chapter1)
        z.writestr("OEBPS/chapter2.xhtml", chapter2)
        z.writestr("OEBPS/toc.ncx" if version == 2 else "OEBPS/nav.xhtml", ncx if version == 2 else nav)

with ZipFile(out / "unsafe-path.epub", "w") as z:
    z.writestr("../escape", b"unsafe")
