# Slime Launcher — Architecture & Agent Reference

## Agent Onboarding
Start with [`CONTRIBUTING.md`](../CONTRIBUTING.md) at the repository root — it defines the contribution workflow, the non-negotiable data-integrity rules, and the automated release process (tag `v*.*.*` → `.github/workflows/release.yml`). Then read this file for the full architecture map.

## What This App Is
A native C++20/Qt 6 (Widgets) controller-navigable launcher frontend for **native Prism Launcher** on Arch Linux, styled as a high-contrast brutalist/minimalist console UI (XMCL-inspired).

## Non-Negotiable Boundaries

1. **Prism as Source of Truth.** Slime Launcher is purely a frontend and orchestration interface. It treats the local Prism directory configuration and instance metadata as authoritative.
2. **Read vs. Write Rules.**
   - Slime *may* read instance metadata (JSON/INI) directly from Prism paths using `QJsonDocument` / `QSettings`.
   - Slime *must never* write or mutate raw core configuration blocks (`instances/*/instance.cfg`, `accounts.json`, `mmc-pack.json`, `prismlauncher.cfg`) — that would corrupt Prism's internal state machine.
   - Slime writes exactly one file: `~/.config/SlimeLauncher/slime.conf` (theme + validated Prism paths).
   - Accounts (`accounts.json`) are read-only to Slime. Sign-in flows delegate to Prism.
3. **Execution Routing via CLI.** All launch operations route through Prism's standard CLI backend, spawned asynchronously via `QProcess`:
   ```cpp
   QProcess::startDetached("prismlauncher", {"--launch", instanceId});
   ```
   Widgets never spawn processes directly — everything goes through `PrismBridge`.
4. **Native-only target.** Arch Linux, packages from official repos (`extra/prismlauncher`) or AUR (`prismlauncher-bin`). Flatpak is explicitly unsupported (sandbox isolation blocks inter-process communication and file monitoring). A Flatpak install must be detected and rejected with a descriptive warning during setup.
5. **Fallback Safety.** Any operation without a stable, safe programmatic abstraction (deep instance editing, Microsoft account sign-in, mod management) must provide an explicit fallback button that opens the native Prism desktop UI instead of attempting it.
6. **Minimum Prism version: 9.0.** The CLI verbs and metadata formats Slime reads are those of Prism 9.x.

## Data Ownership Hierarchy

```
Prism Launcher (authoritative owner)
├── ~/.local/share/PrismLauncher/            (or ~/.local/share/prism-launcher/ — resolved at setup)
│   ├── instances/
│   │   └── <id>/
│   │       ├── instance.cfg                 — QSettings INI: name, iconKey, lastLaunchTime (read-only)
│   │       ├── mmc-pack.json                — loader/version JSON (read-only)
│   │       └── .minecraft/logs/latest.log   — live log tail target (read-only)
│   ├── accounts.json                        — MS account sessions (read-only)
│   ├── icons/                               — instance icons (read-only)
│   └── prismlauncher.cfg                    — Prism global config (never read by Slime)
└── /usr/bin/prismlauncher                   — native binary (execution via CLI only)
```

Slime-owned data:
```
~/.config/SlimeLauncher/slime.conf           — theme, Prism binary path, instances dir path
```

## Module Map

### Backend (`src/`)
- **`PrismBridge`** (`PrismBridge.h/.cpp`) — the single gateway to Prism on-disk data and processes.
  - `scanInstances()` — walks `instances/` with `QDir`, parses `instance.cfg` (via `QSettings` IniFormat) and `mmc-pack.json` (via `QJsonDocument`), resolves icon files, computes last-played and running state.
  - `validateEnvironment()` — locates the native binary via candidate paths + `PATH` lookup; scans candidate data roots; detects Flatpak installs and reports them as hard errors with remediation text.
  - `launchInstance(id)` / `openPrismUi()` — async `QProcess::startDetached` operations returning success/error.
  - `isInstanceRunning(id)` — checks Prism's `.minecraft/logs/latest.log` mtime recency plus Prism lock files to compute the NOW PLAYING state (read-only heuristic).
  - `tailLog(instanceId, callback)` — background `QFileSystemWatcher` on the instance log for the LogViewer.
  - `readAccounts()` — parses `accounts.json` for display only.
  - Emits Qt signals; owns no UI. All parsing returns result structs; no exceptions cross module boundaries.
