#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <atomic>

struct MidiMapping {
    enum class Target {
        DeckAVolume, DeckBVolume,
        DeckATempo,  DeckBTempo,
        DeckAPan,    DeckBPan,
        DeckAEQHigh, DeckAEQMid, DeckAEQLow,
        DeckBEQHigh, DeckBEQMid, DeckBEQLow,
        Crossfader,
        ReverbAmount, DelayAmount, FilterFreq,
        DeckAPlay,  DeckBPlay,
        DeckALoad,  DeckBLoad,
        DeckASync,  DeckBSync,
        DeckALoop,  DeckBLoop,
        DeckACue,   DeckBCue,   // headphone monitor select
        DeckAHotCue0, DeckAHotCue1, DeckAHotCue2, DeckAHotCue3,
        DeckBHotCue0, DeckBHotCue1, DeckBHotCue2, DeckBHotCue3,
        SamplerPad0,  SamplerPad1,  SamplerPad2,  SamplerPad3,
        SamplerPad4,  SamplerPad5,  SamplerPad6,  SamplerPad7,
        SamplerPad8,  SamplerPad9,  SamplerPad10, SamplerPad11,
        SamplerPad12, SamplerPad13, SamplerPad14, SamplerPad15,
        // Jog / scratch / pregain — Party Mix and similar
        DeckAJog,         DeckBJog,          // relative jog wheel CC
        DeckAScratchMode, DeckBScratchMode,  // vinyl/CDJ toggle button
        DeckACuePt,       DeckBCuePt,        // DJ cue point (set / jump-to)
        DeckAGain,        DeckBGain,         // pregain knob
        // Beat-loop pad targets: 0.5 / 1 / 2 / 4 bars
        DeckABeatLoop0, DeckABeatLoop1, DeckABeatLoop2, DeckABeatLoop3,
        DeckBBeatLoop0, DeckBBeatLoop1, DeckBBeatLoop2, DeckBBeatLoop3,
        Unknown
    };
    Target target   = Target::Unknown;
    bool   isButton = false;

    static juce::String targetName(Target t);
};

enum class DeviceCapability { DeckControls, SamplerPads, HotCues, FXControls };

struct DeviceMapping {
    std::map<int, MidiMapping> ccMap;
    std::map<int, MidiMapping> noteMap;
    int channelFilter = 0; // 0 = any
    // If > 0, messages on this channel auto-remap DeckA* targets -> DeckB*.
    int deckBChannel = 0;

    // Global channel (e.g. Ch 16 on Party Mix) — separate CC/note maps
    // so its note numbers don't collide with per-deck note numbers.
    int globalChannel = 0;
    std::map<int, MidiMapping> globalCCMap;
    std::map<int, MidiMapping> globalNoteMap;

    // Pad mode system: pads arrive on separate MIDI channels with their own
    // mode-select buttons, so they can't share the main noteMap.
    int padChannelA = 0;   // MIDI channel for Deck A pads (e.g. Ch 5 on Party Mix)
    int padChannelB = 0;   // MIDI channel for Deck B pads (e.g. Ch 6)
    int padHotCueNote  = -1;  // note on padChannel that selects Hot Cue mode
    int padLoopNote    = -1;  // note that selects Loop mode
    int padSamplerNote = -1;  // note that selects Sampler mode
    std::array<int, 4> padNotes = { { -1, -1, -1, -1 } }; // pad 1-4 note numbers
    // Runtime pad mode per deck (0=HotCue, 1=Loop, 2=Sampler). Mutable so
    // dispatchMessage() can update it even when called through a const reference.
    mutable std::array<int, 2> padModeIdx = { { 0, 0 } };

    // LED feedback — send Note On to light pad LEDs on devices that support it.
    // padLEDFirstNote is the MIDI note for pad 0; pads 1-15 are +1, +2, etc.
    // Set to -1 to disable LED output for this profile.
    int padLEDFirstNote = -1;
    int padLEDChannel   = 1;

    std::vector<DeviceCapability> capabilities;
    juce::String profileName;
};

// ── Live MIDI event (for monitor display) ────────────────────────────────────
struct MidiLogEvent {
    juce::String device;
    juce::String text;   // e.g. "CC 0 = 127 → DeckAVolume"
    double       timestampMs = 0.0;
};

class MidiController : public juce::MidiInputCallback
{
public:
    using Callback = std::function<void(MidiMapping::Target, float)>;

    MidiController();
    ~MidiController() override;

    void setCallback(Callback cb) { callback = cb; }

    // ── Device management ─────────────────────────────────────────────────
    void scanDevices();   // call at ~1 Hz
    void handleHostMidiMessage(const juce::MidiMessage& msg);

