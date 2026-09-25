---
name: Architecture
description: Module map and data flow of Slime Launcher
---

# Architecture

Slime Launcher is a thin native frontend over Prism Launcher. One class owns all Prism interaction; UI widgets only consume parsed result structs.

## Data flow

```text
Prism on-disk data (read-only)
        │
        ▼
   PrismBridge ──── scanInstances() ────► QVector<InstanceCardModel>
        │                                      │
        │                                      ▼
        │                                 DashboardWindow
        │                                 (card grid + hero)
        │
        ├── launchInstance(id) ──► prismlauncher --launch <id>
        ├── openPrismUi()      ──► prismlauncher
        └── tailLog(id)        ──► LogTail (QFileSystemWatcher) ──► LogViewer
```

## Module map

| Module | Files | Responsibility |
|---|---|---|
| `PrismBridge` | `src/PrismBridge.h/.cpp` | Single gateway to Prism data and processes: environment validation, instance scanning, launch routing, log tailing, account reads |
| `GamepadFilter` | `src/GamepadFilter.h/.cpp` | Application-wide event filter reading `/dev/input/js*`, translating to synthetic Qt key events |
| `ThemeManager` | `src/ThemeManager.h/.cpp` | Builds the single global QSS string per theme; persists to `slime.conf` |
| `SetupWizard` | `src/SetupWizard.h/.cpp` | 3-page first-run wizard: theme → paths → accounts |
| `DashboardWindow` | `src/DashboardWindow.h/.cpp` | Main window: top bar, SideNav, card grid, control strip, stacked log/error surfaces |
| `InstanceCard` | `src/InstanceCard.h/.cpp` | Minimalist tile: name, loader chip, version flags, NOW PLAYING badge, painted focus glow |
| `LogViewer` | `src/LogViewer.h/.cpp` | Read-only `QPlainTextEdit` panel with follow mode |
| `ErrorPanel` | `src/ErrorPanel.h/.cpp` | Full-screen failure surface with Retry / Open Prism |
| `SideNav` | `src/SideNav.h/.cpp` | Left rail: navigation actions + read-only account chip |
| `Constants` | `src/Constants.h` | All user-facing strings, path candidates, QSS object-name registry |

## Key invariants

1. **Widgets never spawn processes.** Every `QProcess` call lives in `PrismBridge`.
2. **Widgets never parse Prism files.** They receive `InstanceCardModel` structs.
3. **No exceptions across module boundaries.** Operations return `OpResult { ok, error }`.
4. **One stylesheet.** All colors live in `ThemeManager::buildQss()`; widgets reference stable object names (`#InstanceCard`, `#PrimaryButton`, …).
5. **RAII ownership.** Qt parent-child ownership or C++ members; no unowned raw `new`.

## Threading model

Everything runs on the GUI thread. Disk parsing is short (INI + one small JSON per instance) and scans run on demand (F5). Process spawns are detached; log tailing is event-driven via `QFileSystemWatcher`, so no polling loops or worker threads exist.

## Startup sequence

```text
main()
 ├─ load slime.conf (~/.config/SlimeLauncher/)
 ├─ configured paths valid?
 │    ├─ no  → SetupWizard (theme → paths → accounts) → persist
 │    └─ yes → apply theme QSS
 ├─ install GamepadFilter
 └─ DashboardWindow
      ├─ PrismBridge::scanInstances()
      ├─ rebuild card grid (hero = most recently played)
      └─ event loop
```