- **`GamepadFilter`** (`GamepadFilter.h/.cpp`) — `QObject` event filter installed on the whole application.
  - Reads `/dev/input/js*` via low-level `open()`/`read()` fds integrated with the Qt event loop through `QSocketNotifier` (no SDL2 dependency).
  - Maps gamepad → synthetic Qt key events posted to the focus widget: A → Enter/Return, B → Escape, D-pad + left stick → arrows, Start → F5, Y → Tab.
  - Analog stick deadzone 0.35; hold-repeat with acceleration after 450 ms.
  - Up to 4 gamepads; hot-plug re-enumeration on each event loop pass.
- **`ThemeManager`** (`ThemeManager.h/.cpp`) — owns theme state and the single global QSS string.
  - Builds QSS for `Theme::Dark` (default) and `Theme::Light`; applied via `QApplication::setStyleSheet`.
  - Persists theme + Prism paths to `~/.config/SlimeLauncher/slime.conf` (`QSettings`).
  - QSS keys on stable object names — see CLAUDE.md for the registry.
- **`Constants.h`** — user-facing strings, path candidates, timing values, QSS object-name registry.

### UI (`src/ui/`)
- **`SetupWizard`** (`SetupWizard.h/.cpp`) — `QWizard`, 3 pages:
  1. **Theme Selection** — dark/light live preview swatches; global QSS already applied, so selection previews instantly; saved on finish.
  2. **Prism Path Validation** — auto-scan of standard paths with a live result panel; Flatpak-detection warning block (descriptive, native-package remediation); manual path entry for custom installs; Finish enabled only on validated native install.
  3. **Account & Auth Interface** — read-only account list from `accounts.json` (name, type, last-sync), "Open Prism to sign in / manage accounts" fallback button. Slime never touches credentials.
- **`DashboardWindow`** (`DashboardWindow.h/.cpp`) — console-style main window.
  - Top bar: title, search filter, refresh indicator, clock.
  - Side nav: All / Refresh / Open Prism / account chip (read-only).
  - Center: scrollable instance card grid (custom widget layout, not QListView) with a hero card (most recently played) on top.
  - Bottom control strip: Play / Logs / Open Prism bound to the selected instance.
  - Shortcuts: Enter=play, F5=refresh, Escape=clear search/back, L=logs, O=open prism.
- **`InstanceCard`** (`InstanceCard.h/.cpp`) — minimalist high-contrast tile: name, version flag strip, loader chip (Fabric/Forge/NeoForge/Quilt/Vanilla), last-played relative time, NOW PLAYING state.
  - Selected state = 3px neon ring + painted glow (dynamic property `selected="true"`, glow painted in `paintEvent` because QSS has no box-shadow).
  - Click/Enter = select, double-click = play. Emits `activated(InstanceInfo)` / `selected(InstanceInfo)`.
- **`LogViewer`** (`LogViewer.h/.cpp`) — panel with read-only `QPlainTextEdit` (maximumBlockCount 4000), follow toggle (auto-scroll pauses on manual scroll-up), copy/clear actions, status line.
  - Backed by `PrismBridge::tailLog` watching `latest.log` (and legacy `1.log`). Handles missing-file gracefully ("waiting for log file…").
  - Strictly read-only — no write operations.
- **`ErrorPanel`** (`ErrorPanel.h/.cpp`) — full-screen failure surface for fatal paths (no Prism install, no instances dir, spawn failure).
  - Shows error code + human description + Retry / Open Prism actions. Stacked over the dashboard so navigation state survives a retry.
- **`SideNav`** (`SideNav.h/.cpp`) — left rail: All / Refresh / Open Prism buttons + read-only account chip. Fully gamepad/keyboard navigable via the shared focus model.

## UI/UX Specifications
- High-contrast brutalist/minimalist: flat `#0a0a0a` backgrounds, 1px `#262626` hairlines, neon `#39ff14` accent used sparingly (focus, selection, primary CTA).
- Hard edges everywhere — no `border-radius` on primary surfaces.
- Hero card ≈ 2× tile height; grid tiles are uniform 220×140.
- Focus ring = 3px solid neon + painted 8px soft glow; rendered by widgets, never by QSS `outline`.
- Cards show: name, version flag strip, loader chip, last-played relative time, NOW PLAYING badge.
- All interactive elements have hover, focus, and pressed states; keyboard and gamepad drive the identical focus model.
- Light theme: near-white background, near-black text, same neon accent family, same layout.

## Error Handling Philosophy
- Every `PrismBridge` call returns a typed result (value + error string) — never throws across boundaries.
- Missing paths, unreadable JSON, unknown loaders degrade gracefully: card renders with "Unknown" chip, never crashes.
- Spawn failures surface in `ErrorPanel` with the failed command and exit context.
- Gamepad device loss is silent; keyboard/mouse always remain functional.
