#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class TimecodeAnalyzer
{
public:
    static constexpr float kNominalFreq = 1000.0f;  // Hz — standard timecode carrier

    void process(const float* left, const float* right, int numSamples, double sampleRate);

    float getSpeedRatio()  const { return speedRatio.load(); }   // 0..4 (1.0 = normal)
    float getDirection()   const { return direction.load(); }    // +1 forward, -1 reverse
    bool  hasSignal()      const { return signalPresent.load(); }

private:
    std::atomic<float> speedRatio    { 0.0f };
    std::atomic<float> direction     { 1.0f };
    std::atomic<bool>  signalPresent { false };
    float              speedSmooth   { 0.0f };
};
