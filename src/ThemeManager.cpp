#include "ThemeManager.h"

#include <QApplication>

namespace {

// Dark console theme — deep charcoal with electric cyan focus accents.
constexpr char BG_DARK[] = "#0f0f13";
constexpr char SURFACE_DARK[] = "#1a1a24";
constexpr char SURFACE2_DARK[] = "#222230";
constexpr char LINE_DARK[] = "#2a2a38";
constexpr char TEXT_DARK[] = "#f2f4f8";
constexpr char MUTED_DARK[] = "#9a9eb0";
constexpr char ACCENT_DARK[] = "#00f0ff";
constexpr char ACCENT2_DARK[] = "#8a2be2";
constexpr char DANGER_DARK[] = "#ff4d6a";

// Light theme — same layout language, full-contrast surfaces.
constexpr char BG_LIGHT[] = "#f4f5f9";
constexpr char SURFACE_LIGHT[] = "#ffffff";
constexpr char SURFACE2_LIGHT[] = "#e9eaf2";
constexpr char LINE_LIGHT[] = "#d5d7e2";
constexpr char TEXT_LIGHT[] = "#15161c";
constexpr char MUTED_LIGHT[] = "#5c5f70";
constexpr char ACCENT_LIGHT[] = "#0090a8";
constexpr char ACCENT2_LIGHT[] = "#6a1fb8";
constexpr char DANGER_LIGHT[] = "#d9264a";

QString buttonQss(const QString& base, const QString& bg, const QString& fg,
                  const QString& border, const QString& hoverBg, const QString& accent) {
    return QStringLiteral(
               "%1 { background: %2; color: %3; border: 1px solid %4; border-radius: 10px;"
               " padding: 8px 18px; font-weight: 600; }"
               "%1:hover { background: %5; border-color: %6; }"
               "%1:pressed { background: %4; }"
               "%1:focus { border: 2px solid %6; padding: 7px 17px; }"
               "%1:disabled { color: %7; border-color: %4; background: transparent; }")
        .arg(base, bg, fg, border, hoverBg, accent, MUTED_DARK);
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
    const QString danger = dark ? QLatin1String(DANGER_DARK) : QLatin1String(DANGER_LIGHT);

    QString qss;
    qss += QStringLiteral(
               "QWidget { background: %1; color: %2;"
               " font-family: 'Inter','Segoe UI','DejaVu Sans',sans-serif; font-size: 13px; }"
               "QMainWindow, QDialog { background: %1; }"
               "QLabel { background: transparent; }"
               "QLineEdit { background: %3; border: 1px solid %4; border-radius: 10px;"
               " padding: 8px 12px; color: %2; selection-background-color: %5; }"
               "QLineEdit:focus { border: 2px solid %5; padding: 7px 11px; }"
               "QPlainTextEdit { background: %3; border: 1px solid %4; border-radius: 10px; color: %2; }"
               "QListWidget { background: %3; border: 1px solid %4; border-radius: 10px; }"
               "QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px; }"
               "QScrollBar::handle:vertical { background: %4; border-radius: 5px; min-height: 30px; }"
               "QScrollBar::handle:vertical:hover { background: %5; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
               "QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px 4px; }"
               "QScrollBar::handle:horizontal { background: %4; border-radius: 5px; min-width: 30px; }"
               "QMenu { background: %3; border: 1px solid %4; border-radius: 10px; padding: 6px; }"
               "QMenu::item { padding: 7px 22px; border-radius: 6px; }"
               "QMenu::item:selected { background: %6; color: #0a0a0e; }"
               "QMenu::separator { height: 1px; background: %4; margin: 5px 8px; }"
               "QToolTip { background: %3; color: %2; border: 1px solid %4; border-radius: 6px; padding: 5px; }")
               .arg(bg, text, surface, line, accent, accent);

    qss += buttonQss(QStringLiteral("QPushButton"), surface, text, line, surface2, accent);
    qss += buttonQss(QStringLiteral("QPushButton#PrimaryButton"), accent, QStringLiteral("#0a0a0e"),
                     accent, surface2, accent);
    qss += buttonQss(QStringLiteral("QPushButton#DangerButton"), danger, QStringLiteral("#ffffff"),
                     danger, danger, danger);

    // Chrome bars: translucent glass panels floating over the ambient bg.
    const QString barBg = dark ? QStringLiteral("rgba(26, 26, 36, 0.72)")
                               : QStringLiteral("rgba(255, 255, 255, 0.78)");
    qss += QStringLiteral(
               "QWidget#TopBar { background: %1; border: none;"
               " border-bottom: 1px solid %2; }"
               "QWidget#BottomBar { background: %1; border: none;"
               " border-top: 1px solid %2; }"
               "QLineEdit#SearchField { background: %3; border: 1px solid %4; border-radius: 17px;"
               " padding: 7px 16px; }"
               "QLineEdit#SearchField:focus { border: 2px solid %5; padding: 6px 15px; }")
               .arg(barBg, line, surface, line, accent);

    // Cards are fully painter-drawn; QSS only keeps their backgrounds clear.
    qss += QStringLiteral(
               "InstanceCard { background: transparent; }"
               "QFrame#InstanceCard { background: transparent; border: none; }"
               "QFrame#HeroCard { background: transparent; border: none; }"
               "QPlainTextEdit#LogView { font-family: 'JetBrains Mono','DejaVu Sans Mono',monospace;"
               " font-size: 12px; }"
               "QLabel#StatusChip { background: rgba(0, 240, 255, 0.12); color: %2;"
               " border-radius: 7px; padding: 2px 9px; font-weight: 700; }")
               .arg(bg, accent);

    return qss;
}
