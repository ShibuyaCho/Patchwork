#include "DeckComponent.h"

static constexpr juce::uint32 kDeckA = 0xffff9500;  // Pioneer amber
static constexpr juce::uint32 kDeckB = 0xff0090d4;  // Pioneer CDJ blue

DeckComponent::DeckComponent(DeckProcessor& d, const juce::String& deckName)
    : deck(d), name(deckName),
      platter(d, (deckName == "A") ? juce::Colour(kDeckA) : juce::Colour(kDeckB))
{
    accentColour = (deckName == "A") ? juce::Colour(kDeckA) : juce::Colour(kDeckB);

    setupKnob(volumeSlider, volumeLabel, "VOL",  0.0,   1.0,   0.8);
    setupKnob(tempoSlider,  tempoLabel,  "BPM",  60.0,  200.0, 120.0);
    setupKnob(panSlider,    panLabel,    "PAN",  -1.0,  1.0,   0.0);

    volumeSlider.onValueChange = [this] { deck.setVolume((float)volumeSlider.getValue()); };
    tempoSlider.onValueChange  = [this] { deck.setTempo((float)tempoSlider.getValue()); };
    panSlider.onValueChange    = [this] { deck.setPan((float)panSlider.getValue()); };

    // Transport
    setupSmallButton(playButton, "PLAY", 0xff0d2a0d, 0xff22c55e);
    playButton.onClick = [this] { deck.togglePlay(); };

    setupSmallButton(stopButton, "STOP", 0xff2a0d0d, 0xffef4444);
    stopButton.onClick = [this] { deck.stop(); };

    setupSmallButton(cueButton, "CUE", 0xff1a0a1a, 0xffb06af5);
    cueButton.setClickingTogglesState(true);
    cueButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a1a5a));
    cueButton.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xffb06af5));
    cueButton.onClick = [this] { deck.setCueActive(cueButton.getToggleState()); };

    setupSmallButton(syncButton, "SYNC", 0xff0a1a2a, 0xff38bdf8);
    syncButton.onClick = [this] {
        if (getOtherDeckBpm)
        {
            float otherBpm = getOtherDeckBpm();
            if (otherBpm > 0.0f)
            {
                deck.setTempo(otherBpm);
                tempoSlider.setValue(otherBpm, juce::dontSendNotification);
            }
        }
    };

    setupSmallButton(slipButton, "SLIP", 0xff1a1a0a, 0xfffbbf24);
    slipButton.setClickingTogglesState(true);
    slipButton.onClick = [this] { deck.setSlipMode(slipButton.getToggleState()); };

    setupSmallButton(keyLockButton, "KEY", 0xff0a1a0a, 0xff4ade80);
    keyLockButton.setClickingTogglesState(true);
    keyLockButton.onClick = [this] { deck.setKeyLock(keyLockButton.getToggleState()); };

    // Pitch slider: -12 to +12 semitones (only active when key lock on)
    pitchSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 14);
    pitchSlider.setRange(-12.0, 12.0, 0.5);
    pitchSlider.setValue(0.0);
    pitchSlider.setColour(juce::Slider::thumbColourId,    juce::Colour(0xff4ade80));
    pitchSlider.setColour(juce::Slider::trackColourId,    juce::Colour(0xff1e1e30));
    pitchSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff4ade80));
    pitchSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
    pitchSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    pitchSlider.onValueChange = [this] { deck.setPitchSemitones((float)pitchSlider.getValue()); };
    addAndMakeVisible(pitchSlider);

    pitchLabel.setText("PITCH", juce::dontSendNotification);
    pitchLabel.setJustificationType(juce::Justification::centredRight);
    pitchLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    pitchLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80).withAlpha(0.8f));
    addAndMakeVisible(pitchLabel);

    // File loading
    loadButton.setButtonText("LOAD");
    loadButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff111122));
    loadButton.setColour(juce::TextButton::textColourOffId, accentColour);
    loadButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load audio file", juce::File{}, "*.mp3;*.wav;*.aiff;*.flac;*.ogg");
        chooser->launchAsync(juce::FileBrowserComponent::openMode,
            [this, chooser](const juce::FileChooser& fc) {
                auto results = fc.getResults();
                if (!results.isEmpty() && deck.loadFile(results[0]))
                    fileNameLabel.setText(deck.getLoadedFileName(), juce::dontSendNotification);
            });
    };
    addAndMakeVisible(loadButton);

    fileNameLabel.setText("Drop file here or press LOAD", juce::dontSendNotification);
    fileNameLabel.setJustificationType(juce::Justification::centredLeft);
    fileNameLabel.setFont(juce::Font(10.0f));
    fileNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff6a6a8a));
    addAndMakeVisible(fileNameLabel);

    positionLabel.setText("0:00 / 0:00", juce::dontSendNotification);
    positionLabel.setJustificationType(juce::Justification::centredLeft);
    positionLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(positionLabel);

    bpmLabel.setText("-- BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    bpmLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    bpmLabel.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(bpmLabel);

    addAndMakeVisible(platter);

    keyLabel.setText("--", juce::dontSendNotification);
    keyLabel.setJustificationType(juce::Justification::centredLeft);
    keyLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    keyLabel.setColour(juce::Label::textColourId, accentColour.withAlpha(0.8f));
    addAndMakeVisible(keyLabel);

    // Hot cue buttons
    for (int i = 0; i < DeckProcessor::NUM_HOT_CUES; ++i)
    {
        hotCueButtons[i].idx = i;
        hotCueButtons[i].setButtonText(juce::String(i + 1));
        hotCueButtons[i].setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff111122));
        hotCueButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colour(kCueColours[i]));
        hotCueButtons[i].onAction = [this](int idx, bool isRight) {
            if (isRight) deck.clearHotCue(idx);
            else         deck.activateHotCue(idx);
        };
        hotCueButtons[i].onClick = [this, i] { deck.activateHotCue(i); };
        addAndMakeVisible(hotCueButtons[i]);
    }

    // Loop controls
    setupSmallButton(loopInButton,    "IN",   0xff0a1a0a, 0xff22c55e);
    setupSmallButton(loopToggleButton,"LOOP", 0xff0a1a0a, 0xff22c55e);
    setupSmallButton(loopOutButton,   "OUT",  0xff0a1a0a, 0xff22c55e);
    loopToggleButton.setClickingTogglesState(true);

    loopInButton.onClick     = [this] { deck.setLoopIn(); };
    loopOutButton.onClick    = [this] { deck.setLoopOut(); };
    loopToggleButton.onClick = [this] { deck.toggleLoop(); };

    const float autoLoopBars[] = { 0.5f, 1.0f, 2.0f, 4.0f };
    const char* autoLoopText[] = { "1/2", "1",  "2",  "4" };
    for (int i = 0; i < 4; ++i)
    {
        setupSmallButton(autoLoopButtons[i], autoLoopText[i], 0xff0a1a0a, 0xff86efac);
        float bars = autoLoopBars[i];
        autoLoopButtons[i].onClick = [this, bars] { deck.setAutoLoop(bars); };
    }

    // Beat jump buttons
    const int jumpBeats[] = { -4, -1, 1, 4 };
    const char* jumpText[] = { "<<4", "<1", "1>", "4>>" };
    for (int i = 0; i < 4; ++i)
    {
        setupSmallButton(beatJumpButtons[i], jumpText[i], 0xff111122, 0xffcbd5e1);
        int beats = jumpBeats[i];
        beatJumpButtons[i].onClick = [this, beats] { deck.beatJump(beats); };
    }
}

