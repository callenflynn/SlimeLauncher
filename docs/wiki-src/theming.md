---
name: Theming
description: QSS system, color tokens, and the object-name registry
---

# Theming

All styling flows through **one** QSS string per theme, built by `ThemeManager::buildQss()` and applied globally via `qApp->setStyleSheet()`. Widgets contain no colors of their own (the only exceptions are the painted focus glow in `InstanceCard::paintEvent` and fixed swatch previews in the wizard).

## Color tokens

| Token | Dark | Light |
|---|---|---|
| `BG` | `#0a0a0a` | `#fafafa` |
| `SURFACE` | `#141414` | `#ffffff` |
| `SURFACE2` | `#1a1a1a` | `#f0f0f0` |
| `LINE` | `#262626` | `#d4d4d4` |
| `TEXT` | `#f2f2f2` | `#111111` |
| `MUTED` | `#8a8a8a` | `#666666` |
| `ACCENT` | `#39ff14` | `#2ea80a` |
| Danger | `#8b0000` | `#8b0000` |

Design rules:

- Flat surfaces, **hard edges** — no `border-radius` on primary surfaces (brutalist aesthetic)
- Accent is used sparingly: focus rings, primary CTA, `NOW PLAYING` badge
- 1px hairlines separate regions; panels alternate `BG` / `SURFACE` for depth

## Object-name registry

QSS keys on `setObjectName`, not class names. These names are a stable contract — renaming a widget requires updating `ThemeManager` and `src/Constants.h`:

| Object name | Used by |
|---|---|
| `InstanceCard` | Grid tile frame |
| `HeroCard` | Double-size featured tile |
| `SlimeButton` | Base button styling |
| `PrimaryButton` | Neon accent CTA (Play, Validate, Retry) |
| `DangerButton` | Destructive actions |
| `SideNav` | Left navigation rail |
| `LogView` | Log `QPlainTextEdit` (monospace) |
| `StatusChip` | Loader chips and account chip |

## State styling

- **Hover** — CSS `:hover`
- **Selection** — dynamic property: `QFrame#InstanceCard[selected="true"]` → 3px accent ring + painted glow
- **Playing** — dynamic property `playing` drives the `NOW PLAYING` badge
- **Focus** — `:focus` → 3px accent border on buttons and inputs

## Adding a theme

1. Add the color set to the `buildQss()` branch in `src/ThemeManager.cpp`.
2. Extend `ThemeManager::Theme` and the persistence mapping in `setTheme()`/`main.cpp`.
3. Add a swatch button on the wizard's theme page.

## Persistence

The chosen theme is written to `slime.conf` (`General/Theme=dark|light`) when the wizard finishes. It is re-applied on every startup before the dashboard shows.
