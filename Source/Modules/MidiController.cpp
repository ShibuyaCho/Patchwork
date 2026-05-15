#include "MidiController.h"

// ── Deck A -> Deck B target remap ────────────────────────────────────────────
// Used for dual-channel controllers (e.g. Party Mix ch1=DeckA, ch2=DeckB)
// where both channels send the same CC/note numbers.
MidiMapping::Target MidiController::remapDeckAtoB(MidiMapping::Target t)
{
    using T = MidiMapping::Target;
    switch (t)
    {
        case T::DeckAVolume:  return T::DeckBVolume;
        case T::DeckATempo:   return T::DeckBTempo;
        case T::DeckAPan:     return T::DeckBPan;
        case T::DeckAEQHigh:  return T::DeckBEQHigh;
        case T::DeckAEQMid:   return T::DeckBEQMid;
        case T::DeckAEQLow:   return T::DeckBEQLow;
        case T::DeckAPlay:    return T::DeckBPlay;
        case T::DeckALoad:    return T::DeckBLoad;
        case T::DeckASync:    return T::DeckBSync;
        case T::DeckALoop:    return T::DeckBLoop;
        case T::DeckACue:     return T::DeckBCue;
        case T::DeckAHotCue0: return T::DeckBHotCue0;
        case T::DeckAHotCue1: return T::DeckBHotCue1;
        case T::DeckAHotCue2:    return T::DeckBHotCue2;
        case T::DeckAHotCue3:    return T::DeckBHotCue3;
        case T::DeckAJog:        return T::DeckBJog;
        case T::DeckAScratchMode:return T::DeckBScratchMode;
        case T::DeckACuePt:      return T::DeckBCuePt;
        case T::DeckAGain:       return T::DeckBGain;
        case T::DeckABeatLoop0:  return T::DeckBBeatLoop0;
        case T::DeckABeatLoop1:  return T::DeckBBeatLoop1;
        case T::DeckABeatLoop2:  return T::DeckBBeatLoop2;
        case T::DeckABeatLoop3:  return T::DeckBBeatLoop3;
        default:                 return t;
    }
}

// ── Pad target resolver ───────────────────────────────────────────────────────
// Returns the right target for a pad press given mode and pad index (0-3).
// Sampler mode: deck A → pads 0-3, deck B → pads 4-7 (separate banks).
MidiMapping::Target MidiController::padTarget(int modeIdx, int padIdx, bool isDeckB)
{
    using T = MidiMapping::Target;
    if (padIdx < 0 || padIdx > 3) return T::Unknown;

    if (modeIdx == 0) // Hot Cue
    {
        const T a[] = { T::DeckAHotCue0, T::DeckAHotCue1, T::DeckAHotCue2, T::DeckAHotCue3 };
        const T b[] = { T::DeckBHotCue0, T::DeckBHotCue1, T::DeckBHotCue2, T::DeckBHotCue3 };
        return isDeckB ? b[padIdx] : a[padIdx];
    }
    if (modeIdx == 1) // Beat Loop (pads = 0.5 / 1 / 2 / 4 bars)
    {
        const T a[] = { T::DeckABeatLoop0, T::DeckABeatLoop1, T::DeckABeatLoop2, T::DeckABeatLoop3 };
        const T b[] = { T::DeckBBeatLoop0, T::DeckBBeatLoop1, T::DeckBBeatLoop2, T::DeckBBeatLoop3 };
        return isDeckB ? b[padIdx] : a[padIdx];
    }
    if (modeIdx == 2) // Sampler
    {
        int base = isDeckB ? 4 : 0;
        return static_cast<T>(static_cast<int>(T::SamplerPad0) + base + padIdx);
    }
    return T::Unknown;
}

// ── Target name table (for monitor display) ───────────────────────────────────
juce::String MidiMapping::targetName(Target t)
{
    switch (t)
    {
        case Target::DeckAVolume:  return "Deck A Volume";
        case Target::DeckBVolume:  return "Deck B Volume";
        case Target::DeckATempo:   return "Deck A Tempo";
        case Target::DeckBTempo:   return "Deck B Tempo";
        case Target::DeckAPan:     return "Deck A Pan";
        case Target::DeckBPan:     return "Deck B Pan";
        case Target::DeckAEQHigh:  return "Deck A EQ High";
        case Target::DeckAEQMid:   return "Deck A EQ Mid";
        case Target::DeckAEQLow:   return "Deck A EQ Low";
        case Target::DeckBEQHigh:  return "Deck B EQ High";
        case Target::DeckBEQMid:   return "Deck B EQ Mid";
        case Target::DeckBEQLow:   return "Deck B EQ Low";
        case Target::Crossfader:   return "Crossfader";
        case Target::ReverbAmount: return "Reverb";
        case Target::DelayAmount:  return "Delay";
        case Target::FilterFreq:   return "Filter Freq";
        case Target::DeckAPlay:    return "Deck A Play";
        case Target::DeckBPlay:    return "Deck B Play";
        case Target::DeckALoad:    return "Deck A Load";
        case Target::DeckBLoad:    return "Deck B Load";
        case Target::DeckASync:    return "Deck A Sync";
        case Target::DeckBSync:    return "Deck B Sync";
        case Target::DeckALoop:    return "Deck A Loop";
        case Target::DeckBLoop:    return "Deck B Loop";
        case Target::DeckACue:     return "Deck A Cue (Headphones)";
        case Target::DeckBCue:     return "Deck B Cue (Headphones)";
        case Target::DeckAHotCue0: return "Deck A Hot Cue 1";
        case Target::DeckAHotCue1: return "Deck A Hot Cue 2";
        case Target::DeckAHotCue2: return "Deck A Hot Cue 3";
        case Target::DeckAHotCue3: return "Deck A Hot Cue 4";
        case Target::DeckBHotCue0:    return "Deck B Hot Cue 1";
        case Target::DeckBHotCue1:    return "Deck B Hot Cue 2";
        case Target::DeckBHotCue2:    return "Deck B Hot Cue 3";
        case Target::DeckBHotCue3:    return "Deck B Hot Cue 4";
        case Target::DeckAJog:        return "Deck A Jog";
        case Target::DeckBJog:        return "Deck B Jog";
        case Target::DeckAScratchMode:return "Deck A Scratch Toggle";
        case Target::DeckBScratchMode:return "Deck B Scratch Toggle";
        case Target::DeckACuePt:      return "Deck A Cue Point";
        case Target::DeckBCuePt:      return "Deck B Cue Point";
        case Target::DeckAGain:       return "Deck A Gain";
        case Target::DeckBGain:       return "Deck B Gain";
        case Target::DeckABeatLoop0:  return "Deck A Loop 0.5 bar";
        case Target::DeckABeatLoop1:  return "Deck A Loop 1 bar";
        case Target::DeckABeatLoop2:  return "Deck A Loop 2 bars";
        case Target::DeckABeatLoop3:  return "Deck A Loop 4 bars";
        case Target::DeckBBeatLoop0:  return "Deck B Loop 0.5 bar";
        case Target::DeckBBeatLoop1:  return "Deck B Loop 1 bar";
        case Target::DeckBBeatLoop2:  return "Deck B Loop 2 bars";
        case Target::DeckBBeatLoop3:  return "Deck B Loop 4 bars";
        default:
        {
            int padBase = (int)Target::SamplerPad0;
            int idx     = (int)t - padBase;
            if (idx >= 0 && idx < 16)
                return "Sampler Pad " + juce::String(idx + 1);
            return "Unknown";
        }
    }
}

