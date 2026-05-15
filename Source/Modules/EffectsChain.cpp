#include "EffectsChain.h"
#include <cstring>

static constexpr float kPi = juce::MathConstants<float>::pi;

void EffectsChain::prepareToPlay(double sr, int blockSz)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32)blockSz;
    spec.numChannels      = 2;

    mainFilter.prepare(spec);  filterDirty  = true;
    compressor.prepare(spec);
    compressor.setAttack(5.0f);
    compressor.setRelease(80.0f);

    flangerDelay.prepare(spec);
    flangerDelay.setMaximumDelayInSamples(4096);

    chorusDelay.prepare(spec);
    chorusDelay.setMaximumDelayInSamples(16384);

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(192001);

    reverb.prepare(spec);  reverbDirty = true;

    flangerPhase = chorusPhase = phaserPhase = tremoloPhase = 0.0f;
    bitPhase = 0.0;
    std::memset(bitHeld, 0, sizeof(bitHeld));
    std::memset(apZ1,    0, sizeof(apZ1));
}

void EffectsChain::releaseResources()
{
    mainFilter.reset();
    compressor.reset();
    flangerDelay.reset();
    chorusDelay.reset();
    delayLine.reset();
    reverb.reset();
    std::memset(apZ1, 0, sizeof(apZ1));
}

void EffectsChain::updateFilter()
{
    double freq = juce::jlimit(20.0, sampleRate * 0.49, (double)filterFreq);
    double q    = (double)filterQ;
    switch (filterType)
    {
        case FilterType::LowPass:  *mainFilter.state = *FilterCoefs::makeLowPass (sampleRate, freq, q); break;
        case FilterType::HighPass: *mainFilter.state = *FilterCoefs::makeHighPass(sampleRate, freq, q); break;
        case FilterType::BandPass: *mainFilter.state = *FilterCoefs::makeBandPass(sampleRate, freq, q); break;
    }
    filterDirty = false;
}

void EffectsChain::updateReverb()
{
    juce::dsp::Reverb::Parameters p;
    p.roomSize   = reverbRoom;
    p.wetLevel   = reverbWet;
    p.dryLevel   = 1.0f - reverbWet * 0.5f;
    p.damping    = 0.5f;
    p.width      = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters(p);
    reverbDirty = false;
}

float EffectsChain::allPass(float in, float& z1, float coeff) noexcept
{
    float y = -coeff * in + z1;
    z1 = in + coeff * y;
    return y;
}

