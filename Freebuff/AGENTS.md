# Slime Launcher — Architecture & Agent Reference

## Agent Onboarding
Start with [`CONTRIBUTING.md`](../CONTRIBUTING.md) at the repository root — it defines the contribution workflow, the non-negotiable data-integrity rules, and the automated release process (tag `v*.*.*` → `.github/workflows/release.yml`). Then read this file for the full architecture map.

## What This App Is
A native C++20/Qt 6 (Widgets) controller-navigable launcher frontend for **native Prism Launcher** on Linux, styled as a modern console dashboard (Playnite / Steam Big Picture inspired): rounded 2:3 poster cards, charcoal glassmorphism surfaces, electric-cyan focus animations, and per-instance artwork.

## Non-Negotiable Boundaries

1. **Prism as Source of Truth.** Slime Launcher is purely a frontend and orchestration interface. It treats the local Prism directory configuration and instance metadata as authoritative.
2. **Read vs. Write Rules.**
   - Slime *may* read instance metadata (JSON/INI) directly from Prism paths using `QJsonDocument` / `QSettings`.
   - Slime *must never* write or mutate raw core configuration blocks (`instances/*/instance.cfg`, `accounts.json`, `mmc-pack.json`, `prismlauncher.cfg`) — that would corrupt Prism's internal state machine.
   - Slime owns exactly two write domains: `~/.config/SlimeLauncher/slime.conf` (theme + validated Prism paths) and the `slimelauncher/` asset folder inside each instance (`card.png`, `background.png`, `metadata.json`) managed exclusively by `ImageProcessor`.
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
│   │       ├── slimelauncher/               — Slime-owned asset folder (auto-created)
│   │       │   ├── card.png                 — 2:3 poster (300x450–600x900 px)
│   │       │   ├── background.png           — optional hero wallpaper (reserved)
│   │       │   └── metadata.json            — artwork provenance
│   │       └── .minecraft/logs/latest.log   — live log tail target (read-only)
│   ├── accounts.json                        — MS account sessions (read-only)
│   ├── icons/                               — instance icons (read-only)
│   └── prismlauncher.cfg                    — Prism global config (never read by Slime)
└── /usr/bin/prismlauncher                   — native binary (execution via CLI only)
```

Slime-owned data:
```
~/.config/SlimeLauncher/slime.conf           — theme, Prism binary path, instances dir path
instances/<id>/slimelauncher/                — per-instance poster artwork + metadata
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
  - Reads `/dev/input/js*` via low-level `open()`/`read()` fds pumped by a poll timer (no SDL2 dependency).
  - Console mapping: A(0) → Enter + play, B(1) → Escape + back, X(2) → options menu, Y(3) → search overlay, LB(4)/RB(5) → previous/next view, Back(8) → back, Start(9)/Menu(10) → open Prism. D-pad + left stick → arrow keys.
  - Semantic actions are emitted as dedicated Qt signals (`nextViewRequested`, `optionsRequested`, `searchRequested`, `prismRequested`, `backRequested`, …) that `main.cpp` binds to dashboard slots; face-button keys are also posted to the focus widget so keyboard and gamepad share one focus model.
  - Analog stick deadzone 0.35; hold-repeat (face buttons/arrows only) after 450 ms, then every 120 ms. Shoulders and menu actions fire once per press.
  - Up to 4 gamepads; hot-plug re-enumeration on each event loop pass.
- **`ThemeManager`** (`ThemeManager.h/.cpp`) — owns theme state and the single global QSS string.
  - Builds QSS for `Theme::Dark` (default) and `Theme::Light`; applied via `QApplication::setStyleSheet`.
  - Rounded design language: 14px card radius, 10px control radius, glass chrome bars, charcoal `#0f0f13` base, cyan `#00f0ff` focus, violet `#8a2be2` support accent.
  - Persists theme + Prism paths to `~/.config/SlimeLauncher/slime.conf` (`QSettings`).
  - QSS keys on stable object names — see CLAUDE.md for the registry.
- **`ImageProcessor`** (`ImageProcessor.h/.cpp`) — the 2:3 image engine and asset-folder manager.
  - `processToCardRatio()` center-crops any image to exact 2:3 and pins it into the 300x450–600x900 px window (height snaps to a multiple of 3 so the pair is exact; no 1px drift).
  - `ensureAssets()` auto-creates `<instance>/slimelauncher/` and seeds a deterministic default poster (SHA-256 of the instance id over the five bundled `:/cards/` resources) when `card.png` is missing.
  - `importCardArtwork()` runs a user-picked file through the same engine and flips `metadata.json` to `artwork: custom`.
- **`Constants.h`** — user-facing strings, path candidates, timing values, QSS object-name registry, design tokens (`COLOR_*`), and asset-pipeline names/paths.

### UI (`src/ui/`)
- **`SetupWizard`** (`SetupWizard.h/.cpp`) — `QWizard`, 3 pages:
  1. **Theme Selection** — dark/light live preview swatches; global QSS already applied, so selection previews instantly; saved on finish.
  2. **Prism Path Validation** — auto-scan of standard paths with a live result panel; Flatpak-detection warning block (descriptive, native-package remediation); manual path entry for custom installs; Finish enabled only on validated native install.
  3. **Account & Auth Interface** — read-only account list from `accounts.json` (name, type, last-sync), "Open Prism to sign in / manage accounts" fallback button. Slime never touches credentials.