// ── Constructor / destructor ──────────────────────────────────────────────────

void MidiController::buildHIDMapping()
{
    // Maschine Mikro MK3 pads (report 0x02, pad indices 0-15) arrive as
    // note-on/off on channel 10, notes 36-51.  Map directly to sampler pads.
    for (int i = 0; i < 16; ++i)
        mapNote(hidMapping, 36 + i,
                static_cast<MidiMapping::Target>(
                    (int)MidiMapping::Target::SamplerPad0 + i));

    // Buttons (report 0x01, bytes 2-5) arrive as note-on/off on channel 10,
    // notes 80-111 (byte_offset*8 + bit).  Pre-wire the most useful ones;
    // the rest can be learnt via MIDI Learn.
    mapNote(hidMapping, 80,  MidiMapping::Target::DeckAPlay);
    mapNote(hidMapping, 81,  MidiMapping::Target::DeckBPlay);
    mapNote(hidMapping, 82,  MidiMapping::Target::DeckASync);
    mapNote(hidMapping, 83,  MidiMapping::Target::DeckBSync);
    mapNote(hidMapping, 84,  MidiMapping::Target::DeckALoop);
    mapNote(hidMapping, 85,  MidiMapping::Target::DeckBLoop);
    mapNote(hidMapping, 86,  MidiMapping::Target::DeckACue);
    mapNote(hidMapping, 87,  MidiMapping::Target::DeckBCue);

    hidMapping.channelFilter = 0;  // accept any channel
    hidMapping.profileName   = "HID (Maschine Mikro MK3)";
}

void MidiController::injectHIDMessage(const juce::MidiMessage& msg,
                                       const juce::String& sourceName)
{
    juce::ScopedLock sl(deviceLock);
    dispatchMessage(hidMapping, msg, sourceName);
}

MidiController::MidiController()
{
    buildHIDMapping();
    // Host-routed MIDI keeps the combined Numark + Maschine fallback
    applyNumarkPartyMix(hostFallback);
    applyMaschineMikro(hostFallback);
    hostFallback.profileName = "Host MIDI";
}

