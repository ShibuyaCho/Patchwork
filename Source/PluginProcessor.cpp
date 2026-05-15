#include "PluginProcessor.h"
#include "PluginEditor.h"

PatchworkAudioProcessor::PatchworkAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Main",   juce::AudioChannelSet::stereo(), true)
                     .withOutput("Cue",    juce::AudioChannelSet::stereo(), false)), // optional headphone bus
      deckA(this),
      deckB(this),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    midiController.setCallback([this](MidiMapping::Target t, float v) {
        onMidiTarget(t, v);
    });
    midiController.scanDevices(); // initial device scan
    startTimerHz(1);              // 1 Hz rescan for plug/unplug

    // OSC sender for Reaper — bind to the machine's primary non-loopback IP,
    // which is what Reaper exposes as its OSC listen address.
    // Falls back to loopback if no network interface is found.
    oscTargetIP = juce::IPAddress::getLocalAddress().toString();
    if (oscTargetIP.isEmpty() || oscTargetIP == "0.0.0.0")
        oscTargetIP = "127.0.0.1";
    oscSender.connect(oscTargetIP, 8000);
}

PatchworkAudioProcessor::~PatchworkAudioProcessor()
{
    stopTimer();
}

void PatchworkAudioProcessor::onMidiTarget(MidiMapping::Target target, float value)
{
    using T = MidiMapping::Target;
    switch (target)
    {
        case T::DeckAVolume:  deckA.setVolume(value); break;
        case T::DeckBVolume:  deckB.setVolume(value); break;
        case T::DeckATempo:   deckA.setTempo(60.0f + value * 140.0f); break;
        case T::DeckBTempo:   deckB.setTempo(60.0f + value * 140.0f); break;
        case T::DeckAPan:     deckA.setPan(value * 2.0f - 1.0f); break;
        case T::DeckBPan:     deckB.setPan(value * 2.0f - 1.0f); break;
        case T::DeckAEQHigh:  deckA.setEQHigh((value - 0.5f) * 24.0f); break;
        case T::DeckAEQMid:   deckA.setEQMid((value - 0.5f) * 24.0f); break;
        case T::DeckAEQLow:   deckA.setEQLow((value - 0.5f) * 24.0f); break;
        case T::DeckBEQHigh:  deckB.setEQHigh((value - 0.5f) * 24.0f); break;
        case T::DeckBEQMid:   deckB.setEQMid((value - 0.5f) * 24.0f); break;
        case T::DeckBEQLow:   deckB.setEQLow((value - 0.5f) * 24.0f); break;
        case T::Crossfader:   setCrossfader(value); break;
        case T::ReverbAmount: effectsChain.setReverbAmount(value); break;
        case T::DelayAmount:  effectsChain.setDelayAmount(value); break;
        case T::FilterFreq:   effectsChain.setFilterFrequency(20.0f + value * 19980.0f); break;
        case T::DeckAPlay:    if (value > 0.5f) deckA.togglePlay();   break;
        case T::DeckBPlay:    if (value > 0.5f) deckB.togglePlay();   break;
        case T::DeckALoop:    if (value > 0.5f) deckA.toggleLoop();   break;
        case T::DeckBLoop:    if (value > 0.5f) deckB.toggleLoop();   break;

        case T::DeckASync:
            if (value > 0.5f) {
                float bpm = deckB.getDetectedBPM() > 0 ? deckB.getDetectedBPM() : deckB.getTempo();
                if (bpm > 0) deckA.setTempo(bpm);
            }
            break;
        case T::DeckBSync:
            if (value > 0.5f) {
                float bpm = deckA.getDetectedBPM() > 0 ? deckA.getDetectedBPM() : deckA.getTempo();
                if (bpm > 0) deckB.setTempo(bpm);
            }
            break;

        case T::DeckAHotCue0: if (value > 0.5f) deckA.activateHotCue(0); break;
        case T::DeckAHotCue1: if (value > 0.5f) deckA.activateHotCue(1); break;
        case T::DeckAHotCue2: if (value > 0.5f) deckA.activateHotCue(2); break;
        case T::DeckAHotCue3: if (value > 0.5f) deckA.activateHotCue(3); break;
        case T::DeckBHotCue0: if (value > 0.5f) deckB.activateHotCue(0); break;
        case T::DeckBHotCue1: if (value > 0.5f) deckB.activateHotCue(1); break;
        case T::DeckBHotCue2: if (value > 0.5f) deckB.activateHotCue(2); break;
        case T::DeckBHotCue3: if (value > 0.5f) deckB.activateHotCue(3); break;

        case T::DeckACue:
            if (value > 0.5f) deckA.setCueActive(!deckA.isCueActive());
            break;
        case T::DeckBCue:
            if (value > 0.5f) deckB.setCueActive(!deckB.isCueActive());
            break;

        // Pregain knobs
        case T::DeckAGain: deckA.setGain(value * 2.0f); break; // 0-127 → 0-2.0x
        case T::DeckBGain: deckB.setGain(value * 2.0f); break;

        // Jog wheel — relative CC (MIDI 64=center, below=forward, above=backward)
        case T::DeckAJog:
        case T::DeckBJog:
        {
            auto& deck = (target == T::DeckAJog) ? deckA : deckB;
            float raw   = value * 127.0f;
            float delta = (raw < 64.0f) ? raw : raw - 128.0f; // -63..+63
            if (deck.isScratchModeEnabled())
            {
                if (delta != 0.0f) deck.engageScratch(delta * 0.04f);
                else               deck.disengageScratch();
            }
            else
            {
                if (delta != 0.0f) deck.applyNudge(delta * 0.003f);
                else               deck.releaseNudge();
            }
            break;
        }

        // Scratch/CDJ mode toggle (vinyl button)
        case T::DeckAScratchMode:
            if (value > 0.5f) deckA.toggleScratchMode();
            break;
        case T::DeckBScratchMode:
            if (value > 0.5f) deckB.toggleScratchMode();
            break;

        // DJ cue point — jump to or set the cue point (mapped to hot cue 0)
        case T::DeckACuePt: if (value > 0.5f) deckA.activateHotCue(0); break;
        case T::DeckBCuePt: if (value > 0.5f) deckB.activateHotCue(0); break;

        // Beat-loop pads: 0.5 / 1 / 2 / 4 bars
        case T::DeckABeatLoop0: if (value > 0.5f) deckA.setAutoLoop(0.5f); break;
        case T::DeckABeatLoop1: if (value > 0.5f) deckA.setAutoLoop(1.0f); break;
        case T::DeckABeatLoop2: if (value > 0.5f) deckA.setAutoLoop(2.0f); break;
        case T::DeckABeatLoop3: if (value > 0.5f) deckA.setAutoLoop(4.0f); break;
        case T::DeckBBeatLoop0: if (value > 0.5f) deckB.setAutoLoop(0.5f); break;
        case T::DeckBBeatLoop1: if (value > 0.5f) deckB.setAutoLoop(1.0f); break;
        case T::DeckBBeatLoop2: if (value > 0.5f) deckB.setAutoLoop(2.0f); break;
        case T::DeckBBeatLoop3: if (value > 0.5f) deckB.setAutoLoop(4.0f); break;

        default:
        {
            // Sampler pads
            int padBase = static_cast<int>(MidiMapping::Target::SamplerPad0);
            int padIdx  = static_cast<int>(target) - padBase;
            if (padIdx >= 0 && padIdx < 16)
            {
                if (value > 0.0f)
                    sampler.triggerPad(padIdx, value);
                else
                    sampler.releasePad(padIdx);
                // Immediately push LED state so pad controllers light up without
                // waiting for the next 1 Hz timer tick.
                juce::MessageManager::callAsync([this] { refreshDeviceLEDs(); });
            }
            break;
        }
    }
}

