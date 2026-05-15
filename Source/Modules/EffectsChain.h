#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

class EffectsChain
{
public:
    enum class FilterType { LowPass = 0, HighPass, BandPass };

    EffectsChain() : flangerDelay(4096), chorusDelay(16384), delayLine(192001) {}

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock(juce::AudioBuffer<float>& buffer);

    // ── Filter ──────────────────────────────────────────────────────────
    void setFilterEnabled(bool e)     { filterEnabled = e; }
    void setFilterType(FilterType t)  { filterType = t; filterDirty = true; }
    void setFilterFrequency(float hz) { filterFreq = juce::jlimit(20.0f, 20000.0f, hz); filterDirty = true; }
    void setFilterQ(float q)          { filterQ = juce::jlimit(0.1f, 10.0f, q);         filterDirty = true; }
    bool      getFilterEnabled() const { return filterEnabled; }
    FilterType getFilterType()   const { return filterType; }
    float      getFilterFreq()   const { return filterFreq; }
    float      getFilterQ()      const { return filterQ; }

    // ── Compressor ───────────────────────────────────────────────────────
    void setCompEnabled(bool e)       { compEnabled = e; }
    void setCompThreshold(float db)   { compThresh = juce::jlimit(-60.0f, 0.0f, db); }
    void setCompRatio(float r)        { compRatio  = juce::jlimit(1.0f, 20.0f, r); }
    bool  getCompEnabled()   const { return compEnabled; }
    float getCompThreshold() const { return compThresh; }
    float getCompRatio()     const { return compRatio; }

    // ── Distortion ───────────────────────────────────────────────────────
    void setDistEnabled(bool e)       { distEnabled = e; }
    void setDistDrive(float d)        { distDrive = juce::jlimit(1.0f, 12.0f, d); }
    void setDistMix(float m)          { distMix   = juce::jlimit(0.0f, 1.0f, m); }
    bool  getDistEnabled() const { return distEnabled; }
    float getDistDrive()   const { return distDrive; }
    float getDistMix()     const { return distMix; }

    // ── Bitcrusher ───────────────────────────────────────────────────────
    void setBitcrushEnabled(bool e)   { bitcrushEnabled = e; }
    void setBitDepth(float bits)      { bitDepth = juce::jlimit(1.0f, 16.0f, bits); }
    void setBitRate(float r)          { bitRate  = juce::jlimit(0.0f, 1.0f, r); }
    bool  getBitcrushEnabled() const { return bitcrushEnabled; }
    float getBitDepth()        const { return bitDepth; }
    float getBitRate()         const { return bitRate; }

    // ── Flanger ──────────────────────────────────────────────────────────
    void setFlangerEnabled(bool e)    { flangerEnabled = e; }
    void setFlangerRate(float hz)     { flangerRate  = juce::jlimit(0.05f, 5.0f, hz); }
    void setFlangerDepth(float ms)    { flangerDepth = juce::jlimit(0.5f, 10.0f, ms); }
    bool  getFlangerEnabled() const { return flangerEnabled; }
    float getFlangerRate()    const { return flangerRate; }
    float getFlangerDepth()   const { return flangerDepth; }

    // ── Chorus ───────────────────────────────────────────────────────────
    void setChorusEnabled(bool e)     { chorusEnabled = e; }
    void setChorusRate(float hz)      { chorusRate  = juce::jlimit(0.1f, 5.0f, hz); }
    void setChorusDepth(float ms)     { chorusDepth = juce::jlimit(5.0f, 40.0f, ms); }
    bool  getChorusEnabled() const { return chorusEnabled; }
    float getChorusRate()    const { return chorusRate; }
    float getChorusDepth()   const { return chorusDepth; }

    // ── Phaser ───────────────────────────────────────────────────────────
    void setPhaserEnabled(bool e)     { phaserEnabled = e; }
    void setPhaserRate(float hz)      { phaserRate  = juce::jlimit(0.1f, 5.0f, hz); }
    void setPhaserDepth(float d)      { phaserDepth = juce::jlimit(0.0f, 1.0f, d); }
    bool  getPhaserEnabled() const { return phaserEnabled; }
    float getPhaserRate()    const { return phaserRate; }
    float getPhaserDepth()   const { return phaserDepth; }