void DeckComponent::setupKnob(juce::Slider& s, juce::Label& l, const juce::String& labelText,
                               double min, double max, double def)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    s.setRange(min, max);
    s.setValue(def);
    s.setColour(juce::Slider::rotarySliderFillColourId,    accentColour);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1e1e30));
    s.setColour(juce::Slider::thumbColourId,               accentColour.brighter(0.3f));
    s.setColour(juce::Slider::textBoxTextColourId,         juce::Colours::white);
    s.setColour(juce::Slider::textBoxBackgroundColourId,   juce::Colour(0x00000000));
    s.setColour(juce::Slider::textBoxOutlineColourId,      juce::Colour(0x00000000));
    addAndMakeVisible(s);

    l.setText(labelText, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(9.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, accentColour.withAlpha(0.8f));
    addAndMakeVisible(l);
}

void DeckComponent::setupSmallButton(juce::TextButton& btn, const juce::String& text,
                                      juce::uint32 bgColour, juce::uint32 textColour)
{
    btn.setButtonText(text);
    btn.setColour(juce::TextButton::buttonColourId,   juce::Colour(bgColour));
    btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(textColour).withAlpha(0.3f));
    btn.setColour(juce::TextButton::textColourOffId,  juce::Colour(textColour));
    btn.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    addAndMakeVisible(btn);
}

void DeckComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(bounds, 8.0f);

    auto header = bounds.removeFromTop(34);
    g.setColour(accentColour.withAlpha(0.12f));
    g.fillRect(header);

    g.setColour(accentColour.withAlpha(0.8f));
    g.fillRect(getLocalBounds().toFloat().removeFromTop(2));

    g.setColour(accentColour.withAlpha(0.25f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);

    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.setColour(accentColour);
    g.drawText("DECK " + name, getLocalBounds().removeFromTop(34), juce::Justification::centred);

    // Playing LED
    float ledX = (float)getWidth() - 22.0f;
    float ledY = 11.0f;
    if (deck.isPlaying())
    {
        g.setColour(juce::Colour(0xff22c55e).withAlpha(0.3f));
        g.fillEllipse(ledX - 3, ledY - 3, 18, 18);
        g.setColour(juce::Colour(0xff22c55e));
        g.fillEllipse(ledX, ledY, 12, 12);
    }
    else
    {
        g.setColour(juce::Colour(0xff1e2a1e));
        g.fillEllipse(ledX, ledY, 12, 12);
    }

    // Slip mode indicator
    if (deck.isSlipMode())
    {
        g.setColour(juce::Colour(0xfffbbf24));
        g.setFont(juce::Font(8.0f, juce::Font::bold));
        g.drawText("SLIP", getLocalBounds().removeFromTop(34).withTrimmedRight(40),
                   juce::Justification::centredRight);
    }
}

void DeckComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(34); // consumed by header in paint()

    // File row
    auto fileRow = area.removeFromTop(22);
    loadButton.setBounds(fileRow.removeFromLeft(48));
    fileRow.removeFromLeft(4);
    fileNameLabel.setBounds(fileRow);

    // Position | Key | BPM row
    auto posRow = area.removeFromTop(20);
    positionLabel.setBounds(posRow.removeFromLeft(posRow.getWidth() / 2));
    keyLabel.setBounds(posRow.removeFromLeft(posRow.getWidth() / 2));
    bpmLabel.setBounds(posRow);

    area.removeFromTop(3);

    // Transport row 1: PLAY | STOP | CUE | SYNC | SLIP | KEY
    auto transport = area.removeFromTop(28);
    int tw = (transport.getWidth() - 15) / 6;
    playButton  .setBounds(transport.removeFromLeft(tw)); transport.removeFromLeft(3);
    stopButton  .setBounds(transport.removeFromLeft(tw)); transport.removeFromLeft(3);
    cueButton   .setBounds(transport.removeFromLeft(tw)); transport.removeFromLeft(3);
    syncButton  .setBounds(transport.removeFromLeft(tw)); transport.removeFromLeft(3);
    slipButton  .setBounds(transport.removeFromLeft(tw)); transport.removeFromLeft(3);
    keyLockButton.setBounds(transport);

    // Pitch row (small, below transport)
    auto pitchRow = area.removeFromTop(18);
    pitchLabel.setBounds(pitchRow.removeFromLeft(40));
    pitchRow.removeFromLeft(4);
    pitchSlider.setBounds(pitchRow);

    area.removeFromTop(3);

    // Hot cues (left half) + Beat jump (right half) in one row
    auto performRow = area.removeFromTop(28);
    int half = (performRow.getWidth() - 4) / 2;

    auto cueArea  = performRow.removeFromLeft(half);
    performRow.removeFromLeft(4);
    auto jumpArea = performRow;

    int cueW  = (cueArea.getWidth()  - 3 * 2) / 4;
    int jumpW = (jumpArea.getWidth() - 3 * 2) / 4;

    for (int i = 0; i < 4; ++i)
    {
        hotCueButtons[i].setBounds(cueArea.removeFromLeft(cueW));
        if (i < 3) cueArea.removeFromLeft(2);

        beatJumpButtons[i].setBounds(jumpArea.removeFromLeft(jumpW));
        if (i < 3) jumpArea.removeFromLeft(2);
    }

    area.removeFromTop(3);

    // Loop row: IN | LOOP | OUT | 1/2 | 1 | 2 | 4
    auto loopRow = area.removeFromTop(28);
    int lw = (loopRow.getWidth() - 6 * 2) / 7;

    loopInButton.setBounds(loopRow.removeFromLeft(lw));      loopRow.removeFromLeft(2);
    loopToggleButton.setBounds(loopRow.removeFromLeft(lw));  loopRow.removeFromLeft(2);
    loopOutButton.setBounds(loopRow.removeFromLeft(lw));     loopRow.removeFromLeft(2);
    for (int i = 0; i < 4; ++i)
    {
        autoLoopButtons[i].setBounds(loopRow.removeFromLeft(lw));
        if (i < 3) loopRow.removeFromLeft(2);
    }

    area.removeFromTop(5);

    // Knob row: VOL / BPM / PAN
    auto knobRow1 = area.removeFromTop(82);
    int kw = knobRow1.getWidth() / 3;
    struct KP { juce::Slider* s; juce::Label* l; };
    KP row1[] = { {&volumeSlider, &volumeLabel}, {&tempoSlider, &tempoLabel}, {&panSlider, &panLabel} };
    for (auto& p : row1)
    {
        auto cell = knobRow1.removeFromLeft(kw);
        p.l->setBounds(cell.removeFromTop(13));
        p.s->setBounds(cell);
    }

    area.removeFromTop(3);

    // Platter fills remaining space
    platter.setBounds(area);
}

