# Xd-rework

A rework of [xdBot](https://github.com/Zilko/xdBot) by Zilko, updated for **Geode 5.6.1** and **Geometry Dash 2.2081**.

## Features

- Macro recording & playback
- Frame Fixes and Input Fixes accuracy modes
- Speedhack
- Frame Stepper
- NoClip
- Trajectory Preview
- Coin Finder
- Layout Mode
- Clickbot with custom sounds
- Autoclicker
- Renderer (with FFmpeg, Windows only)
- Autosave
- TPS Bypass

## Building

### Prerequisites

- [Geode SDK](https://geode-sdk.org/) v5.6.1
- CMake 3.21+
- C++20 compatible compiler

### Local Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### GitHub Actions

Every push to `main` automatically triggers a build via GitHub Actions. The resulting `.geode` file is uploaded as an artifact.

## Credits

- **Original mod:** [Zilko/xdBot](https://github.com/Zilko/xdBot)
- **Rework by:** flinger-bit
