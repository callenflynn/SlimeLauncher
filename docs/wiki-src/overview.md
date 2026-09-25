---
name: Overview
description: What Slime Launcher is and what it does
---

# Slime Launcher

![Slime Launcher logo — slime block](slime.png)

Slime Launcher is a native **C++20 / Qt 6** controller-navigable frontend for [Prism Launcher](https://prismlauncher.org) on Linux. It renders your Prism instance library as a console-style grid — Playnite / Xbox Big Picture style — and routes every launch through Prism's own CLI.

**Prism Launcher is the source of truth.** Slime Launcher reads Prism's metadata and spawns Prism's processes. It never mutates Prism's configuration; see [Compatibility boundaries](compatibility-boundaries.html).

!!! note
    Slime Launcher targets **native (non-Flatpak) Prism Launcher** installations. Flatpak is explicitly unsupported — see [Installing Prism](installing-prism.html).

## Features

- **Dashboard grid** — instance cards with loader chips (Fabric / Forge / NeoForge / Quilt / Vanilla), game version flags, last-played times, and a live `NOW PLAYING` state.
- **Hero card** — the most recently played instance is pinned at double size.
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
| Target platform | Arch Linux (native packages) |
| Prism compatibility | v9.0+ |
| Memory model | RAII / Qt parent-child ownership |

## Documentation map

- **Getting started** — building from source, installing Prism natively.
- **Architecture** — module map, compatibility boundaries, data ownership.
- **Using Slime Launcher** — the setup wizard, controller navigation, log viewer.
- **Maintenance** — theming, troubleshooting, editing this wiki.
