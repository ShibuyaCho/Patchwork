# PATCHWORK - The Universal DJ Platform

A professional VST3 DJ plugin that works on **any DAW**, **any operating system**, with **any MIDI controller**.

## Features

- **Dual Turntables** - Independent deck control with hot cues, loops, and slip mode
- **16-Pad Sampler** - Record and trigger samples with velocity control
- **20+ Professional Effects** - Reverb, delay, compression, filter, flanger, chorus, and more
- **Universal MIDI Learn** - Works with any USB MIDI controller out of the box
- **JSON Controller Profiles** - Edit or create device profiles without recompiling
- **All DAWs** - Ableton, FL Studio, Logic Pro, Cubase, Pro Tools, Reaper, and more
- **All Platforms** - Windows, macOS, Linux
- **Full Session State** - Hot cues, loops, and loaded tracks survive DAW project save/reload
- **100% Free & Open Source** - GPL v3 licensed

---

## Quick Start

### Linux / macOS

```bash
git clone https://github.com/yourusername/patchwork.git
cd patchwork
bash scripts/setup.sh
```

That's it. The script installs system packages, clones JUCE, builds the plugin, copies controller profiles to your user profile directory, and installs the VST3.

### Windows

```powershell
git clone https://github.com/yourusername/patchwork.git
cd patchwork
.\scripts\setup.ps1
```

Requires Git, CMake, and Visual Studio 2019+ to be installed first.

---

## Manual Build

### Prerequisites
- CMake 3.16+
- C++ Compiler (MSVC 2019+, Xcode 12+, or GCC 9+)
- JUCE 7.0+

### Build

```bash
git clone https://github.com/yourusername/patchwork.git
cd patchwork
git clone https://github.com/juce-framework/JUCE.git
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Install

- Windows: `%APPDATA%\VST3\Patchwork.vst3`
- macOS: `~/Library/Audio/Plug-Ins/VST3/Patchwork.vst3`
- Linux: `~/.vst3/Patchwork.vst3`

---

## Custom Controller Setup

Patchwork ships with built-in profiles for Numark Party Mix, Pioneer DDJ, Hercules DJControl, Denon MC, Maschine Mikro, Akai APC, Launchpad, and more.

To add or customise a profile, drop a `.json` file in your profiles directory — **no recompilation needed**:

| Platform | Profiles directory |
|----------|--------------------|
| Linux    | `~/.config/Patchwork/profiles/` |
| macOS    | `~/Library/Application Support/Patchwork/profiles/` |
| Windows  | `%APPDATA%\Patchwork\profiles\` |

See [docs/CONTROLLER_PROFILES.md](docs/CONTROLLER_PROFILES.md) for the full profile format and target identifier reference.

You can also use **MIDI Learn** inside the plugin to map any control interactively, then export the result as a shareable JSON profile.

---

## Documentation

- [Build Guide](docs/VST3_BUILD_GUIDE.md)
- [Controller Profiles](docs/CONTROLLER_PROFILES.md)
- [Development Roadmap](docs/ROADMAP.md)

---

Built by DJs, for DJs who value freedom and open source.

**Patch your sound together.**
