---
name: simutrans-otrp
description: Use when working on Simutrans OTRP patch development, understanding OTRP features, or referencing Simutrans/OTRP documentation. Covers Simutrans base, OTRP extended features (road lanes, coupling, scheduling, MCP server, Squirrel API), compilation, and wiki structure.
version: 1.0.0
author: Hermes Agent
license: MIT
metadata:
  hermes:
    tags: [simutrans, otrp, game-modding, transport-sim, cpp]
    related_skills: []
---

# Simutrans & OTRP Patch Development

## Overview

Simutrans is a freeware open-source transportation simulator (C++). OTRP (OTRPatch) is a Japanese community fork by ひめし (teamhimeh) that adds advanced features on top of Simutrans Standard. The primary development is done via SVN at `svn://servers.simutrans.org/simutrans/trunk` with a GitHub mirror at `github.com/simutrans/simutrans`. OTRP is hosted at `github.com/teamhimeh/simutrans`.

## When to Use

- Developing or debugging the OTRP patch
- Understanding Simutrans source code structure or compilation
- Looking up OTRP-specific features (coupling, scheduling, road lanes, MCP server, etc.)
- Referencing OTRP configuration parameters (simuconf.tab / advanced settings)
- Understanding OTRP save data compatibility or Squirrel API extensions
- When the user mentions Simutrans, OTRP, ひめし, teamhimeh, or the OTRP wiki

## Key Repositories & URLs

| Resource | URL |
|---|---|
| Simutrans main repo (GitHub mirror) | https://github.com/simutrans/simutrans |
| OTRP repo (teamhimeh) | https://github.com/teamhimeh/simutrans |
| OTRP Wiki Home | https://github.com/teamhimeh/simutrans/wiki/OTRP-Home |
| OTRP Releases | https://github.com/teamhimeh/simutrans/releases |
| OTRP International Forum thread | https://forum.simutrans.com/index.php?topic=16659.0 |
| Simutrans Official Site | https://www.simutrans.com/ |
| Simutrans International Forum | https://forum.simutrans.com/ |
| Japanese Forum | https://forum.japanese.simutrans.com/ |
| Simutrans Wiki | https://wiki.simutrans.com |
| Simutrans German Wiki (compiling etc) | https://simutrans-germany.com/wiki/ |
| OTRP MCP agent skills | https://github.com/teamhimeh/simutrans-agent-skills |
| OTRP Squirrel API docs | https://teamhimeh.github.io/simutrans/otrp-squirrel-api/ |

## Simutrans Base — Build & Architecture

Simutrans is written in C++. Build systems: **make** (Linux/Mac), **MSVC** (Windows), **CMake** (cross-platform).

### Required Libraries
- **Required:** zlib, bzip2 (or zstd), libpng, libSDL2 (Linux/Mac), libfreetype
- **Optional:** libzstd, libminiupnpc, libfluidsynth, libSDL2_mixer, libfontconfig

### Quick Build (Linux/Mac)
```bash
autoconf   # or: autoreconf -ivf (MacOS)
./configure
make -j 4
```

### Quick Build (MSVC)
Uses vcpkg for dependencies. Open `simutrans.sln` with projects: Simutrans-Main, Simutrans SDL2, Simutrans GDI (Windows-only), Simutrans Server.

### Key Directories in Source
- `simutrans/` — main game source
- `script/api/` — Squirrel API bindings
- `documentation/` — coding guidelines, etc.

