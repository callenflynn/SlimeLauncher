#include "Constants.h"
#include "DashboardWindow.h"
#include "GamepadFilter.h"
#include "PrismBridge.h"
#include "SetupWizard.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QFont>
#include <QSettings>

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
    QApplication::setApplicationName(QLatin1String(Constants::APP_ID));
    QApplication::setOrganizationName(QLatin1String(Constants::APP_ID));
    QApplication::setApplicationDisplayName(QLatin1String(Constants::APP_NAME));
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
