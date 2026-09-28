#include "epubloader.h"

#include <QCryptographicHash>
#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QSet>
#include <QTextDocument>
#include <QUrl>
#include <zip.h>

namespace {
constexpr qint64 maxArchiveBytes = 100LL * 1024 * 1024;
constexpr qint64 maxExpandedBytes = 128LL * 1024 * 1024;
constexpr qint64 maxEntryBytes = 16LL * 1024 * 1024;
constexpr int maxEntries = 3000;
constexpr int maxChapters = 600;
constexpr int maxImageSide = 12000;

QString localName(const QDomNode &node) {
    QString name = node.nodeName();
    return name.section(QLatin1Char(':'), -1).toLower();
}

QString normalizedPath(const QString &path) {
    QString decoded = QUrl::fromPercentEncoding(path.toUtf8());
    decoded.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (decoded.startsWith(QLatin1Char('/')) || decoded.contains(QChar::Null))
        return {};
    const QString cleaned = QDir::cleanPath(decoded);
    if (cleaned == QStringLiteral("..") || cleaned.startsWith(QStringLiteral("../"))
        || cleaned == QStringLiteral("."))
        return {};
    return cleaned;
}

QString resolvePath(const QString &base, const QString &href) {
    const QString target = href.section(QLatin1Char('#'), 0, 0);
    if (target.isEmpty())
        return base;
    if (target.contains(QLatin1Char(':')) || target.startsWith(QStringLiteral("//")))
        return {};
    return normalizedPath(QFileInfo(base).path() + QLatin1Char('/') + target);
}

QDomElement firstDescendant(const QDomNode &parent, const QString &name) {
    for (QDomNode child = parent.firstChild(); !child.isNull(); child = child.nextSibling()) {
        if (child.isElement() && localName(child) == name)
            return child.toElement();
        QDomElement found = firstDescendant(child, name);
        if (!found.isNull())
            return found;
    }
    return {};
}

void collectElements(const QDomNode &parent, const QString &name, QList<QDomElement> &out) {
    for (QDomNode child = parent.firstChild(); !child.isNull(); child = child.nextSibling()) {
        if (child.isElement() && localName(child) == name)
            out.append(child.toElement());
        collectElements(child, name, out);
    }
}

QString escapeText(const QString &text) { return text.toHtmlEscaped(); }

QString attribute(const QDomElement &element, const QString &name) {
    for (int i = 0; i < element.attributes().count(); ++i) {
        const QDomAttr attr = element.attributes().item(i).toAttr();
        if (attr.name().section(QLatin1Char(':'), -1).toLower() == name)
            return attr.value();
    }
    return {};
}

class Archive {
public:
    explicit Archive(const QString &path) {
        int error = 0;
        m_zip = zip_open(QFile::encodeName(path).constData(), ZIP_RDONLY, &error);
        if (!m_zip) { m_error = QStringLiteral("Could not open EPUB archive."); return; }
        const zip_int64_t count = zip_get_num_entries(m_zip, 0);
        if (count < 0 || count > maxEntries) { m_error = QStringLiteral("EPUB has too many files."); return; }
        qint64 expanded = 0;
        for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); ++i) {
            zip_stat_t stat;
            if (zip_stat_index(m_zip, i, 0, &stat) != 0) { m_error = QStringLiteral("Invalid EPUB file entry."); return; }
            const QString raw = QString::fromUtf8(stat.name);
            if (raw.endsWith(QLatin1Char('/')))
                continue;
            const QString key = normalizedPath(raw);
            if (key.isEmpty() || key != raw || m_entries.contains(key)) {
                m_error = QStringLiteral("EPUB contains an unsafe or duplicate path."); return;
            }
            if (stat.encryption_method != ZIP_EM_NONE) { m_error = QStringLiteral("Encrypted EPUB files are not supported."); return; }
            if (stat.size > maxEntryBytes || expanded + static_cast<qint64>(stat.size) > maxExpandedBytes) {
                m_error = QStringLiteral("EPUB content exceeds the size limit."); return;
            }
            expanded += static_cast<qint64>(stat.size);
            m_entries.insert(key, i);
        }
    }
    ~Archive() { if (m_zip) zip_close(m_zip); }
    QString error() const { return m_error; }
    bool contains(const QString &path) const { return m_entries.contains(path); }
    QByteArray read(const QString &path) const {
        if (!m_zip || !m_entries.contains(path)) return {};
        zip_file_t *file = zip_fopen_index(m_zip, m_entries.value(path), 0);
        if (!file) return {};
        QByteArray data;
        char chunk[8192];
        while (true) {
            const zip_int64_t read = zip_fread(file, chunk, sizeof(chunk));
            if (read < 0) { data.clear(); break; }
            if (read == 0) break;
            data.append(chunk, static_cast<qsizetype>(read));
            if (data.size() > maxEntryBytes) { data.clear(); break; }
        }
        zip_fclose(file);
        return data;
    }