    // ── Tremolo ──────────────────────────────────────────────────────────
    void setTremoloEnabled(bool e)    { tremoloEnabled = e; }
    void setTremoloRate(float hz)     { tremoloRate  = juce::jlimit(0.1f, 20.0f, hz); }
    void setTremoloDepth(float d)     { tremoloDepth = juce::jlimit(0.0f, 1.0f, d); }
    bool  getTremoloEnabled() const { return tremoloEnabled; }
    float getTremoloRate()    const { return tremoloRate; }
    float getTremoloDepth()   const { return tremoloDepth; }

    // ── Delay ────────────────────────────────────────────────────────────
    void setDelayEnabled(bool e)      { delayEnabled = e; }
    void setDelayTime(float sec)      { delayTimeSec  = juce::jlimit(0.01f, 2.0f,  sec); }
    void setDelayFeedback(float fb)   { delayFeedback = juce::jlimit(0.0f,  0.95f, fb); }
    void setDelayMix(float m)         { delayMix      = juce::jlimit(0.0f,  1.0f,  m); }
    bool  getDelayEnabled()  const { return delayEnabled; }
    float getDelayTime()     const { return delayTimeSec; }
    float getDelayFeedback() const { return delayFeedback; }
    float getDelayMix()      const { return delayMix; }

    // ── Reverb ───────────────────────────────────────────────────────────
    void setReverbEnabled(bool e)     { reverbEnabled = e; }
    void setReverbRoom(float r)       { reverbRoom = juce::jlimit(0.0f, 1.0f, r); reverbDirty = true; }
    void setReverbWet(float w)        { reverbWet  = juce::jlimit(0.0f, 1.0f, w); reverbDirty = true; }
    bool  getReverbEnabled() const { return reverbEnabled; }
    float getReverbRoom()    const { return reverbRoom; }
    float getReverbWet()     const { return reverbWet; }

    // ── Legacy MIDI-facing API ────────────────────────────────────────────
    void setReverbAmount(float a)   { setReverbEnabled(a > 0.01f); setReverbWet(a); setReverbRoom(a); }
    void setDelayAmount(float a)    { setDelayEnabled(a > 0.01f);  setDelayMix(a); }
    float getReverbAmount() const   { return reverbWet; }
    float getDelayAmount()  const   { return delayMix; }

private:
    double sampleRate = 44100.0;
    using Filter      = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;

    // Filter
    bool filterEnabled = false;
    FilterType filterType = FilterType::LowPass;
    float filterFreq = 1000.0f, filterQ = 0.7f;
    bool filterDirty = true;
    juce::dsp::ProcessorDuplicator<Filter, FilterCoefs> mainFilter;

    // Compressor
    bool compEnabled = false;
    float compThresh = -10.0f, compRatio = 4.0f;
    juce::dsp::Compressor<float> compressor;

    // Distortion
    bool distEnabled = false;
    float distDrive = 3.0f, distMix = 0.5f;

    // Bitcrusher
    bool bitcrushEnabled = false;
    float bitDepth = 8.0f, bitRate = 0.5f;
    double bitPhase = 0.0;
    float  bitHeld[2] = {};

    // Flanger
    bool flangerEnabled = false;
    float flangerRate = 0.3f, flangerDepth = 3.0f;
    float flangerPhase = 0.0f;
    juce::dsp::DelayLine<float> flangerDelay;

    // Chorus
    bool chorusEnabled = false;
    float chorusRate = 0.5f, chorusDepth = 20.0f;
    float chorusPhase = 0.0f;
    juce::dsp::DelayLine<float> chorusDelay;

    // Phaser (4-stage all-pass per channel)
    bool phaserEnabled = false;
    float phaserRate = 0.5f, phaserDepth = 0.8f;
    float phaserPhase = 0.0f;
    float apZ1[2][4] = {};

    // Tremolo
    bool tremoloEnabled = false;
    float tremoloRate = 4.0f, tremoloDepth = 0.8f;
    float tremoloPhase = 0.0f;

    // Delay
    bool delayEnabled = false;
    float delayTimeSec = 0.25f, delayFeedback = 0.4f, delayMix = 0.0f;
    juce::dsp::DelayLine<float> delayLine;

    // Reverb
    bool reverbEnabled = false;
    float reverbRoom = 0.5f, reverbWet = 0.3f;
    bool reverbDirty = true;
    juce::dsp::Reverb reverb;

    void updateFilter();
    void updateReverb();
    static float allPass(float in, float& z1, float coeff) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsChain)
};