void PatchworkAudioProcessor::setCrossfader(float value)
{
    crossfader = juce::jlimit(0.0f, 1.0f, value);
}

void PatchworkAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;
    deckA.prepareToPlay(sampleRate, samplesPerBlock);
    deckB.prepareToPlay(sampleRate, samplesPerBlock);
    effectsChain.prepareToPlay(sampleRate, samplesPerBlock);
    sampler.prepareToPlay(sampleRate, samplesPerBlock);
}

void PatchworkAudioProcessor::releaseResources()
{
    deckA.releaseResources();
    deckB.releaseResources();
    effectsChain.releaseResources();
    sampler.releaseResources();
}

bool PatchworkAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    // Cue bus: must be stereo or disabled (0 channels)
    auto cueBus = layouts.getChannelSet(false, 1);
    if (!cueBus.isDisabled() && cueBus != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void PatchworkAudioProcessor::sendMMCRecord(bool start)
{
    if (usesOSC())
    {
        // Reaper OSC: /record is a toggle button — always send 1.0 as a pulse trigger.
        // Sending 0.0 on release would double-toggle and cancel the action.
        if (start)
        {
            juce::OSCMessage msg("/record");
            msg.addFloat32(1.0f);
            oscSender.send(msg);
        }
    }
    else
    {
        // MMC sysex — injected into the MIDI output buffer on the next audio block
        if (start) pendingRecordOn  = true;
        else       pendingRecordOff = true;
    }
}

void PatchworkAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Save stereo input for DVS before we overwrite the buffer with our output
    if (deckA.isDVSEnabled() || deckB.isDVSEnabled())
    {
        dvsInputBuffer.makeCopyOf(buffer);
        const float* dL = dvsInputBuffer.getReadPointer(0);
        const float* dR = dvsInputBuffer.getNumChannels() > 1 ? dvsInputBuffer.getReadPointer(1) : dL;
        int n = dvsInputBuffer.getNumSamples();
        double sr = currentSampleRate;

        if (deckA.isDVSEnabled())
        {
            timecodeA.process(dL, dR, n, sr);
            deckA.setDVSRate(timecodeA.getSpeedRatio(), timecodeA.getDirection());
        }
        if (deckB.isDVSEnabled())
        {
            timecodeB.process(dL, dR, n, sr);
            deckB.setDVSRate(timecodeB.getSpeedRatio(), timecodeB.getDirection());
        }
    }

    // Consume incoming MIDI
    handleMidiInput(midiMessages);

    // Clear incoming — we own the outgoing MIDI buffer from here
    midiMessages.clear();

    // Inject MMC sysex if record was toggled from UI
    // MMC Record Strobe: F0 7F 7F 06 06 F7
    // MMC Record Exit:   F0 7F 7F 06 07 F7
    if (pendingRecordOn.exchange(false))
    {
        const juce::uint8 data[] = { 0x7F, 0x7F, 0x06, 0x06 };
        midiMessages.addEvent(juce::MidiMessage::createSysExMessage(data, 4), 0);
    }
    if (pendingRecordOff.exchange(false))
    {
        const juce::uint8 data[] = { 0x7F, 0x7F, 0x06, 0x07 };
        midiMessages.addEvent(juce::MidiMessage::createSysExMessage(data, 4), 0);
    }

    const int numSamples = buffer.getNumSamples();
    const int totalCh    = buffer.getNumChannels(); // 2 = main only, 4 = main + cue
    const bool hasCueBus = (totalCh >= 4);

    // Deck processing always in stereo
    juce::AudioBuffer<float> bufA(2, numSamples);
    juce::AudioBuffer<float> bufB(2, numSamples);
    bufA.clear();
    bufB.clear();

    deckA.processBlock(bufA);
    deckB.processBlock(bufB);

    // ── Cue / headphone bus (channels 2-3) ───────────────────────────────
    // Written BEFORE the main mix so we use pre-crossfader, pre-effects signal.
    if (hasCueBus)
    {
        buffer.clear(2, 0, numSamples);
        buffer.clear(3, 0, numSamples);

        bool cueA = deckA.isCueActive();
        bool cueB = deckB.isCueActive();
        float norm = (cueA && cueB) ? 0.5f : 1.0f; // prevent clipping if both cued
        float vol  = headphoneVolume.load() * norm;

        if (cueA)
        {
            buffer.addFrom(2, 0, bufA, 0, 0, numSamples, vol);
            buffer.addFrom(3, 0, bufA, 1, 0, numSamples, vol);
        }
        if (cueB)
        {
            buffer.addFrom(2, 0, bufB, 0, 0, numSamples, vol);
            buffer.addFrom(3, 0, bufB, 1, 0, numSamples, vol);
        }
    }

    // ── Main mix (channels 0-1) ───────────────────────────────────────────
    float gainA = juce::jmin(1.0f, 2.0f * (1.0f - crossfader));
    float gainB = juce::jmin(1.0f, 2.0f * crossfader);

    buffer.clear(0, 0, numSamples);
    buffer.clear(1, 0, numSamples);
    buffer.addFrom(0, 0, bufA, 0, 0, numSamples, gainA);
    buffer.addFrom(1, 0, bufA, 1, 0, numSamples, gainA);
    buffer.addFrom(0, 0, bufB, 0, 0, numSamples, gainB);
    buffer.addFrom(1, 0, bufB, 1, 0, numSamples, gainB);

    // Sampler and effects only on the main mix channels
    juce::AudioBuffer<float> mainView(buffer.getArrayOfWritePointers(), 2, numSamples);
    sampler.processBlock(mainView);
    effectsChain.processBlock(mainView);

    buffer.applyGain(0, 0, numSamples, *apvts.getRawParameterValue("master_volume"));
    buffer.applyGain(1, 0, numSamples, *apvts.getRawParameterValue("master_volume"));

    // Feed post-master mix to recorder and broadcast engine
    mixRecorder    .processBlock(mainView);
    broadcastEngine.processBlock(mainView);
}

void PatchworkAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer&)
{
    buffer.clear();
}

void PatchworkAudioProcessor::handleMidiInput(const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
        midiController.handleHostMidiMessage(metadata.getMessage());
}

bool PatchworkAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PatchworkAudioProcessor::createEditor()
{
    return new PatchworkAudioProcessorEditor(*this);
}

const juce::String PatchworkAudioProcessor::getName() const { return JucePlugin_Name; }
bool PatchworkAudioProcessor::acceptsMidi()  const { return true; }
bool PatchworkAudioProcessor::producesMidi() const { return true; }
bool PatchworkAudioProcessor::isMidiEffect() const { return false; }
double PatchworkAudioProcessor::getTailLengthSeconds() const { return 2.0; }
int PatchworkAudioProcessor::getNumPrograms() { return 1; }
int PatchworkAudioProcessor::getCurrentProgram() { return 0; }
void PatchworkAudioProcessor::setCurrentProgram(int) {}
const juce::String PatchworkAudioProcessor::getProgramName(int) { return {}; }
void PatchworkAudioProcessor::changeProgramName(int, const juce::String&) {}

void PatchworkAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PatchworkAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
PatchworkAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "master_volume", "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "crossfader", "Crossfader",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    return layout;
}

void PatchworkAudioProcessor::timerCallback()
{
    midiController.scanDevices();
    refreshDeviceLEDs();
}

