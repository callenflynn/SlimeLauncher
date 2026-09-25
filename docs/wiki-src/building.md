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

- `/usr/bin/slime-launcher` — the binary
- `/usr/share/applications/slime-launcher.desktop` — desktop entry

## Toolchain notes for other distros

The project targets Arch but builds anywhere Qt 6 and a C++20 compiler exist:

| Distro | Packages |
|---|---|
| Debian / Ubuntu 24.04+ | `cmake ninja-build qt6-base-dev g++` |
| Fedora | `cmake ninja-build qt6-qtbase-devel gcc-c++` |