    // Inject a MIDI message synthesised from raw HID data (e.g. Maschine MK3).
    // Dispatched through the dedicated HID mapping so pads fire sampler targets.
    void injectHIDMessage(const juce::MidiMessage& msg, const juce::String& sourceName);

    // Opens any device by identifier, bypassing the auto-block list.
    // Safe to call from UI thread; no-op if already open.
    void forceOpenDevice(const juce::String& identifier);

    // ── Monitor ───────────────────────────────────────────────────────────
    juce::Array<MidiLogEvent>        getRecentEvents() const;
    juce::StringArray                getConnectedDeviceNames() const;
    juce::Array<juce::MidiDeviceInfo> getAllAvailableDevices() const;
    // Names of devices seen but blocked from direct-open (e.g. Maschine HID)
    juce::StringArray                getBlockedDeviceNames() const;

    // ── MIDI Learn ────────────────────────────────────────────────────────
    void beginLearn(MidiMapping::Target target);
    void cancelLearn();
    bool                 isLearning()       const { return learningTarget.load() != (int)MidiMapping::Target::Unknown; }
    MidiMapping::Target  getLearningTarget() const { return (MidiMapping::Target)learningTarget.load(); }

    // Called on message thread when learn captures a mapping
    std::function<void(MidiMapping::Target, bool isNote, int number, const juce::String& device)> onLearnComplete;

    // ── LED feedback ──────────────────────────────────────────────────────
    // Call at ~1 Hz and immediately after triggering a pad.
    // velocityForPad(padIdx) returns 0 (off), 1-126 (dim/loaded), 127 (playing).
    void refreshPadLEDs(const std::function<int(int)>& velocityForPad);

    // ── Custom mapping persistence ────────────────────────────────────────
    void saveCustomMappings(const juce::File& file) const;
    void loadCustomMappings(const juce::File& file);

private:
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& msg) override;
    Callback callback;

    struct ActiveDevice {
        juce::String                      identifier;
        juce::String                      name;
        std::unique_ptr<juce::MidiInput>  input;
        std::unique_ptr<juce::MidiOutput> output;   // for LED feedback
        DeviceMapping                     mapping;
    };

    std::vector<ActiveDevice>   activeDevices;
    juce::StringArray           blockedDeviceNames;
    mutable juce::CriticalSection deviceLock;

    DeviceMapping hostFallback;
    DeviceMapping hidMapping;
    void buildHIDMapping();

    // Event log — written from MIDI thread, read from UI thread
    static constexpr int kMaxLog = 32;
    juce::Array<MidiLogEvent>   eventLog;
    mutable juce::CriticalSection logLock;
    void logEvent(const juce::String& device, const juce::MidiMessage& msg,
                  MidiMapping::Target resolved);

    // Learn state
    std::atomic<int> learningTarget { (int)MidiMapping::Target::Unknown };
    // captured on MIDI thread, consumed on UI thread
    struct LearnCapture { bool valid = false; MidiMapping::Target target; bool isNote; int number; juce::String device; };
    LearnCapture learnCapture;
    mutable juce::CriticalSection learnLock;

    // Device helpers
    static bool shouldOpenDirectly(const juce::String& name);
    void openDevice (const juce::MidiDeviceInfo& info);
    void closeDevice(const juce::String& identifier);
    DeviceMapping buildProfile(const juce::String& name);

    // Profile builders
    void applyNumarkPartyMix(DeviceMapping&);
    void applyMaschineMikro (DeviceMapping&);
    void applyLaunchpad     (DeviceMapping&);
    void applyAkaiAPC       (DeviceMapping&);
    void applyPioneerDDJ    (DeviceMapping&);
    void applyHercules      (DeviceMapping&);
    void applyDenonMC       (DeviceMapping&);
    void applyGenericDJ     (DeviceMapping&);
    void applyGenericPads   (DeviceMapping&);

    static void  mapCC  (DeviceMapping&, int cc,   MidiMapping::Target, bool isButton = false);
    static void  mapNote(DeviceMapping&, int note,  MidiMapping::Target);

    void dispatchMessage(const DeviceMapping&, const juce::MidiMessage&,
                         const juce::String& deviceName);
    static float               normalizeCC(int v) { return v / 127.0f; }
    static MidiMapping::Target remapDeckAtoB(MidiMapping::Target t);
    // Returns the correct target for a pad press given mode (0=HotCue,1=Loop,2=Sampler),
    // pad index 0-3, and which deck the pad belongs to.
    static MidiMapping::Target padTarget(int modeIdx, int padIdx, bool isDeckB);
};