private:
    zip_t *m_zip = nullptr;
    QHash<QString, zip_uint64_t> m_entries;
    QString m_error;
};

QString imageDataUrl(const Archive &archive, const QString &path, QSize *sizeOut) {
    const QByteArray bytes = archive.read(path);
    if (bytes.isEmpty() || bytes.size() > 12 * 1024 * 1024) return {};
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QSize size = reader.size();
    if (!size.isValid() || size.width() > maxImageSide || size.height() > maxImageSide)
        return {};
    *sizeOut = size;
    const QString mime = QMimeDatabase().mimeTypeForData(bytes).name();
    if (mime != QStringLiteral("image/png") && mime != QStringLiteral("image/jpeg")
        && mime != QStringLiteral("image/gif") && mime != QStringLiteral("image/webp"))
        return {};
    return QStringLiteral("data:%1;base64,%2").arg(mime, QString::fromLatin1(bytes.toBase64()));
}

QString renderNode(const QDomNode &node, const Archive &archive, const QString &chapterPath,
                   const QHash<QString, int> &chapterIndexes, QString &plain, int depth = 0) {
    if (depth > 64) return {};
    if (node.isText() || node.isCDATASection()) {
        const QString content = node.nodeValue();
        plain += content;
        return escapeText(content);
    }
    if (!node.isElement()) return {};
    const QDomElement element = node.toElement();
    const QString name = localName(node);
    if (name == QStringLiteral("svg")) {
        const QDomElement image = firstDescendant(node, QStringLiteral("image"));
        if (image.isNull()) return {};
        QSize imageSize;
        const QString data = imageDataUrl(archive, resolvePath(chapterPath, attribute(image, QStringLiteral("href"))), &imageSize);
        if (data.isEmpty()) return {};
        const int width = qMin(500, imageSize.width());
        const int height = static_cast<int>(imageSize.height() * static_cast<double>(width) / imageSize.width());
        return QStringLiteral("<p><img src=\"%1\" width=\"%2\" height=\"%3\" /></p>")
            .arg(data).arg(width).arg(height);
    }
    if (name == QStringLiteral("script") || name == QStringLiteral("style")
        || name == QStringLiteral("iframe") || name == QStringLiteral("form")
        || name == QStringLiteral("video")
        || name == QStringLiteral("audio")) return {};
    if (name == QStringLiteral("img")) {
        const QString path = resolvePath(chapterPath, attribute(element, QStringLiteral("src")));
        QSize imageSize;
        const QString data = imageDataUrl(archive, path, &imageSize);
        if (data.isEmpty()) return QStringLiteral("<p>[Image unavailable]</p>");
        const int width = qMin(500, imageSize.width());
        const int height = static_cast<int>(imageSize.height() * static_cast<double>(width) / imageSize.width());
        return QStringLiteral("<p><img src=\"%1\" width=\"%2\" height=\"%3\" /></p>")
            .arg(data).arg(width).arg(height);
    }
    static const QSet<QString> allowed = {
        QStringLiteral("p"), QStringLiteral("div"), QStringLiteral("span"),
        QStringLiteral("h1"), QStringLiteral("h2"), QStringLiteral("h3"),
        QStringLiteral("h4"), QStringLiteral("h5"), QStringLiteral("h6"),
        QStringLiteral("blockquote"), QStringLiteral("ul"), QStringLiteral("ol"),
        QStringLiteral("li"), QStringLiteral("em"), QStringLiteral("strong"),
        QStringLiteral("b"), QStringLiteral("i"), QStringLiteral("u"),
        QStringLiteral("sup"), QStringLiteral("sub"), QStringLiteral("pre"),
        QStringLiteral("code"), QStringLiteral("br"), QStringLiteral("hr"),
        QStringLiteral("a"), QStringLiteral("table"), QStringLiteral("tr"),
        QStringLiteral("td"), QStringLiteral("th")
    };
    QString children;
    for (QDomNode child = node.firstChild(); !child.isNull(); child = child.nextSibling())
        children += renderNode(child, archive, chapterPath, chapterIndexes, plain, depth + 1);
    if (!allowed.contains(name)) return children;
    if (name == QStringLiteral("br") || name == QStringLiteral("hr")) {
        plain += QLatin1Char('\n');
        return QStringLiteral("<%1/>").arg(name);
    }
    QString attrs;
    const QString id = attribute(element, QStringLiteral("id"));
    if (!id.isEmpty() && id.size() < 200)
        attrs += QStringLiteral(" id=\"%1\"").arg(id.toHtmlEscaped());
    if (name == QStringLiteral("a")) {
        const QString href = attribute(element, QStringLiteral("href"));
        const QString target = resolvePath(chapterPath, href);
        if (chapterIndexes.contains(target)) {
            const QString fragment = href.contains(QLatin1Char('#')) ? href.section(QLatin1Char('#'), 1) : QString();
            attrs += QStringLiteral(" href=\"book:%1#%2\"").arg(chapterIndexes.value(target)).arg(fragment.toHtmlEscaped());
        } else if (href.startsWith(QStringLiteral("https://")) || href.startsWith(QStringLiteral("http://"))) {
            attrs += QStringLiteral(" href=\"%1\"").arg(href.toHtmlEscaped());
        }
    }
    if (name == QStringLiteral("p") || name.startsWith(QLatin1Char('h'))
        || name == QStringLiteral("div") || name == QStringLiteral("li"))
        plain += QLatin1Char('\n');
    return QStringLiteral("<%1%2>%3</%1>").arg(name, attrs, children);
}