### Contribution (Standard)
- **Do NOT open GitHub Pull Requests** — use the [Patches & Projects Forum](https://forum.simutrans.com/index.php/board,33.0.html)
- Coding guidelines in `simutrans/documentation/coding_styles.txt`
- Translations via [SimuTranslator](https://translator.simutrans.com/)

## OTRP — Overview

OTRP is based on **Simutrans Standard 122.0**. It adds:

### Road Features
- **Two-lane one-way roads** (4-lane highways possible)
- **Overtaking modes**: oneway [O], halt [H], twoway [T] (default), only loading [L], prohibited [P], inverted [I]
- **City road prevention** — prevent roads from becoming city roads
- **No citycars** — block private car entry
- **Lane guidance signs** — direct vehicles to specific lanes at intersections
- Traffic light at-grade crossings support

### Schedule Features
- **Absolute departure time scheduling** (month divided into 1440 slots by default; configurable via `spacing_shift_divisor`)
- **Load/unload control**: No Load, No Unload, Unload All, Temporary variants
- **Departure slot sharing between lines**
- **Convoy max speed per segment**, acceleration limit per segment
- **Station passing** (treat as waypoint)
- **"Skip if no passengers"** mode (bus/tram realistic behavior)

### Coupling (増解結) — Train Coupling/Decoupling
- Pre-implementation of a feature discussed on International Forum
- W convoy (waits) + C convoy (attempts coupling) at same coordinates
- Requires **"Require parent convoy to enter"** choose signal before coupling point
- Multiple coupling at same station (v48+)
- Coupling end function, coupling tile limit
- Reverse convoy coupling relationship supported

### Other Key Features
- **Diagonal tile stops** (with dedicated addon)
- **Time-based passenger routing** (instead of route-cost method)
- **Convoy/consist reversing** (with reverse images)
- **Non-electrified section entry** for EMUs (pantograph down graphics)
- **Vehicle offset** per way type (for pak128 repositioning)
- **MCP Server** (v54+) — AI tools can execute Squirrel scripts in-game
- **Convoy template** (v55.3+)
- **Range building/demolition tools** for large-scale development
- **Script generator** — export map infrastructure to Squirrel scripts
- **Overloading** support (v50+, passengers only)
- Power plant industry type (`kraftwerk` / `Power Plant`)
- WAV music file BGM support (v54+)

## OTRP Installation

1. Base: Install Simutrans Standard **122.0** (not latest!) from [SourceForge](https://sourceforge.net/projects/simutrans/files/simutrans/122-0/)
2. Addon paks: Put `ribi-arrow.pak`, `good.Reverse.pak`, `good.Reverse_No_Electric.pak`, `good.No_Electric.pak` into pakset folder
3. Binary: Place OTRP executable in same directory as `simutrans.exe`
4. Language: Put `ja.OTRP.tab` in `pak_<name>/text/`
5. Menu config: Add `simple_tool[37]=,:` to `menuconf.tab` for RibiArrow display

**Linux**: Requires `zstd` library v1.4.4+. **Mac**: Requires security bypass in System Settings.

## OTRP Additional Tools (menuconf.tab)

### general_tool
| # | Name | Description |
|---|---|---|
| 48 | TOOL_REMOVE_HALT | |
| 49 | TOOL_EXTINGUISH_WAITING_GOODS | Clear waiting goods at station |
| 51 | TOOL_PIPETTE | Pipette tool (v50.3+) |
| 52 | TOOL_RECREATE_HALT_NAME | Regenerate station name |
| 53 | TOOL_CHANGE_WAY_SETTINGS | Override road settings |
| 54 | TOOL_CHANGE_WAY_OFFSET | Vehicle offset per way |

### simple_tool
| # | Name | Description |
|---|---|---|
| 36 | TOOL_CHANGE_ROADSIGN | (internal) |
| 37 | TOOL_SHOW_RIBI | Toggle ribi arrow display |
| 41 | TOOL_SWITCH_PUBLIC_PLAYER | Switch to public player (KUTA) |
| 48 | TOOL_RESET_GAME_SPEED | Reset network game speed (v51+) |
| 49 | TOOL_FIX_GAME_SPEED | Fix network game speed (v51+) |

### dialog_tool
| # | Name | Description |
|---|---|---|
| 36 | DIALOG_ROUTE_SEARCH | Route search window (supports freight v51+) |

## OTRP Key Config Parameters

All in simuconf.tab or advanced settings (mostly saved in-game):

| Parameter | Tab | Description |
|---|---|---|
| `spacing_shift_divisor` | General | Divisions per month for departure timing (default: 1440) |
| `citycar_max_look_forward` | Route | How far citycars plan routes ahead |
| `citycar_route_weight_crowded` | Route | Weight for crowded road (default: 20) |
| `citycar_route_weight_vacant` | Route | Base weight for vacant road (default: 100) |
| `citycar_route_weight_speed` | Route | Speed multiplier weight (default: 0) |
| `routecost_halt` | Route | Route cost per stop (default: 1) |
| `routecost_wait` | Route | Route cost per transfer (default: 8) |
| `allow_overloading` | Route | Enable overloading (v50+) |
| `overloading_revenue_reduced` | Route | Revenue capped at 100% load |
| `overloading_runningcost_increase` | Route | Running cost scales with load |
| `advance_to_end` | Route | Train advances to platform end (uncheck for precise stop) |
| `first_come_first_serve` | Route | FCFS passenger boarding |
| `reverse_by_default` | Route | Auto-reverse convoy on turnaround (v50+) |
| `bits_per_month` | — | Max 28 (v51+) |

## OTRP Squirrel API Extensions

### IO Library (v29.5+)
Full [Squirrel I/O library](http://www.squirrel-lang.org/squirreldoc/stdlib/stdiolib.html) available.
- `file.readstr(n)` — read max n bytes as String (multi-byte safe)
- `file.writestr(str)` — write String to file

### gui class (v29.6+)
- `gui.jump(coord pos)` — jump view to coordinates
- `gui.close_all_windows()` — close all windows
- `gui.take_screenshot()` — screenshot
- `gui.set_zoom(int val)` — zoom 0 (min) to 9 (max)

### halt class (v51+)
`halt_x::get_connections()` return type changed to `array<connection>` where `connection = {halt_x, integer weight, line_x/convoy_x}`.

### schedule_x class (v51+)
Added `base_waiting_time` member.

### schedule_entry_x class (v51+)
`x, y, z` coordinates properly accessible.

## MCP Server (v54+)

Start with `-mcp-port PORT` (default 13354). Provides `run_squirrel` tool accepting Squirrel code + optional `player_nr`. Connect Claude Code via `.mcp.json`:
```json
{
  "mcpServers": {
    "simutrans": {
      "type": "stdio",
      "command": "nc",
      "args": ["localhost", "13354"]
    }
  }
}
```

## OTRP Wiki Structure

Key wiki pages (all under `github.com/teamhimeh/simutrans/wiki/`):

| Page | Content |
|---|---|
| [[OTRP-Home]] | Main documentation, features, installation, settings |
| [[OTRP-Releases-(v31‐v35)]] | Old release notes |
| [[スケジュールウィンドウの説明]] | Complete schedule window UI reference |
| [[増解結機能-Coupling]] | Train coupling/decoupling detailed guide |
| [[連結入出庫機能]] | Coupled convoy depot entry/exit |
| [[編成反転機能]] | Convoy/coupling reversal |
| [[斜めタイル停留所について]] | Diagonal tile stops |
| [[所要時間ベース旅客経路探索について]] | Time-based passenger routing |
| [[電化の取り扱いについて]] | Electrification handling |
| [[信号システムについて]] | Signal system (OTRP extensions) |
| [[wayによる車両オフセット機能]] | Vehicle offset per way |
| [[編成テンプレート機能]] | Convoy template (v55.3+) |
| [[MCP-Server]] | MCP server setup and usage |
| [[開発者向け:-Feature-Flagの利用について]] | Feature flag usage for devs |
| [[ツール定義]] | Tool definitions for addon makers |

## Save Data Compatibility

- **Loads**: Simutrans Standard 120.3+ save files
- **Saves**: Once saved by OTRP, becomes OTRP-only (unless "Readable by standard" button used, which strips OTRP data)
- **No compatibility** with Simutrans Extended
- Addons: Standard 122.0 addons fully compatible; 123.0+ mostly compatible

## OTRP Version History Snapshot

Key milestones:
- v29.5: Squirrel IO library
- v29.6: gui class extensions
- v32: Advanced schedule settings, coupling basics
- v35_1: Departure slot groups
- v40: Route time addition per stop
- v41: Convoy reversal
- v44_5: Diagonal stop basics
- v45: Route jumping between lines, coupling wait mode
- v46_1: Line/convoy name display
- v47: Lane guidance signs, per-segment speed limits, convoy speed editing
- v48: Multiple coupling, coupling end, max load rate, coupling tile limit
- v49: Temporary load/unload, station skip if no passengers, industry unit size
- v50: Overloading, city density display, station attribute toggle
- v50.2: Half-height elevated ways, city size sorting
- v51: 124.3.1 pakset support, line colors, gradient friction toggle, `bits_per_month` max 28
- v52: Acceleration limit per segment
- v53: Depot transfer from list, inter-depot convoy transfer
- v54: MCP server, power plant industry, WAV BGM, advanced industry connection
- v55.3: Convoy template

## Common Pitfalls

1. **Wrong base version**: OTRP requires Simutrans Standard **122.0**, not the latest. Pakset must also be 122.0-compatible (v51+ added partial 124.3.1 support).
2. **Save overwrite**: OTRP save files become OTRP-only on save. Back up standard saves before loading.
3. **tool number conflicts**: OTRP tool numbers conflict with Standard v124+. Using Standard menuconf.tab directly may cause issues.
4. **Coupling**: Requires choose signal with "Require parent convoy to enter" checked. Without it, coupling won't initiate.
5. **zstd on Linux**: Must install zstd v1.4.4+ from source before running OTRP.
6. **PRs not accepted upstream**: Standard Simutrans does not accept GitHub PRs — use the forum patch system.
7. **Mac binaries**: Not notarized by Apple; must be manually allowed in System Settings.
