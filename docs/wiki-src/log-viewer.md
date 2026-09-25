---
name: Log viewer
description: Real-time instance log tailing for crash triage
---

# Log viewer

The log viewer streams an instance's game output in real time — for crash triage without leaving the ten-foot UI.

## Opening it

- Press `L`, or
- Select a card and press **Logs** in the bottom strip.

The view stacks over the grid; `Escape` (or **B** / the **Close** button) returns to the dashboard.

## What it shows

Prism writes the game log at `<instance>/.minecraft/logs/latest.log` (legacy `1.log` and PolyMC-heritage `minecraft/` layouts are checked too). Slime tails whichever exists first.

```text
[10:30:01] [main/INFO]: Loading Minecraft 1.21.4 with Fabric Loader 0.16.9
[10:30:02] [Render thread/INFO]: OpenAL initialized on device OpenAL Soft
...
```

## Behavior

| Aspect | Detail |
|---|---|
| Buffer | Last **4000 lines** (`QPlainTextEdit::maximumBlockCount`) |
| Follow mode | On by default — auto-scrolls to the newest line |
| Manual scroll-up | Pauses follow automatically; the button flips to `Follow: OFF` |
| Rotation | Watched via `QFileSystemWatcher`; the new file is re-armed automatically |
| Missing file | Shows `Waiting for log file…` and retries (~20 s) before giving up with a status |
| Copy | **Copy** button puts the visible buffer on the clipboard |

The viewer is strictly **read-only** — there is no write path to the log from Slime.

## Crash triage recipe

1. Select the instance that crashed → `L`.
2. Scroll to the end; look for the last `[main/FATAL]` or stack frame lines.
3. Press **Copy** and paste into your issue tracker.
4. `Escape` back; use **Open Prism** if you need to change Java args or mods (see [Compatibility boundaries](compatibility-boundaries.html) for why that delegates to Prism).
