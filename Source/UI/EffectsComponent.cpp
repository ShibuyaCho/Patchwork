#include "EffectsComponent.h"

// ─── Module ──────────────────────────────────────────────────────────────────

void EffectsComponent::Module::init(const juce::String& name, juce::Colour col, bool withType)
{
    title      = name;
    accent     = col;
    hasTypeBtn = withType;

    onButton.setButtonText("ON");
    onButton.setClickingTogglesState(true);
    onButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff0e0e1a));
    onButton.setColour(juce::TextButton::buttonOnColourId, col.withAlpha(0.35f));
    onButton.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xff4a4a6a));
    onButton.setColour(juce::TextButton::textColourOnId,   col);
    addAndMakeVisible(onButton);

    if (hasTypeBtn)
    {
        typeButton.setButtonText("LP");
        typeButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff111128));
        typeButton.setColour(juce::TextButton::textColourOffId, col);
        addAndMakeVisible(typeButton);
    }
}

void EffectsComponent::Module::setupKnob(juce::Slider& s, juce::Label& l,
                                          const juce::String& text,
                                          double min, double max, double def,
                                          bool logScale)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(min, max);
    s.setValue(def);
    if (logScale) s.setSkewFactorFromMidPoint(std::sqrt(min * max));
    s.setColour(juce::Slider::rotarySliderFillColourId,    accent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1e1e30));
    s.setColour(juce::Slider::thumbColourId,               accent.brighter(0.4f));
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(8.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, accent.withAlpha(0.75f));
    addAndMakeVisible(l);
}

void EffectsComponent::Module::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Module background
    g.setColour(juce::Colour(0xff0c0c16));
    g.fillRoundedRectangle(b, 5.0f);

    // Accent top bar
    g.setColour(accent);
    g.fillRect(b.removeFromTop(3));

    // Thin border
    g.setColour(accent.withAlpha(0.2f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.0f);

    // Title
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.setColour(accent.withAlpha(onButton.getToggleState() ? 1.0f : 0.45f));
    auto titleArea = getLocalBounds().removeFromTop(16);
    if (hasTypeBtn)
        titleArea.removeFromRight(32);
    g.drawText(title, titleArea, juce::Justification::centred);
}

void EffectsComponent::Module::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(3); // accent bar

    // Title row (with optional type button on right)
    auto titleRow = area.removeFromTop(16);
    if (hasTypeBtn)
        typeButton.setBounds(titleRow.removeFromRight(32));

    area.removeFromTop(2);

    // ON button
    onButton.setBounds(area.removeFromTop(20));

    area.removeFromTop(4);

    // Two knobs side by side, labels above
    int half = area.getWidth() / 2;
    auto left  = area.removeFromLeft(half);
    auto right = area;

    label1.setBounds(left.removeFromTop(12));
    knob1.setBounds(left.reduced(2, 0));

    label2.setBounds(right.removeFromTop(12));
    knob2.setBounds(right.reduced(2, 0));
}

// ─── EffectsComponent ────────────────────────────────────────────────────────