MidiController::~MidiController()
{
    juce::ScopedLock sl(deviceLock);
    for (auto& d : activeDevices)
        if (d.input) d.input->stop();
    activeDevices.clear();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MidiController::mapCC(DeviceMapping& m, int cc, MidiMapping::Target t, bool isButton)
{
    m.ccMap[cc] = { t, isButton };
}

void MidiController::mapNote(DeviceMapping& m, int note, MidiMapping::Target t)
{
    m.noteMap[note] = { t, true };
}

// ── Device blocklist ──────────────────────────────────────────────────────────
// Devices listed here are NOT opened directly — route them through the DAW.
// Maschine is intentionally NOT blocked: Patchwork opens the MIDI port and
// sends Note On LED feedback. Close NI software first to avoid HID conflicts.
bool MidiController::shouldOpenDirectly(const juce::String& name)
{
    auto lo = name.toLowerCase();
    if (lo.contains("traktor kontrol"))  return false;
    if (lo.contains("komplete kontrol")) return false;
    return true;
}

// ── Profile builders ──────────────────────────────────────────────────────────

void MidiController::applyNumarkPartyMix(DeviceMapping& m)
{
    // Numark Party Mix: Ch1=Deck A, Ch2=Deck B — same CC/note numbers on both.
    // deckBChannel = 2 auto-remaps DeckA* -> DeckB* for all Ch2 messages.
    // Global controls (crossfader, master) arrive on Ch 16 with different note numbers.
    // Pads arrive on Ch 5 (Deck A) / Ch 6 (Deck B) with mode-select buttons.
    m.deckBChannel = 2;

    // ── Per-deck CCs (verified against Mixxx mapping) ─────────────────────
    mapCC(m, 6,  MidiMapping::Target::DeckAJog);     // jog wheel, relative (64=center)
    mapCC(m, 9,  MidiMapping::Target::DeckATempo);   // pitch/rate fader
    mapCC(m, 23, MidiMapping::Target::DeckAGain);    // pregain knob
    mapCC(m, 24, MidiMapping::Target::DeckAEQHigh);  // treble (no mid on Party Mix)
    mapCC(m, 25, MidiMapping::Target::DeckAEQLow);   // bass
    mapCC(m, 28, MidiMapping::Target::DeckAVolume);  // channel volume fader

    // ── Per-deck notes ────────────────────────────────────────────────────
    mapNote(m, 0,  MidiMapping::Target::DeckAPlay);
    mapNote(m, 1,  MidiMapping::Target::DeckACuePt);    // DJ cue point (not headphones)
    mapNote(m, 2,  MidiMapping::Target::DeckASync);
    mapNote(m, 7,  MidiMapping::Target::DeckAScratchMode); // vinyl/CDJ toggle
    mapNote(m, 27, MidiMapping::Target::DeckACue);      // PFL = headphone monitor

    // ── Global channel (Ch 16) — crossfader, master, load buttons ─────────
    m.globalChannel = 16;
    m.globalCCMap[8]  = { MidiMapping::Target::Crossfader,  false };
    m.globalCCMap[10] = { MidiMapping::Target::DeckAGain,   false }; // master gain
    m.globalNoteMap[2] = { MidiMapping::Target::DeckALoad,  true  }; // Load Deck A
    m.globalNoteMap[3] = { MidiMapping::Target::DeckBLoad,  true  }; // Load Deck B

    // ── Pad channels: Ch 5 = Deck A pads, Ch 6 = Deck B pads ──────────────
    // Mode buttons select what the 4 performance pads do.
    m.padChannelA    = 5;
    m.padChannelB    = 6;
    m.padHotCueNote  = 0;    // HotCue mode
    m.padLoopNote    = 11;   // Loop mode
    m.padSamplerNote = 14;   // Sampler mode  (note 24 = Effect mode, unused)
    m.padNotes       = { { 20, 21, 22, 23 } };

    m.capabilities = { DeviceCapability::DeckControls, DeviceCapability::HotCues,
                       DeviceCapability::FXControls, DeviceCapability::SamplerPads };
    m.profileName  = "Numark Party Mix";
}

void MidiController::applyMaschineMikro(DeviceMapping& m)
{
    // Maschine Mikro in MIDI mode: 16 pads on Ch 1, notes 36-51 (GM drum range).
    // MK1/MK2/MK3 all use this layout when no NI software is running.
    // Close Maschine/NI software before use to avoid HID LED conflicts.
    for (int i = 0; i < 16; ++i)
        mapNote(m, 36 + i, static_cast<MidiMapping::Target>(
            static_cast<int>(MidiMapping::Target::SamplerPad0) + i));

    // Encoder knobs (user-assignable in Maschine's MIDI editor)
    mapCC(m, 70, MidiMapping::Target::ReverbAmount);
    mapCC(m, 71, MidiMapping::Target::DelayAmount);
    mapCC(m, 72, MidiMapping::Target::FilterFreq);

    // LED feedback: send Note On (velocity = brightness) back to device.
    // Empty pad = off (0), loaded = dim (45), playing = full (127).
    m.padLEDFirstNote = 36;
    m.padLEDChannel   = 1;

    m.capabilities = { DeviceCapability::SamplerPads, DeviceCapability::FXControls };
    m.profileName  = "Maschine Mikro";
}

void MidiController::applyLaunchpad(DeviceMapping& m)
{
    // Programmer mode layout: note = row*10 + col (1-indexed, bottom-left origin)
    // Bottom-left 4x4 → sampler pads 0-15
    int pad = 0;
    for (int row = 1; row <= 4; ++row)
        for (int col = 1; col <= 4; ++col, ++pad)
            mapNote(m, row * 10 + col,
                    static_cast<MidiMapping::Target>(
                        static_cast<int>(MidiMapping::Target::SamplerPad0) + pad));

    // Right scene buttons → transport
    mapNote(m, 19, MidiMapping::Target::DeckAPlay);
    mapNote(m, 29, MidiMapping::Target::DeckACue);
    mapNote(m, 39, MidiMapping::Target::DeckBPlay);
    mapNote(m, 49, MidiMapping::Target::DeckBCue);
    mapNote(m, 59, MidiMapping::Target::DeckASync);
    mapNote(m, 69, MidiMapping::Target::DeckBSync);
    mapNote(m, 79, MidiMapping::Target::DeckALoop);
    mapNote(m, 89, MidiMapping::Target::DeckBLoop);

    // Top row → hot cues
    mapNote(m, 91, MidiMapping::Target::DeckAHotCue0);
    mapNote(m, 92, MidiMapping::Target::DeckAHotCue1);
    mapNote(m, 93, MidiMapping::Target::DeckAHotCue2);
    mapNote(m, 94, MidiMapping::Target::DeckAHotCue3);
    mapNote(m, 95, MidiMapping::Target::DeckBHotCue0);
    mapNote(m, 96, MidiMapping::Target::DeckBHotCue1);
    mapNote(m, 97, MidiMapping::Target::DeckBHotCue2);
    mapNote(m, 98, MidiMapping::Target::DeckBHotCue3);

    m.capabilities = { DeviceCapability::SamplerPads, DeviceCapability::HotCues };
    m.profileName  = "Launchpad";
}

void MidiController::applyAkaiAPC(DeviceMapping& m)
{
    // APC Mini: grid notes 0-63, first 16 → pads; APC40 adds faders
    for (int i = 0; i < 16; ++i)
        mapNote(m, i, static_cast<MidiMapping::Target>(
            static_cast<int>(MidiMapping::Target::SamplerPad0) + i));

    mapCC(m, 7,  MidiMapping::Target::DeckAVolume);
    mapCC(m, 11, MidiMapping::Target::DeckBVolume);
    mapCC(m, 14, MidiMapping::Target::Crossfader);
    mapCC(m, 16, MidiMapping::Target::DeckATempo);
    mapCC(m, 17, MidiMapping::Target::DeckBTempo);

    m.capabilities = { DeviceCapability::SamplerPads, DeviceCapability::DeckControls };
    m.profileName  = "Akai APC";
}

void MidiController::applyGenericDJ(DeviceMapping& m)
{
    // Best-guess layout for Pioneer DDJ / Hercules / Denon
    mapCC(m, 8,  MidiMapping::Target::DeckAVolume);
    mapCC(m, 9,  MidiMapping::Target::DeckBVolume);
    mapCC(m, 23, MidiMapping::Target::Crossfader);
    mapCC(m, 10, MidiMapping::Target::DeckATempo);
    mapCC(m, 11, MidiMapping::Target::DeckBTempo);
    mapCC(m, 40, MidiMapping::Target::DeckAEQLow);
    mapCC(m, 41, MidiMapping::Target::DeckAEQMid);
    mapCC(m, 42, MidiMapping::Target::DeckAEQHigh);
    mapCC(m, 43, MidiMapping::Target::DeckBEQLow);
    mapCC(m, 44, MidiMapping::Target::DeckBEQMid);
    mapCC(m, 45, MidiMapping::Target::DeckBEQHigh);
    mapCC(m, 80, MidiMapping::Target::ReverbAmount);
    mapCC(m, 81, MidiMapping::Target::DelayAmount);
    mapCC(m, 82, MidiMapping::Target::FilterFreq);

    mapNote(m, 11, MidiMapping::Target::DeckAPlay);
    mapNote(m, 12, MidiMapping::Target::DeckBPlay);
    mapNote(m, 13, MidiMapping::Target::DeckASync);
    mapNote(m, 14, MidiMapping::Target::DeckBSync);
    mapNote(m, 15, MidiMapping::Target::DeckALoop);
    mapNote(m, 16, MidiMapping::Target::DeckBLoop);
    mapNote(m, 20, MidiMapping::Target::DeckAHotCue0);
    mapNote(m, 21, MidiMapping::Target::DeckAHotCue1);
    mapNote(m, 22, MidiMapping::Target::DeckAHotCue2);
    mapNote(m, 23, MidiMapping::Target::DeckAHotCue3);
    mapNote(m, 24, MidiMapping::Target::DeckBHotCue0);
    mapNote(m, 25, MidiMapping::Target::DeckBHotCue1);
    mapNote(m, 26, MidiMapping::Target::DeckBHotCue2);
    mapNote(m, 27, MidiMapping::Target::DeckBHotCue3);

    m.capabilities = { DeviceCapability::DeckControls, DeviceCapability::HotCues,
                       DeviceCapability::FXControls };
    m.profileName  = "DJ Controller";
}

// ── Pioneer DDJ-SB3 / DDJ-400 / DDJ-200 / DDJ-SR2 ───────────────────────────
// Channel 1 = Deck 1 (left), Channel 2 = Deck 2 (right).
// CC and note numbers verified against Mixxx DDJ-SB3 mapping.
void MidiController::applyPioneerDDJ(DeviceMapping& m)
{
    m.deckBChannel = 2;

    // Per-deck CCs
    mapCC(m, 0x00,  MidiMapping::Target::DeckAGain);    // pregain
    mapCC(m, 0x09,  MidiMapping::Target::DeckATempo);   // tempo fader
    mapCC(m, 0x07,  MidiMapping::Target::DeckAVolume);  // channel volume
    mapCC(m, 0x26,  MidiMapping::Target::DeckAEQHigh);  // EQ Hi
    mapCC(m, 0x27,  MidiMapping::Target::DeckAEQMid);   // EQ Mid
    mapCC(m, 0x28,  MidiMapping::Target::DeckAEQLow);   // EQ Lo
    mapCC(m, 0x06,  MidiMapping::Target::DeckAJog);     // jog wheel (relative)

    // Per-deck notes
    mapNote(m, 0x0B, MidiMapping::Target::DeckAPlay);
    mapNote(m, 0x0C, MidiMapping::Target::DeckACuePt);
    mapNote(m, 0x2D, MidiMapping::Target::DeckASync);
    mapNote(m, 0x40, MidiMapping::Target::DeckALoop);   // auto-loop
    mapNote(m, 0x46, MidiMapping::Target::DeckAHotCue0);
    mapNote(m, 0x47, MidiMapping::Target::DeckAHotCue1);
    mapNote(m, 0x48, MidiMapping::Target::DeckAHotCue2);
    mapNote(m, 0x49, MidiMapping::Target::DeckAHotCue3);
    mapNote(m, 0x0D, MidiMapping::Target::DeckACue);    // PFL
    mapNote(m, 0x3A, MidiMapping::Target::DeckAScratchMode);

    // Global channel 16 — crossfader, load buttons
    m.globalChannel = 16;
    m.globalCCMap[0x3F] = { MidiMapping::Target::Crossfader, false };
    m.globalNoteMap[0x28] = { MidiMapping::Target::DeckALoad, true };
    m.globalNoteMap[0x29] = { MidiMapping::Target::DeckBLoad, true };

    m.capabilities = { DeviceCapability::DeckControls, DeviceCapability::HotCues,
                       DeviceCapability::FXControls, DeviceCapability::SamplerPads };
    m.profileName  = "Pioneer DDJ";
}

// ── Hercules DJControl Inpulse 200/300/500 ───────────────────────────────────
// Channel 1 = Deck 1, Channel 2 = Deck 2 (same note/CC numbers on both).
void MidiController::applyHercules(DeviceMapping& m)
{
    m.deckBChannel = 2;

    mapCC(m, 0x09,  MidiMapping::Target::DeckATempo);
    mapCC(m, 0x01,  MidiMapping::Target::DeckAVolume);
    mapCC(m, 0x16,  MidiMapping::Target::DeckAEQHigh);
    mapCC(m, 0x15,  MidiMapping::Target::DeckAEQMid);
    mapCC(m, 0x14,  MidiMapping::Target::DeckAEQLow);
    mapCC(m, 0x00,  MidiMapping::Target::DeckAGain);
    mapCC(m, 0x06,  MidiMapping::Target::DeckAJog);

    mapNote(m, 0x00, MidiMapping::Target::DeckAPlay);
    mapNote(m, 0x01, MidiMapping::Target::DeckASync);
    mapNote(m, 0x02, MidiMapping::Target::DeckACuePt);
    mapNote(m, 0x03, MidiMapping::Target::DeckALoop);
    mapNote(m, 0x04, MidiMapping::Target::DeckAHotCue0);
    mapNote(m, 0x05, MidiMapping::Target::DeckAHotCue1);
    mapNote(m, 0x06, MidiMapping::Target::DeckAHotCue2);
    mapNote(m, 0x07, MidiMapping::Target::DeckAHotCue3);
    mapNote(m, 0x09, MidiMapping::Target::DeckACue);

    m.globalChannel = 16;
    m.globalCCMap[0x11] = { MidiMapping::Target::Crossfader, false };
    m.globalNoteMap[0x00] = { MidiMapping::Target::DeckALoad, true };
    m.globalNoteMap[0x01] = { MidiMapping::Target::DeckBLoad, true };

    m.capabilities = { DeviceCapability::DeckControls, DeviceCapability::HotCues };
    m.profileName  = "Hercules DJControl";
}

// ── Denon MC3000 / SC2000 / MC4000 ──────────────────────────────────────────
void MidiController::applyDenonMC(DeviceMapping& m)
{
    m.deckBChannel = 2;

    mapCC(m, 0x12,  MidiMapping::Target::DeckATempo);
    mapCC(m, 0x07,  MidiMapping::Target::DeckAVolume);
    mapCC(m, 0x08,  MidiMapping::Target::DeckAEQHigh);
    mapCC(m, 0x09,  MidiMapping::Target::DeckAEQMid);
    mapCC(m, 0x0A,  MidiMapping::Target::DeckAEQLow);
    mapCC(m, 0x0C,  MidiMapping::Target::DeckAGain);
    mapCC(m, 0x06,  MidiMapping::Target::DeckAJog);

    mapNote(m, 0x00, MidiMapping::Target::DeckAPlay);
    mapNote(m, 0x01, MidiMapping::Target::DeckACuePt);
    mapNote(m, 0x02, MidiMapping::Target::DeckASync);
    mapNote(m, 0x03, MidiMapping::Target::DeckALoop);
    mapNote(m, 0x10, MidiMapping::Target::DeckAHotCue0);
    mapNote(m, 0x11, MidiMapping::Target::DeckAHotCue1);
    mapNote(m, 0x12, MidiMapping::Target::DeckAHotCue2);
    mapNote(m, 0x13, MidiMapping::Target::DeckAHotCue3);
    mapNote(m, 0x0C, MidiMapping::Target::DeckACue);

    m.globalChannel = 16;
    m.globalCCMap[0x0E] = { MidiMapping::Target::Crossfader, false };
    m.globalNoteMap[0x28] = { MidiMapping::Target::DeckALoad, true };
    m.globalNoteMap[0x29] = { MidiMapping::Target::DeckBLoad, true };

    m.capabilities = { DeviceCapability::DeckControls, DeviceCapability::HotCues };
    m.profileName  = "Denon MC";
}

void MidiController::applyGenericPads(DeviceMapping& m)
{
    // Any unrecognised device: notes 0-15 and GM drum range 36-51 → sampler pads
    for (int i = 0; i < 16; ++i)
    {
        auto t = static_cast<MidiMapping::Target>(
            static_cast<int>(MidiMapping::Target::SamplerPad0) + i);
        mapNote(m, i,      t);
        mapNote(m, 36 + i, t);
    }
    m.capabilities = { DeviceCapability::SamplerPads };
    m.profileName  = "Generic (use MIDI Learn)";
}

// ── JSON Profile system ───────────────────────────────────────────────────────

juce::File MidiController::getUserProfilesDir()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("Patchwork/profiles");
}

