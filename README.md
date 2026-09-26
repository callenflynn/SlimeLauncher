# Slime Launcher

<img src="assets/slime.png" alt="Slime Launcher logo — slime block" width="140" />

A native C++20 / Qt 6 controller-navigable launcher frontend for **native (non-Flatpak) Prism Launcher** on Linux. Console-style dashboard UI (Playnite / Steam Big Picture inspired): rounded 2:3 poster cards, electric-cyan focus glows, ambient charcoal surfaces — with a tiny footprint.

**Prism Launcher is the source of truth.** Slime Launcher reads Prism's instance metadata and routes all launches through Prism's CLI. It never mutates Prism data.

## Features

- **Setup wizard** — theme selection (dark default `#0f0f13` charcoal + neon `#00f0ff`, light alternative), automatic detection of native Prism installs with explicit Flatpak rejection, read-only account overview.
- **Poster dashboard** — responsive 2:3 card grid with per-instance artwork, dark gradient overlays, loader badges (Fabric/Forge/NeoForge/Quilt/Vanilla), version tags, last-played times, and a live `NOW PLAYING` state. Cards scale up smoothly (1.06x) with a neon focus ring on hover/keyboard/gamepad focus.
- **Artwork pipeline** — every instance gets an auto-created `slimelauncher/` asset folder (`card.png`, `background.png`, `metadata.json`). Missing posters are seeded from bundled default art; custom images are center-cropped to exact 2:3 (300x450–600x900 px) via "Change Card Artwork…" in the card context menu.
- **Controller navigation** — gamepad → focus mapping via `/dev/input/js*` (A=select, B=back, X/Start=refresh, D-pad/stick=arrows) sharing one focus model with keyboard and mouse.
- **Log viewer** — live tail of the selected instance's `latest.log` with follow-mode, copy, and rotation handling.
- **Fallback safety** — every advanced operation (account sign-in, deep instance editing) delegates to the native Prism UI via a dedicated button.

## Build (Arch Linux)

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bin/slime-launcher
```

Headless smoke tests (no display required):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build
./build/bin/prism-bridge-smoke   # with SMOKE_HOME pointing at a fake Prism tree
```

## Install

```bash
cmake --install build --prefix /usr   # binary, .desktop entry, hicolor icons, default cards
```

The install step ships `share/applications/slime-launcher.desktop` (registered by GNOME App Grid, KDE Application Launcher, Rofi, dmenu, Hyprland app launchers), `bin/slimelauncher` (matching `Exec=slimelauncher` / `StartupWMClass=slimelauncher`), hicolor app icons up to 256px plus a scalable SVG, and the default poster cards.

## Requirements

- Linux (targets Arch Linux; works on any distro with native Prism)
- Prism Launcher ≥ 9.0, native package (`extra/prismlauncher` or AUR `prismlauncher-bin`)
- Qt 6.2+ (Widgets), C++20 compiler
- Flatpak Prism is explicitly unsupported

## Documentation

- **[Wiki](https://callenflynn.github.io/SlimeLauncher/)** — user and contributor docs, built from `docs/wiki-src/` with [nsdocs](https://github.com/CStaks/nsDocs)
- [`Freebuff/CLAUDE.md`](Freebuff/CLAUDE.md) — build commands, coding rules, QSS/styling rules, state lifecycle
- [`Freebuff/AGENTS.md`](Freebuff/AGENTS.md) — architecture, compatibility boundaries, data ownership, component specs

### Building the wiki

```bash
pip install git+https://github.com/CStaks/nsDocs.git
nsdocs                                # rebuilds docs/wiki/ from docs/wiki-src/
nsdocs --check                        # CI mode: fails if committed HTML is stale
python3 -m http.server -d docs/wiki 8000   # local preview
```

Pages are plain Markdown in `docs/wiki-src/` with front-matter titles; the sidebar and index are generated from the `nav:` map in [`nsdocs.yml`](nsdocs.yml). Commit both the `.md` source and the regenerated `docs/wiki/` HTML — CI verifies they never drift (and deploys to GitHub Pages on `main`).
