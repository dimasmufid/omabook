#include "readercontroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrent>

ReaderController::ReaderController(QObject *parent) : QObject(parent) {
    QSettings settings;
    m_fontSize = qBound(14, settings.value(QStringLiteral("fontSize"), 20).toInt(), 32);
    loadState();
    connect(&m_watcher, &QFutureWatcher<BookData>::finished, this, &ReaderController::finishOpen);
}

QString ReaderController::chapterTitle() const {
    return hasBook() ? m_book.chapters.at(m_index).title : QString();
}
QString ReaderController::chapterHtml() const {
    if (!hasBook()) return {};
    QString html = m_book.chapters.at(m_index).html;
    static const QRegularExpression block(QStringLiteral("<(p|li|blockquote|h[1-6])(?=[ >])"));
    html.replace(block, QStringLiteral("<\\1 style=\"line-height:155%\""));
    return html;
}
int ReaderController::progress() const {
    if (!hasBook()) return 0;
    qint64 before = 0;
    qint64 total = 0;
    for (int i = 0; i < m_book.chapters.size(); ++i) {
        const int length = m_book.chapters.at(i).plainText.size();
        total += length;
        if (i < m_index) before += length;
    }
    before += qMin(m_offset, m_book.chapters.at(m_index).plainText.size());
    return total > 0 ? qBound(0, static_cast<int>(before * 100 / total), 100) : 0;
}

void ReaderController::open(const QUrl &url) {
    if (!url.isLocalFile()) { m_error = QStringLiteral("Choose a local EPUB file."); emit changed(); return; }
    openPath(url.toLocalFile());
}
void ReaderController::openPath(const QString &path) {
    if (m_loading) { m_error = QStringLiteral("A book is still opening."); emit changed(); return; }
    m_error.clear();
    m_loading = true;
    emit changed();
    m_watcher.setFuture(QtConcurrent::run([path]() { return EpubLoader::load(path); }));
}
void ReaderController::finishOpen() {
    BookData loaded = m_watcher.result();
    m_loading = false;
    if (!loaded.error.isEmpty()) { m_error = loaded.error; emit changed(); return; }
    if (hasBook()) savePosition(m_offset);
    m_book = std::move(loaded);
    m_index = 0;
    m_offset = 0;
    const QVariantMap locator = m_positions.value(m_book.path).toMap();
    const QString chapterId = locator.value(QStringLiteral("chapter")).toString();
    for (int i = 0; i < m_book.chapters.size(); ++i)
        if (m_book.chapters.at(i).id == chapterId) { m_index = i; break; }
    if (!locator.isEmpty()) {
        const QString quote = locator.value(QStringLiteral("quote")).toString();
        if (locator.value(QStringLiteral("fingerprint")).toString() == m_book.fingerprint)
            m_offset = qBound(0, locator.value(QStringLiteral("offset")).toInt(), m_book.chapters.at(m_index).plainText.size());
        else if (!quote.isEmpty()) {
            const int match = m_book.chapters.at(m_index).plainText.indexOf(quote);
            if (match >= 0) m_offset = match;
            else m_error = QStringLiteral("This book changed; reading position was adjusted.");
        }
    }
    emit changed();
    QTimer::singleShot(0, this, [this]() { emit restorePosition(m_offset); });
}
void ReaderController::goToChapter(int index) {
    if (!hasBook() || index < 0 || index >= chapterCount()) return;
    if (index == m_index) return;
    savePosition(m_offset);
    m_index = index;
    m_offset = 0;
    savePosition(0);
    emit changed();
    QTimer::singleShot(0, this, [this]() { emit restorePosition(0); });
}
void ReaderController::savePosition(int offset) {
    if (!hasBook()) return;
    const int bounded = qBound(0, offset, m_book.chapters.at(m_index).plainText.size());
    const QVariantMap previous = m_positions.value(m_book.path).toMap();
    if (bounded == m_offset && previous.value(QStringLiteral("chapter")).toString() == m_book.chapters.at(m_index).id)
        return;
    m_offset = bounded;
    m_positions.insert(m_book.path, QVariantMap{
        {QStringLiteral("fingerprint"), m_book.fingerprint},
        {QStringLiteral("chapter"), m_book.chapters.at(m_index).id},
        {QStringLiteral("offset"), m_offset},
        {QStringLiteral("quote"), m_book.chapters.at(m_index).plainText.mid(m_offset, 64)}
    });
    writeState();
    emit changed();
}
QVariantList ReaderController::search(const QString &query) const {
    QVariantList results;
    const QString needle = query.trimmed();
    if (!hasBook() || needle.size() < 2) return results;
    for (int i = 0; i < m_book.chapters.size(); ++i) {
        const QString &text = m_book.chapters.at(i).plainText;
        qsizetype at = 0;
        while ((at = text.indexOf(needle, at, Qt::CaseInsensitive)) >= 0 && results.size() < 200) {
            const int start = qMax(0, static_cast<int>(at) - 45);
            results.append(QVariantMap{{QStringLiteral("chapter"), i},
                                       {QStringLiteral("offset"), static_cast<int>(at)},
                                       {QStringLiteral("title"), m_book.chapters.at(i).title},
                                       {QStringLiteral("excerpt"), text.mid(start, 110).simplified()}});
            at += needle.size();
        }
        if (results.size() >= 200) break;
    }
    return results;
}
void ReaderController::openSearchResult(int chapter, int offset) {
    if (chapter < 0 || chapter >= chapterCount()) return;
    savePosition(m_offset);
    m_index = chapter;
    m_offset = qMax(0, offset);
    savePosition(m_offset);
    emit changed();
    QTimer::singleShot(0, this, [this]() { emit restorePosition(m_offset); });
}
void ReaderController::setFontSize(int size) {
    size = qBound(14, size, 32);
    if (size == m_fontSize) return;
    m_fontSize = size;
    QSettings().setValue(QStringLiteral("fontSize"), size);
    emit changed();
}
void ReaderController::closeBook() {
    if (hasBook()) savePosition(m_offset);
    m_book = {}; m_index = 0; m_offset = 0; emit changed();
}
void ReaderController::openLink(const QString &href) {
    if (href.startsWith(QStringLiteral("book:"))) {
        const QString location = href.mid(5);
        bool okay = false;
        const int chapter = location.section(QLatin1Char('#'), 0, 0).toInt(&okay);
        if (okay) openSearchResult(chapter, 0);
    } else {
        const QUrl url(href);
        if (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"))
            emit externalLinkRequested(url);
    }
}
QString ReaderController::statePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/reading-state.json");
}
void ReaderController::loadState() {
    QFile file(statePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        file.close();
        QFile::rename(statePath(), statePath() + QStringLiteral(".corrupt"));
        return;
    }
    const QVariantMap state = document.object().toVariantMap();
    m_positions = state.value(QStringLiteral("positions")).toMap();
}
void ReaderController::writeState() const {
    const QString path = statePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return;
    const QVariantMap state{{QStringLiteral("version"), 1},
                            {QStringLiteral("positions"), m_positions}};
    file.write(QJsonDocument::fromVariant(state).toJson(QJsonDocument::Compact));
    file.commit();
}
