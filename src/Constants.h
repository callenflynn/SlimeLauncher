#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Constants {
inline namespace V1 {

constexpr char APP_NAME[] = "Slime Launcher";
constexpr char APP_ID[] = "SlimeLauncher";
constexpr char APP_VERSION[] = "1.0.0";

constexpr char OBJ_HERO_CARD[] = "HeroCard";
constexpr char OBJ_INSTANCE_CARD[] = "InstanceCard";
constexpr char OBJ_BUTTON[] = "SlimeButton";
constexpr char OBJ_PRIMARY_BUTTON[] = "PrimaryButton";
constexpr char OBJ_DANGER_BUTTON[] = "DangerButton";
constexpr char OBJ_SIDE_NAV[] = "SideNav";
constexpr char OBJ_LOG_VIEW[] = "LogView";
constexpr char OBJ_STATUS_CHIP[] = "StatusChip";

constexpr const char* PRISM_BIN_CANDIDATES[] = {
    "/usr/bin/prismlauncher",
    "/usr/local/bin/prismlauncher",
    "/opt/prismlauncher/bin/prismlauncher",
};

constexpr const char* PRISM_DATA_CANDIDATES[] = {
    "~/.local/share/PrismLauncher",
    "~/.local/share/prism-launcher",
    "~/.local/share/PolyMC",
};

constexpr int REFRESH_DEBOUNCE_MS = 250;
constexpr int GAMEPAD_DEADZONE_PCT = 35;
constexpr int GAMEPAD_REPEAT_DELAY_MS = 450;
constexpr int GAMEPAD_REPEAT_INTERVAL_MS = 120;
constexpr int LOG_TAIL_POLL_MS = 400;
constexpr int LOG_MAX_BLOCKS = 4000;
constexpr int CLOCK_TICK_MS = 1000;
constexpr int CARD_SPACING = 14;
constexpr int GRID_MAX_COLUMNS = 6;
constexpr QSize CARD_SIZE(220, 140);
constexpr QSize DEFAULT_WIN_SIZE(1280, 800);
constexpr QSize MIN_WIN_SIZE(960, 600);

constexpr char MSG_NO_PRISM[] = "No native Prism Launcher installation was found on this system.";
constexpr char MSG_FLATPAK[] = "A Flatpak installation of Prism Launcher was detected. Slime Launcher requires a native (non-Flatpak) install.";
constexpr char MSG_NO_INSTANCES[] = "The instances directory exists but contains no valid instances.";
constexpr char MSG_SPAWN_FAIL[] = "Failed to spawn the Prism CLI process.";
constexpr char MSG_NO_LOG[] = "No log file is available for this instance yet.";
constexpr char MSG_WAITING_LOG[] = "Waiting for log file…";
constexpr char MSG_NOTHING_SELECTED[] = "Select an instance to see its details here.";
constexpr char MSG_EMPTY_GRID[] = "No instances found. Open Prism to create one, then refresh.";

}  // namespace V1
}  // namespace Constants
