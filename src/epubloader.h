#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

struct BookChapter {
    QString id;
    QString path;
    QString title;
    QString html;
    QString plainText;
};

struct BookData {
    QString path;
    QString title;
    QString author;
    QString fingerprint;
    QVector<BookChapter> chapters;
    QStringList contents;
    QString error;
};

class EpubLoader {
public:
    static BookData load(const QString &path);
};