MidiMapping::Target MidiController::targetFromString(const juce::String& id)
{
    using T = MidiMapping::Target;
    // Named targets
    if (id == "DeckAVolume")     return T::DeckAVolume;
    if (id == "DeckBVolume")     return T::DeckBVolume;
    if (id == "DeckATempo")      return T::DeckATempo;
    if (id == "DeckBTempo")      return T::DeckBTempo;
    if (id == "DeckAPan")        return T::DeckAPan;
    if (id == "DeckBPan")        return T::DeckBPan;
    if (id == "DeckAEQHigh")     return T::DeckAEQHigh;
    if (id == "DeckAEQMid")      return T::DeckAEQMid;
    if (id == "DeckAEQLow")      return T::DeckAEQLow;
    if (id == "DeckBEQHigh")     return T::DeckBEQHigh;
    if (id == "DeckBEQMid")      return T::DeckBEQMid;
    if (id == "DeckBEQLow")      return T::DeckBEQLow;
    if (id == "Crossfader")      return T::Crossfader;
    if (id == "ReverbAmount")    return T::ReverbAmount;
    if (id == "DelayAmount")     return T::DelayAmount;
    if (id == "FilterFreq")      return T::FilterFreq;
    if (id == "DeckAPlay")       return T::DeckAPlay;
    if (id == "DeckBPlay")       return T::DeckBPlay;
    if (id == "DeckALoad")       return T::DeckALoad;
    if (id == "DeckBLoad")       return T::DeckBLoad;
    if (id == "DeckASync")       return T::DeckASync;
    if (id == "DeckBSync")       return T::DeckBSync;
    if (id == "DeckALoop")       return T::DeckALoop;
    if (id == "DeckBLoop")       return T::DeckBLoop;
    if (id == "DeckACue")        return T::DeckACue;
    if (id == "DeckBCue")        return T::DeckBCue;
    if (id == "DeckAHotCue0")    return T::DeckAHotCue0;
    if (id == "DeckAHotCue1")    return T::DeckAHotCue1;
    if (id == "DeckAHotCue2")    return T::DeckAHotCue2;
    if (id == "DeckAHotCue3")    return T::DeckAHotCue3;
    if (id == "DeckBHotCue0")    return T::DeckBHotCue0;
    if (id == "DeckBHotCue1")    return T::DeckBHotCue1;
    if (id == "DeckBHotCue2")    return T::DeckBHotCue2;
    if (id == "DeckBHotCue3")    return T::DeckBHotCue3;
    if (id == "DeckAJog")        return T::DeckAJog;
    if (id == "DeckBJog")        return T::DeckBJog;
    if (id == "DeckAScratchMode")return T::DeckAScratchMode;
    if (id == "DeckBScratchMode")return T::DeckBScratchMode;
    if (id == "DeckACuePt")      return T::DeckACuePt;
    if (id == "DeckBCuePt")      return T::DeckBCuePt;
    if (id == "DeckAGain")       return T::DeckAGain;
    if (id == "DeckBGain")       return T::DeckBGain;
    if (id == "DeckABeatLoop0")  return T::DeckABeatLoop0;
    if (id == "DeckABeatLoop1")  return T::DeckABeatLoop1;
    if (id == "DeckABeatLoop2")  return T::DeckABeatLoop2;
    if (id == "DeckABeatLoop3")  return T::DeckABeatLoop3;
    if (id == "DeckBBeatLoop0")  return T::DeckBBeatLoop0;
    if (id == "DeckBBeatLoop1")  return T::DeckBBeatLoop1;
    if (id == "DeckBBeatLoop2")  return T::DeckBBeatLoop2;
    if (id == "DeckBBeatLoop3")  return T::DeckBBeatLoop3;
    // Range: SamplerPad0 – SamplerPad15
    if (id.startsWith("SamplerPad"))
    {
        int idx = id.substring(10).getIntValue();
        if (idx >= 0 && idx < 16)
            return static_cast<T>(static_cast<int>(T::SamplerPad0) + idx);
    }
    return T::Unknown;
}

