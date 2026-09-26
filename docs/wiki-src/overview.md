---
name: Overview
description: What Slime Launcher is and what it does
---
[← SLIME LAUNCHER](../)


# Slime Launcher

![Slime Launcher logo — slime block](slime.png)

Slime Launcher is a native **C++20 / Qt 6** controller-navigable frontend for [Prism Launcher](https://prismlauncher.org) on Linux. It renders your Prism instance library as a modern console dashboard — rounded 2:3 poster cards with smooth focus animations, in the spirit of Playnite and Steam Big Picture — and routes every launch through Prism's own CLI.

**Prism Launcher is the source of truth.** Slime Launcher reads Prism's metadata and spawns Prism's processes. It never mutates Prism's configuration; see [Compatibility boundaries](compatibility-boundaries.html).

!!! note
    Slime Launcher targets **native (non-Flatpak) Prism Launcher** installations. Flatpak is explicitly unsupported — see [Installing Prism](installing-prism.html).

## Features

- **Poster dashboard** — responsive 2:3 card grid with per-instance artwork, gradient overlays, loader badges (Fabric / Forge / NeoForge / Quilt / Vanilla), game version tags, last-played times, and a live `NOW PLAYING` state. Focused cards scale up smoothly (1.06x) with a cyan/violet neon ring.
- **Artwork pipeline** — each instance gets a `slimelauncher/` asset folder with a 2:3 `card.png` (auto-seeded from bundled posters), plus `background.png` and `metadata.json`. Custom artwork is center-cropped to exact 2:3 via "Change Card Artwork…".
- **Controller navigation** — gamepad-to-focus mapping that shares one focus model with keyboard and mouse.
- **Live log viewer** — real-time tail of the selected instance's `latest.log` with follow mode.
- **Setup wizard** — first-run theme selection, automatic Prism path detection with Flatpak rejection, and a read-only account overview.
- **Fallback safety** — every advanced operation delegates to the native Prism UI via a dedicated button.

## At a glance

| | |
|---|---|
| Language | C++20 |
| UI framework | Qt 6 Widgets (no QML) |
| Build system | CMake ≥ 3.22 + Ninja |
| Target platform | Linux, native Prism packages |
| Prism compatibility | v9.0+ |
| Memory model | RAII / Qt parent-child ownership |

## Documentation map

- **Getting started** — building from source, installing Prism natively.
- **Architecture** — module map, compatibility boundaries, data ownership.
- **Using Slime Launcher** — the setup wizard, controller navigation, log viewer.
- **Maintenance** — theming, troubleshooting, editing this wiki.
