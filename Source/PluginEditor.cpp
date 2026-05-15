#include "PluginEditor.h"

// Industrial road-case colour palette
static constexpr juce::uint32 kDeckA  = 0xffff9500;  // Pioneer amber
static constexpr juce::uint32 kDeckB  = 0xff0090d4;  // Pioneer CDJ blue
static constexpr juce::uint32 kBg     = 0xff1c1c1c;
static constexpr juce::uint32 kPanel  = 0xff252525;

PatchworkAudioProcessorEditor::PatchworkAudioProcessorEditor(PatchworkAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      deckAComponent(p.getDeckA(), "A"),
      deckBComponent(p.getDeckB(), "B"),
      waveformAComponent(p.getDeckA(), juce::Colour(kDeckA)),
      waveformBComponent(p.getDeckB(), juce::Colour(kDeckB))
{
    setSize(1440, 840);
    setLookAndFeel(&lookAndFeel);

    // ── Main deck components ──────────────────────────────────────────
    addAndMakeVisible(deckAComponent);
    addAndMakeVisible(deckBComponent);
    addAndMakeVisible(waveformAComponent);
    addAndMakeVisible(waveformBComponent);

    // ── Mixer strip ──────────────────────────────────────────────────
    mixerStrip = std::make_unique<MixerStripComponent>(p.getDeckA(), p.getDeckB(), p);
    addAndMakeVisible(*mixerStrip);

    // ── REC button ───────────────────────────────────────────────────
    recButton.setButtonText("REC");
    recButton.setClickingTogglesState(true);
    recButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2a0808));
    recButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffcc1111));
    recButton.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xffcc1111));
    recButton.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    recButton.onClick = [this] {
        audioProcessor.sendMMCRecord(recButton.getToggleState());
    };
    addAndMakeVisible(recButton);

    recModeLabel.setText(audioProcessor.usesOSC()
                             ? ("OSC " + audioProcessor.getOscTargetIP())
                             : "MMC",
                         juce::dontSendNotification);
    recModeLabel.setFont(juce::Font(7.0f));
    recModeLabel.setJustificationType(juce::Justification::centred);
    recModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a4a6a));
    addAndMakeVisible(recModeLabel);

    // ── Master volume ────────────────────────────────────────────────
    setupSlider(masterVolumeSlider, masterVolumeLabel, "MASTER", 0.0, 1.0, 0.8);
    masterVolumeSlider.onValueChange = [this] {
        if (auto* param = audioProcessor.getValueTreeState().getParameter("master_volume"))
            param->setValueNotifyingHost(
                param->convertTo0to1((float)masterVolumeSlider.getValue()));
    };

    auto styleWinBtn = [](juce::TextButton& b, const juce::String& text) {
        b.setButtonText(text);
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff252525));
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
        b.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xff9a9a9a));
        b.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    };

    // ── Window buttons ────────────────────────────────────────────────
    styleWinBtn(fxWindowBtn,      "FX");
    styleWinBtn(samplerWindowBtn, "PADS");
    styleWinBtn(midiWindowBtn,    "MIDI");
    styleWinBtn(libraryWindowBtn, "LIBRARY");
    addAndMakeVisible(fxWindowBtn);
    addAndMakeVisible(samplerWindowBtn);
    addAndMakeVisible(midiWindowBtn);

    // ── Floating sub-windows ─────────────────────────────────────────
    effectsComponent    = std::make_unique<EffectsComponent>(p.getEffects());
    samplerComponent    = std::make_unique<SamplerComponent>(p.getSampler());
    midiMonitorComponent = std::make_unique<MidiMonitorComponent>(p.getMidiController());

    fxWindow = std::make_unique<FloatingWindow>("Patchwork FX Rack",
                                                juce::Colour(kBg));
    fxWindow->setContentNonOwned(effectsComponent.get(), false);
    fxWindow->setResizable(true, false);

    samplerWindow = std::make_unique<FloatingWindow>("Patchwork Sampler",
                                                      juce::Colour(kBg));
    samplerWindow->setContentNonOwned(samplerComponent.get(), false);
    samplerWindow->setResizable(true, false);

    midiWindow = std::make_unique<FloatingWindow>("Patchwork MIDI",
                                                   juce::Colour(kBg));
    midiWindow->setContentNonOwned(midiMonitorComponent.get(), false);
    midiWindow->setResizable(true, false);

    libraryComponent = std::make_unique<LibraryComponent>();
    libraryComponent->onLoadTrack = [this, &p](const juce::File& file, bool deckB)
    {
        auto& deck = deckB ? p.getDeckB() : p.getDeckA();
        if (deck.loadFile(file))
        {
            float        bpm = deck.getDetectedBPM();
            juce::String key = deck.getDetectedKey();
            libraryComponent->updateTrackAnalysis(
                file,
                bpm > 0.0f ? juce::String(bpm, 1) : "--",
                key.isEmpty() ? "--" : key);
        }
    };

    // Library is floating only — opened via LIBRARY button
    libraryWindow = std::make_unique<FloatingWindow>("Patchwork Library",
                                                      juce::Colour(kBg));
    libraryWindow->setContentNonOwned(libraryComponent.get(), false);
    libraryWindow->setResizable(true, false);

    addAndMakeVisible(libraryWindowBtn);

    fxWindowBtn.onClick = [this] {
        if (fxWindow->isVisible())
        {
            fxWindow->setVisible(false);
        }
        else
        {
            fxWindow->centreWithSize(1280, 400);
            fxWindow->setVisible(true);
            fxWindow->toFront(true);
        }
    };

    samplerWindowBtn.onClick = [this] {
        if (samplerWindow->isVisible())
        {
            samplerWindow->setVisible(false);
        }
        else
        {
            samplerWindow->centreWithSize(980, 580);
            samplerWindow->setVisible(true);
            samplerWindow->toFront(true);
        }
    };

    midiWindowBtn.onClick = [this] {
        if (midiWindow->isVisible())
        {
            midiWindow->setVisible(false);
        }
        else
        {
            midiWindow->centreWithSize(860, 460);
            midiWindow->setVisible(true);
            midiWindow->toFront(true);
        }
    };

    libraryWindowBtn.onClick = [this] {
        if (libraryWindow->isVisible()) {
            libraryWindow->setVisible(false);
        } else {
            libraryWindow->centreWithSize(1000, 520);
            libraryWindow->setVisible(true);
            libraryWindow->toFront(true);
        }
    };

    broadcastComponent = std::make_unique<BroadcastComponent>(
        p.getBroadcastEngine(), p.getMixRecorder());
    broadcastWindow = std::make_unique<FloatingWindow>("Patchwork Broadcast & Recording",
                                                        juce::Colour(kBg));
    broadcastWindow->setContentNonOwned(broadcastComponent.get(), false);
    broadcastWindow->setResizable(true, false);

    broadcastWindowBtn.setButtonText("OUT");
    broadcastWindowBtn.setClickingTogglesState(true);
    broadcastWindowBtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff252525));
    broadcastWindowBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
    broadcastWindowBtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(kDeckA).withAlpha(0.8f));
    broadcastWindowBtn.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    addAndMakeVisible(broadcastWindowBtn);
    broadcastWindowBtn.onClick = [this] {
        if (broadcastWindow->isVisible()) { broadcastWindow->setVisible(false); return; }
        broadcastWindow->centreWithSize(420, 360);
        broadcastWindow->setVisible(true);
        broadcastWindow->toFront(true);
    };

    // ── MIDI device label ────────────────────────────────────────────────
    devicesLabel.setFont(juce::Font(8.0f));
    devicesLabel.setColour(juce::Label::textColourId, juce::Colour(0xff44446a));
    devicesLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(devicesLabel);

    // ── Master clock ─────────────────────────────────────────────────────
    masterBpmLabel.setText("120.0", juce::dontSendNotification);
    masterBpmLabel.setJustificationType(juce::Justification::centred);
    masterBpmLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    masterBpmLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterBpmLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
    masterBpmLabel.setEditable(true);
    masterBpmLabel.onEditorHide = [this] {
        float bpm = masterBpmLabel.getText().getFloatValue();
        if (bpm >= 60.0f && bpm <= 200.0f)
            audioProcessor.setMasterBPM(bpm);
    };
    addAndMakeVisible(masterBpmLabel);

    auto styleHdrBtn = [](juce::TextButton& b, const juce::String& text,
                          juce::uint32 bg, juce::uint32 fg) {
        b.setButtonText(text);
        b.setColour(juce::TextButton::buttonColourId,  juce::Colour(bg));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(fg));
    };

    styleHdrBtn(masterBpmDownBtn, "-",      0xff252525, 0xffc0c0c0);
    masterBpmDownBtn.onClick = [this] {
        audioProcessor.setMasterBPM(audioProcessor.getMasterBPM() - 0.5f);
    };
    addAndMakeVisible(masterBpmDownBtn);

    styleHdrBtn(masterBpmUpBtn, "+",        0xff252525, 0xffc0c0c0);
    masterBpmUpBtn.onClick = [this] {
        audioProcessor.setMasterBPM(audioProcessor.getMasterBPM() + 0.5f);
    };
    addAndMakeVisible(masterBpmUpBtn);

    styleHdrBtn(tapTempoBtn, "TAP",         0xff252525, 0xffc0c0c0);
    tapTempoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kDeckA));
    tapTempoBtn.onClick = [this] { audioProcessor.tapTempo(); };
    addAndMakeVisible(tapTempoBtn);

    styleHdrBtn(syncAllBtn, "SYNC ALL",     0xff252525, 0xffc0c0c0);
    syncAllBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kDeckB));
    syncAllBtn.onClick = [this] { audioProcessor.syncDecksToMaster(); };
    addAndMakeVisible(syncAllBtn);

    styleHdrBtn(beatGridBtn, "GRID",        0xff252525, 0xffc0c0c0);
    beatGridBtn.setClickingTogglesState(true);
    beatGridBtn.onClick = [this] {
        bool on = beatGridBtn.getToggleState();
        waveformAComponent.setShowBeatGrid(on);
        waveformBComponent.setShowBeatGrid(on);
    };
    addAndMakeVisible(beatGridBtn);

    styleHdrBtn(quantizeBtn, "QUANT",       0xff252525, 0xffc0c0c0);
    quantizeBtn.setClickingTogglesState(true);
    quantizeBtn.onClick = [this] {
        audioProcessor.setQuantizeEnabled(quantizeBtn.getToggleState());
    };
    addAndMakeVisible(quantizeBtn);

    // ── Sync wiring ──────────────────────────────────────────────────
    deckAComponent.setOtherDeckBpmGetter([&p] {
        float d = p.getDeckB().getDetectedBPM();
        return d > 0.0f ? d : p.getDeckB().getTempo();
    });
    deckBComponent.setOtherDeckBpmGetter([&p] {
        float d = p.getDeckA().getDetectedBPM();
        return d > 0.0f ? d : p.getDeckA().getTempo();
    });

    startTimerHz(30);
}