QDomDocument parseXml(const QByteArray &bytes) {
    QDomDocument document;
    if (bytes.contains("<!ENTITY") || bytes.contains("<!entity")) return document;
    document.setContent(bytes, QDomDocument::ParseOption::UseNamespaceProcessing);
    return document;
}
}

BookData EpubLoader::load(const QString &path) {
    BookData book;
    book.path = QFileInfo(path).canonicalFilePath();
    const QFileInfo fileInfo(path);
    if (!fileInfo.isFile() || fileInfo.size() > maxArchiveBytes || fileInfo.size() == 0) {
        book.error = QStringLiteral("The EPUB file is missing or too large."); return book;
    }
    Archive archive(path);
    if (!archive.error().isEmpty()) { book.error = archive.error(); return book; }
    const QDomDocument container = parseXml(archive.read(QStringLiteral("META-INF/container.xml")));
    const QDomElement rootfile = firstDescendant(container, QStringLiteral("rootfile"));
    const QString opfPath = normalizedPath(rootfile.attribute(QStringLiteral("full-path")));
    if (opfPath.isEmpty() || !archive.contains(opfPath)) {
        book.error = QStringLiteral("EPUB package metadata is missing."); return book;
    }
    const QByteArray opfBytes = archive.read(opfPath);
    const QDomDocument opf = parseXml(opfBytes);
    if (opf.documentElement().isNull()) { book.error = QStringLiteral("EPUB package metadata is invalid."); return book; }
    const QDomElement metadata = firstDescendant(opf, QStringLiteral("metadata"));
    book.title = firstDescendant(metadata, QStringLiteral("title")).text().trimmed();
    book.author = firstDescendant(metadata, QStringLiteral("creator")).text().trimmed();
    if (book.title.isEmpty()) book.title = fileInfo.completeBaseName();
    const QDomElement rendition = firstDescendant(metadata, QStringLiteral("meta"));
    QList<QDomElement> metas; collectElements(metadata, QStringLiteral("meta"), metas);
    for (const auto &meta : metas) {
        if ((meta.attribute(QStringLiteral("property")) == QStringLiteral("rendition:layout")
             && meta.text().trimmed() == QStringLiteral("pre-paginated"))
            || (meta.attribute(QStringLiteral("name")) == QStringLiteral("fixed-layout")
                && meta.attribute(QStringLiteral("content")) == QStringLiteral("true"))) {
            book.error = QStringLiteral("Fixed-layout EPUBs are not supported yet."); return book;
        }
    }
    Q_UNUSED(rendition);
    struct ManifestItem { QString id; QString path; QString properties; QString mediaType; };
    QHash<QString, ManifestItem> items;
    QList<QDomElement> itemNodes; collectElements(firstDescendant(opf, QStringLiteral("manifest")), QStringLiteral("item"), itemNodes);
    QString navPath;
    QString ncxPath;
    for (const auto &item : itemNodes) {
        ManifestItem entry{item.attribute(QStringLiteral("id")), resolvePath(opfPath, item.attribute(QStringLiteral("href"))),
                           item.attribute(QStringLiteral("properties")), item.attribute(QStringLiteral("media-type"))};
        if (entry.id.isEmpty() || entry.path.isEmpty() || !archive.contains(entry.path)) continue;
        items.insert(entry.id, entry);
        if (entry.properties.split(QLatin1Char(' ')).contains(QStringLiteral("nav"))) navPath = entry.path;
        if (entry.mediaType == QStringLiteral("application/x-dtbncx+xml")) ncxPath = entry.path;
    }
    QList<QDomElement> itemRefs; collectElements(firstDescendant(opf, QStringLiteral("spine")), QStringLiteral("itemref"), itemRefs);
    QHash<QString, int> chapterIndexes;
    for (const auto &ref : itemRefs) {
        const QString id = ref.attribute(QStringLiteral("idref"));
        if (!items.contains(id) || !items.value(id).mediaType.contains(QStringLiteral("html"))) continue;
        if (book.chapters.size() >= maxChapters) { book.error = QStringLiteral("EPUB has too many chapters."); return book; }
        const ManifestItem item = items.value(id);
        chapterIndexes.insert(item.path, book.chapters.size());
        book.chapters.append({id, item.path, {}, {}, {}});
    }
    if (book.chapters.isEmpty()) { book.error = QStringLiteral("EPUB has no readable chapters."); return book; }
    QHash<QString, QString> navigationTitles;
    if (!navPath.isEmpty()) {
        const QDomDocument nav = parseXml(archive.read(navPath));
        QList<QDomElement> anchors; collectElements(nav, QStringLiteral("a"), anchors);
        for (const auto &anchor : anchors) {
            const QString target = resolvePath(navPath, attribute(anchor, QStringLiteral("href")));
            if (chapterIndexes.contains(target) && !navigationTitles.contains(target))
                navigationTitles.insert(target, anchor.text().trimmed());
        }
    } else if (!ncxPath.isEmpty()) {
        const QDomDocument ncx = parseXml(archive.read(ncxPath));
        QList<QDomElement> points; collectElements(ncx, QStringLiteral("navPoint"), points);
        for (const auto &point : points) {
            const QDomElement content = firstDescendant(point, QStringLiteral("content"));
            const QString target = resolvePath(ncxPath, content.attribute(QStringLiteral("src")));
            if (chapterIndexes.contains(target) && !navigationTitles.contains(target))
                navigationTitles.insert(target, firstDescendant(point, QStringLiteral("text")).text().trimmed());
        }
    }
    for (int i = 0; i < book.chapters.size(); ++i) {
        BookChapter &chapter = book.chapters[i];
        chapter.title = navigationTitles.value(chapter.path);
        const QByteArray bytes = archive.read(chapter.path);
        const QDomDocument xhtml = parseXml(bytes);
        if (xhtml.documentElement().isNull()) {
            chapter.title = QStringLiteral("Section %1").arg(i + 1);
            chapter.html = QStringLiteral("<p>[Chapter unavailable]</p>");
            continue;
        }
        QDomElement body = firstDescendant(xhtml, QStringLiteral("body"));
        if (body.isNull()) body = xhtml.documentElement();
        QString html;
        QString plain;
        for (QDomNode child = body.firstChild(); !child.isNull(); child = child.nextSibling())
            html += renderNode(child, archive, chapter.path, chapterIndexes, plain);
        if (chapter.title.isEmpty()) chapter.title = firstDescendant(body, QStringLiteral("h1")).text().trimmed();
        if (chapter.title.isEmpty() && html.contains(QStringLiteral("<img"))) chapter.title = QStringLiteral("Cover");
        if (chapter.title.isEmpty()) chapter.title = QStringLiteral("Section %1").arg(i + 1);
        chapter.html = html;
        QTextDocument document;
        document.setHtml(html);
        chapter.plainText = document.toPlainText();
        book.contents.append(chapter.title);
    }
    const QByteArray fingerprintSource = opfBytes + QByteArray::number(fileInfo.size()) + QByteArray::number(fileInfo.lastModified().toMSecsSinceEpoch());
    book.fingerprint = QString::fromLatin1(QCryptographicHash::hash(fingerprintSource, QCryptographicHash::Sha256).toHex());
    return book;
}
