#include "ThemeManager.h"

#include <QApplication>

namespace {

constexpr char ACCENT_DARK[] = "#39ff14";
constexpr char BG_DARK[] = "#0a0a0a";
constexpr char SURFACE_DARK[] = "#141414";
constexpr char SURFACE2_DARK[] = "#1a1a1a";
constexpr char LINE_DARK[] = "#262626";
constexpr char TEXT_DARK[] = "#f2f2f2";
constexpr char MUTED_DARK[] = "#8a8a8a";

constexpr char ACCENT_LIGHT[] = "#2ea80a";
constexpr char BG_LIGHT[] = "#fafafa";
constexpr char SURFACE_LIGHT[] = "#ffffff";
constexpr char SURFACE2_LIGHT[] = "#f0f0f0";
constexpr char LINE_LIGHT[] = "#d4d4d4";
constexpr char TEXT_LIGHT[] = "#111111";
constexpr char MUTED_LIGHT[] = "#666666";

QString buttonQss(const QString& base, const QString& bg, const QString& fg,
                  const QString& border, const QString& hoverBg) {
    return QStringLiteral(
               "%1 { background: %2; color: %3; border: 1px solid %4; padding: 8px 18px; }"
               "%1:hover { background: %5; }"
               "%1:pressed { background: %4; }"
               "%1:focus { border: 3px solid %6; }")
        .arg(base, bg, fg, border, hoverBg, ACCENT_DARK);
}

}  // namespace

ThemeManager::ThemeManager(Theme theme, const QString& binaryPath, const QString& instancesDir, QObject* parent)
    : QObject(parent),
      m_theme(theme),
      m_settings(new QSettings(QSettings::IniFormat, QSettings::UserScope,
                               QStringLiteral("SlimeLauncher"), QStringLiteral("slime"), this)) {
    Q_UNUSED(binaryPath);
    Q_UNUSED(instancesDir);
    apply();
}

void ThemeManager::setTheme(Theme theme) {
    m_theme = theme;
    apply();
}

void ThemeManager::apply() {
    qApp->setStyleSheet(buildQss(m_theme));
}

void ThemeManager::persist(const QString& binaryPath, const QString& instancesDir) {
    m_settings->setValue(QStringLiteral("General/Theme"),
                         m_theme == Theme::Dark ? QStringLiteral("dark") : QStringLiteral("light"));
    m_settings->setValue(QStringLiteral("General/BinaryPath"), binaryPath);
    m_settings->setValue(QStringLiteral("General/InstancesDir"), instancesDir);
    m_settings->sync();
}

QString ThemeManager::buildQss(Theme theme) {
    const bool dark = (theme == Theme::Dark);
    const QString bg = dark ? QLatin1String(BG_DARK) : QLatin1String(BG_LIGHT);
    const QString surface = dark ? QLatin1String(SURFACE_DARK) : QLatin1String(SURFACE_LIGHT);
    const QString surface2 = dark ? QLatin1String(SURFACE2_DARK) : QLatin1String(SURFACE2_LIGHT);
    const QString line = dark ? QLatin1String(LINE_DARK) : QLatin1String(LINE_LIGHT);
    const QString text = dark ? QLatin1String(TEXT_DARK) : QLatin1String(TEXT_LIGHT);
    const QString muted = dark ? QLatin1String(MUTED_DARK) : QLatin1String(MUTED_LIGHT);
    const QString accent = dark ? QLatin1String(ACCENT_DARK) : QLatin1String(ACCENT_LIGHT);

    QString qss;
    qss += QStringLiteral(
        "QWidget { background: %1; color: %2; font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; font-size: 13px; }"
        "QMainWindow, QDialog { background: %1; }"
        "QLabel { background: transparent; }"
        "QLineEdit { background: %3; border: 1px solid %4; padding: 7px 10px; color: %2; selection-background-color: %5; }"
        "QLineEdit:focus { border: 3px solid %5; }"
        "QPlainTextEdit { background: %3; border: 1px solid %4; color: %2; }"
        "QScrollBar:vertical { background: %1; width: 10px; }"
        "QScrollBar::handle:vertical { background: %4; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: %5; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }")
        .arg(bg, text, surface, line, accent);

    qss += buttonQss(QStringLiteral("QPushButton"), surface, text, line, surface2);
    qss += buttonQss(QStringLiteral("QPushButton#PrimaryButton"), accent, QStringLiteral("#0a0a0a"), accent, surface2);
    qss += buttonQss(QStringLiteral("QPushButton#DangerButton"), QStringLiteral("#8b0000"), QStringLiteral("#ffffff"),
                     QStringLiteral("#8b0000"), QStringLiteral("#a50000"));

    qss += QStringLiteral(
        "QFrame#InstanceCard { background: %1; border: 1px solid %2; }"
        "QFrame#InstanceCard[selected=\"true\"] { border: 3px solid %3; background: %4; }"
        "QFrame#InstanceCard QLabel { color: %5; }"
        "QFrame#HeroCard { background: %1; border: 1px solid %3; }"
        "QFrame#HeroCard[selected=\"true\"] { border: 3px solid %3; }"
        "QFrame#SideNav { background: %4; border-right: 1px solid %2; }"
        "QPlainTextEdit#LogView { font-family: 'JetBrains Mono','DejaVu Sans Mono',monospace; font-size: 12px; }"
        "QLabel#StatusChip { background: %3; color: %6; padding: 2px 8px; font-weight: 600; }")
        .arg(dark ? QLatin1String(SURFACE_DARK) : QLatin1String(SURFACE_LIGHT), line, accent,
             dark ? QLatin1String(BG_DARK) : QLatin1String(BG_LIGHT),
             dark ? QLatin1String(TEXT_DARK) : QLatin1String(TEXT_LIGHT),
             dark ? QLatin1String("#0a0a0a") : QLatin1String("#ffffff"));

    return qss;
}
