---
name: Compatibility boundaries
description: The read-vs-write rules that keep Prism state intact
---

# Compatibility boundaries

These rules are non-negotiable. They guarantee Slime Launcher can never corrupt a Prism installation and that upstream Prism updates keep working.

## 1. Prism as source of truth

Slime Launcher is a **frontend and orchestration interface only**. The local Prism directory configuration and instance metadata are authoritative. If Prism's data and Slime's view ever disagree, Prism wins: press `F5` and Slime re-reads everything from disk.

## 2. Read vs. write rules

| Data | Read | Write |
|---|---|---|
| `instances/*/instance.cfg` | ✔ `QSettings` IniFormat | **never** |
| `instances/*/mmc-pack.json` | ✔ `QJsonDocument` | **never** |
| `instances/*/.minecraft/logs/latest.log` | ✔ tail only | **never** |
| `accounts.json` | ✔ display only | **never** |
| `icons/*` | ✔ | **never** |
| `prismlauncher.cfg` (Prism global config) | not even read | **never** |
| `~/.config/SlimeLauncher/slime.conf` | ✔ | ✔ (the **only** file Slime writes) |

`slime.conf` stores exactly three keys: `General/Theme`, `General/BinaryPath`, `General/InstancesDir`. It is written once per wizard completion and never during normal operation.

## 3. Execution routing via CLI

All launch operations route through Prism's standard CLI backend, spawned asynchronously:

```cpp
QProcess::startDetached("prismlauncher", {"--launch", instanceId});
```

- Opening the native Prism UI: `QProcess::startDetached(binaryPath, {})`.
- Widgets never spawn processes directly — every spawn goes through `PrismBridge`.
- The instance **directory name** is the CLI identifier; it is what Slime passes to `--launch`.

## 4. Native-only target

Flatpak Prism is unsupported (sandbox isolation blocks IPC and file monitoring). Setup performs explicit detection of:

- `~/.var/app/org.prismlauncher.PrismLauncher` (user Flatpak)
- `/var/lib/flatpak/app/org.prismlauncher.PrismLauncher` (system Flatpak)

and refuses to continue with remediation instructions.

## 5. Fallback safety

Any operation without a stable, safe programmatic abstraction gets an explicit fallback button that opens the native Prism UI instead:

- Microsoft account sign-in and session management
- Deep instance editing (settings, mods, worlds, screenshots)
- Mod management and updates

Slime Launcher never reimplements these flows.

## 6. Minimum Prism version

**Prism 9.0+.** The CLI verbs (`--launch`) and metadata formats Slime reads are those of Prism 9.x.
