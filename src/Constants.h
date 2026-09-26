#pragma once

#include <QColor>
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
constexpr char APP_VERSION[] = "2.0.0";

// ---- Design tokens (mirrored by ThemeManager QSS) -------------------------
// Deep charcoal console theme with electric cyan focus and violet support
// accents. Widgets never hardcode colors; these constants are the single
// source for painted surfaces and the ThemeManager exposes them to QSS.
inline const QColor COLOR_BG = QColor(0x0f, 0x0f, 0x13);
inline const QColor COLOR_BG_DEEP = QColor(0x0a, 0x0a, 0x0e);
inline const QColor COLOR_SURFACE = QColor(0x1a, 0x1a, 0x24);
inline const QColor COLOR_SURFACE_HI = QColor(0x22, 0x22, 0x30);
inline const QColor COLOR_LINE = QColor(0x2a, 0x2a, 0x38);
inline const QColor COLOR_TEXT = QColor(0xf2, 0xf4, 0xf8);
inline const QColor COLOR_MUTED = QColor(0x9a, 0x9e, 0xb0);
inline const QColor COLOR_ACCENT = QColor(0x00, 0xf0, 0xff);   // electric cyan
inline const QColor COLOR_ACCENT2 = QColor(0x8a, 0x2b, 0xe2);  // violet
inline const QColor COLOR_DANGER = QColor(0xff, 0x4d, 0x6a);

constexpr int CARD_RADIUS = 12;      // rounded corner radius on cards
constexpr int BUTTON_RADIUS = 10;    // rounded corner radius on buttons
constexpr int CARD_COLUMNS_MIN = 3;  // responsive grid clamps
constexpr int CARD_COLUMNS_MAX = 8;

// ---- Poster card geometry (2:3 aspect ratio) ------------------------------
constexpr int CARD_WIDTH = 216;
constexpr int CARD_HEIGHT = 324;  // 216 * 3 / 2
constexpr int GRID_SPACING = 22;
constexpr int GRID_MARGIN = 28;
constexpr qreal FOCUS_SCALE = 1.08;  // active card scale-up transform

// ---- Object-name registry (stable — QSS keys on these) --------------------
constexpr char OBJ_HERO_CARD[] = "HeroCard";
constexpr char OBJ_INSTANCE_CARD[] = "InstanceCard";
constexpr char OBJ_BUTTON[] = "SlimeButton";
constexpr char OBJ_PRIMARY_BUTTON[] = "PrimaryButton";
constexpr char OBJ_DANGER_BUTTON[] = "DangerButton";
constexpr char OBJ_LOG_VIEW[] = "LogView";
constexpr char OBJ_STATUS_CHIP[] = "StatusChip";
constexpr char OBJ_SEARCH[] = "SearchField";
constexpr char OBJ_TOP_BAR[] = "TopBar";
constexpr char OBJ_BOTTOM_BAR[] = "BottomBar";

// ---- Prism discovery candidates -------------------------------------------
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

// ---- Slime asset pipeline --------------------------------------------------
constexpr char SLIME_DIR_NAME[] = "slimelauncher";  // per-instance asset folder
constexpr char CARD_ASSET_NAME[] = "card.png";
constexpr char BACKGROUND_ASSET_NAME[] = "background.png";
constexpr char METADATA_ASSET_NAME[] = "metadata.json";
constexpr char METADATA_ARTWORK_KEY[] = "artwork";
constexpr char METADATA_ARTWORK_CUSTOM[] = "custom";
constexpr char METADATA_ARTWORK_DEFAULT[] = "default";
constexpr char METADATA_SOURCE_KEY[] = "source";
constexpr char METADATA_UPDATED_KEY[] = "updated";

// Embedded default poster cards (copied on first scan when an instance has no
// custom card.png yet). Keep in sync with assets/default_cards + resources.qrc.
constexpr const char* DEFAULT_CARDS[] = {
    ":/cards/card1.png",
    ":/cards/card2.png",
    ":/cards/card3.jpg",
    ":/cards/card4.jpg",
    ":/cards/card5.jpg",
};
constexpr int DEFAULT_CARD_COUNT = 5;

// ---- Timing ----------------------------------------------------------------
constexpr int REFRESH_DEBOUNCE_MS = 250;
constexpr int GAMEPAD_DEADZONE_PCT = 35;
constexpr int GAMEPAD_REPEAT_DELAY_MS = 450;
constexpr int GAMEPAD_REPEAT_INTERVAL_MS = 120;
constexpr int LOG_TAIL_POLL_MS = 400;
constexpr int LOG_MAX_BLOCKS = 4000;
constexpr int CLOCK_TICK_MS = 1000;
constexpr int FOCUS_ANIMATION_MS = 140;

// ---- Window -----------------------------------------------------------------
constexpr QSize DEFAULT_WIN_SIZE(1440, 900);
constexpr QSize MIN_WIN_SIZE(1024, 640);

// ---- User-facing strings -----------------------------------------------------
constexpr char MSG_NO_PRISM[] = "No native Prism Launcher installation was found on this system.";
constexpr char MSG_FLATPAK[] = "A Flatpak installation of Prism Launcher was detected. Slime Launcher requires a native (non-Flatpak) install.";
constexpr char MSG_NO_INSTANCES[] = "The instances directory exists but contains no valid instances.";
constexpr char MSG_SPAWN_FAIL[] = "Failed to spawn the Prism CLI process.";
constexpr char MSG_NO_LOG[] = "No log file is available for this instance yet.";
constexpr char MSG_WAITING_LOG[] = "Waiting for log file…";
constexpr char MSG_NOTHING_SELECTED[] = "Select a game to see its details here.";
constexpr char MSG_EMPTY_GRID[] = "No instances found. Open Prism to create one, then refresh.";
constexpr char MSG_ARTWORK_PICK_TITLE[] = "Choose card artwork";
constexpr char MSG_ARTWORK_PICK_FILTER[] = "Images (*.png *.jpg *.jpeg *.webp *.bmp)";
constexpr char MSG_ARTWORK_FAILED[] = "Could not process the selected image.";
constexpr char MSG_CONTEXT_PLAY[] = "Play";
constexpr char MSG_ACCOUNT_NONE[] = "No account";
constexpr char MSG_ACCOUNT_IN[] = "Logged in as %1";
constexpr char MSG_ACCOUNT_OFF[] = "Account: %1";
constexpr char MSG_CONTEXT_EDIT[] = "Edit in Prism";
constexpr char MSG_CONTEXT_ARTWORK[] = "Change Card Artwork…";
constexpr char MSG_CONTEXT_LOGS[] = "View Logs";

}  // namespace V1
}  // namespace Constants