- **`DashboardWindow`** (`DashboardWindow.h/.cpp`) — full-screen console main window.
  - Top bar: wordmark, active-account chip (`Logged in as …`), clock. **No sidebar, no persistent search field.**
  - `HeroBackdrop` (`HeroBackdrop.h/.cpp`) sits behind the content stack and cross-fades to the selected instance's wallpaper (`slimelauncher/background.png`, else a blurred/darkened card derivative) on every selection change.
  - Center stack: Library (responsive 2:3 poster grid, 3–8 columns, arrow/D-pad spatial navigation), Settings (`SettingsView`: theme toggle, account readout, Prism fallback actions), Logs, Error. LB/RB cycle Library ↔ Settings.
  - `SearchOverlay` (`SearchOverlay.h/.cpp`): modal centered filter field opened by Y / `F`; Esc cancels, Enter applies.
  - Footer: detail line + Play button + `ControllerLegend` (`ControllerLegend.h/.cpp`) — vector-drawn `[A] Play [X] Options [Y] Search [LB/RB] Views [MENU] Open Prism` badges (pure `QPainter` primitives, no font-dependent glyphs).
  - Shortcuts: Enter=play, F5=refresh, Escape=back/clear filter, E=options, F=search, O=open Prism, L=logs, `[`/`]`=view cycle (keyboard fallbacks for LB/RB).
- **`InstanceCard`** (`InstanceCard.h/.cpp`) — painter-drawn 2:3 poster card (Steam Big Picture style).
  - Artwork priority: instance `slimelauncher/card.png` → Prism icon (center-cropped in memory) → deterministic default poster → painted fallback surface. Never renders empty.
  - Bottom gradient overlay carries name, `MC <version> · last-played`, and a loader pill with per-loader hue (Fabric/Forge/NeoForge/Quilt, violet for Vanilla/unknown).
  - Focus (hover, selection, or keyboard) animates a 1.0 → 1.06 scale (`QVariantAnimation`, 140 ms OutCubic) and paints the cyan ring + violet under-glow + ambient halo.
  - Hover-revealed quick-action strip with vector-drawn icons (triangle=play, framed landscape=artwork, pencil=edit, lines=logs) plus a right-click context menu with the same actions; `showContextMenu()` is public so the dashboard can open it for gamepad X. Click/Enter = select, double-click/triangle = play. Emits `selected`, `activated`, `editRequested`, `artworkChangeRequested`, `logsRequested`.
- **`LogViewer`** (`LogViewer.h/.cpp`) — panel with read-only `QPlainTextEdit` (maximumBlockCount 4000), follow toggle (auto-scroll pauses on manual scroll-up), copy/clear actions, status line.
  - Backed by `PrismBridge::tailLog` watching `latest.log` (and legacy `1.log`). Handles missing-file gracefully ("waiting for log file…").
  - Strictly read-only — no write operations.
- **`ErrorPanel`** (`ErrorPanel.h/.cpp`) — full-screen failure surface for fatal paths (no Prism install, no instances dir, spawn failure).
  - Shows error code + human description + Retry / Open Prism actions. Stacked over the dashboard so navigation state survives a retry.
- **`SideNav`** (`SideNav.h/.cpp`) — left rail: All / Refresh / Open Prism buttons + read-only account chip. Fully gamepad/keyboard navigable via the shared focus model.

## UI/UX Specifications
- Modern console dashboard: deep charcoal `#0f0f13` background, glassmorphism chrome bars, card surfaces `#1a1a24` with 14px rounded corners.
- Accent system: electric cyan `#00f0ff` for focus/selection/primary CTA, violet `#8a2be2` as the secondary accent; danger `#ff4d6a`.
- Poster grid: vertical 2:3 cards (216x324 logical size), responsive 3–8 column flow with 22px gutters.
- Focus treatment: animated 1.06x scale + painted neon ring (cyan over violet) + ambient halo — rendered by the widget, never by QSS `outline`.
- Cards show: poster artwork, name, `MC <version> · last played`, loader pill, NOW PLAYING badge, hover quick-action strip.
- All interactive elements have hover, focus, and pressed states; keyboard and gamepad drive the identical focus model.
- Light theme: near-white background, near-black text, same layout language with muted cyan/violet accents.

## Desktop Integration
- `packaging/slime-launcher.desktop` ships `Exec=slimelauncher`, `Icon=slime-launcher`, `Categories=Game;Utility;`, `StartupWMClass=slimelauncher`.
- CMake installs the entry to `share/applications`, hicolor PNG icons (16–256px) + `scalable/slime-launcher.svg` to `share/icons/hicolor`, a `bin/slimelauncher` symlink beside the real binary, and the default cards to `share/slime-launcher/default_cards/`.
- `main.cpp` exports `QT_APPLICATION_NAME=slimelauncher` so the X11 WM_CLASS matches `StartupWMClass` (correct window matching in Hyprland, KWin, and GNOME).
- The default posters are also embedded into the binary via `resources.qrc` (`:/cards/…`) so seeding works from a bare build tree or tarball.

## Error Handling Philosophy
- Every `PrismBridge` call returns a typed result (value + error string) — never throws across boundaries.
- Missing paths, unreadable JSON, unknown loaders degrade gracefully: card renders with "Unknown" chip, never crashes.
- Spawn failures surface in `ErrorPanel` with the failed command and exit context.
- Gamepad device loss is silent; keyboard/mouse always remain functional.
