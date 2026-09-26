---
name: Troubleshooting
description: Common failures, diagnostics, and the headless smoke test
---

# Troubleshooting

## Setup refuses to continue

**"A Flatpak installation of Prism Launcher was detected."**
Slime found `~/.var/app/org.prismlauncher.PrismLauncher` or a system Flatpak install. Install the native package instead:

```bash
sudo pacman -S prismlauncher        # Arch official repo
paru -S prismlauncher-bin           # AUR
```

**"No native Prism Launcher installation was found."**
`prismlauncher` is not in `/usr/bin`, `/usr/local/bin`, `/opt/prismlauncher/bin`, or your `PATH`. Install it, or enter the full binary path manually on the wizard's path page and press **Validate**.

**"Prism binary found, but no instances directory."**
Launch the native Prism Launcher once — it creates `~/.local/share/PrismLauncher/instances` on first run — then retry. Custom data roots can be entered manually.

## Dashboard problems

**Grid is empty but Prism shows instances.**
Press `F5` to force a re-scan. If still empty, confirm the *InstancesDir* in `~/.config/SlimeLauncher/slime.conf` points at the directory that actually contains your instances. Every instance must have an `instance.cfg` — directories without one are skipped by design.

**A card shows the wrong artwork, or you want your own.**
Right-click the card (or press the Menu key on it) and choose **Change Card Artwork…**. Any PNG/JPG/WebP is accepted — it is center-cropped to an exact 2:3 poster (300x450–600x900 px) and saved to `<instance>/slimelauncher/card.png`. To re-roll the default art instead, delete that file and press `F5`; a bundled poster is re-seeded deterministically.

**"NOW PLAYING" never appears.**
The badge is a heuristic: `latest.log` modified within the last 90 seconds. If Prism writes logs elsewhere (custom data root), the card cannot know — this is cosmetic only and never affects launching.

**A card shows "Unknown" / "version unknown".**
`mmc-pack.json` is missing or unparsable for that instance. The card degrades gracefully instead of failing; launching still works because Prism resolves everything at runtime.

## Launch failures

**"Failed to spawn the Prism CLI process."**
The error includes the exact command. Check:

1. The binary at `BinaryPath` exists and is executable: `test -x /usr/bin/prismlauncher`
2. The instance id (directory name in `instances/`) is spelled as shown on the card
3. Running the same command manually shows Prism's own error output

## Log viewer shows nothing

- The instance has never run: no `latest.log` exists yet. It appears after the first launch (the view retries for ~20 s).
- You scrolled up: follow mode paused — press **Follow: ON** to resume.
- Custom instance layouts: Slime checks `.minecraft/logs/latest.log`, `minecraft/logs/latest.log`, and legacy `1.log` variants. Anything else is not watched.

## Resetting Slime's own state

```bash
rm -f ~/.config/SlimeLauncher/slime.conf
```

The setup wizard runs again on next launch. Prism data is never touched.

## Headless smoke test

The repository ships an integration test that drives `PrismBridge` against a fake Prism tree — useful for verifying parsing and launch routing without a display:

```bash
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build

# fake Prism environment
mkdir -p /tmp/smoke/bin /tmp/smoke/data/instances/test/.minecraft/logs
printf '#!/bin/sh\necho "$@" > /tmp/smoke/argv.txt\n' > /tmp/smoke/bin/prismlauncher
chmod +x /tmp/smoke/bin/prismlauncher
printf '[General]\nname=Test\n' > /tmp/smoke/data/instances/test/instance.cfg
printf '{"components":[{"uid":"net.minecraft","version":"1.21.4"}]}' \
  > /tmp/smoke/data/instances/test/mmc-pack.json

SMOKE_HOME=/tmp/smoke QT_QPA_PLATFORM=offscreen ./build/bin/prism-bridge-smoke
```

Expected output ends with `ALL CHECKS PASSED` — it validates empty-environment rejection, environment validation, instance parsing (including `slimelauncher/` asset provisioning and the 2:3 crop engine), CLI launch routing, and live log tailing.
