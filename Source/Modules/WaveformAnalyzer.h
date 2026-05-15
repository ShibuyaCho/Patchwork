#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class WaveformAnalyzer
{
public:
    void analyze(const juce::AudioBuffer<float>& buffer, double sampleRate);
    float        getDetectedBPM() const { return detectedBPM; }
    juce::String getDetectedKey() const { return detectedKey; }

private:
    float        detectedBPM = 0.0f;
    juce::String detectedKey;

    float        detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate);
    juce::String detectKey(const juce::AudioBuffer<float>& buffer, double sampleRate);
};