juce::String MidiMapping::targetId(Target t)
{
    using T = Target;
    switch (t)
    {
        case T::DeckAVolume:     return "DeckAVolume";
        case T::DeckBVolume:     return "DeckBVolume";
        case T::DeckATempo:      return "DeckATempo";
        case T::DeckBTempo:      return "DeckBTempo";
        case T::DeckAPan:        return "DeckAPan";
        case T::DeckBPan:        return "DeckBPan";
        case T::DeckAEQHigh:     return "DeckAEQHigh";
        case T::DeckAEQMid:      return "DeckAEQMid";
        case T::DeckAEQLow:      return "DeckAEQLow";
        case T::DeckBEQHigh:     return "DeckBEQHigh";
        case T::DeckBEQMid:      return "DeckBEQMid";
        case T::DeckBEQLow:      return "DeckBEQLow";
        case T::Crossfader:      return "Crossfader";
        case T::ReverbAmount:    return "ReverbAmount";
        case T::DelayAmount:     return "DelayAmount";
        case T::FilterFreq:      return "FilterFreq";
        case T::DeckAPlay:       return "DeckAPlay";
        case T::DeckBPlay:       return "DeckBPlay";
        case T::DeckALoad:       return "DeckALoad";
        case T::DeckBLoad:       return "DeckBLoad";
        case T::DeckASync:       return "DeckASync";
        case T::DeckBSync:       return "DeckBSync";
        case T::DeckALoop:       return "DeckALoop";
        case T::DeckBLoop:       return "DeckBLoop";
        case T::DeckACue:        return "DeckACue";
        case T::DeckBCue:        return "DeckBCue";
        case T::DeckAHotCue0:    return "DeckAHotCue0";
        case T::DeckAHotCue1:    return "DeckAHotCue1";
        case T::DeckAHotCue2:    return "DeckAHotCue2";
        case T::DeckAHotCue3:    return "DeckAHotCue3";
        case T::DeckBHotCue0:    return "DeckBHotCue0";
        case T::DeckBHotCue1:    return "DeckBHotCue1";
        case T::DeckBHotCue2:    return "DeckBHotCue2";
        case T::DeckBHotCue3:    return "DeckBHotCue3";
        case T::DeckAJog:        return "DeckAJog";
        case T::DeckBJog:        return "DeckBJog";
        case T::DeckAScratchMode:return "DeckAScratchMode";
        case T::DeckBScratchMode:return "DeckBScratchMode";
        case T::DeckACuePt:      return "DeckACuePt";
        case T::DeckBCuePt:      return "DeckBCuePt";
        case T::DeckAGain:       return "DeckAGain";
        case T::DeckBGain:       return "DeckBGain";
        case T::DeckABeatLoop0:  return "DeckABeatLoop0";
        case T::DeckABeatLoop1:  return "DeckABeatLoop1";
        case T::DeckABeatLoop2:  return "DeckABeatLoop2";
        case T::DeckABeatLoop3:  return "DeckABeatLoop3";
        case T::DeckBBeatLoop0:  return "DeckBBeatLoop0";
        case T::DeckBBeatLoop1:  return "DeckBBeatLoop1";
        case T::DeckBBeatLoop2:  return "DeckBBeatLoop2";
        case T::DeckBBeatLoop3:  return "DeckBBeatLoop3";
        default:
        {
            int idx = static_cast<int>(t) - static_cast<int>(T::SamplerPad0);
            if (idx >= 0 && idx < 16)
                return "SamplerPad" + juce::String(idx);
            return "Unknown";
        }
    }
}