PatchworkAudioProcessorEditor::~PatchworkAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
    if (fxWindow)        fxWindow->setVisible(false);
    if (samplerWindow)   samplerWindow->setVisible(false);
    if (midiWindow)      midiWindow->setVisible(false);
    if (libraryWindow)   libraryWindow->setVisible(false);
    if (broadcastWindow) broadcastWindow->setVisible(false);
}

void PatchworkAudioProcessorEditor::timerCallback()
{
    waveformAComponent.update();
    waveformBComponent.update();
    if (samplerComponent) samplerComponent->updatePadStates();

    deckAComponent.update();
    deckBComponent.update();
    if (mixerStrip) mixerStrip->update();

    // Keep toggle button states in sync with window visibility
    fxWindowBtn.setToggleState(fxWindow && fxWindow->isVisible(),
                               juce::dontSendNotification);
    samplerWindowBtn.setToggleState(samplerWindow && samplerWindow->isVisible(),
                                    juce::dontSendNotification);
    midiWindowBtn.setToggleState(midiWindow && midiWindow->isVisible(),
                                 juce::dontSendNotification);
    libraryWindowBtn.setToggleState(libraryWindow && libraryWindow->isVisible(),
                                    juce::dontSendNotification);
    broadcastWindowBtn.setToggleState(broadcastWindow && broadcastWindow->isVisible(),
                                      juce::dontSendNotification);

    // Push latest BPM/key to library for currently-loaded files
    if (libraryComponent)
    {
        auto sync = [this](DeckProcessor& deck)
        {
            if (!deck.hasFile()) return;
            float        bpm = deck.getDetectedBPM();
            juce::String key = deck.getDetectedKey();
            libraryComponent->updateTrackAnalysis(
                deck.getLoadedFile(),
                bpm > 0.0f ? juce::String(bpm, 1) : "--",
                key.isEmpty() ? "--" : key);
        };
        sync(audioProcessor.getDeckA());
        sync(audioProcessor.getDeckB());
    }

    // Update master BPM label (only if not being edited)
    if (!masterBpmLabel.isBeingEdited())
        masterBpmLabel.setText(juce::String(audioProcessor.getMasterBPM(), 1),
                               juce::dontSendNotification);

    beatGridBtn .setToggleState(waveformAComponent.isShowingBeatGrid(),
                                juce::dontSendNotification);
    quantizeBtn .setToggleState(audioProcessor.isQuantizeEnabled(),
                                juce::dontSendNotification);

    // Update MIDI device display (cheap — just a StringArray join)
    auto devs = audioProcessor.getConnectedDevices();
    devicesLabel.setText(devs.isEmpty() ? "No MIDI controllers" : devs.joinIntoString("  |  "),
                         juce::dontSendNotification);
}

