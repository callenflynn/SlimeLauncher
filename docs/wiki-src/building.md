---
name: Building
description: Build Slime Launcher from source on Arch Linux
---

# Building

## Prerequisites (Arch Linux)

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base
```

Qt 6.2 or newer is required. Any C++20-capable GCC or Clang works.

## Configure and build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary lands at `build/bin/slime-launcher`.

!!! note
    Add `-DBUILD_TESTING=ON` to also build the headless smoke test (see [Troubleshooting](troubleshooting.html)).

## Compile options

| Option | Default | Purpose |
|---|---|---|
| `-DCMAKE_BUILD_TYPE` | `Release` | `Debug` adds no extra flags itself; use `Debug` + `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` for clangd |
| `-DBUILD_TESTING` | `OFF` | Builds `tests/PrismBridgeSmokeTest.cpp` into `build/bin/prism-bridge-smoke` |

## Static analysis

The build compiles clean at `-Wall -Wextra -Wpedantic`. For deeper checks:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build src
cppcheck --enable=warning,performance,portability src
```

## Install

```bash
cmake --install build --prefix /usr
```

This installs:

- `/usr/bin/slime-launcher` — the binary (plus a `/usr/bin/slimelauncher` symlink matching the desktop entry's `Exec=`)
- `/usr/share/applications/slime-launcher.desktop` — desktop entry (`StartupWMClass=slimelauncher`), registered by GNOME App Grid, KDE Application Launcher, Rofi, dmenu, and Hyprland app launchers
- `/usr/share/icons/hicolor/<size>x<size>/apps/slime-launcher.png` — app icons from 16px up to 256px, plus `scalable/slime-launcher.svg`
- `/usr/share/slime-launcher/default_cards/` — the bundled default poster cards

For a user-local install (no root), use `--prefix ~/.local`, which puts the entry in `~/.local/share/applications` and the icons in `~/.local/share/icons/hicolor`.

## Toolchain notes for other distros

The project targets Arch but builds anywhere Qt 6 and a C++20 compiler exist:

| Distro | Packages |
|---|---|
| Debian / Ubuntu 24.04+ | `cmake ninja-build qt6-base-dev g++` |
| Fedora | `cmake ninja-build qt6-qtbase-devel gcc-c++` |