DeviceMapping MidiController::profileFromJSON(const juce::var& json)
{
    DeviceMapping m;
    m.profileName    = json["name"].toString();
    m.deckBChannel   = (int)json["deckBChannel"];
    m.globalChannel  = (int)json["globalChannel"];
    m.padChannelA    = (int)json["padChannelA"];
    m.padChannelB    = (int)json["padChannelB"];
    m.padHotCueNote  = json["padHotCueNote"].isVoid()  ? -1 : (int)json["padHotCueNote"];
    m.padLoopNote    = json["padLoopNote"].isVoid()    ? -1 : (int)json["padLoopNote"];
    m.padSamplerNote = json["padSamplerNote"].isVoid() ? -1 : (int)json["padSamplerNote"];
    m.padLEDFirstNote= json["padLEDFirstNote"].isVoid()? -1 : (int)json["padLEDFirstNote"];
    m.padLEDChannel  = json["padLEDChannel"].isVoid()  ?  1 : (int)json["padLEDChannel"];

    auto padNotesVar = json["padNotes"];
    if (padNotesVar.isArray())
        for (int i = 0; i < 4 && i < padNotesVar.size(); ++i)
            m.padNotes[(size_t)i] = (int)padNotesVar[i];

    // Helper: parse a {"number": "TargetId"} JSON object into a map entry
    auto parseMap = [&](const char* key, std::map<int, MidiMapping>& outMap, bool isButton)
    {
        auto obj = json[key];
        if (auto* dynObj = obj.getDynamicObject())
            for (auto& prop : dynObj->getProperties())
            {
                int  number = prop.name.toString().getIntValue();
                auto target = targetFromString(prop.value.toString());
                if (target != MidiMapping::Target::Unknown)
                    outMap[number] = { target, isButton };
            }
    };
    parseMap("cc",           m.ccMap,         false);
    parseMap("notes",        m.noteMap,        true);
    parseMap("globalCC",     m.globalCCMap,    false);
    parseMap("globalNotes",  m.globalNoteMap,  true);

    auto capsVar = json["capabilities"];
    if (capsVar.isArray())
        for (int i = 0; i < capsVar.size(); ++i)
        {
            auto cap = capsVar[i].toString();
            if      (cap == "DeckControls") m.capabilities.push_back(DeviceCapability::DeckControls);
            else if (cap == "SamplerPads")  m.capabilities.push_back(DeviceCapability::SamplerPads);
            else if (cap == "HotCues")      m.capabilities.push_back(DeviceCapability::HotCues);
            else if (cap == "FXControls")   m.capabilities.push_back(DeviceCapability::FXControls);
        }

    return m;
}

bool MidiController::tryLoadJSONProfile(const juce::String& deviceName, DeviceMapping& out)
{
    const juce::File searchDirs[] = {
        getUserProfilesDir(),
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory().getChildFile("profiles"),
    };

    auto lo = deviceName.toLowerCase();

    for (auto& dir : searchDirs)
    {
        if (!dir.isDirectory()) continue;
        for (auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto json = juce::JSON::parse(f);
            if (!json.isObject()) continue;

            auto matchVar = json["match"];
            if (!matchVar.isArray()) continue;

            for (int i = 0; i < matchVar.size(); ++i)
            {
                if (lo.contains(matchVar[i].toString().toLowerCase()))
                {
                    out = profileFromJSON(json);
                    return true;
                }
            }
        }
    }
    return false;
}

