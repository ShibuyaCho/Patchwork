#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>

class SamplerEngine
{
public:
    static constexpr int NUM_PADS = 16;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock(juce::AudioBuffer<float>& buffer);

    void setVolume(float volume);
    bool loadSample(int padIndex, const juce::File& file);
    void triggerPad(int padIndex, float velocity = 1.0f);
    void releasePad(int padIndex);
    bool isPadLoaded(int padIndex) const;
    bool isPadPlaying(int padIndex) const;
    juce::String getPadName(int padIndex) const;

private:
    struct Pad {
        juce::AudioBuffer<float> buffer;
        juce::String name;
        int readPosition = 0;
        bool playing     = false;
        float velocity   = 1.0f;
        bool loaded      = false;
    };

    std::array<Pad, NUM_PADS> pads;
    juce::AudioFormatManager formatManager;
    double sampleRate = 44100.0;
    float masterVolume = 0.8f;
};
