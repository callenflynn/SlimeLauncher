# Slime Launcher — Development Instructions

## Project Identity
Slime Launcher is a native C++20 / Qt 6 (Widgets) controller-navigable, console-style launcher UI for **native (non-Flatpak) Prism Launcher** installations on Linux. It is a frontend and orchestration layer only. **Prism Launcher is the source of truth** — Slime Launcher never mutates Prism data. Its only write domains inside a Prism install are `slimelauncher/` asset folders (see *Artwork pipeline* below).

## Contributor Onboarding
Before making any change, read [`CONTRIBUTING.md`](../CONTRIBUTING.md) at the repository root. It defines the contribution workflow, the hard data-integrity rules summarized here, and the release process. AI agents must follow it in addition to this document.

## Build System & Commands

### Prerequisites (Arch Linux)
```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base
```

### Configure
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Build
```bash
cmake --build build
```

### Debug build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Run
```bash
./build/bin/slime-launcher
```

### Lint / Analyze
```bash
clang-tidy -p build src
cppcheck --enable=warning,performance,portability --inline-suppr src
```
Warnings are treated as errors (`-Werror`) in CI-style builds; local builds allow warnings but the codebase should compile warning-free at `-Wall -Wextra`.

## Language & Standard
- C++20, Qt 6.4+ (compiles against 6.2+).
- Qt Widgets (not QML) for the UI — lower memory footprint, native focus model, direct widget-level spatial navigation. The "rounded console" look is achieved with painter-drawn widgets + QSS, not Qt Quick.
- No exceptions are thrown across module boundaries; errors are returned as parsed result structs.

## Coding Rules
- **RAII everywhere.** Every `QWidget`, `QProcess`, and file handle is owned via smart pointers or Qt parent-child ownership. No raw `new` without an owner.
- **No placeholder comments.** Every line ships or is deleted.
- **strict warning policy**: `-Wall -Wextra -Wpedantic` on; no code may introduce a warning.
- Strings rendered to the user live in constants (`src/Constants.h`); no magic strings in widgets.
- Every `QProcess` spawn must go through `PrismBridge` — widgets never spawn processes directly.
- All instance data parsing lives in `PrismBridge`. Widgets receive parsed `InstanceCardModel` structs, never raw file paths to mutate.
- Header-only small helpers are allowed; larger implementations go in `.cpp` files.
- Keep widget classes focused: one widget per file, named for the widget.

## QSS Styling Rules
- All styling flows from `ThemeManager`, which compiles one QSS string per theme (`SlimeDark` / `SlimeLight`).
- Colors are defined as hex literals in `ThemeManager.cpp` — never hardcode colors in widgets. Painter-drawn surfaces use the `Constants::COLOR_*` tokens, which mirror the QSS values; keep the two in sync.
- The object names that QSS keys on are stable: `#HeroCard`, `#InstanceCard`, `#SlimeButton`, `#PrimaryButton`, `#DangerButton`, `#SideNav`, `#LogView`, `#StatusChip`, `#TopBar`, `#BottomBar`, `#SearchField` — do not rename widgets without updating `ThemeManager`.
- Dark is default: `#0f0f13` base, `#1a1a24` surfaces, `#00f0ff` electric-cyan focus accent, `#8a2be2` violet secondary.
- Light theme is full-contrast, near-white on near-black text, same layout with muted cyan/violet accents.
- **Rounded design language**: 14px radius on card surfaces, 10px on buttons/inputs/menus, 17px pill search field. Do not reintroduce hard-edged surfaces.

## Artwork Pipeline Rules (`ImageProcessor`)
- `src/ImageProcessor.h/.cpp` owns everything under `<instance>/slimelauncher/`: `card.png`, `background.png` (reserved), `metadata.json`. No other code may write inside an instance directory.
- Target poster ratio is **exactly 2:3**, clamped to **300x450–600x900 px**. Processing order: EXIF-orientation-aware load → center crop to 2:3 → height clamp snapped to a multiple of 3 (guarantees an exact integer 2:3 pair; no 1px drift).
- Default seeding is **deterministic** (SHA-256 of the instance id over the five bundled `:/cards/` resources): rescans must never reshuffle a card's default artwork.
- Custom uploads always flow through `PrismBridge::setInstanceCardArtwork` → `ImageProcessor::importCardArtwork`, which writes `metadata.json` with `artwork: custom`.
- Card widgets fall back gracefully: `slimelauncher/card.png` → Prism icon (cropped in memory) → default poster → painted fallback. Missing art must never blank a card or crash.

## Desktop Integration Rules
- `packaging/slime-launcher.desktop` is the contract with Linux menus (Rofi, dmenu, GNOME, KDE, Hyprland): `Exec=slimelauncher`, `Icon=slime-launcher`, `Categories=Game;Utility;`, `StartupWMClass=slimelauncher`.
- CMake must install the entry to `share/applications`, the hicolor icons (PNG 16–256px + scalable SVG) to `share/icons/hicolor`, the `bin/slimelauncher` symlink, and the unpacked default cards to `share/slime-launcher/default_cards/`.
- `main.cpp` exports `QT_APPLICATION_NAME=slimelauncher` so WM_CLASS matches `StartupWMClass`. Keep binary name, desktop entry, and WM class aligned when renaming.

## State Management Lifecycle Rules
1. **Prism is source of truth.** On every dashboard refresh (`F5`), `PrismBridge::scanInstances()` re-reads instance JSON/INI from disk and rebuilds the model. The UI never caches beyond a single refresh cycle. The same pass provisions `slimelauncher/` assets via `ImageProcessor::ensureAssets` (create folder, seed default poster if `card.png` is absent).
2. **Slime config** (`~/.config/SlimeLauncher/slime.conf`) stores theme + validated Prism paths. Written only through `ThemeManager`/`SetupWizard` completion.
3. **Accounts are read-only** to Slime: `accounts.json` is parsed for display only; sign-in flows delegate to Prism.
4. A wizard must be completed before the dashboard shows. `SetupWizard` completes only when a valid non-Flatpak Prism binary + instance dir is confirmed.
5. On refresh, cards that disappear from disk are dropped from the model; new ones are appended in name-sorted order.
6. Gamepad input never mutates state directly — it emits focus-move/select requests that route through the same handlers as keyboard/mouse. Arrow/D-pad navigation over the poster grid is column-aware (bounded moves at row edges).
