#pragma once

#include "Constants.h"

#include <QObject>
#include <QSettings>
#include <QString>

// Theme state + global QSS. Owns the single stylesheet string applied to the
// application and persists theme + Prism paths to ~/.config/SlimeLauncher/slime.conf.
class ThemeManager : public QObject {
    Q_OBJECT

public:
    enum class Theme { Dark, Light };

    explicit ThemeManager(Theme theme, const QString& binaryPath, const QString& instancesDir, QObject* parent = nullptr);

    Theme theme() const { return m_theme; }
    void setTheme(Theme theme);
    void persist(const QString& binaryPath, const QString& instancesDir);

    // Applies the QSS for the current theme to qApp.
    void apply();

    // Builds the QSS string for a given theme (exposed for tests/tools).
    static QString buildQss(Theme theme);

private:
    Theme m_theme;
    QSettings* m_settings;
};
