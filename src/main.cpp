#include "Constants.h"
#include "DashboardWindow.h"
#include "GamepadFilter.h"
#include "PrismBridge.h"
#include "SetupWizard.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>

namespace {

int runDashboard(QApplication& app, PrismBridge& bridge, ThemeManager::Theme theme,
                 const QString& binaryPath, const QString& instancesDir) {
    ThemeManager themes(theme, binaryPath, instancesDir);
    themes.apply();

    GamepadFilter gamepad;
    app.installNativeEventFilter(&gamepad);

    DashboardWindow dashboard(&bridge);
    dashboard.show();
    return app.exec();
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    // Window identity: the X11 WM_CLASS instance name is the lowercased
    // applicationName ("slimelauncher"), matching StartupWMClass in the
    // desktop entry; the desktop file name sets the Wayland app_id so
    // compositors (Hyprland, KWin, GNOME) associate the window correctly.
    QApplication::setApplicationName(QLatin1String(Constants::APP_ID));
    QApplication::setOrganizationName(QLatin1String(Constants::APP_ID));
    QApplication::setApplicationDisplayName(QLatin1String(Constants::APP_NAME));
    QGuiApplication::setDesktopFileName(QLatin1String("slime-launcher"));

    // App icon: the slime block. Looked up from the freedesktop icon theme
    // (installed by CMake into hicolor) with a repo-relative fallback so the
    // binary also works uninstalled from a build tree or release tarball.
    QIcon appIcon = QIcon::fromTheme(QLatin1String("slime-launcher"));
    if (appIcon.isNull()) {
        const QString localIcon = QCoreApplication::applicationDirPath()
                                  + QLatin1String("/../share/icons/slime.png");
        if (QFileInfo::exists(localIcon)) {
            appIcon = QIcon(localIcon);
        } else {
            appIcon = QIcon(QLatin1String(":/icons/slime.png"));
        }
    }
    QApplication::setWindowIcon(appIcon);

    QFont uiFont(QStringLiteral("Inter"));
    uiFont.setPointSize(10);
    QApplication::setFont(uiFont);

    // Slime-owned config: ~/.config/SlimeLauncher/slime.conf
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QLatin1String(Constants::APP_ID), QLatin1String("slime"));

    PrismBridge bridge;

    const QString configuredTheme = settings.value(QLatin1String("General/Theme")).toString();
    const QString configuredBinary = settings.value(QLatin1String("General/BinaryPath")).toString();
    const QString configuredInstances = settings.value(QLatin1String("General/InstancesDir")).toString();

    const bool setupDone = !configuredBinary.isEmpty() && !configuredInstances.isEmpty();
    if (!setupDone) {
        // First run: setup wizard. Theme previews apply instantly; paths are
        // validated through the bridge; Finish persists theme + paths.
        ThemeManager themes(ThemeManager::Theme::Dark, QString(), QString());
        SetupWizard wizard(&bridge, &themes);
        if (wizard.exec() != QDialog::Accepted || wizard.chosenBinaryPath().isEmpty()) {
            return 0;  // cancelled or never validated a native install
        }
        bridge.setBinaryPath(wizard.chosenBinaryPath());
        bridge.setInstancesDir(wizard.chosenInstancesDir());
        themes.persist(wizard.chosenBinaryPath(), wizard.chosenInstancesDir());
        return runDashboard(app, bridge, wizard.chosenTheme(),
                            wizard.chosenBinaryPath(), wizard.chosenInstancesDir());
    }

    bridge.setBinaryPath(configuredBinary);
    bridge.setInstancesDir(configuredInstances);
    const ThemeManager::Theme theme =
        (configuredTheme == QLatin1String("light")) ? ThemeManager::Theme::Light
                                                    : ThemeManager::Theme::Dark;
    return runDashboard(app, bridge, theme, configuredBinary, configuredInstances);
}