void MidiController::exportProfileToJSON(const juce::String& deviceName,
                                          const juce::File&   outputFile) const
{
    DeviceMapping mapping;
    {
        juce::ScopedLock sl(deviceLock);
        for (auto& d : activeDevices)
            if (d.name == deviceName) { mapping = d.mapping; break; }
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("name",          mapping.profileName.isEmpty() ? deviceName : mapping.profileName);
    root->setProperty("match",         juce::Array<juce::var>{ deviceName.toLowerCase() });
    root->setProperty("deckBChannel",  mapping.deckBChannel);
    root->setProperty("globalChannel", mapping.globalChannel);
    root->setProperty("padChannelA",   mapping.padChannelA);
    root->setProperty("padChannelB",   mapping.padChannelB);
    root->setProperty("padHotCueNote", mapping.padHotCueNote);
    root->setProperty("padLoopNote",   mapping.padLoopNote);
    root->setProperty("padSamplerNote",mapping.padSamplerNote);
    root->setProperty("padLEDFirstNote",mapping.padLEDFirstNote);
    root->setProperty("padLEDChannel", mapping.padLEDChannel);

    juce::Array<juce::var> padNotesArr;
    for (int n : mapping.padNotes) padNotesArr.add(n);
    root->setProperty("padNotes", padNotesArr);

    auto buildMapVar = [](const std::map<int, MidiMapping>& map) -> juce::var
    {
        auto* obj = new juce::DynamicObject();
        for (auto& [num, mm] : map)
            obj->setProperty(juce::String(num), MidiMapping::targetId(mm.target));
        return juce::var(obj);
    };
    root->setProperty("cc",          buildMapVar(mapping.ccMap));
    root->setProperty("notes",       buildMapVar(mapping.noteMap));
    root->setProperty("globalCC",    buildMapVar(mapping.globalCCMap));
    root->setProperty("globalNotes", buildMapVar(mapping.globalNoteMap));

    juce::Array<juce::var> caps;
    for (auto cap : mapping.capabilities)
    {
        switch (cap)
        {
            case DeviceCapability::DeckControls: caps.add("DeckControls"); break;
            case DeviceCapability::SamplerPads:  caps.add("SamplerPads");  break;
            case DeviceCapability::HotCues:      caps.add("HotCues");      break;
            case DeviceCapability::FXControls:   caps.add("FXControls");   break;
        }
    }
    root->setProperty("capabilities", caps);

    outputFile.getParentDirectory().createDirectory();
    outputFile.replaceWithText(juce::JSON::toString(juce::var(root), true));
}


// ── Profile selection ─────────────────────────────────────────────────────────

DeviceMapping MidiController::buildProfile(const juce::String& name)
{
    DeviceMapping m;

    // JSON profiles take priority — users can customise or add controllers without recompiling.
    if (tryLoadJSONProfile(name, m))
        return m;

    auto lo = name.toLowerCase();

    if      (lo.contains("launchpad"))
        applyLaunchpad(m);
    else if (lo.contains("apc"))
        applyAkaiAPC(m);
    else if (lo.contains("party mix") || lo.contains("partymix"))
        applyNumarkPartyMix(m);
    else if (lo.contains("maschine") || lo.contains("mikro") ||
             lo.contains("native instruments") || lo.contains("ni maschine"))
        applyMaschineMikro(m);
    else if (lo.contains("ddj") || lo.contains("pioneer dj"))
        applyPioneerDDJ(m);
    else if (lo.contains("hercules") || lo.contains("inpulse") || lo.contains("djcontrol"))
        applyHercules(m);
    else if (lo.contains("denon") || lo.contains("mc3000") || lo.contains("mc4000"))
        applyDenonMC(m);
    else if (lo.contains("pioneer") || lo.contains("numark") ||
             lo.contains("mix")     ||
             (lo.contains("dj") && !lo.contains("ableton")))
        applyGenericDJ(m);
    else
        applyGenericPads(m);

    return m;
}

// ── Device lifecycle ──────────────────────────────────────────────────────────

void MidiController::scanDevices()
{
    auto available = juce::MidiInput::getAvailableDevices();

    // Close devices that disappeared
    {
        juce::ScopedLock sl(deviceLock);
        for (int i = (int)activeDevices.size() - 1; i >= 0; --i)
        {
            bool found = false;
            for (auto& info : available)
                if (info.identifier == activeDevices[i].identifier)
                    { found = true; break; }
            if (!found)
            {
                if (activeDevices[i].input) activeDevices[i].input->stop();
                activeDevices.erase(activeDevices.begin() + i);
            }
        }
    }

    // Open new devices
    juce::StringArray newBlocked;
    for (auto& info : available)
    {
        bool alreadyOpen    = false;
        bool alreadyBlocked = false;
        {
            juce::ScopedLock sl(deviceLock);
            for (auto& d : activeDevices)
                if (d.identifier == info.identifier)
                    { alreadyOpen = true; break; }
            alreadyBlocked = blockedDeviceNames.contains(info.name);
        }
        if (alreadyOpen || alreadyBlocked) continue;

        if (!shouldOpenDirectly(info.name))
        {
            juce::ScopedLock sl(deviceLock);
            if (!blockedDeviceNames.contains(info.name))
                blockedDeviceNames.add(info.name);
        }
        else
        {
            openDevice(info);
        }
    }

    // Remove blocked entries for devices that are no longer plugged in
    {
        juce::ScopedLock sl(deviceLock);
        juce::StringArray stillPresent;
        for (auto& info : available)
            if (!shouldOpenDirectly(info.name))
                stillPresent.add(info.name);
        for (int i = blockedDeviceNames.size() - 1; i >= 0; --i)
            if (!stillPresent.contains(blockedDeviceNames[i]))
                blockedDeviceNames.remove(i);
    }
}

void MidiController::openDevice(const juce::MidiDeviceInfo& info)
{
    auto input = juce::MidiInput::openDevice(info.identifier, this);
    if (!input) return;

    // Try to open a matching MIDI output port for LED feedback.
    // On most systems the output port has the same name as the input.
    std::unique_ptr<juce::MidiOutput> output;
    for (auto& outInfo : juce::MidiOutput::getAvailableDevices())
    {
        if (outInfo.name == info.name)
        {
            output = juce::MidiOutput::openDevice(outInfo.identifier);
            break;
        }
    }

    ActiveDevice d;
    d.identifier = info.identifier;
    d.name       = info.name;
    d.mapping    = buildProfile(info.name);
    d.input      = std::move(input);
    d.output     = std::move(output);
    d.input->start();

    juce::ScopedLock sl(deviceLock);
    activeDevices.push_back(std::move(d));
}

void MidiController::closeDevice(const juce::String& identifier)
{
    juce::ScopedLock sl(deviceLock);
    for (int i = (int)activeDevices.size() - 1; i >= 0; --i)
        if (activeDevices[i].identifier == identifier)
        {
            if (activeDevices[i].input) activeDevices[i].input->stop();
            activeDevices.erase(activeDevices.begin() + i);
            return;
        }
}

juce::Array<juce::MidiDeviceInfo> MidiController::getAllAvailableDevices() const
{
    return juce::MidiInput::getAvailableDevices();
}

void MidiController::forceOpenDevice(const juce::String& identifier)
{
    {
        juce::ScopedLock sl(deviceLock);
        for (auto& d : activeDevices)
            if (d.identifier == identifier) return;  // already open
    }
    auto available = juce::MidiInput::getAvailableDevices();
    for (auto& info : available)
        if (info.identifier == identifier)
            { openDevice(info); return; }
}

juce::StringArray MidiController::getConnectedDeviceNames() const
{
    juce::ScopedLock sl(deviceLock);
    juce::StringArray out;
    for (auto& d : activeDevices)
        out.add(d.name + " [" + d.mapping.profileName + "]");
    return out;
}

juce::StringArray MidiController::getBlockedDeviceNames() const
{
    juce::ScopedLock sl(deviceLock);
    return blockedDeviceNames;
}

// ── Event log ─────────────────────────────────────────────────────────────────

void MidiController::logEvent(const juce::String& device,
                               const juce::MidiMessage& msg,
                               MidiMapping::Target resolved)
{
    juce::String text;
    if (msg.isController())
        text = "CC " + juce::String(msg.getControllerNumber())
             + " = " + juce::String(msg.getControllerValue());
    else if (msg.isNoteOn())
        text = "Note " + juce::String(msg.getNoteNumber())
             + " ON  vel " + juce::String(msg.getVelocity());
    else if (msg.isNoteOff())
        text = "Note " + juce::String(msg.getNoteNumber()) + " OFF";
    else
        return;

    if (resolved != MidiMapping::Target::Unknown)
        text += "  ->  " + MidiMapping::targetName(resolved);

    MidiLogEvent ev;
    ev.device      = device;
    ev.text        = text;
    ev.timestampMs = juce::Time::getMillisecondCounterHiRes();

    juce::ScopedLock sl(logLock);
    eventLog.add(ev);
    while (eventLog.size() > kMaxLog)
        eventLog.remove(0);
}

juce::Array<MidiLogEvent> MidiController::getRecentEvents() const
{
    juce::ScopedLock sl(logLock);
    return eventLog;
}

// ── MIDI Learn ────────────────────────────────────────────────────────────────

void MidiController::beginLearn(MidiMapping::Target target)
{
    learningTarget.store((int)target);
    juce::ScopedLock sl(learnLock);
    learnCapture.valid = false;
}

void MidiController::cancelLearn()
{
    learningTarget.store((int)MidiMapping::Target::Unknown);
}

// ── Dispatch ──────────────────────────────────────────────────────────────────

void MidiController::dispatchMessage(const DeviceMapping& m,
                                      const juce::MidiMessage& msg,
                                      const juce::String& deviceName)
{
    // ── MIDI Learn intercept ──────────────────────────────────────────────
    auto lt = (MidiMapping::Target)learningTarget.load();
    if (lt != MidiMapping::Target::Unknown)
    {
        bool isNote = false;
        int  number = -1;
        if (msg.isController())   { isNote = false; number = msg.getControllerNumber(); }
        else if (msg.isNoteOn())  { isNote = true;  number = msg.getNoteNumber();       }

        if (number >= 0)
        {
            learningTarget.store((int)MidiMapping::Target::Unknown);
            {
                juce::ScopedLock sl(learnLock);
                learnCapture = { true, lt, isNote, number, deviceName };
            }
            // Apply the new mapping to the device that sent it
            // (find device and update its map under lock)
            {
                juce::ScopedLock sl(deviceLock);
                for (auto& d : activeDevices)
                {
                    if (d.name == deviceName)
                    {
                        if (isNote) mapNote(d.mapping, number, lt);
                        else        mapCC  (d.mapping, number, lt);
                        break;
                    }
                }
                // Also update host fallback so DAW-routed MIDI works
                if (isNote) mapNote(hostFallback, number, lt);
                else        mapCC  (hostFallback, number, lt);
            }
            if (onLearnComplete)
                juce::MessageManager::callAsync([this, lt, isNote, number, deviceName] {
                    if (onLearnComplete) onLearnComplete(lt, isNote, number, deviceName);
                });
        }
        return; // swallow the message during learn
    }

    // ── Global channel (e.g. Ch 16 on Party Mix — crossfader, load) ──────
    const int ch = msg.getChannel();
    if (m.globalChannel > 0 && ch == m.globalChannel)
    {
        if (!callback) return;
        MidiMapping::Target resolved = MidiMapping::Target::Unknown;
        if (msg.isController())
        {
            auto it = m.globalCCMap.find(msg.getControllerNumber());
            if (it != m.globalCCMap.end())
            {
                resolved = it->second.target;
                callback(resolved, normalizeCC(msg.getControllerValue()));
            }
        }
        else if (msg.isNoteOn())
        {
            auto it = m.globalNoteMap.find(msg.getNoteNumber());
            if (it != m.globalNoteMap.end())
            {
                resolved = it->second.target;
                callback(resolved, msg.getFloatVelocity());
            }
        }
        logEvent(deviceName, msg, resolved);
        return;
    }

    // ── Pad channels (e.g. Ch 5/6 on Party Mix) ──────────────────────────
    if (m.padChannelA > 0 && (ch == m.padChannelA || ch == m.padChannelB))
    {
        const bool isDeckBPad = (ch == m.padChannelB);
        int& modeRef = isDeckBPad ? m.padModeIdx[1] : m.padModeIdx[0];

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            // Mode-select buttons — update state and return
            if (m.padHotCueNote  >= 0 && note == m.padHotCueNote)  { modeRef = 0; return; }
            if (m.padLoopNote    >= 0 && note == m.padLoopNote)    { modeRef = 1; return; }
            if (m.padSamplerNote >= 0 && note == m.padSamplerNote) { modeRef = 2; return; }
            // Performance pads
            for (int i = 0; i < 4; ++i)
            {
                if (m.padNotes[i] == note)
                {
                    auto t = padTarget(modeRef, i, isDeckBPad);
                    if (t != MidiMapping::Target::Unknown && callback)
                        callback(t, msg.getFloatVelocity());
                    logEvent(deviceName, msg, t);
                    return;
                }
            }
        }
        // Note-offs on pad channels: release beat-loop or sampler
        else if (msg.isNoteOff())
        {
            for (int i = 0; i < 4; ++i)
            {
                if (m.padNotes[i] == msg.getNoteNumber())
                {
                    auto t = padTarget(modeRef, i, isDeckBPad);
                    if (t != MidiMapping::Target::Unknown && callback)
                        callback(t, 0.0f);
                    return;
                }
            }
        }
        return; // ignore all other messages on pad channels
    }

    // ── Normal dispatch ───────────────────────────────────────────────────
    if (!callback) return;

    const bool isDeckBChannel = (m.deckBChannel > 0 &&
                                  ch == m.deckBChannel);

    MidiMapping::Target resolved = MidiMapping::Target::Unknown;

    if (msg.isController())
    {
        auto it = m.ccMap.find(msg.getControllerNumber());
        if (it != m.ccMap.end())
        {
            resolved = it->second.target;
            if (isDeckBChannel) resolved = remapDeckAtoB(resolved);
            callback(resolved, normalizeCC(msg.getControllerValue()));
        }
    }
    else if (msg.isNoteOn())
    {
        auto it = m.noteMap.find(msg.getNoteNumber());
        if (it != m.noteMap.end())
        {
            resolved = it->second.target;
            if (isDeckBChannel) resolved = remapDeckAtoB(resolved);
            callback(resolved, msg.getFloatVelocity());
        }
    }
    else if (msg.isNoteOff())
    {
        auto it = m.noteMap.find(msg.getNoteNumber());
        if (it != m.noteMap.end())
        {
            resolved = it->second.target;
            if (isDeckBChannel) resolved = remapDeckAtoB(resolved);
            callback(resolved, 0.0f);
        }
    }

    logEvent(deviceName, msg, resolved);
}