void PatchworkAudioProcessorEditor::setupSlider(juce::Slider& s, juce::Label& l,
                                                  const juce::String& name,
                                                  double min, double max, double def)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(min, max);
    s.setValue(def);
    s.setColour(juce::Slider::thumbColourId,      juce::Colours::white);
    s.setColour(juce::Slider::trackColourId,      juce::Colour(0xff2a2a2a));
    s.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(9.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(0xff5a5a7a));
    addAndMakeVisible(l);
}

void PatchworkAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    // Main background — dark charcoal
    g.setColour(juce::Colour(kBg));
    g.fillAll();

    // Header — brushed steel simulation
    {
        juce::ColourGradient hdr(juce::Colour(0xff2e2e2e), 0, 0,
                                  juce::Colour(0xff202020), 0, 40, false);
        g.setGradientFill(hdr);
        g.fillRect(0, 0, getWidth(), 40);
    }

    // Header bottom groove
    g.setColour(juce::Colour(0xff0e0e0e));
    g.fillRect(0, 38, getWidth(), 1);
    g.setColour(juce::Colour(0xff3e3e3e));
    g.fillRect(0, 39, getWidth(), 1);

    // Deck indicator stripes — amber for A (left), blue for B (right)
    const int cx = getWidth() / 2;
    g.setColour(juce::Colour(kDeckA).withAlpha(0.75f));
    g.fillRect(0, 37, cx - 120, 2);
    g.setColour(juce::Colour(kDeckB).withAlpha(0.75f));
    g.fillRect(cx + 120, 37, getWidth() - cx - 120, 2);

    // Title
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f).withStyle("Bold")));
    g.setColour(juce::Colours::white.withAlpha(0.88f));
    g.drawText("PATCHWORK", getLocalBounds().removeFromTop(40), juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
    g.setColour(juce::Colour(0xff484848));
    g.drawText("v1.0", juce::Rectangle<int>(getWidth() - 44, 14, 38, 12),
               juce::Justification::centredRight);

    // Center groove divider between decks
    g.setColour(juce::Colour(0xff111111));
    g.fillRect(cx - 1, 40, 2, getHeight() - 40);
    g.setColour(juce::Colour(0xff383838));
    g.drawVerticalLine(cx, 40.0f, (float)getHeight());
}

void PatchworkAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);
}

void PatchworkAudioProcessorEditor::resized()
{
    // Header controls (right side): REC | OUT | LIB | MIDI | FX | SMPL
    recButton         .setBounds(getWidth() - 278, 4,  60, 22);
    recModeLabel      .setBounds(getWidth() - 278, 26, 60,  8);
    broadcastWindowBtn.setBounds(getWidth() - 214, 4,  36, 28);
    libraryWindowBtn  .setBounds(getWidth() - 174, 4,  40, 28);
    midiWindowBtn     .setBounds(getWidth() - 130, 4,  40, 28);
    fxWindowBtn       .setBounds(getWidth() -  86, 4,  36, 28);
    samplerWindowBtn  .setBounds(getWidth() -  46, 4,  42, 28);

    // Master clock (centre of header): [-] [BPM] [+] [TAP] | [SYNC ALL] [GRID] [QUANT]
    int cx = getWidth() / 2;
    masterBpmDownBtn.setBounds(cx - 82, 5, 18, 24);
    masterBpmLabel  .setBounds(cx - 62, 5, 56, 24);
    masterBpmUpBtn  .setBounds(cx -  4, 5, 18, 24);
    tapTempoBtn     .setBounds(cx +  16, 5, 34, 24);
    syncAllBtn      .setBounds(cx +  54, 5, 58, 24);
    beatGridBtn     .setBounds(cx + 116, 5, 36, 24);
    quantizeBtn     .setBounds(cx + 156, 5, 44, 24);

    devicesLabel.setBounds(4, 22, cx - 100, 12);

    auto area = getLocalBounds();
    area.removeFromTop(40); // header

    // Waveforms
    auto waveArea = area.removeFromTop(100);
    auto waveHalf = waveArea.getWidth() / 2;
    waveformAComponent.setBounds(waveArea.removeFromLeft(waveHalf).reduced(4, 4));
    waveformBComponent.setBounds(waveArea.reduced(4, 4));

    // Middle: Deck A | Mixer | Deck B
    int mixW = 190;
    auto deckArea = area;
    auto deckAArea = deckArea.removeFromLeft((deckArea.getWidth() - mixW) / 2);
    auto mixArea   = deckArea.removeFromLeft(mixW);
    auto deckBArea = deckArea;

    deckAComponent.setBounds(deckAArea.reduced(4, 2));
    deckBComponent.setBounds(deckBArea.reduced(4, 2));
    if (mixerStrip) mixerStrip->setBounds(mixArea.reduced(2, 2));

    // Master volume bar (left side of header area bottom)
    masterVolumeLabel .setBounds(4, 4, 60, 10);
    masterVolumeSlider.setBounds(4, 14, 120, 18);
}
