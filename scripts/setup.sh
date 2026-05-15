#!/usr/bin/env bash
# Patchwork one-command setup script
# Installs system dependencies, clones JUCE, builds the plugin, and installs it.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

log()  { printf '\033[1;32m[setup]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[setup]\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[setup]\033[0m %s\n' "$*" >&2; exit 1; }

cd "$REPO_ROOT"

# ── Detect platform ────────────────────────────────────────────────────────────
OS="$(uname -s)"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

# ── Install system dependencies ────────────────────────────────────────────────
if [[ "$OS" == "Linux" ]]; then
    log "Detected Linux — installing system packages..."
    if command -v apt-get &>/dev/null; then
        sudo apt-get update -qq
        sudo apt-get install -y \
            cmake build-essential pkg-config git \
            libgtk-3-dev libwebkit2gtk-4.1-dev \
            libasound2-dev libx11-dev libxext-dev \
            libxinerama-dev libxcursor-dev libxrandr-dev libxrender-dev libxcomposite-dev \
            libfreetype6-dev libfontconfig1-dev \
            libsoundtouch-dev libshout3-dev libmp3lame-dev libusb-1.0-0-dev \
            libcurl4-openssl-dev
    elif command -v dnf &>/dev/null; then
        sudo dnf install -y \
            cmake gcc g++ pkgconfig git \
            gtk3-devel webkit2gtk4.1-devel \
            alsa-lib-devel libX11-devel libXext-devel \
            soundtouch-devel libshout-devel lame-devel libusb1-devel \
            libcurl-devel freetype-devel fontconfig-devel
    elif command -v pacman &>/dev/null; then
        sudo pacman -Sy --noconfirm \
            cmake gcc pkgconf git \
            gtk3 webkit2gtk \
            alsa-lib libx11 libxext \
            soundtouch libshout lame libusb \
            curl freetype2 fontconfig
    else
        warn "Unknown package manager — install dependencies manually (see docs/VST3_BUILD_GUIDE.md)"
    fi

elif [[ "$OS" == "Darwin" ]]; then
    log "Detected macOS — installing Homebrew packages..."
    if ! command -v brew &>/dev/null; then
        die "Homebrew not found. Install it from https://brew.sh first."
    fi
    brew install cmake pkg-config soundtouch shout lame libusb
    # GTK and WebKit are only needed on Linux; macOS uses native frameworks via JUCE

else
    die "Unsupported OS: $OS (Windows users: run scripts/setup.ps1)"
fi

# ── Clone JUCE ─────────────────────────────────────────────────────────────────
if [[ ! -d "JUCE" ]]; then
    log "Cloning JUCE framework..."
    git clone --depth=1 https://github.com/juce-framework/JUCE.git
else
    log "JUCE directory already present — skipping clone."
fi

# ── Install bundled controller profiles ───────────────────────────────────────
if [[ "$OS" == "Linux" ]]; then
    PROFILES_DEST="$HOME/.config/Patchwork/profiles"
elif [[ "$OS" == "Darwin" ]]; then
    PROFILES_DEST="$HOME/Library/Application Support/Patchwork/profiles"
fi

if [[ -n "${PROFILES_DEST:-}" ]]; then
    log "Installing controller profiles to $PROFILES_DEST ..."
    mkdir -p "$PROFILES_DEST"
    cp -n profiles/*.json "$PROFILES_DEST/" 2>/dev/null && log "Profiles installed." \
        || log "Profiles already exist — not overwriting user customisations."
fi

# ── CMake configure + build ────────────────────────────────────────────────────
log "Configuring build..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$REPO_ROOT"

log "Building with $JOBS parallel jobs..."
cmake --build build --config Release -j"$JOBS"

# ── Install plugin ─────────────────────────────────────────────────────────────
if [[ "$OS" == "Linux" ]]; then
    PLUGIN_SRC="$(find build -name 'Patchwork.vst3' -type d 2>/dev/null | head -1)"
    PLUGIN_DEST="$HOME/.vst3"
elif [[ "$OS" == "Darwin" ]]; then
    PLUGIN_SRC="$(find build -name 'Patchwork.vst3' -type d 2>/dev/null | head -1)"
    PLUGIN_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
fi

if [[ -n "${PLUGIN_SRC:-}" && -d "$PLUGIN_SRC" ]]; then
    mkdir -p "$PLUGIN_DEST"
    cp -r "$PLUGIN_SRC" "$PLUGIN_DEST/"
    log "Plugin installed to $PLUGIN_DEST/Patchwork.vst3"
else
    warn "Built plugin not found automatically. Check build/ and install manually."
fi

log "Done! Open your DAW, rescan plugins, and load Patchwork."
log "Controller profiles: $PROFILES_DEST"
log "To add a custom profile, drop a .json file there — see docs/CONTROLLER_PROFILES.md"