void MidiController::handleIncomingMidiMessage(juce::MidiInput* source,
                                                const juce::MidiMessage& msg)
{
    juce::ScopedLock sl(deviceLock);
    for (auto& d : activeDevices)
        if (d.input.get() == source)
            { dispatchMessage(d.mapping, msg, d.name); return; }
}

void MidiController::handleHostMidiMessage(const juce::MidiMessage& msg)
{
    dispatchMessage(hostFallback, msg, "Host MIDI");
}

// ── LED feedback ──────────────────────────────────────────────────────────────

void MidiController::refreshPadLEDs(const std::function<int(int)>& velocityForPad)
{
    juce::ScopedLock sl(deviceLock);
    for (auto& d : activeDevices)
    {
        if (!d.output || d.mapping.padLEDFirstNote < 0) continue;

        const int ch   = d.mapping.padLEDChannel;
        const int base = d.mapping.padLEDFirstNote;
        for (int i = 0; i < 16; ++i)
        {
            const int note = base + i;
            if (note > 127) break;
            const int vel = juce::jlimit(0, 127, velocityForPad(i));
            auto msg = (vel > 0)
                ? juce::MidiMessage::noteOn (ch, note, (juce::uint8)vel)
                : juce::MidiMessage::noteOff(ch, note);
            d.output->sendMessageNow(msg);
        }
    }
}

// ── Custom mapping persistence ────────────────────────────────────────────────

void MidiController::saveCustomMappings(const juce::File& file) const
{
    juce::XmlElement root("MidiMappings");

    auto saveMap = [&](const juce::String& deviceName, const DeviceMapping& m)
    {
        auto* dev = root.createNewChildElement("Device");
        dev->setAttribute("name", deviceName);
        for (auto& [cc, mm] : m.ccMap)
        {
            auto* el = dev->createNewChildElement("CC");
            el->setAttribute("number", cc);
            el->setAttribute("target", (int)mm.target);
        }
        for (auto& [note, mm] : m.noteMap)
        {
            auto* el = dev->createNewChildElement("Note");
            el->setAttribute("number", note);
            el->setAttribute("target", (int)mm.target);
        }
    };

    {
        juce::ScopedLock sl(deviceLock);
        saveMap("__host__", hostFallback);
        for (auto& d : activeDevices)
            saveMap(d.name, d.mapping);
    }

    file.getParentDirectory().createDirectory();
    root.writeTo(file);
}

void MidiController::loadCustomMappings(const juce::File& file)
{
    if (!file.existsAsFile()) return;
    auto xml = juce::parseXML(file);
    if (!xml) return;

    auto loadMap = [&](const juce::String& deviceName) -> DeviceMapping
    {
        DeviceMapping m = buildProfile(deviceName); // start with auto-profile
        for (auto* dev : xml->getChildIterator())
        {
            if (dev->getStringAttribute("name") != deviceName) continue;
            for (auto* el : dev->getChildIterator())
            {
                int  number = el->getIntAttribute("number");
                auto target = (MidiMapping::Target)el->getIntAttribute("target");
                if (el->getTagName() == "CC")   mapCC  (m, number, target);
                if (el->getTagName() == "Note") mapNote(m, number, target);
            }
            break;
        }
        return m;
    };

    juce::ScopedLock sl(deviceLock);
    hostFallback = loadMap("__host__");
    for (auto& d : activeDevices)
        d.mapping = loadMap(d.name);
}
