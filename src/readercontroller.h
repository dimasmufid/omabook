#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QVariantList>

#include "epubloader.h"

class ReaderController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool hasBook READ hasBook NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString author READ author NOTIFY changed)
    Q_PROPERTY(QString chapterTitle READ chapterTitle NOTIFY changed)
    Q_PROPERTY(QString chapterHtml READ chapterHtml NOTIFY changed)
    Q_PROPERTY(QStringList contents READ contents NOTIFY changed)
    Q_PROPERTY(int chapterIndex READ chapterIndex NOTIFY changed)
    Q_PROPERTY(int chapterCount READ chapterCount NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(int fontSize READ fontSize NOTIFY changed)
public:
    explicit ReaderController(QObject *parent = nullptr);
    bool loading() const { return m_loading; }
    bool hasBook() const { return !m_book.chapters.isEmpty(); }
    QString title() const { return m_book.title; }
    QString author() const { return m_book.author; }
    QString chapterTitle() const;
    QString chapterHtml() const;
    QStringList contents() const { return m_book.contents; }
    int chapterIndex() const { return m_index; }
    int chapterCount() const { return m_book.chapters.size(); }
    int progress() const;
    QString error() const { return m_error; }
    int fontSize() const { return m_fontSize; }
    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void openPath(const QString &path);
    Q_INVOKABLE void goToChapter(int index);
    Q_INVOKABLE void nextChapter() { goToChapter(m_index + 1); }
    Q_INVOKABLE void previousChapter() { goToChapter(m_index - 1); }
    Q_INVOKABLE void savePosition(int offset);
    Q_INVOKABLE QVariantList search(const QString &query) const;
    Q_INVOKABLE void openSearchResult(int chapter, int offset);
    Q_INVOKABLE void setFontSize(int size);
    Q_INVOKABLE void closeBook();
    Q_INVOKABLE void openLink(const QString &href);
signals:
    void changed();
    void restorePosition(int offset);
    void externalLinkRequested(const QUrl &url);
private:
    QString statePath() const;
    void loadState();
    void writeState() const;
    void finishOpen();
    BookData m_book;
    QFutureWatcher<BookData> m_watcher;
    QVariantMap m_positions;
    QString m_error;
    int m_index = 0;
    int m_offset = 0;
    int m_fontSize = 20;
    bool m_loading = false;
};
