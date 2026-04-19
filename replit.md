# Reworked

## Overview

A rework of [xdBot](https://github.com/Zilko/xdBot) (by Zilko) — a Geometry Dash macro bot — updated for **Geode 5.6.1** and **GD 2.2081**.

The Replit project hosts the source code and a landing page. The actual `.geode` mod file is compiled via GitHub Actions on push to `main`.

## Project Structure

```
.github/workflows/build.yml   GitHub Actions CI — builds .geode file (Win64, macOS, Android32, Android64)
src/
  main.cpp                    Entry point & PlayLayer/PauseLayer hooks
  includes.hpp                Shared includes and Global class declaration
  macro.hpp / macro.cpp       Macro data structure and logic
  global.cpp                  Global singleton implementation
  keybinds.cpp                Keyboard bindings (Windows, uses geode.custom-keybinds)
  gdr/                        GDR replay format library
  hacks/                      Individual hack implementations
    clickbot.cpp              Click sound playback bot
    autoclicker.cpp           Auto-clicker hack
    ...
  ui/                         UI layers and popups
    macro_editor.cpp          Macro editing layer
    button_edit_layer.cpp     Button position editor
    record_layer.cpp          Main recording/playback UI
    render_settings_layer.cpp Renderer settings
    load_macro_layer.cpp      Macro browser/loader
    clickbot_layer.cpp        Clickbot settings
    ...
  practice_fixes/             Practice mode fix hooks
  renderer/                   Frame renderer (FFmpeg-based, Windows)
  utils/                      Utility helpers
resources/                    Sprites and audio files bundled in the .geode
  GJ_commentSide2_001_White.png   White side border sprite (used in record layer)
  GJ_commentTop2_001_White.png    White top border sprite (used in record layer)
  default_hold_click.mp3          Clickbot click-hold sound
  default_release_click.mp3       Clickbot click-release sound
  default_hold_left.mp3           Clickbot left-hold sound
  default_release_left.mp3        Clickbot left-release sound
  default_hold_right.mp3          Clickbot right-hold sound
  default_release_right.mp3       Clickbot right-release sound
  sounds/                     Clickbot sound pack directories
CMakeLists.txt                CMake build configuration
mod.json                      Geode mod manifest
server.js                     Replit preview landing page (Node.js)
```

## Tech Stack

- **Language:** C++20
- **Framework:** [Geode SDK](https://geode-sdk.org/) v5.6.1
- **Game:** Geometry Dash 2.2081
- **Build:** CMake + GitHub Actions
- **Replit preview:** Node.js HTTP server on port 5000

## Build Fixes Applied (2025-04-18)

### Windows (Win64)
- **CMakeLists.txt**: Added `EXTERNALS geode.custom-keybinds:>=2.0.0` to `setup_geode_mod()` so the keybinds header is fetched during compilation.

### Android32 / Android64
- **autoclicker.cpp**: Fixed `processCommands(float dt)` → `processCommands(float dt, bool isHalfTick, bool isLastTick)` to match GJBaseGameLayer's actual signature in GD 2.2081.
- **clickbot.cpp**: Fixed `pl->m_isDualMode` (doesn't exist in GD 2.2081 bindings) → `pl->m_levelSettings->m_twoPlayerMode`.
- **utils.hpp**: Changed `setBackgroundColor(CCScale9Sprite*)` to a template function accepting any type with `setColor()`. This resolves the `NineSlice*` vs `CCScale9Sprite*` incompatibility in Geode v5.6.1 where `Popup::m_bgSprite` is now `NineSlice*`.
- **macro_editor.hpp**: Changed `hoveredBg`, `selectedBg`, `selectedInputBg`, `listBg` from `CCScale9Sprite*` to `NineSlice*` (Geode v5.6.1 returns `NineSlice*` from `CCScale9Sprite::create()`).
- **record_layer.hpp**: Changed `tpsBg` from `CCScale9Sprite*` to `NineSlice*`.
- **All UI init functions** (macro_editor.cpp, button_edit_layer.cpp, record_layer.cpp, render_settings_layer.cpp, load_macro_layer.cpp): Renamed local variable `bg` to `bgSpr` and changed type to `auto*` to avoid conflict with the `const char* bg` function parameter and NineSlice type mismatch.

### macOS
- **build.yml**: Changed macOS runner from `macos-latest` (ARM64, causes `--platform` CLI error) to `macos-13` (x86_64, known-working with Geode CLI).

### Resources
- Created `resources/` directory with all files referenced in `mod.json`:
  - 2 white PNG border sprites for the UI
  - 6 silent MP3 files as placeholders for clickbot sounds

## Building

The mod is built via GitHub Actions on every push to `main`. The workflow targets:
- **Win64** — `windows-latest`
- **macOS** — `macos-13`
- **Android64** — `ubuntu-latest`
- **Android32** — `ubuntu-latest`

## Based On

- **xdBot v2.4.1** by Zilko — https://github.com/Zilko/xdBot
