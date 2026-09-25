# Slime Launcher — Development Instructions

## Project Identity
Slime Launcher is a native C++20 / Qt 6 (Widgets) controller-navigable, console-style launcher UI for **native (non-Flatpak) Prism Launcher** installations on Arch Linux. It is a frontend and orchestration layer only. **Prism Launcher is the source of truth** — Slime Launcher never mutates Prism data.

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
- Qt Widgets (not QML) for the UI — lower memory footprint, native focus model, direct widget-level spatial navigation.
- No exceptions are thrown across module boundaries; errors are returned as parsed result structs.

## Coding Rules
- **RAII everywhere.** Every `QWidget`, `QProcess`, and file handle is owned via smart pointers or Qt parent-child ownership. No raw `new` without an owner.
- **No placeholder comments.** Every line ships or is deleted.
- **strict warning policy**: `-Wall -Wextra -Wpedantic` on; no code may introduce a warning.
- Strings rendered to the user live in constants (`src/Constants.h`); no magic strings in widgets.
- Every `QProcess` spawn must go through `PrismBridge` — widgets never spawn processes directly.
- All instance data parsing lives in `PrismBridge`. Widgets receive parsed `InstanceInfo` structs, never raw file paths to mutate.
- Header-only small helpers are allowed; larger implementations go in `.cpp` files.
- Keep widget classes focused: one widget per file, named for the widget.

## QSS Styling Rules
- All styling flows from `ThemeManager`, which compiles one QSS string per theme (`SlimeDark` / `SlimeLight`).
- Colors are defined as hex literals in `ThemeManager.cpp` — never hardcode colors in widgets.
- The object names that QSS keys on are stable: `#HeroCard`, `#InstanceCard`, `#SlimeButton`, `#PrimaryButton`, `#DangerButton`, `#SideNav`, `#LogView`, `#StatusChip` — do not rename widgets without updating `ThemeManager`.
- Dark is default: `#0a0a0a` base with `#39ff14`-family neon highlights.
- Light theme is full-contrast, near-white on near-black text, same neon accent family, no gradients.
- Do not add `border-radius` to primary surfaces (brutalist/minimalist aesthetic — hard edges).

## State Management Lifecycle Rules
1. **Prism is source of truth.** On every dashboard refresh (`F5`), `PrismBridge::scanInstances()` re-reads instance JSON/INI from disk and rebuilds the model. The UI never caches beyond a single refresh cycle.
2. **Slime config** (`~/.config/SlimeLauncher/slime.conf`) is the only file Slime writes. It stores theme + validated Prism paths. Written only through `ThemeManager`/`SetupWizard` completion.
3. **Accounts are read-only** to Slime: `accounts.json` is parsed for display only; sign-in flows delegate to Prism.
4. A wizard must be completed before the dashboard shows. `SetupWizard` completes only when a valid non-Flatpak Prism binary + instance dir is confirmed.
5. On refresh, cards that disappear from disk are dropped from the model; new ones are appended in name-sorted order.
6. Gamepad input never mutates state directly — it emits focus-move/select requests that route through the same handlers as keyboard/mouse.
