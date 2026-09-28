#include "omarchytheme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace {
QString currentPath() { return QDir::homePath() + QStringLiteral("/.local/state/omarchy/current"); }
QString validColor(const QString &candidate, const QString &fallback) {
    const QColor color(candidate);
    return color.isValid() && candidate.startsWith(QLatin1Char('#')) ? color.name() : fallback;
}
}

OmarchyTheme::OmarchyTheme(QObject *parent) : QObject(parent), m_system(this) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_debounce, &QTimer::timeout, this, &OmarchyTheme::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_system, &SystemTheme::darkModeChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_system, &SystemTheme::textScaleChanged, this, &OmarchyTheme::changed);
    reload();
}

void OmarchyTheme::armWatcher() {
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty()) m_watcher.removePaths(watched);
    const QString current = currentPath();
    const QString theme = current + QStringLiteral("/theme");
    const QString colors = theme + QStringLiteral("/colors.toml");
    const QString state = QDir::homePath() + QStringLiteral("/.local/state/omarchy");
    for (const QString &path : {state, current, theme, colors})
        if (QFile::exists(path)) m_watcher.addPath(path);
}

void OmarchyTheme::reload() {
    m_dark = m_system.darkMode();
    m_background = m_dark ? QStringLiteral("#101010") : QStringLiteral("#ffffff");
    m_foreground = m_dark ? QStringLiteral("#eeeeee") : QStringLiteral("#222324");
    m_accent = m_dark ? QStringLiteral("#5584aa") : QStringLiteral("#2077b2");
    m_selection = m_dark ? QStringLiteral("#186a9a") : QStringLiteral("#bbdcf0");
    m_muted = m_dark ? QStringLiteral("#999999") : QStringLiteral("#666666");
    QString mode;
    QFile file(currentPath() + QStringLiteral("/theme/colors.toml"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1Char('#'))) continue;
            const int equal = line.indexOf(QLatin1Char('='));
            if (equal < 0) continue;
            const QString key = line.left(equal).trimmed();
            QString value = line.mid(equal + 1).trimmed();
            if (value.size() >= 2 && (value.front() == QLatin1Char('"') || value.front() == QLatin1Char('\'')))
                value = value.mid(1, value.size() - 2);
            if (key == QStringLiteral("mode")) mode = value;
            else if (key == QStringLiteral("background")) m_background = validColor(value, m_background);
            else if (key == QStringLiteral("foreground")) m_foreground = validColor(value, m_foreground);
            else if (key == QStringLiteral("accent")) m_accent = validColor(value, m_accent);
            else if (key == QStringLiteral("selection")) m_selection = validColor(value, m_selection);
            else if (key == QStringLiteral("muted")) m_muted = validColor(value, m_muted);
        }
    }
    if (mode == QStringLiteral("dark")) m_dark = true;
    else if (mode == QStringLiteral("light")) m_dark = false;
    else m_dark = QColor(m_background).lightnessF() < 0.5;
    armWatcher();
    emit changed();
}
