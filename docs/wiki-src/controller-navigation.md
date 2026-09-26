---
name: Controller navigation
description: Gamepad mapping, keyboard shortcuts, and focus model
---

# Controller navigation

Slime Launcher is built for the couch: a gamepad, keyboard, and mouse all drive the **same focus model**. The gamepad layer is `GamepadFilter` (`src/GamepadFilter.cpp`), an application-wide event filter that reads `/dev/input/js*` directly — no SDL2 dependency — and posts synthetic Qt key events to the focused widget.

## Gamepad mapping

| Input | Action |
|---|---|
| **A** (button 0) | Select / activate (Enter) |
| **B** (button 1) | Back / close (Escape) |
| **X** (button 2) | Refresh (F5) |
| **Y** (button 3) | Tab |
| **Start** (button 9) | Refresh (F5) |
| **D-pad / left stick** | Move focus (arrow keys) |

- Analog stick deadzone: **35%**
- Hold-repeat: initial delay **450 ms**, then every **120 ms**
- Up to **4 gamepads**; hot-plug re-enumeration happens automatically (~every 2 s)
- Device loss is silent — keyboard and mouse always remain functional

!!! note
    Most controllers work out of the box: any device exposing `/dev/input/js*` (xpad, steamcontroller, generic HID joypads) is supported. If your pad lacks a standard button layout, the A/B/X/Y indices above follow the Linux joystick API convention.

## Keyboard shortcuts

| Key | Action |
|---|---|
| `Enter` | Play the selected instance |
| `F5` | Force-refresh the instance sync from disk |
| `Escape` | Back out: close log/error view → clear search filter |
| `L` | Open the log viewer for the selected instance |
| `O` | Open the native Prism UI |
| Arrows / Tab | Move focus across cards and navigation |
| `Menu` key | Open the card context menu (Play / Change Artwork / Logs / Edit) |

## Mouse

- **Click** a card to select it
- **Double-click** to play it
- Standard scroll mechanics everywhere

## Focus visuals

The focused card **scales up to 1.06x** with a 140 ms eased animation and receives a **neon focus ring** (cyan stroke over a violet under-glow) plus an ambient radial halo — all painted by `InstanceCard::paintEvent` (QSS has no box-shadow). Hovering or D-pad-navigating to a card triggers the same animation. Buttons get a 2px accent border. Hover, focus, and pressed states exist on every interactive element.

## How it works internally

```text
/dev/input/js0 ──read()──► GamepadFilter (poll timer, 16 ms)
                             │  decode js_event
                             │  deadzone + repeat logic
                             ▼
                 QKeyEvent(KeyPress/KeyRelease)
                             ▼
                 QApplication::sendEvent(focusWidget)
```

Because the gamepad feeds the *same* event stream as the keyboard, no widget ever needs special gamepad handling — a card reacts to a gamepad **A** exactly like a real Enter keypress.
