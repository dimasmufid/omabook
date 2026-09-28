#include <QGuiApplication>
#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <cstdio>

#include "omarchytheme.h"
#include "readercontroller.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("omabook"));
    app.setOrganizationName(QStringLiteral("Dimas Mufid"));
    app.setOrganizationDomain(QStringLiteral("dimasmufid.dev"));
    app.setDesktopFileName(QStringLiteral("omabook"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("omabook")));
    QQuickStyle::setStyle(QStringLiteral("Material"));

    ReaderController reader;
    OmarchyTheme theme;
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) std::fprintf(stderr, "%s\n", qPrintable(warning.toString()));
    });
    engine.rootContext()->setContextProperty(QStringLiteral("reader"), &reader);
    engine.rootContext()->setContextProperty(QStringLiteral("omarchyTheme"), &theme);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty()) { qCritical() << "Omabook UI failed to load"; return 1; }
    if (app.arguments().size() > 1)
        QTimer::singleShot(0, &reader, [&reader, path = app.arguments().at(1)]() { reader.openPath(path); });
    return app.exec();
}
