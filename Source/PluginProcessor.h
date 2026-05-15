#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_osc/juce_osc.h>
#include <atomic>
#include "Modules/DeckProcessor.h"
#include "Modules/TimecodeAnalyzer.h"
#include "Modules/EffectsChain.h"
#include "Modules/SamplerEngine.h"
#include "Modules/MidiController.h"
#include "Modules/MixRecorder.h"
#include "Modules/BroadcastEngine.h"

class PatchworkAudioProcessor : public juce::AudioProcessor,
                                 private juce::Timer
{
public:
    PatchworkAudioProcessor();
    ~PatchworkAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Public access for editor
    DeckProcessor& getDeckA() { return deckA; }
    DeckProcessor& getDeckB() { return deckB; }

    TimecodeAnalyzer& getTimecodeA() { return timecodeA; }
    TimecodeAnalyzer& getTimecodeB() { return timecodeB; }
    EffectsChain&  getEffects() { return effectsChain; }
    SamplerEngine& getSampler() { return sampler; }

    void setCrossfader(float value); // 0=A, 1=B
    float getCrossfader() const { return crossfader; }

    // Master clock
    void  setMasterBPM(float bpm);
    float getMasterBPM()  const { return masterBPM.load(); }
    void  tapTempo();                  // call on each button tap
    void  syncDecksToMaster();         // set both decks to master BPM
    bool  isQuantizeEnabled() const { return quantizeEnabled.load(); }
    void  setQuantizeEnabled(bool b) { quantizeEnabled.store(b); }

    // Headphone / cue bus
    void  setHeadphoneVolume(float v) { headphoneVolume.store(juce::jlimit(0.0f, 1.0f, v)); }
    float getHeadphoneVolume() const  { return headphoneVolume.load(); }

    juce::StringArray getConnectedDevices()  const { return midiController.getConnectedDeviceNames(); }
    MidiController&   getMidiController()          { return midiController; }

    // Called from UI thread — auto-routes to OSC or MMC based on detected host
    void sendMMCRecord(bool start);
    bool usesOSC() const { return hostType.isReaper(); }
    juce::String getOscTargetIP() const { return oscTargetIP; }

    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

    MixRecorder&     getMixRecorder()     { return mixRecorder; }
    BroadcastEngine& getBroadcastEngine() { return broadcastEngine; }

private:
    DeckProcessor deckA, deckB;
    EffectsChain effectsChain;
    SamplerEngine sampler;
    MidiController midiController;

    float crossfader = 0.5f;
    std::atomic<float> headphoneVolume { 0.8f };

    std::atomic<float> masterBPM   { 120.0f };
    std::atomic<bool>  quantizeEnabled { false };
    std::vector<double> tapTimes;        // timestamps of recent taps (ms)

    std::atomic<bool> pendingRecordOn  { false };
    std::atomic<bool> pendingRecordOff { false };

    juce::PluginHostType hostType;
    juce::OSCSender      oscSender;
    juce::String         oscTargetIP;

    juce::AudioProcessorValueTreeState apvts;

    MixRecorder     mixRecorder;
    BroadcastEngine broadcastEngine;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    TimecodeAnalyzer timecodeA, timecodeB;
    juce::AudioBuffer<float> dvsInputBuffer;  // saves input before we clear the output

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void handleMidiInput(const juce::MidiBuffer& midiMessages);
    void onMidiTarget(MidiMapping::Target target, float value);
    void timerCallback() override;
    void refreshDeviceLEDs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchworkAudioProcessor)
};
