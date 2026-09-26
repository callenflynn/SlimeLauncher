---
name: Setup wizard
description: First-run theme selection, path validation, and accounts
---

# Setup wizard

The wizard runs on first launch (when `slime.conf` has no validated paths yet) and on demand from the error panel's **Retry** action.

## Page 1 — Theme

Two preview swatches rendered in their own theme colors:

- **DARK** (default) — `#0f0f13` charcoal base, `#00f0ff` electric-cyan accents
- **LIGHT** — `#f4f5f9` base, full contrast, `#6a1fb8` violet accent

Selection applies the global QSS immediately, so the rest of the wizard previews your choice. The choice persists when you finish.

## Page 2 — Prism installation

Slime scans the standard paths automatically:

- binary: `/usr/bin/prismlauncher`, `/usr/local/bin/prismlauncher`, `/opt/prismlauncher/bin/prismlauncher`, then `PATH`
- data roots: `~/.local/share/PrismLauncher/instances`, `~/.local/share/prism-launcher/instances`, `~/.local/share/PolyMC/instances`

**Validate** runs `PrismBridge::validateEnvironment()` and shows one of:

| Result | Meaning |
|---|---|
| `OK • <binary> <instances dir>` | Native install found and validated; **Finish** is available |
| Red Flatpak warning | Flatpak detected; remediation block lists native install commands for Arch, AUR, Debian, Fedora |
| Orange "not found" | No native binary or no instances dir; detail line shows what was searched |

Manual overrides: type a custom binary path and/or instances directory, then press **Validate** again. A custom path wins over auto-scan and is persisted.

!!! warning
    Finish stays disabled until validation passes — the wizard cannot be completed against a Flatpak install or a missing binary.

## Page 3 — Accounts

A read-only list parsed from Prism's `accounts.json` (name, type, last sync). Slime never touches credentials:

- To **sign in with a Microsoft account** or manage sessions, press **Open Prism to sign in / manage accounts** — this launches the native Prism UI and returns here after you're done.

## What gets persisted

On Finish, exactly one file is written:

```ini
# ~/.config/SlimeLauncher/slime.conf
[General]
Theme=dark
BinaryPath=/usr/bin/prismlauncher
InstancesDir=/home/you/.local/share/PrismLauncher/instances
```

The dashboard then loads with the chosen theme and validated paths.