void PatchworkAudioProcessor::refreshDeviceLEDs()
{
    midiController.refreshPadLEDs([this](int i) -> int {
        if (sampler.isPadPlaying(i)) return 127;
        if (sampler.isPadLoaded(i))  return 45;
        return 0;
    });
}

void PatchworkAudioProcessor::setMasterBPM(float bpm)
{
    masterBPM.store(juce::jlimit(60.0f, 200.0f, bpm));
}

void PatchworkAudioProcessor::tapTempo()
{
    double now = juce::Time::getMillisecondCounterHiRes();
    tapTimes.push_back(now);
    // Keep only last 8 taps; reset if gap > 3 seconds
    if (tapTimes.size() > 1 && (now - tapTimes[tapTimes.size() - 2]) > 3000.0)
        tapTimes = { now };
    if (tapTimes.size() > 8)
        tapTimes.erase(tapTimes.begin());

    if (tapTimes.size() >= 2)
    {
        double totalMs  = tapTimes.back() - tapTimes.front();
        double avgMs    = totalMs / (double)(tapTimes.size() - 1);
        float  bpm      = (float)(60000.0 / avgMs);
        setMasterBPM(bpm);
    }
}

void PatchworkAudioProcessor::syncDecksToMaster()
{
    float bpm = masterBPM.load();
    deckA.setTempo(bpm);
    deckB.setTempo(bpm);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PatchworkAudioProcessor();
}
