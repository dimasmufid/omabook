#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

#include "systemtheme.h"

class OmarchyTheme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString background READ background NOTIFY changed)
    Q_PROPERTY(QString foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString selection READ selection NOTIFY changed)
    Q_PROPERTY(QString muted READ muted NOTIFY changed)
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(qreal textScale READ textScale NOTIFY changed)
public:
    explicit OmarchyTheme(QObject *parent = nullptr);
    QString background() const { return m_background; }
    QString foreground() const { return m_foreground; }
    QString accent() const { return m_accent; }
    QString selection() const { return m_selection; }
    QString muted() const { return m_muted; }
    bool dark() const { return m_dark; }
    qreal textScale() const { return m_system.textScale(); }
signals:
    void changed();
private:
    void reload();
    void armWatcher();
    SystemTheme m_system;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_background;
    QString m_foreground;
    QString m_accent;
    QString m_selection;
    QString m_muted;
    bool m_dark = true;
};