EffectsComponent::EffectsComponent(EffectsChain& effects) : fx(effects)
{
    // Accent colours per effect
    filterMod .init("FILTER",   juce::Colour(0xff38bdf8), true); // sky blue + type btn
    compMod   .init("COMP",     juce::Colour(0xff34d399));        // emerald
    distMod   .init("DIST",     juce::Colour(0xffef4444));        // red
    crushMod  .init("CRUSH",    juce::Colour(0xffff7c2a));        // orange
    flangerMod.init("FLANGER",  juce::Colour(0xfffbbf24));        // yellow
    chorusMod .init("CHORUS",   juce::Colour(0xff818cf8));        // indigo
    phaserMod .init("PHASER",   juce::Colour(0xffa78bfa));        // violet
    tremoloMod.init("TREMOLO",  juce::Colour(0xfff472b6));        // pink
    delayMod  .init("DELAY",    juce::Colour(0xff00c8f0));        // cyan
    reverbMod .init("REVERB",   juce::Colour(0xffa855f7));        // purple

    // ── Filter knobs ──────────────────────────────────────────────────
    filterMod.setupKnob(filterMod.knob1, filterMod.label1, "FREQ", 20.0, 20000.0, 1000.0, true);
    filterMod.setupKnob(filterMod.knob2, filterMod.label2, "Q",    0.1,  10.0,    0.7);
    filterMod.onButton.onClick   = [this] { fx.setFilterEnabled(filterMod.onButton.getToggleState()); };
    filterMod.knob1.onValueChange = [this] { fx.setFilterFrequency((float)filterMod.knob1.getValue()); };
    filterMod.knob2.onValueChange = [this] { fx.setFilterQ((float)filterMod.knob2.getValue()); };
    filterMod.typeButton.onClick  = [this] { cycleFilterType(); };

    // ── Compressor knobs ──────────────────────────────────────────────
    compMod.setupKnob(compMod.knob1, compMod.label1, "THRESH", -60.0, 0.0,  -10.0);
    compMod.setupKnob(compMod.knob2, compMod.label2, "RATIO",  1.0,   20.0,  4.0);
    compMod.onButton.onClick    = [this] { fx.setCompEnabled(compMod.onButton.getToggleState()); };
    compMod.knob1.onValueChange = [this] { fx.setCompThreshold((float)compMod.knob1.getValue()); };
    compMod.knob2.onValueChange = [this] { fx.setCompRatio((float)compMod.knob2.getValue()); };

    // ── Distortion knobs ──────────────────────────────────────────────
    distMod.setupKnob(distMod.knob1, distMod.label1, "DRIVE", 1.0, 12.0, 3.0);
    distMod.setupKnob(distMod.knob2, distMod.label2, "MIX",   0.0, 1.0,  0.5);
    distMod.onButton.onClick    = [this] { fx.setDistEnabled(distMod.onButton.getToggleState()); };
    distMod.knob1.onValueChange = [this] { fx.setDistDrive((float)distMod.knob1.getValue()); };
    distMod.knob2.onValueChange = [this] { fx.setDistMix((float)distMod.knob2.getValue()); };

    // ── Bitcrusher knobs ──────────────────────────────────────────────
    crushMod.setupKnob(crushMod.knob1, crushMod.label1, "BITS", 1.0, 16.0, 8.0);
    crushMod.setupKnob(crushMod.knob2, crushMod.label2, "RATE", 0.0, 1.0,  0.5);
    crushMod.onButton.onClick    = [this] { fx.setBitcrushEnabled(crushMod.onButton.getToggleState()); };
    crushMod.knob1.onValueChange = [this] { fx.setBitDepth((float)crushMod.knob1.getValue()); };
    crushMod.knob2.onValueChange = [this] { fx.setBitRate((float)crushMod.knob2.getValue()); };

    // ── Flanger knobs ─────────────────────────────────────────────────
    flangerMod.setupKnob(flangerMod.knob1, flangerMod.label1, "RATE",  0.05, 5.0,  0.3);
    flangerMod.setupKnob(flangerMod.knob2, flangerMod.label2, "DEPTH", 0.5,  10.0, 3.0);
    flangerMod.onButton.onClick    = [this] { fx.setFlangerEnabled(flangerMod.onButton.getToggleState()); };
    flangerMod.knob1.onValueChange = [this] { fx.setFlangerRate((float)flangerMod.knob1.getValue()); };
    flangerMod.knob2.onValueChange = [this] { fx.setFlangerDepth((float)flangerMod.knob2.getValue()); };

    // ── Chorus knobs ──────────────────────────────────────────────────
    chorusMod.setupKnob(chorusMod.knob1, chorusMod.label1, "RATE",  0.1, 5.0,  0.5);
    chorusMod.setupKnob(chorusMod.knob2, chorusMod.label2, "DEPTH", 5.0, 40.0, 20.0);
    chorusMod.onButton.onClick    = [this] { fx.setChorusEnabled(chorusMod.onButton.getToggleState()); };
    chorusMod.knob1.onValueChange = [this] { fx.setChorusRate((float)chorusMod.knob1.getValue()); };
    chorusMod.knob2.onValueChange = [this] { fx.setChorusDepth((float)chorusMod.knob2.getValue()); };

    // ── Phaser knobs ──────────────────────────────────────────────────
    phaserMod.setupKnob(phaserMod.knob1, phaserMod.label1, "RATE",  0.1, 5.0, 0.5);
    phaserMod.setupKnob(phaserMod.knob2, phaserMod.label2, "DEPTH", 0.0, 1.0, 0.8);
    phaserMod.onButton.onClick    = [this] { fx.setPhaserEnabled(phaserMod.onButton.getToggleState()); };
    phaserMod.knob1.onValueChange = [this] { fx.setPhaserRate((float)phaserMod.knob1.getValue()); };
    phaserMod.knob2.onValueChange = [this] { fx.setPhaserDepth((float)phaserMod.knob2.getValue()); };

    // ── Tremolo knobs ─────────────────────────────────────────────────
    tremoloMod.setupKnob(tremoloMod.knob1, tremoloMod.label1, "RATE",  0.1, 20.0, 4.0);
    tremoloMod.setupKnob(tremoloMod.knob2, tremoloMod.label2, "DEPTH", 0.0, 1.0,  0.8);
    tremoloMod.onButton.onClick    = [this] { fx.setTremoloEnabled(tremoloMod.onButton.getToggleState()); };
    tremoloMod.knob1.onValueChange = [this] { fx.setTremoloRate((float)tremoloMod.knob1.getValue()); };
    tremoloMod.knob2.onValueChange = [this] { fx.setTremoloDepth((float)tremoloMod.knob2.getValue()); };

    // ── Delay knobs ───────────────────────────────────────────────────
    delayMod.setupKnob(delayMod.knob1, delayMod.label1, "TIME",  0.01, 2.0,  0.25);
    delayMod.setupKnob(delayMod.knob2, delayMod.label2, "MIX",   0.0,  1.0,  0.0);
    delayMod.onButton.onClick    = [this] { fx.setDelayEnabled(delayMod.onButton.getToggleState()); };
    delayMod.knob1.onValueChange = [this] { fx.setDelayTime((float)delayMod.knob1.getValue()); };
    delayMod.knob2.onValueChange = [this] {
        fx.setDelayMix((float)delayMod.knob2.getValue());
        fx.setDelayFeedback(0.4f); // keep sane default
    };

    // ── Reverb knobs ──────────────────────────────────────────────────
    reverbMod.setupKnob(reverbMod.knob1, reverbMod.label1, "ROOM", 0.0, 1.0, 0.5);
    reverbMod.setupKnob(reverbMod.knob2, reverbMod.label2, "WET",  0.0, 1.0, 0.0);
    reverbMod.onButton.onClick    = [this] { fx.setReverbEnabled(reverbMod.onButton.getToggleState()); };
    reverbMod.knob1.onValueChange = [this] { fx.setReverbRoom((float)reverbMod.knob1.getValue()); };
    reverbMod.knob2.onValueChange = [this] { fx.setReverbWet((float)reverbMod.knob2.getValue()); };

    // Add all modules to this component
    for (auto* m : { &filterMod, &compMod, &distMod, &crushMod, &flangerMod,
                     &chorusMod, &phaserMod, &tremoloMod, &delayMod, &reverbMod })
        addAndMakeVisible(m);
}

