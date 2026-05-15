#include "SamplerEngine.h"

void SamplerEngine::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    formatManager.registerBasicFormats();
}

void SamplerEngine::releaseResources()
{
    for (auto& pad : pads)
        pad.playing = false;
}

bool SamplerEngine::loadSample(int padIndex, const juce::File& file)
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;

    auto& pad = pads[padIndex];
    pad.buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&pad.buffer, 0, (int)reader->lengthInSamples, 0, true, true);
    pad.name     = file.getFileNameWithoutExtension();
    pad.loaded   = true;
    pad.playing  = false;
    pad.readPosition = 0;
    return true;
}

void SamplerEngine::triggerPad(int padIndex, float velocity)
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return;
    auto& pad = pads[padIndex];
    if (!pad.loaded) return;
    pad.readPosition = 0;
    pad.velocity     = velocity;
    pad.playing      = true;
}

void SamplerEngine::releasePad(int padIndex)
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return;
    // For one-shot pads, release does nothing; add gate logic here if needed
}

bool SamplerEngine::isPadLoaded(int padIndex) const
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return false;
    return pads[padIndex].loaded;
}

bool SamplerEngine::isPadPlaying(int padIndex) const
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return false;
    return pads[padIndex].playing;
}

juce::String SamplerEngine::getPadName(int padIndex) const
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return {};
    return pads[padIndex].name;
}

void SamplerEngine::setVolume(float volume)
{
    masterVolume = juce::jlimit(0.0f, 1.0f, volume);
}

void SamplerEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (auto& pad : pads)
    {
        if (!pad.playing || !pad.loaded) continue;

        const int padSamples = pad.buffer.getNumSamples();
        const int padChans   = pad.buffer.getNumChannels();
        float gain           = pad.velocity * masterVolume;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            int padCh = (ch < padChans) ? ch : padChans - 1;
            auto* dest      = buffer.getWritePointer(ch);
            const auto* src = pad.buffer.getReadPointer(padCh);

            for (int i = 0; i < numSamples; ++i)
            {
                int pos = pad.readPosition + i;
                if (pos < padSamples)
                    dest[i] += src[pos] * gain;
            }
        }

        pad.readPosition += numSamples;
        if (pad.readPosition >= padSamples)
        {
            pad.playing      = false;
            pad.readPosition = 0;
        }
    }
}
