# Patchwork one-command setup for Windows
# Run in PowerShell as Administrator or with execution policy Bypass.
# Usage: .\scripts\setup.ps1
#
# Prerequisites: Git, CMake, Visual Studio 2019+, and optionally Scoop or winget.

$ErrorActionPreference = "Stop"
$RepoRoot = (Get-Item "$PSScriptRoot\..").FullName

function Log  { Write-Host "[setup] $args" -ForegroundColor Green }
function Warn { Write-Host "[setup] $args" -ForegroundColor Yellow }
function Die  { Write-Error "[setup] $args"; exit 1 }

Set-Location $RepoRoot

# ── Check prerequisites ────────────────────────────────────────────────────────
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Die "cmake not found. Download from https://cmake.org or install via: winget install cmake"
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Die "git not found. Download from https://git-scm.com"
}

# ── Install vcpkg dependencies (optional but recommended) ─────────────────────
if (Get-Command vcpkg -ErrorAction SilentlyContinue) {
    Log "Installing dependencies via vcpkg..."
    vcpkg install soundtouch:x64-windows libshout:x64-windows mp3lame:x64-windows libusb:x64-windows
} else {
    Warn "vcpkg not found. If the build fails, install vcpkg and rerun."
    Warn "https://github.com/microsoft/vcpkg"
}

# ── Clone JUCE ─────────────────────────────────────────────────────────────────
if (-not (Test-Path "JUCE")) {
    Log "Cloning JUCE framework..."
    git clone --depth=1 https://github.com/juce-framework/JUCE.git
} else {
    Log "JUCE directory already present — skipping clone."
}

# ── Install bundled controller profiles ───────────────────────────────────────
$ProfilesDest = "$env:APPDATA\Patchwork\profiles"
Log "Installing controller profiles to $ProfilesDest ..."
New-Item -ItemType Directory -Force -Path $ProfilesDest | Out-Null
Get-ChildItem profiles\*.json | ForEach-Object {
    $dest = Join-Path $ProfilesDest $_.Name
    if (-not (Test-Path $dest)) {
        Copy-Item $_.FullName $dest
    }
}
Log "Profiles installed (existing user profiles were not overwritten)."

# ── CMake configure + build ────────────────────────────────────────────────────
$Jobs = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
Log "Configuring build..."
cmake -B build -G "Visual Studio 17 2022" -A x64 $RepoRoot

Log "Building with $Jobs parallel jobs..."
cmake --build build --config Release -- /m:$Jobs

# ── Install plugin ─────────────────────────────────────────────────────────────
$PluginSrc = Get-ChildItem -Recurse build -Filter "Patchwork.vst3" -Directory |
             Select-Object -First 1 -ExpandProperty FullName
$PluginDest = "$env:APPDATA\VST3"

if ($PluginSrc) {
    New-Item -ItemType Directory -Force -Path $PluginDest | Out-Null
    Copy-Item -Recurse -Force $PluginSrc $PluginDest
    Log "Plugin installed to $PluginDest\Patchwork.vst3"
} else {
    Warn "Built plugin not found automatically. Check build\ and install manually."
}

Log "Done! Open your DAW, rescan plugins, and load Patchwork."
Log "Controller profiles: $ProfilesDest"
Log "To add a custom profile, drop a .json file there -- see docs/CONTROLLER_PROFILES.md"
