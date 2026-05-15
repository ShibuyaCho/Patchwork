#include "DeckProcessor.h"
#include <SoundTouch.h>
#include "../PluginProcessor.h"
#include <cmath>

DeckProcessor::~DeckProcessor() = default;

DeckProcessor::DeckProcessor(PatchworkAudioProcessor* parent)
    : processorPtr(parent)
{
    formatManager.registerBasicFormats();
    for (auto& h : hotCueSamples) h.store(-1);

    soundTouch = std::make_unique<soundtouch::SoundTouch>();
    soundTouch->setChannels(2);
    soundTouch->setSampleRate(44100);
}

void DeckProcessor::prepareToPlay(double sr, int blockSz)
{
    sampleRate = sr;
    blockSize  = blockSz;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32)blockSz;
    spec.numChannels      = 2;

    highFilter.prepare(spec);
    midFilter.prepare(spec);
    lowFilter.prepare(spec);
    updateEQ();

    resetSoundTouch();
}

void DeckProcessor::releaseResources()
{
    highFilter.reset();
    midFilter.reset();
    lowFilter.reset();
}

void DeckProcessor::resetSoundTouch()
{
    soundTouch->setChannels(2);
    soundTouch->setSampleRate((unsigned int)sampleRate);
    soundTouch->setTempoChange(0.0f);
    soundTouch->setPitchSemiTones(pitchSemitones.load());
    soundTouch->clear();
}

void DeckProcessor::setKeyLock(bool enabled)
{
    keyLockEnabled.store(enabled);
    resetSoundTouch();
}

void DeckProcessor::setPitchSemitones(float s)
{
    pitchSemitones.store(juce::jlimit(-12.0f, 12.0f, s));
    soundTouch->setPitchSemiTones(pitchSemitones.load());
}

void DeckProcessor::updateEQ()
{
    *highFilter.state = *FilterCoefs::makeHighShelf(sampleRate, 8000.0f, 0.7f,
                                                     juce::Decibels::decibelsToGain(eqHighDb));
    *midFilter.state  = *FilterCoefs::makePeakFilter(sampleRate, 1000.0f, 0.7f,
                                                      juce::Decibels::decibelsToGain(eqMidDb));
    *lowFilter.state  = *FilterCoefs::makeLowShelf(sampleRate, 200.0f, 0.7f,
                                                    juce::Decibels::decibelsToGain(eqLowDb));
}

bool DeckProcessor::loadFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;

    fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

    readPosition = 0.0;
    slipPosition = 0.0;
    loadedFileName = file.getFileName();
    loadedFile     = file;
    playing = false;
    looping.store(false);
    for (auto& h : hotCueSamples) h.store(-1);
    ++loadVersion;

    // Run BPM analysis on load (may take a moment for long tracks)
    analyzer.analyze(fileBuffer, sampleRate);

    return true;
}

void DeckProcessor::play()  { playing = true;  if (slipMode) slipPosition = readPosition; }
void DeckProcessor::pause() { playing = false; }
void DeckProcessor::stop()  { playing = false; readPosition = 0.0; slipPosition = 0.0; }

void DeckProcessor::togglePlay()
{
    if (playing) pause(); else play();
}

void DeckProcessor::setPosition(double positionInSeconds)
{
    readPosition = juce::jlimit(0.0,
                                (double)(fileBuffer.getNumSamples() - 1),
                                positionInSeconds * sampleRate);
}

double DeckProcessor::getPosition() const       { return readPosition / sampleRate; }
double DeckProcessor::getLengthInSeconds() const { return fileBuffer.getNumSamples() / sampleRate; }

