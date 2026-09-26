---
name: Installing Prism natively
description: Native Prism Launcher installation and Flatpak rejection policy
---
[← SLIME LAUNCHER](../)


# Installing Prism natively

Slime Launcher requires a **native Prism Launcher** installation. Flatpak is explicitly unsupported because its sandbox blocks the inter-process communication and file monitoring that Slime Launcher depends on.

## Arch Linux (recommended)

Official repository:

```bash
sudo pacman -S prismlauncher
```

AUR (git builds):

```bash
paru -S prismlauncher-git
```

## Other distributions

| Distro | Command |
|---|---|
| Debian 13+ / Ubuntu 25.04+ | `sudo apt install prismlauncher` |
| Fedora 41+ | `sudo dnf install prismlauncher` |

Or download the official binary tarball from [prismlauncher.org](https://prismlauncher.org/download) and place it in `/usr/local/bin`.

## Minimum version

Prism Launcher **9.0 or newer** is required. The CLI verbs and metadata formats Slime Launcher reads (`instance.cfg`, `mmc-pack.json`) are those of Prism 9.x.

## Data directory

After first launch, Prism creates its data root at:

```text
~/.local/share/PrismLauncher/
├── instances/          ← your Minecraft instances
├── accounts.json       ← account sessions
└── icons/              ← instance icons
```

Slime Launcher also checks `~/.local/share/prism-launcher/` and the PolyMC-heritage `~/.local/share/PolyMC/` as fallback roots. See [Data ownership](data-ownership.html).

## Why not Flatpak?

The Flatpak sandbox breaks three things Slime Launcher needs:

1. **CLI hand-off** — `prismlauncher --launch <id>` must spawn the real binary with access to your instance data.
2. **File monitoring** — the log viewer watches `latest.log` through inotify; sandboxed paths differ per app.
3. **Direct metadata reads** — Slime Launcher reads `instances/*/instance.cfg` and `mmc-pack.json` at their standard XDG paths.

If setup detects `~/.var/app/org.prismlauncher.PrismLauncher` or a system Flatpak install, it blocks with a remediation panel listing the native install commands above.
