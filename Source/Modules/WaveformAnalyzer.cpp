#include "WaveformAnalyzer.h"
#include <vector>
#include <cmath>
#include <algorithm>

float WaveformAnalyzer::detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Need at least 8 seconds to get a reliable reading
    if (numSamples < (int)(sampleRate * 8)) return 0.0f;

    // Frame-based energy (512 samples ≈ 11ms at 44.1kHz)
    const int frameSize = 512;
    const int numFrames = numSamples / frameSize;

    std::vector<float> energy(numFrames, 0.0f);
    for (int f = 0; f < numFrames; ++f)
    {
        int offset = f * frameSize;
        float e = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = buffer.getReadPointer(ch, offset);
            for (int s = 0; s < frameSize; ++s)
                e += data[s] * data[s];
        }
        energy[f] = e / (float)(frameSize * numChannels);
    }

    // Onset strength = positive energy differences (flux)
    std::vector<float> onset(numFrames, 0.0f);
    for (int f = 1; f < numFrames; ++f)
        onset[f] = std::max(0.0f, energy[f] - energy[f - 1]);

    // Lag range in frames for 60–180 BPM
    int minLag = (int)(sampleRate * 60.0 / 180.0 / frameSize);
    int maxLag = (int)(sampleRate * 60.0 / 60.0  / frameSize);
    maxLag = std::min(maxLag, numFrames / 2 - 1);
    if (minLag >= maxLag || minLag < 1) return 0.0f;

    // Autocorrelation — limit window to first 512 onset frames for speed
    const int corrLen = std::min(numFrames / 2, 512);

    float bestCorr = 0.0f;
    int   bestLag  = minLag;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        for (int f = 0; f < corrLen && f + lag < numFrames; ++f)
            corr += onset[f] * onset[f + lag];
        if (corr > bestCorr) { bestCorr = corr; bestLag = lag; }
    }

    double beatPeriodSec = (double)bestLag * frameSize / sampleRate;
    float bpm = (float)(60.0 / beatPeriodSec);

    // Octave-fold into 70–175 BPM
    while (bpm > 0.0f && bpm < 70.0f)  bpm *= 2.0f;
    while (bpm > 0.0f && bpm > 175.0f) bpm /= 2.0f;

    return bpm;
}

// Goertzel algorithm: energy at a single frequency in a block of samples.
static float goertzel(const float* data, int N, float freq, float sampleRate)
{
    float w     = 2.0f * juce::MathConstants<float>::pi * freq / (float)sampleRate;
    float coeff = 2.0f * std::cos(w);
    float s1 = 0.0f, s2 = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        float s = data[i] + coeff * s1 - s2;
        s2 = s1; s1 = s;
    }
    return s1 * s1 + s2 * s2 - coeff * s1 * s2;
}

// Pearson correlation between two 12-element vectors.
static float pearson12(const float* a, const float* b)
{
    float sumA = 0.0f, sumB = 0.0f;
    for (int i = 0; i < 12; ++i) { sumA += a[i]; sumB += b[i]; }
    float mA = sumA / 12.0f, mB = sumB / 12.0f;
    float num = 0.0f, varA = 0.0f, varB = 0.0f;
    for (int i = 0; i < 12; ++i)
    {
        float da = a[i] - mA, db = b[i] - mB;
        num += da * db; varA += da * da; varB += db * db;
    }
    return (varA < 1e-8f || varB < 1e-8f) ? 0.0f : num / std::sqrt(varA * varB);
}

juce::String WaveformAnalyzer::detectKey(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    static const float ksMajor[12] = {6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
                                       2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
    static const float ksMinor[12] = {6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
                                       2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};
    static const char* keyNames[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples < (int)(sampleRate * 4)) return {};

    // Downsample by averaging every 8 samples — crude but sufficient for pitch detection
    const int stride   = 8;
    const double subRate = sampleRate / stride;
    // Analyse at most first 30 s to keep analysis fast
    const int analyseLen = std::min(numSamples, (int)(sampleRate * 30.0));
    const int downLen    = analyseLen / stride;

    std::vector<float> mono(downLen, 0.0f);
    for (int i = 0; i < downLen; ++i)
    {
        float sum = 0.0f;
        int   base = i * stride;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* p = buffer.getReadPointer(ch, base);
            for (int j = 0; j < stride; ++j) sum += p[j];
        }
        mono[i] = sum / (float)(stride * numChannels);
    }

    // Build chromagram: for each pitch class sum Goertzel energy across octaves 3-6
    float chroma[12] = {};
    const int frameSize = 1024;
    const int hopSize   = 512;
    int numFrames = 0;

    for (int pos = 0; pos + frameSize <= downLen; pos += hopSize)
    {
        ++numFrames;
        for (int pc = 0; pc < 12; ++pc)
        {
            float energy = 0.0f;
            // C3=130.8 Hz … B6 ≈ 1975 Hz (Nyquist ≈ subRate/2)
            for (int oct = 3; oct <= 6; ++oct)
            {
                float freq = 261.63f * std::pow(2.0f, (float)(pc + (oct - 4) * 12) / 12.0f);
                if (freq >= (float)(subRate * 0.5)) break;
                energy += goertzel(mono.data() + pos, frameSize, freq, (float)subRate);
            }
            chroma[pc] += energy;
        }
    }
    if (numFrames == 0) return {};

    // Normalize
    float maxC = *std::max_element(chroma, chroma + 12);
    if (maxC < 1e-10f) return {};
    for (float& v : chroma) v /= maxC;

    // Correlate with all 24 rotated KS templates
    float bestCorr  = -2.0f;
    int   bestRoot  = 0;
    bool  bestMajor = true;
    for (int root = 0; root < 12; ++root)
    {
        // Rotate chroma so that root maps to index 0
        float rotated[12];
        for (int i = 0; i < 12; ++i) rotated[i] = chroma[(i + root) % 12];

        float cm = pearson12(rotated, ksMajor);
        float cn = pearson12(rotated, ksMinor);
        if (cm > bestCorr) { bestCorr = cm; bestRoot = root; bestMajor = true; }
        if (cn > bestCorr) { bestCorr = cn; bestRoot = root; bestMajor = false; }
    }

    return juce::String(keyNames[bestRoot]) + (bestMajor ? " maj" : " min");
}

void WaveformAnalyzer::analyze(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    detectedBPM = detectBPM(buffer, sampleRate);
    detectedKey = detectKey(buffer, sampleRate);
}