void DeckProcessor::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if ((playing || scratching) && fileBuffer.getNumSamples() > 0)
    {
        const int   fileSamplesI  = fileBuffer.getNumSamples();
        const int   fileChans     = fileBuffer.getNumChannels();
        const bool  doLoop        = looping.load();
        const double loopInD      = (double)loopInSample.load();
        const double loopOutD     = (double)loopOutSample.load();
        const bool  useKeyLock    = keyLockEnabled.load() && !scratching;

        if (useKeyLock)
        {
            // ── Key-lock path: read at 1.0x rate, SoundTouch time-stretches ──
            // Update SoundTouch tempo to match tempoRatio
            const float tempoChangePct = (tempoRatio - 1.0f) * 100.0f;
            soundTouch->setTempoChange(tempoChangePct);
            soundTouch->setPitchSemiTones(pitchSemitones.load());

            // Feed enough source samples to satisfy the output request
            // SoundTouch may need more input than output samples; keep feeding until we get enough
            const int stereoStride = 2;
            stInterleavedIn.resize((size_t)(numSamples * 2 * 4)); // generous input buffer

            int fed   = 0;
            int got   = 0;
            stInterleavedOut.resize((size_t)(numSamples * stereoStride));

            while (got < numSamples)
            {
                // Pull output if available
                int avail = (int)soundTouch->numSamples();
                if (avail > 0)
                {
                    int toGet = std::min(avail, numSamples - got);
                    stInterleavedOut.resize((size_t)((got + toGet) * stereoStride));
                    soundTouch->receiveSamples(stInterleavedOut.data() + got * stereoStride, (unsigned int)toGet);
                    got += toGet;
                }

                if (got >= numSamples) break;

                // Feed one sample to SoundTouch
                float loopWrap = [&]() -> float {
                    if (doLoop && loopOutD > loopInD && readPosition >= loopOutD)
                        readPosition = loopInD + std::fmod(readPosition - loopInD, loopOutD - loopInD);
                    return 0.0f;
                }();
                juce::ignoreUnused(loopWrap);

                if (readPosition >= (double)fileSamplesI) { playing = false; break; }

                int   p0   = (int)readPosition;
                float frac = (float)(readPosition - p0);
                int   p1   = std::min(p0 + 1, fileSamplesI - 1);

                float inL = 0.0f, inR = 0.0f;
                {
                    const float* srcL = fileBuffer.getReadPointer(0);
                    inL = srcL[p0] + frac * (srcL[p1] - srcL[p0]);
                    if (fileChans > 1) {
                        const float* srcR = fileBuffer.getReadPointer(1);
                        inR = srcR[p0] + frac * (srcR[p1] - srcR[p0]);
                    } else { inR = inL; }
                }

                float inFrames[2] = { inL, inR };
                soundTouch->putSamples(inFrames, 1);
                ++fed;
                readPosition += 1.0;  // advance at natural rate
                if (slipMode) slipPosition += 1.0;
            }

            // Copy interleaved output into buffer
            for (int i = 0; i < std::min(got, numSamples); ++i)
            {
                if (numChannels >= 1) buffer.getWritePointer(0)[i] += stInterleavedOut[(size_t)(i * 2)];
                if (numChannels >= 2) buffer.getWritePointer(1)[i] += stInterleavedOut[(size_t)(i * 2 + 1)];
            }
            juce::ignoreUnused(fed);
        }
        else
        {
            // ── Normal path: linear interpolation at effectiveRate ────────────
            float effectiveRate;
            if (dvsEnabled.load())
                effectiveRate = dvsSpeedDir.load();   // timecode controls rate directly
            else if (scratching)
                effectiveRate = scratchRate;
            else
                effectiveRate = tempoRatio + nudgeOffset;

            for (int i = 0; i < numSamples; ++i)
            {
                if (doLoop && loopOutD > loopInD && readPosition >= loopOutD)
                    readPosition = loopInD + std::fmod(readPosition - loopInD, loopOutD - loopInD);

                if (readPosition < 0.0)  readPosition = 0.0;

                if (readPosition >= (double)fileSamplesI)
                {
                    if (scratching)
                        readPosition = (double)(fileSamplesI - 1);
                    else
                    { readPosition = 0.0; slipPosition = 0.0; playing = false; break; }
                }

                int   p0   = (int)readPosition;
                float frac = (float)(readPosition - p0);
                int   p1   = std::min(p0 + 1, fileSamplesI - 1);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    int fileCh = (ch < fileChans) ? ch : fileChans - 1;
                    const float* src = fileBuffer.getReadPointer(fileCh);
                    buffer.getWritePointer(ch)[i] += src[p0] + frac * (src[p1] - src[p0]);
                }

                readPosition += effectiveRate;

                if (slipMode) slipPosition += tempoRatio;
            }
        }
    }

    // VU meter — RMS of processed output for display
    {
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            rms += buffer.getSample(0, i) * buffer.getSample(0, i);
        vuLevel.store(std::sqrt(rms / (float)numSamples));
    }

    // EQ
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    lowFilter.process(ctx);
    midFilter.process(ctx);
    highFilter.process(ctx);

    // Volume and pan
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float panGain = 1.0f;
        if (ch == 0) panGain = juce::jmin(1.0f, 1.0f - currentPan);
        if (ch == 1) panGain = juce::jmin(1.0f, 1.0f + currentPan);
        buffer.applyGain(ch, 0, numSamples, currentVolume * currentGain * panGain);
    }
}

void DeckProcessor::setVolume(float v) { currentVolume = juce::jlimit(0.0f, 1.0f, v); }
void DeckProcessor::setGain(float g)   { currentGain   = juce::jlimit(0.0f, 2.0f, g); }
void DeckProcessor::setPan(float p)    { currentPan    = juce::jlimit(-1.0f, 1.0f, p); }

void DeckProcessor::setTempo(float bpm)
{
    currentTempo = juce::jlimit(50.0f, 200.0f, bpm);
    tempoRatio   = currentTempo / 120.0f;
}

