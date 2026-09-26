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
│       ├── slimelauncher/               ← Slime-owned asset folder (see below)
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
| `mmc-pack.json` | `components[].uid`, `components[].version` | Loader badge (`net.fabricmc.*` → Fabric, etc.) and `net.minecraft` version |
| `accounts.json` | `name`, `type`, `lastSync` | Read-only account chip in the SideNav |
| `latest.log` | — | Live tail for the log viewer; mtime recency drives the `NOW PLAYING` heuristic |

## Slime-owned inside an instance: `slimelauncher/`

Each instance directory contains a Slime-owned asset folder, auto-created on scan:

```text
instances/<id>/slimelauncher/
├── card.png          ← 2:3 poster cover (exact 2*width = 3*height, 300x450–600x900 px)
├── background.png    ← optional hero wallpaper (reserved; not yet rendered)
└── metadata.json     ← display overrides
```

`metadata.json` records how the card was produced:

```json
{
    "artwork": "default | custom",
    "source": "default poster 3 | original-file-name.png",
    "updated": "2026-09-25T21:04:10.512Z"
}
```

Provisioning rules (`src/ImageProcessor.cpp`):

1. `ensureAssets()` creates the folder if missing.
2. If no `card.png` exists, a **deterministic** default poster is picked (SHA-256 of the instance id modulo the five bundled cards) — stable across rescans, so cards never reshuffle.
3. The chosen image passes through the center-crop engine (`Qt::KeepAspectRatioByExpanding` semantics, then a final exact-2:3 pin) and is written as PNG.
4. "Change Card Artwork…" overwrites `card.png` with a user-selected image and flips metadata to `artwork: custom`.
5. Nothing outside `slimelauncher/` is ever written inside an instance.

Prism ignores this folder entirely — it is not a Prism config file, and no Prism key collides with it.

## Slime-owned outside Prism

```text
~/.config/SlimeLauncher/slime.conf       ← theme + validated Prism paths
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
