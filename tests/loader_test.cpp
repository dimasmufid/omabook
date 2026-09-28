#include "../src/epubloader.h"

#include <QGuiApplication>
#include <QDebug>
#include <cstdio>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (argc < 2) return 2;
    const QString root = QString::fromLocal8Bit(argv[1]);
    for (int version : {2, 3}) {
        const BookData book = EpubLoader::load(root + QStringLiteral("/sample-epub%1.epub").arg(version));
        if (!book.error.isEmpty() || book.title != QStringLiteral("Omabook Sample")
            || book.author != QStringLiteral("Dimas Mufid") || book.chapters.size() != 2
            || book.contents.size() != 2 || !book.chapters.at(0).html.contains(QStringLiteral("book:1#second"))
            || !book.chapters.at(1).plainText.contains(QStringLiteral("Reading is a simple pleasure"))) {
            qCritical() << "EPUB fixture failed" << version << book.error;
            return 1;
        }
    }
    const BookData unsafe = EpubLoader::load(root + QStringLiteral("/unsafe-path.epub"));
    if (!unsafe.error.contains(QStringLiteral("unsafe"))) {
        qCritical() << "Unsafe path was accepted" << unsafe.error;
        return 1;
    }
    for (int arg = 2; arg < argc; ++arg) {
        const BookData book = EpubLoader::load(QString::fromLocal8Bit(argv[arg]));
        int readable = 0;
        for (const BookChapter &chapter : book.chapters)
            if (!chapter.plainText.trimmed().isEmpty() || chapter.html.contains(QStringLiteral("<img"))) ++readable;
        std::fprintf(stdout, "%s: %d/%d readable chapters; error=%s\n", argv[arg], readable,
                     static_cast<int>(book.chapters.size()), qPrintable(book.error));
        if (!book.error.isEmpty() || readable < book.chapters.size() - 1) return 1;
    }
    qInfo() << "EPUB 2, EPUB 3, navigation, text, links, and unsafe path checks passed";
    return 0;
}
