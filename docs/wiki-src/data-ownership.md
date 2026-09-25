---
name: Data ownership
description: Which files Prism owns, which Slime owns, and what Slime reads
---

# Data ownership

## Prism-owned (authoritative)

```text
~/.local/share/PrismLauncher/            (or ~/.local/share/prism-launcher/)
├── instances/
│   └── <id>/                            ← directory name = CLI launch id
│       ├── instance.cfg                 ← name, iconKey, lastLaunchTime   [read-only to Slime]
│       ├── mmc-pack.json                ← components: loader + game ver   [read-only to Slime]
│       └── .minecraft/
│           └── logs/latest.log          ← live tail target                [read-only to Slime]
├── accounts.json                        ← MS account sessions             [read-only to Slime]
├── icons/<key>.png|jpg|svg|…            ← instance icons                  [read-only to Slime]
└── prismlauncher.cfg                    ← Prism global config             [never touched]
```

### What Slime parses from each file

| File | Keys used | Purpose |
|---|---|---|
| `instance.cfg` | `name`, `iconKey`, `lastLaunchTime` | Card title, icon, last-played time (both flat and `[General]`-section spellings accepted) |
| `mmc-pack.json` | `components[].uid`, `components[].version` | Loader chip (`net.fabricmc.*` → Fabric, etc.) and `net.minecraft` version |
| `accounts.json` | `name`, `type`, `lastSync` | Read-only account chip in the SideNav |
| `latest.log` | — | Live tail for the log viewer; mtime recency drives the `NOW PLAYING` heuristic |

## Slime-owned

```text
~/.config/SlimeLauncher/slime.conf       ← the ONLY file Slime writes
```

Contents:

```ini
[General]
Theme=dark|light
BinaryPath=/usr/bin/prismlauncher
InstancesDir=/home/you/.local/share/PrismLauncher/instances
```

Written once when the setup wizard finishes; never touched again during normal operation.

## Resolution order

On startup (and on every `F5`):

1. `BinaryPath` / `InstancesDir` from `slime.conf` if set — validated, used as-is when good.
2. Otherwise scan candidates:
   - binary: `/usr/bin/prismlauncher`, `/usr/local/bin/prismlauncher`, `/opt/prismlauncher/bin/prismlauncher`, then `PATH`
   - data roots: `~/.local/share/PrismLauncher`, `~/.local/share/prism-launcher`, `~/.local/share/PolyMC` (+ `/instances`)
3. Still nothing → SetupWizard.

## NOW PLAYING heuristic

Slime considers an instance "playing" when its `latest.log` was modified within the last 90 seconds — Prism keeps the file open and appends while the game runs. This is deliberately conservative: a stale read only ever delays a badge, never mutates anything.
