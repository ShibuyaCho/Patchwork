# Controller Profiles

Patchwork uses JSON files to describe how each MIDI controller maps to plugin
parameters.  You can edit existing profiles, create new ones for any controller,
and share them with the community — **no C++ or recompilation required**.

## Profile directory

Patchwork searches for `*.json` files in:

| Platform | Path |
|----------|------|
| Linux    | `~/.config/Patchwork/profiles/` |
| macOS    | `~/Library/Application Support/Patchwork/profiles/` |
| Windows  | `%APPDATA%\Patchwork\profiles\` |

The setup script (`scripts/setup.sh` / `scripts/setup.ps1`) copies the bundled
profiles there automatically.  Any file you add or edit in this directory takes
effect the next time the plugin loads — it will not be overwritten by updates.

---

## Profile format

```json
{
  "name": "My Controller",
  "match": ["mycontroller", "my ctrl"],

  "deckBChannel": 2,
  "globalChannel": 16,

  "padChannelA": 5,
  "padChannelB": 6,
  "padHotCueNote": 0,
  "padLoopNote": 11,
  "padSamplerNote": 14,
  "padNotes": [20, 21, 22, 23],

  "padLEDFirstNote": 36,
  "padLEDChannel": 1,

  "capabilities": ["DeckControls", "HotCues", "FXControls", "SamplerPads"],

  "cc": {
    "6":  "DeckAJog",
    "9":  "DeckATempo",
    "28": "DeckAVolume"
  },
  "notes": {
    "0": "DeckAPlay",
    "2": "DeckASync"
  },
  "globalCC": {
    "8": "Crossfader"
  },
  "globalNotes": {
    "2": "DeckALoad",
    "3": "DeckBLoad"
  }
}
```

### Top-level fields

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Display name shown in the MIDI monitor |
| `match` | array of strings | Sub-strings checked against the device name (case-insensitive). First match wins. |
| `deckBChannel` | int | If > 0, messages on this MIDI channel auto-remap `DeckA*` targets to `DeckB*`. |
| `globalChannel` | int | Channel for crossfader, master volume, and load buttons. |

### Pad mode system

Some controllers (Numark Party Mix, Pioneer DDJ) send pad presses on separate
channels with mode-select buttons.

| Field | Type | Description |
|-------|------|-------------|
| `padChannelA` | int | MIDI channel for Deck A performance pads (0 = disabled) |
| `padChannelB` | int | MIDI channel for Deck B performance pads (0 = disabled) |
| `padHotCueNote` | int | Note number that switches pads to Hot Cue mode (-1 = none) |
| `padLoopNote` | int | Note number that switches pads to Loop mode (-1 = none) |
| `padSamplerNote` | int | Note number that switches pads to Sampler mode (-1 = none) |
| `padNotes` | array[4] | Note numbers for pads 1–4 in the current mode |

### LED feedback

| Field | Type | Description |
|-------|------|-------------|
| `padLEDFirstNote` | int | Note number for sampler pad 0; pads 1–15 are +1, +2, …  Set to -1 to disable. |
| `padLEDChannel` | int | MIDI channel used for LED Note On messages |

### Maps

`cc`, `notes`, `globalCC`, and `globalNotes` are objects mapping **MIDI number
strings** (keys) to **target identifiers** (values).

Numbers are always decimal strings: `"9"`, not `9`.

---

## Target identifiers

### Deck controls (use `DeckA…` or `DeckB…`)

| Identifier | Description |
|------------|-------------|
| `DeckAVolume` / `DeckBVolume` | Channel volume fader |
| `DeckATempo` / `DeckBTempo` | Tempo / pitch fader |
| `DeckAPan` / `DeckBPan` | Pan |
| `DeckAGain` / `DeckBGain` | Pre-gain knob |
| `DeckAEQHigh` / `DeckBEQHigh` | EQ treble |
| `DeckAEQMid` / `DeckBEQMid` | EQ mid |
| `DeckAEQLow` / `DeckBEQLow` | EQ bass |
| `DeckAPlay` / `DeckBPlay` | Play/pause toggle |
| `DeckALoad` / `DeckBLoad` | Load track (open file dialog) |
| `DeckASync` / `DeckBSync` | BPM sync to the other deck |
| `DeckALoop` / `DeckBLoop` | Toggle active loop |
| `DeckACue` / `DeckBCue` | Headphone cue monitor |
| `DeckACuePt` / `DeckBCuePt` | DJ cue point (set or jump) |
| `DeckAJog` / `DeckBJog` | Jog wheel (relative CC, 64 = centre) |
| `DeckAScratchMode` / `DeckBScratchMode` | Toggle vinyl/CDJ jog mode |
| `DeckAHotCue0`–`DeckAHotCue3` | Hot cue buttons 1–4 for Deck A |
| `DeckBHotCue0`–`DeckBHotCue3` | Hot cue buttons 1–4 for Deck B |
| `DeckABeatLoop0`–`DeckABeatLoop3` | Auto-loop 0.5 / 1 / 2 / 4 bars, Deck A |
| `DeckBBeatLoop0`–`DeckBBeatLoop3` | Auto-loop 0.5 / 1 / 2 / 4 bars, Deck B |

### Sampler

`SamplerPad0` – `SamplerPad15`

### Effects / mixer

| Identifier | Description |
|------------|-------------|
| `Crossfader` | Main crossfader |
| `ReverbAmount` | Reverb wet mix |
| `DelayAmount` | Delay wet mix |
| `FilterFreq` | Filter cutoff frequency |

---

## Creating a profile for a new controller

1. Plug in the controller and open Patchwork in your DAW.
2. Check the **MIDI Monitor** panel to see which CC/note each control sends.
3. Copy `profiles/generic-pads.json` to the profiles directory with a new name,
   e.g. `my-controller.json`.
4. Edit `name`, `match`, and the `cc`/`notes` maps.
5. Reload the plugin (or restart your DAW) — your profile is now active.

Alternatively, use **MIDI Learn** in the plugin to map controls interactively,
then click **Export Profile** to save a JSON file you can share or fine-tune.

---

## Sharing profiles

Open a Pull Request adding your `*.json` file under `profiles/` in the
repository.  Community-contributed profiles are bundled with new releases so
everyone benefits.