void DeckProcessor::setEQHigh(float db) { eqHighDb = juce::jlimit(-12.0f, 12.0f, db); updateEQ(); }
void DeckProcessor::setEQMid(float db)  { eqMidDb  = juce::jlimit(-12.0f, 12.0f, db); updateEQ(); }
void DeckProcessor::setEQLow(float db)  { eqLowDb  = juce::jlimit(-12.0f, 12.0f, db); updateEQ(); }

void DeckProcessor::engageScratch(float rate)
{
    if (slipMode && !scratching)
        slipPosition = readPosition; // save where we are before scratch diverges
    scratchRate = juce::jlimit(-8.0f, 8.0f, rate);
    scratching  = true;
}

void DeckProcessor::disengageScratch()
{
    scratchRate = 0.0f;
    scratching  = false;
    if (slipMode)
        readPosition = slipPosition; // snap back to where we'd be without the scratch
}

void DeckProcessor::applyNudge(float offset)  { nudgeOffset = juce::jlimit(-0.5f, 0.5f, offset); }
void DeckProcessor::releaseNudge()             { nudgeOffset = 0.0f; }

void DeckProcessor::setSlipMode(bool enabled)
{
    slipMode = enabled;
    if (enabled) slipPosition = readPosition;
}

// Beat-grid helper: snap a sample position to the nearest beat if quantize is on.
double DeckProcessor::snapToBeat(double posSamples) const
{
    if (!processorPtr || !processorPtr->isQuantizeEnabled()) return posSamples;
    float bpm = analyzer.getDetectedBPM();
    if (bpm <= 0.0f) bpm = currentTempo;
    double beatPeriod = sampleRate * 60.0 / (double)bpm;
    return std::round(posSamples / beatPeriod) * beatPeriod;
}

// Hot cues

void DeckProcessor::activateHotCue(int idx)
{
    if (idx < 0 || idx >= NUM_HOT_CUES) return;
    int64_t stored = hotCueSamples[idx].load();
    if (stored < 0)
        hotCueSamples[idx].store((int64_t)snapToBeat(readPosition)); // set
    else
        readPosition = (double)stored;                                // jump
}

void DeckProcessor::clearHotCue(int idx)
{
    if (idx >= 0 && idx < NUM_HOT_CUES)
        hotCueSamples[idx].store(-1);
}

bool DeckProcessor::isHotCueSet(int idx) const
{
    return idx >= 0 && idx < NUM_HOT_CUES && hotCueSamples[idx].load() >= 0;
}

double DeckProcessor::getHotCueSamples(int idx) const
{
    return (idx >= 0 && idx < NUM_HOT_CUES) ? (double)hotCueSamples[idx].load() : -1.0;
}

// Loop

void DeckProcessor::setLoopIn()
{
    loopInSample.store((int64_t)snapToBeat(readPosition));
    if (looping.load()) looping.store(false);
}

void DeckProcessor::setLoopOut()
{
    int64_t in  = loopInSample.load();
    int64_t out = (int64_t)snapToBeat(readPosition);
    if (out > in)
    {
        loopOutSample.store(out);
        looping.store(true);
    }
}

void DeckProcessor::toggleLoop()
{
    looping.store(!looping.load());
}

void DeckProcessor::setAutoLoop(float bars)
{
    float bpm = analyzer.getDetectedBPM();
    if (bpm <= 0.0f) bpm = currentTempo;

    double beatsPerBar   = 4.0;
    double barLenSamples = sampleRate * 60.0 / bpm * beatsPerBar * (double)bars;

    int64_t in  = (int64_t)readPosition;
    int64_t out = in + (int64_t)barLenSamples;
    out = std::min(out, (int64_t)(fileBuffer.getNumSamples() - 1));

    loopInSample.store(in);
    loopOutSample.store(out);
    looping.store(true);
}

double DeckProcessor::getLoopInSeconds()  const { return (double)loopInSample.load()  / sampleRate; }
double DeckProcessor::getLoopOutSeconds() const { return (double)loopOutSample.load() / sampleRate; }

// Beat jump

void DeckProcessor::setHotCueSamplesRaw(int idx, int64_t samples)
{
    if (idx >= 0 && idx < NUM_HOT_CUES)
        hotCueSamples[idx].store(samples);
}

void DeckProcessor::setLoopPoints(double loopInSamples, double loopOutSamples, bool activate)
{
    loopInSample.store((int64_t)loopInSamples);
    loopOutSample.store((int64_t)loopOutSamples);
    looping.store(activate);
}

void DeckProcessor::beatJump(int beats)
{
    float bpm = analyzer.getDetectedBPM();
    if (bpm <= 0.0f) bpm = currentTempo;

    double beatLenSamples = sampleRate * 60.0 / bpm;
    double newPos = readPosition + beats * beatLenSamples;
    newPos = juce::jlimit(0.0, (double)(fileBuffer.getNumSamples() - 1), newPos);
    readPosition = newPos;
    if (slipMode) slipPosition = newPos;
}
