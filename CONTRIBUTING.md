# Contributing to Slime Launcher

Thanks for helping out. This is a small, focused project — a native Qt 6 frontend for Prism Launcher. Keep changes tight, read-only-safe, and warning-free.

## Quick setup (Arch Linux)

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bin/slime-launcher
```

On Ubuntu 24.04+ use `cmake ninja-build qt6-base-dev g++` instead. See the wiki's [Building](https://callenflynn.github.io/SlimeLauncher/building.html) page for details.

## Ground rules

1. **Prism is the source of truth.** Never write to Prism's files (`instance.cfg`, `mmc-pack.json`, `accounts.json`, `prismlauncher.cfg`). The only file Slime may write is `~/.config/SlimeLauncher/slime.conf`. See `Freebuff/AGENTS.md` for the full boundary list.
2. **All process spawning goes through `PrismBridge`.** Widgets never call `QProcess` directly.
3. **All parsing lives in `PrismBridge`.** Widgets consume `InstanceCardModel` structs, never raw paths.
4. **No exceptions across module boundaries.** Return `OpResult { ok, error }`.
5. **Zero compiler warnings** at `-Wall -Wextra -Wpedantic`, and no placeholder comments.
6. **Styling flows through `ThemeManager`.** No colors inside widgets; QSS keys on the object names registered in `src/Constants.h`.

## Before opening a PR

```bash
cmake --build build          # must compile clean, zero warnings
```

- Keep one logical change per PR.
- Describe *why*, not just *what*.
- New user-facing strings belong in `src/Constants.h`.

## Documentation

- Wiki sources live in `docs/wiki-src/` (built with [nsdocs](https://github.com/CStaks/nsDocs); see `nsdocs.yml` and the wiki's [meta page](https://callenflynn.github.io/SlimeLauncher/wiki.html)).
- Architecture notes for AI assistants and humans live in `Freebuff/CLAUDE.md` and `Freebuff/AGENTS.md`.

## Releases

Releases are automated: pushing a tag `v*.*.*` (e.g. `v1.0.1`) triggers `.github/workflows/release.yml`, which builds, packages `slime-launcher-linux-x86_64.tar.gz`, and publishes a GitHub Release with generated notes. Don't create releases by hand unless the workflow failed.
