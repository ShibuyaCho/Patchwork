#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <memory>
#include "WaveformAnalyzer.h"

namespace soundtouch { class SoundTouch; }  // forward declare to avoid polluting headers

class PatchworkAudioProcessor;

class DeckProcessor
{
public:
    static constexpr int NUM_HOT_CUES = 4;

    explicit DeckProcessor(PatchworkAudioProcessor* parent);
    ~DeckProcessor();  // defined in .cpp so unique_ptr<SoundTouch> can see the complete type

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock(juce::AudioBuffer<float>& buffer);

    // Transport
    void play();
    void pause();
    void stop();
    void togglePlay();
    bool isPlaying() const { return playing; }
    void setPosition(double positionInSeconds);
    double getPosition() const;
    double getLengthInSeconds() const;

    // File loading
    bool loadFile(const juce::File& file);
    juce::String getLoadedFileName() const { return loadedFileName; }
    juce::File   getLoadedFile()     const { return loadedFile; }
    bool hasFile() const { return fileBuffer.getNumSamples() > 0; }
    int getLoadVersion() const { return loadVersion; }

    // Mixer
    void setVolume(float volume);
    void setGain(float gain);    // pregain (applied before volume fader)
    void setPan(float pan);
    void setTempo(float bpm);
    float getVolume() const { return currentVolume; }
    float getGain()   const { return currentGain; }
    float getPan() const    { return currentPan; }
    float getTempo() const  { return currentTempo; }

    // 3-band EQ
    void  setEQHigh(float db);
    void  setEQMid(float db);
    void  setEQLow(float db);
    float getEQHigh() const { return eqHighDb; }
    float getEQMid()  const { return eqMidDb; }
    float getEQLow()  const { return eqLowDb; }

    // Scratch (vinyl) and CDJ (nudge) control
    void engageScratch(float rate);
    void disengageScratch();
    void applyNudge(float offset);
    void releaseNudge();
    bool isScratching() const { return scratching; }

    // Vinyl/CDJ mode toggle — set by the scratch button on the controller.
    // When true, jog wheel input uses engageScratch(); otherwise uses applyNudge().
    void toggleScratchMode() { scratchModeEnabled = !scratchModeEnabled.load(); }
    bool isScratchModeEnabled() const { return scratchModeEnabled.load(); }

    // Timecoded vinyl (DVS) — driven by TimecodeAnalyzer output
    void setDVSEnabled(bool b) { dvsEnabled.store(b); }
    bool isDVSEnabled()        const { return dvsEnabled.load(); }
    void setDVSRate(float speed, float dir) { dvsSpeedDir.store(speed * dir); }

    // VU meter (updated each processBlock)
    float getVULevel() const { return vuLevel.load(); }

    // Slip mode — track position keeps moving while scratch/loop override is active
    void setSlipMode(bool enabled);
    bool isSlipMode() const { return slipMode; }

    // Hot cues (left-click to set/jump, right-click to clear)
    void activateHotCue(int idx);   // set if empty, else jump
    void clearHotCue(int idx);
    bool isHotCueSet(int idx) const;
    double getHotCueSamples(int idx) const;

    // Loop
    void setLoopIn();
    void setLoopOut();
    void toggleLoop();
    void setAutoLoop(float bars);   // sets loopIn at current pos, loopOut N bars ahead
    bool isLooping() const { return looping.load(); }
    double getLoopInSeconds() const;
    double getLoopOutSeconds() const;

    // Beat jump (uses detected BPM; falls back to currentTempo)
    void beatJump(int beats);

    // BPM / key detection results (set after loadFile)
    float        getDetectedBPM() const { return analyzer.getDetectedBPM(); }
    juce::String getDetectedKey() const { return analyzer.getDetectedKey(); }

    // Key lock — when enabled, tempo changes without affecting pitch (uses SoundTouch)
    void setKeyLock(bool enabled);
    bool isKeyLockEnabled() const { return keyLockEnabled.load(); }
    // Pitch shift in semitones (only active when key lock is on)
    void setPitchSemitones(float semitones);
    float getPitchSemitones() const { return pitchSemitones.load(); }

    // Headphone cue monitoring — when active this deck feeds the cue output bus
    void setCueActive(bool b) { cueActive.store(b); }
    bool isCueActive()  const { return cueActive.load(); }

    // Waveform data for display
    const juce::AudioBuffer<float>& getFileBuffer() const { return fileBuffer; }

private:
    PatchworkAudioProcessor* processorPtr;

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> fileBuffer;
    juce::String loadedFileName;
    juce::File   loadedFile;
    double readPosition = 0.0;
    bool playing = false;
    int loadVersion = 0;

    float currentVolume = 0.8f;
    float currentGain   = 1.0f;
    float currentPan    = 0.0f;
    float currentTempo  = 120.0f;
    float tempoRatio    = 1.0f;

    double sampleRate = 44100.0;
    int    blockSize  = 512;

    using Filter     = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;
    juce::dsp::ProcessorDuplicator<Filter, FilterCoefs> highFilter, midFilter, lowFilter;
    float eqHighDb = 0.0f, eqMidDb = 0.0f, eqLowDb = 0.0f;

    bool  scratching  = false;
    float scratchRate = 0.0f;
    float nudgeOffset = 0.0f;
    std::atomic<bool> scratchModeEnabled { false };

    std::atomic<bool>  dvsEnabled  { false };
    std::atomic<float> dvsSpeedDir { 0.0f };    // speed * direction, set each block
    std::atomic<float> vuLevel     { 0.0f };

    // Slip mode
    bool   slipMode    = false;
    double slipPosition = 0.0;

    // Hot cues — sample positions, -1 = unset; atomic so UI thread can read safely
    std::array<std::atomic<int64_t>, NUM_HOT_CUES> hotCueSamples;

    // Loop
    std::atomic<bool>    looping{false};
    std::atomic<int64_t> loopInSample{0};
    std::atomic<int64_t> loopOutSample{0};

    std::atomic<bool>  cueActive { false };

    // Key lock
    std::atomic<bool>  keyLockEnabled  { false };
    std::atomic<float> pitchSemitones  { 0.0f };
    std::unique_ptr<soundtouch::SoundTouch> soundTouch;
    std::vector<float>  stInterleavedIn;   // temp buffer fed into SoundTouch
    std::vector<float>  stInterleavedOut;  // temp buffer received from SoundTouch
    void resetSoundTouch();

    WaveformAnalyzer analyzer;

    void updateEQ();
    double snapToBeat(double posSamples) const;  // beat-grid quantize

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckProcessor)
};
