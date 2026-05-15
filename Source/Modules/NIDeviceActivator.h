#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cstdint>
#include <functional>

class MidiController;

struct NIDeviceInfo
{
    uint16_t     vendorId    = 0;
    uint16_t     productId   = 0;
    juce::String name;
    bool         canActivate = false;
    bool         confirmed   = false;
};

struct NIActivationResult
{
    bool         success = false;
    juce::String log;
    bool         needsUdevRule = false;
    juce::String udevRuleText;
};

struct NIHIDPacket
{
    uint8_t data[64] = {};
    int     len      = 0;
};

// ── LED layout constants (indices into the 80-byte LED data region of report 0x80) ──
// Bytes  0-38 : button LEDs   — brightness byte: 0x00=off 0x7C=dim 0x7E=normal 0x7F=bright
// Bytes 39-54 : pad LEDs      — indexed color: (colorIndex << 2) | brightness(0-3)
//                               colorIndex 0=off, 1=red … 17=white  (see kPadColorNames)
// Bytes 55-79 : strip LEDs    — brightness byte same as buttons
static constexpr int kLEDCount       = 80;
static constexpr int kLEDPadBase     = 39;   // first pad LED byte
static constexpr int kLEDStripBase   = 55;   // first strip LED byte
static constexpr int kLEDPadCount    = 16;
static constexpr int kLEDStripCount  = 25;
static constexpr int kLEDButtonCount = 39;

// Button names matching input report bitmask order (byte 1 bit 0 = index 0, etc.)
static const char* const kButtonNames[39] = {
    "MASCHINE","STAR","BROWSE","VOLUME","SWING","TEMPO","PLUG-IN","SAMPLING",
    "LEFT","RIGHT","PITCH","MOD","PERFORM","NOTES","GROUP","AUTO",
    "LOCK","NOTE REPEAT","RESTART","ERASE","TAP","FOLLOW","PLAY","REC",
    "STOP","SHIFT","FIXED VEL","PAD MODE","KEYBOARD","CHORDS","STEP","SCENE",
    "PATTERN","EVENTS","VARIATION","DUPLICATE","SELECT","SOLO","MUTE"
};

// Pad LED hardware order in the report (index 0 = ledData[39])
// Input pad index → LED data index:  ledIdx = 39 + (3 - inputPad/4)*4 + inputPad%4
static inline int padInputToLEDIdx(int inputPad)
{
    return kLEDPadBase + (3 - inputPad / 4) * 4 + inputPad % 4;
}

// 18-color palette for pad indexed-color system
static const char* const kPadColorNames[18] = {
    "Off","Red","Orange","Lt Orange","Warm Yellow","Yellow",
    "Lime","Green","Mint","Cyan","Turquoise","Blue",
    "Plum","Violet","Purple","Magenta","Fuchsia","White"
};

// ─────────────────────────────────────────────────────────────────────────────
class NIDeviceActivator : private juce::Thread
{
public:
    NIDeviceActivator();
    ~NIDeviceActivator() override;

    juce::Array<NIDeviceInfo> scan();

    void activateAsync(std::function<void(NIActivationResult)> onComplete);
    bool isActivating() const noexcept { return isThreadRunning() && !monitorMode; }

    void startHIDMonitor(MidiController& controller);
    void stopHIDMonitor();
    bool isMonitoring() const noexcept { return isThreadRunning() && monitorMode; }

    juce::Array<NIHIDPacket> getAndClearPackets();
    juce::String             getAndClearMonitorLog();

    // ── LED control (thread-safe) ─────────────────────────────────────────────
    // idx 0-79 maps to the 80 LED bytes described in the layout constants above.
    void setLEDByte(int idx, uint8_t val);
    void setAllLEDs(const uint8_t data[80]);
    // Flash LED at idx to max brightness for ~700 ms so user can identify it.
    void requestProbeLED(int idx);

    // ── udev rule helpers ─────────────────────────────────────────────────────
    static juce::String getUdevRuleText();
    static juce::String writeUdevRule();
    static juce::String applyUdevRules();

private:
    void run() override;
    void runActivation();
    void runMonitor();
    void translatePacket(const NIHIDPacket& pkt);

    std::function<void(NIActivationResult)> pendingCallback;
    void*           libusbCtx     = nullptr;
    MidiController* hidController = nullptr;
    bool            monitorMode   = false;

    juce::CriticalSection    packetLock;
    juce::Array<NIHIDPacket> packetBuffer;
    uint8_t                  lastPacketData[64] = {};
    int                      lastPacketLen = 0;

    // ── Translation state (monitor thread only) ───────────────────────────────
    uint8_t  prevButtonBytes[5] = {};   // bytes 1-5 of report 0x01
    bool     buttonBaselineSet  = false;
    bool     padActive[16]      = {};
    uint8_t  padOverrideByte[16] = {};  // LED byte to show when pad is pressed
    uint8_t  prevEncoderPos     = 0;
    bool     encoderPosSet      = false;
    int      encoderAccum       = 64;   // running 0-127 CC value
    uint16_t prevStripVal       = 0;

    // ── LED state ─────────────────────────────────────────────────────────────
    // ledIdle is the user-configured idle state; pad presses use padOverrideByte.
    // Protected by ledLock for UI-thread writes; monitor thread reads under lock.
    uint8_t              ledIdle[80]  = {};
    std::atomic<bool>    ledsDirty    { false };
    juce::CriticalSection ledLock;
    std::atomic<int>     probeLEDRequest { -1 };

    void sendLEDReport(void* handle);
    void sendDisplay(void* handle);

    // ── Diagnostic log (monitor thread writes, UI timer drains) ──────────────
    juce::CriticalSection monitorLogLock;
    juce::String          monitorLogStr;
    void appendMonitorLog(const juce::String& msg);
};