void EffectsChain::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int N  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    const float sr = (float)sampleRate;

    // ── Filter ─────────────────────────────────────────────────────────
    if (filterEnabled)
    {
        if (filterDirty) updateFilter();
        juce::dsp::AudioBlock<float> blk(buffer);
        mainFilter.process(juce::dsp::ProcessContextReplacing<float>(blk));
    }

    // ── Compressor ─────────────────────────────────────────────────────
    if (compEnabled)
    {
        compressor.setThreshold(compThresh);
        compressor.setRatio(compRatio);
        juce::dsp::AudioBlock<float> blk(buffer);
        compressor.process(juce::dsp::ProcessContextReplacing<float>(blk));
    }

    // ── Distortion ─────────────────────────────────────────────────────
    if (distEnabled)
    {
        for (int c = 0; c < ch; ++c)
        {
            auto* d = buffer.getWritePointer(c);
            for (int i = 0; i < N; ++i)
            {
                float sat = std::tanh(d[i] * distDrive) / std::tanh(distDrive);
                d[i] = d[i] * (1.0f - distMix) + sat * distMix;
            }
        }
    }

    // ── Bitcrusher ─────────────────────────────────────────────────────
    if (bitcrushEnabled)
    {
        float step    = std::pow(2.0f, bitDepth - 1.0f);
        double holdLen = 1.0 + (double)bitRate * 15.0;

        for (int i = 0; i < N; ++i)
        {
            bitPhase += 1.0;
            if (bitPhase >= holdLen)
            {
                bitPhase -= holdLen;
                for (int c = 0; c < ch && c < 2; ++c)
                    bitHeld[c] = std::round(buffer.getReadPointer(c)[i] * step) / step;
            }
            for (int c = 0; c < ch; ++c)
                buffer.getWritePointer(c)[i] = bitHeld[std::min(c, 1)];
        }
    }

    // ── Flanger ────────────────────────────────────────────────────────
    if (flangerEnabled)
    {
        const float phInc  = flangerRate / sr;
        const float fb     = 0.6f;

        for (int i = 0; i < N; ++i)
        {
            float lfo       = (std::sin(2.0f * kPi * flangerPhase) + 1.0f) * 0.5f;
            float delaySamp = (0.5f + flangerDepth * lfo) * sr / 1000.0f;

            for (int c = 0; c < ch; ++c)
            {
                float dry = buffer.getReadPointer(c)[i];
                float wet = flangerDelay.popSample(c, delaySamp, true);
                flangerDelay.pushSample(c, dry + wet * fb);
                buffer.getWritePointer(c)[i] = dry * 0.5f + wet * 0.5f;
            }

            flangerPhase += phInc;
            if (flangerPhase >= 1.0f) flangerPhase -= 1.0f;
        }
    }

    // ── Chorus ─────────────────────────────────────────────────────────
    if (chorusEnabled)
    {
        const float phInc = chorusRate / sr;

        for (int i = 0; i < N; ++i)
        {
            float lfo       = (std::sin(2.0f * kPi * chorusPhase) + 1.0f) * 0.5f;
            float delaySamp = (chorusDepth * 0.3f + chorusDepth * 0.7f * lfo) * sr / 1000.0f;

            for (int c = 0; c < ch; ++c)
            {
                // slightly different phase per channel for width
                float phaseMod  = (c == 1) ? chorusPhase + 0.25f : chorusPhase;
                float lfo2      = (std::sin(2.0f * kPi * phaseMod) + 1.0f) * 0.5f;
                float ds        = (chorusDepth * 0.3f + chorusDepth * 0.7f * lfo2) * sr / 1000.0f;

                float dry = buffer.getReadPointer(c)[i];
                float wet = chorusDelay.popSample(c, ds, true);
                chorusDelay.pushSample(c, dry);
                buffer.getWritePointer(c)[i] = dry * 0.6f + wet * 0.4f;
            }
            (void)delaySamp;

            chorusPhase += phInc;
            if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
        }
    }

    // ── Phaser (4-stage all-pass) ───────────────────────────────────────
    if (phaserEnabled)
    {
        const float phInc = phaserRate / sr;

        for (int i = 0; i < N; ++i)
        {
            float lfo = (std::sin(2.0f * kPi * phaserPhase) + 1.0f) * 0.5f;
            float fc  = 200.0f + lfo * phaserDepth * 1800.0f;
            float t   = std::tan(kPi * fc / sr);
            float coeff = (t - 1.0f) / (t + 1.0f);

            for (int c = 0; c < ch && c < 2; ++c)
            {
                float x = buffer.getReadPointer(c)[i];
                float y = x;
                for (int s = 0; s < 4; ++s)
                    y = allPass(y, apZ1[c][s], coeff);
                buffer.getWritePointer(c)[i] = x * 0.5f + y * 0.5f;
            }

            phaserPhase += phInc;
            if (phaserPhase >= 1.0f) phaserPhase -= 1.0f;
        }
    }

    // ── Tremolo ────────────────────────────────────────────────────────
    if (tremoloEnabled)
    {
        const float phInc = tremoloRate / sr;

        for (int i = 0; i < N; ++i)
        {
            float lfo  = (std::sin(2.0f * kPi * tremoloPhase) + 1.0f) * 0.5f;
            float gain = 1.0f - tremoloDepth * (1.0f - lfo);

            for (int c = 0; c < ch; ++c)
                buffer.getWritePointer(c)[i] *= gain;

            tremoloPhase += phInc;
            if (tremoloPhase >= 1.0f) tremoloPhase -= 1.0f;
        }
    }

    // ── Delay ──────────────────────────────────────────────────────────
    if (delayEnabled)
    {
        float delaySamp = delayTimeSec * sr;
        for (int i = 0; i < N; ++i)
        {
            for (int c = 0; c < ch; ++c)
            {
                float dry = buffer.getReadPointer(c)[i];
                float wet = delayLine.popSample(c, delaySamp, true);
                delayLine.pushSample(c, dry + wet * delayFeedback);
                buffer.getWritePointer(c)[i] = dry * (1.0f - delayMix) + wet * delayMix;
            }
        }
    }

    // ── Reverb ─────────────────────────────────────────────────────────
    if (reverbEnabled)
    {
        if (reverbDirty) updateReverb();
        juce::dsp::AudioBlock<float> blk(buffer);
        reverb.process(juce::dsp::ProcessContextReplacing<float>(blk));
    }
}
