---
name: Theming
description: QSS system, color tokens, and the object-name registry
---
[← SLIME LAUNCHER](../)


# Theming

All styling flows through **one** QSS string per theme, built by `ThemeManager::buildQss()` and applied globally via `qApp->setStyleSheet()`. Widgets contain no colors of their own (the only exceptions are the painted focus ring, glow, and gradient overlay in `InstanceCard::paintEvent`, and fixed swatch previews in the wizard).

## Color tokens

| Token | Dark | Light |
|---|---|---|
| `BG` | `#0f0f13` | `#f4f5f9` |
| `SURFACE` | `#1a1a24` | `#ffffff` |
| `SURFACE2` | `#222230` | `#e9eaf2` |
| `LINE` | `#2a2a38` | `#d5d7e2` |
| `TEXT` | `#f2f4f8` | `#15161c` |
| `MUTED` | `#9a9eb0` | `#5c5f70` |
| `ACCENT` | `#00f0ff` | `#0090a8` |
| `ACCENT2` (violet) | `#8a2be2` | `#6a1fb8` |
| Danger | `#ff4d6a` | `#d9264a` |

Design rules:

- **Rounded console surfaces** — 14px corner radius on cards, 10px on buttons/inputs/menus (the old hard-edge brutalist look is retired)
- Deep charcoal background with glassmorphism chrome: translucent top/bottom bars (`rgba(26, 26, 36, 0.72)` in dark) floating over the content
- Electric cyan is the focus/selection color; violet appears as the secondary accent (ring under-glow, loader pills)
- 1px hairlines separate regions; panels alternate `BG` / `SURFACE` for depth
- The same tokens are mirrored as `Constants::COLOR_*` values for painter-drawn surfaces — `ThemeManager` (QSS) and `Constants.h` (painting) must stay in sync

## Object-name registry

QSS keys on `setObjectName`, not class names. These names are a stable contract — renaming a widget requires updating `ThemeManager` and `src/Constants.h`:

| Object name | Used by |
|---|---|
| `InstanceCard` | Poster card (fully painter-drawn; QSS only keeps its background transparent) |
| `HeroCard` | Legacy alias kept transparent for compatibility |
| `SlimeButton` | Base button styling |
| `PrimaryButton` | Cyan accent CTA (Play, Validate, Retry) |
| `DangerButton` | Destructive actions |
| `SideNav` | Left navigation rail |
| `LogView` | Log `QPlainTextEdit` (monospace) |
| `StatusChip` | Loader chips and account chip |
| `TopBar` / `BottomBar` | Glass chrome bars |
| `SearchField` | Rounded pill search input |

## State styling

- **Hover** — CSS `:hover` on buttons/inputs; cards animate a 1.0 → 1.06 scale via `QVariantAnimation`
- **Selection / focus** — `InstanceCard::paintEvent` renders the neon ring (cyan stroke + violet under-glow) and ambient radial halo; buttons use a 2px accent border via `:focus`
- **Playing** — `InstanceCardModel::playing` drives the `NOW PLAYING` pill
- **Quick actions** — hovering a focused card reveals the Play / Change Artwork / Edit / Logs strip

## Adding a theme

1. Add the color set to the `buildQss()` branch in `src/ThemeManager.cpp`.
2. Extend `ThemeManager::Theme` and the persistence mapping in `setTheme()`/`main.cpp`.
3. Add a swatch button on the wizard's theme page.

## Persistence

The chosen theme is written to `slime.conf` (`General/Theme=dark|light`) when the wizard finishes. It is re-applied on every startup before the dashboard shows.
