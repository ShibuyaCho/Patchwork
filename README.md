# 🎵 PATCHWORK - The Universal DJ Platform

A professional VST3 DJ plugin that works on **any DAW**, **any operating system**, with **any MIDI controller**.

## ✨ Features

- ✅ **Dual Turntables** - Independent deck control
- ✅ **16-Pad Sampler** - Record and trigger samples
- ✅ **20+ Professional Effects** - Reverb, delay, compression, and more
- ✅ **Universal MIDI Learn** - Works with any USB MIDI controller
- ✅ **All DAWs** - Ableton, FL Studio, Logic Pro, Cubase, Pro Tools, etc.
- ✅ **All Platforms** - Windows, macOS, Linux
- ✅ **100% Free & Open Source** - GPL v3 licensed

## 🚀 Getting Started

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

## 📖 Documentation

See `docs/` folder for build guides and architecture details.

## 🎵 Made With Love

Built by DJs, for DJs who value freedom and open source.

---

**Patch your sound together.** 🚀
