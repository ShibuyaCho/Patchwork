# Patchwork VST3 Plugin - Build Guide

## Overview

Patchwork is a professional VST3 DJ plugin built with JUCE.

## Prerequisites

### All Platforms
- Git
- CMake 3.16+
- JUCE 7.0+

### Windows
- Visual Studio 2019 or later

### macOS
- Xcode 12 or later

### Linux
- GCC 9+
- libasound2-dev
- libx11-dev
- libxext-dev

## Build Instructions

```bash
git clone https://github.com/yourusername/patchwork.git
cd patchwork
git clone https://github.com/juce-framework/JUCE.git
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Plugin Locations

- **Windows**: `%APPDATA%\VST3\`
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
- **Linux**: `~/.vst3/`

## Testing

1. Open your DAW
2. Rescan plugins
3. Load Patchwork
4. Play audio

---

For more details, see the documentation in `docs/`