void EffectsComponent::cycleFilterType()
{
    using FT = EffectsChain::FilterType;
    switch (filterTypeState)
    {
        case FT::LowPass:  filterTypeState = FT::HighPass; filterMod.typeButton.setButtonText("HP"); break;
        case FT::HighPass: filterTypeState = FT::BandPass; filterMod.typeButton.setButtonText("BP"); break;
        case FT::BandPass: filterTypeState = FT::LowPass;  filterMod.typeButton.setButtonText("LP"); break;
    }
    fx.setFilterType(filterTypeState);
}

void EffectsComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff09090f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    // Section label
    g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff3a3a5a));
    g.drawText("EFFECTS RACK", getLocalBounds().removeFromTop(14),
               juce::Justification::centred);
}

void EffectsComponent::resized()
{
    auto area = getLocalBounds().reduced(3);
    area.removeFromTop(14); // label

    Module* modules[] = { &filterMod, &compMod, &distMod, &crushMod, &flangerMod,
                          &chorusMod, &phaserMod, &tremoloMod, &delayMod, &reverbMod };
    constexpr int N = 10;
    int gap = 3;
    int modW = (area.getWidth() - gap * (N - 1)) / N;

    for (int i = 0; i < N; ++i)
    {
        modules[i]->setBounds(area.removeFromLeft(modW));
        if (i < N - 1) area.removeFromLeft(gap);
    }
}
