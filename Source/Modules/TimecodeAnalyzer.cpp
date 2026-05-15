#include "TimecodeAnalyzer.h"
#include <cmath>
#include <algorithm>

void TimecodeAnalyzer::process(const float* left, const float* right, int numSamples, double sampleRate)
{
    // RMS signal presence check
    float rms = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        rms += left[i] * left[i] + right[i] * right[i];
    rms = std::sqrt(rms / (float)(numSamples * 2));

    if (rms < 0.01f) { signalPresent.store(false); speedRatio.store(0.0f); return; }
    signalPresent.store(true);

    // Zero-crossing rate on left channel → frequency → speed
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i)
        if ((left[i - 1] < 0.0f) != (left[i] < 0.0f)) ++crossings;
    float freq  = (crossings * 0.5f) * (float)sampleRate / (float)numSamples;
    float speed = freq / kNominalFreq;

    // Smooth with one-pole IIR
    speedSmooth = speedSmooth * 0.85f + speed * 0.15f;
    speedRatio.store(juce::jlimit(0.0f, 4.0f, speedSmooth));

    // Direction: sign of Σ L[i] * (R[i+1] − R[i-1])
    // Negative → quadrature is +90° (forward), positive → −90° (reverse)
    float dirAcc = 0.0f;
    for (int i = 1; i < numSamples - 1; ++i)
        dirAcc += left[i] * (right[i + 1] - right[i - 1]);
    direction.store(dirAcc < 0.0f ? 1.0f : -1.0f);
}