bool DeckComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".mp3" || ext == ".wav" || ext == ".aiff" || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void DeckComponent::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.isEmpty()) return;
    if (deck.loadFile(juce::File(files[0])))
        fileNameLabel.setText(deck.getLoadedFileName(), juce::dontSendNotification);
}

void DeckComponent::updatePositionLabel()
{
    auto fmt = [](double s) -> juce::String {
        int m   = (int)(s / 60.0);
        int sec = (int)s % 60;
        return juce::String::formatted("%d:%02d", m, sec);
    };
    positionLabel.setText(fmt(deck.getPosition()) + " / " + fmt(deck.getLengthInSeconds()),
                          juce::dontSendNotification);
}

void DeckComponent::updateHotCueColors()
{
    for (int i = 0; i < DeckProcessor::NUM_HOT_CUES; ++i)
    {
        bool set = deck.isHotCueSet(i);
        juce::Colour col = juce::Colour(kCueColours[i]);
        hotCueButtons[i].setColour(juce::TextButton::buttonColourId,
                                    set ? col.withAlpha(0.3f) : juce::Colour(0xff111122));
        hotCueButtons[i].setColour(juce::TextButton::textColourOffId,
                                    set ? col : col.withAlpha(0.4f));
    }

    // Loop toggle button reflects active state
    bool looping = deck.isLooping();
    loopToggleButton.setColour(juce::TextButton::buttonColourId,
                                looping ? juce::Colour(0xff22c55e).withAlpha(0.3f)
                                        : juce::Colour(0xff0a1a0a));

    // Slip button reflects active state
    slipButton.setColour(juce::TextButton::buttonColourId,
                          deck.isSlipMode() ? juce::Colour(0xfffbbf24).withAlpha(0.25f)
                                            : juce::Colour(0xff1a1a0a));

    // CUE button reflects active headphone monitoring state
    bool cueOn = deck.isCueActive();
    cueButton.setToggleState(cueOn, juce::dontSendNotification);
    cueButton.setColour(juce::TextButton::buttonColourId,
                         cueOn ? juce::Colour(0xff3a1a5a) : juce::Colour(0xff1a0a1a));

    // KEY LOCK button
    bool keyOn = deck.isKeyLockEnabled();
    keyLockButton.setToggleState(keyOn, juce::dontSendNotification);
    keyLockButton.setColour(juce::TextButton::buttonColourId,
                             keyOn ? juce::Colour(0xff0a3a0a) : juce::Colour(0xff0a1a0a));
    pitchSlider.setEnabled(keyOn);
}

void DeckComponent::update()
{
    // Keep file name label in sync (other code paths may load without updating the label)
    if (deck.hasFile())
        fileNameLabel.setText(deck.getLoadedFileName(), juce::dontSendNotification);

    updatePositionLabel();

    // Sync knob positions from deck state — MIDI can change values directly,
    // so we pull them back here rather than letting the UI go stale.
    volumeSlider.setValue(deck.getVolume(),  juce::dontSendNotification);
    tempoSlider .setValue(deck.getTempo(),   juce::dontSendNotification);
    panSlider   .setValue(deck.getPan(),     juce::dontSendNotification);

    platter.update(1.0 / 30.0);

    float detectedBpm = deck.getDetectedBPM();
    bpmLabel.setText(detectedBpm > 0.0f ? (juce::String(detectedBpm, 1) + " BPM")
                                        : (juce::String(deck.getTempo(), 1) + " BPM"),
                     juce::dontSendNotification);

    juce::String key = deck.getDetectedKey();
    keyLabel.setText(key.isEmpty() ? "--" : key, juce::dontSendNotification);

    updateHotCueColors();
    repaint();
}
