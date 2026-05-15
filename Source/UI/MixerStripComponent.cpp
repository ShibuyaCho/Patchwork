#include "MixerStripComponent.h"

MixerStripComponent::MixerStripComponent(DeckProcessor& a, DeckProcessor& b,
                                          PatchworkAudioProcessor& p)
    : deckA(a), deckB(b), proc(p)
{
    juce::Colour cA(kDeckA), cB(kDeckB);

    // Gain knobs
    setupGainKnob(gainAKnob, cA);
    gainAKnob.onValueChange = [this] { deckA.setGain((float)gainAKnob.getValue()); };

    setupGainKnob(gainBKnob, cB);
    gainBKnob.onValueChange = [this] { deckB.setGain((float)gainBKnob.getValue()); };

    // EQ knobs
    setupEqKnob(eqAHighKnob, cA);
    eqAHighKnob.onValueChange = [this] { deckA.setEQHigh((float)eqAHighKnob.getValue()); };
    setupEqKnob(eqAMidKnob, cA);
    eqAMidKnob.onValueChange  = [this] { deckA.setEQMid((float)eqAMidKnob.getValue()); };
    setupEqKnob(eqALowKnob, cA);
    eqALowKnob.onValueChange  = [this] { deckA.setEQLow((float)eqALowKnob.getValue()); };

    setupEqKnob(eqBHighKnob, cB);
    eqBHighKnob.onValueChange = [this] { deckB.setEQHigh((float)eqBHighKnob.getValue()); };
    setupEqKnob(eqBMidKnob, cB);
    eqBMidKnob.onValueChange  = [this] { deckB.setEQMid((float)eqBMidKnob.getValue()); };
    setupEqKnob(eqBLowKnob, cB);
    eqBLowKnob.onValueChange  = [this] { deckB.setEQLow((float)eqBLowKnob.getValue()); };

    // Volume faders (vertical)
    setupFader(volAFader, 0.0, 1.0, 0.8);
    volAFader.setColour(juce::Slider::thumbColourId, cA.brighter(0.2f));
    volAFader.onValueChange = [this] { deckA.setVolume((float)volAFader.getValue()); };

    setupFader(volBFader, 0.0, 1.0, 0.8);
    volBFader.setColour(juce::Slider::thumbColourId, cB.brighter(0.2f));
    volBFader.onValueChange = [this] { deckB.setVolume((float)volBFader.getValue()); };

    // Crossfader
    crossfader.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossfader.setRange(0.0, 1.0);
    crossfader.setValue(0.5);
    crossfader.setColour(juce::Slider::thumbColourId,      juce::Colours::white);
    crossfader.setColour(juce::Slider::trackColourId,      juce::Colour(0xff2a2a2a));
    crossfader.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    crossfader.onValueChange = [this] { proc.setCrossfader((float)crossfader.getValue()); };
    addAndMakeVisible(crossfader);

    // Master + CUE faders
    setupFader(masterFader, 0.0, 1.0, 0.8);
    masterFader.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    masterFader.onValueChange = [this] {
        if (auto* param = proc.getValueTreeState().getParameter("master_volume"))
            param->setValueNotifyingHost(param->convertTo0to1((float)masterFader.getValue()));
    };

    setupFader(cueFader, 0.0, 1.0, 0.8);
    cueFader.setColour(juce::Slider::thumbColourId, juce::Colour(0xffb06af5));
    cueFader.onValueChange = [this] { proc.setHeadphoneVolume((float)cueFader.getValue()); };

    // DVS toggle buttons
    dvsABtn.setButtonText("DVS A");
    dvsABtn.setClickingTogglesState(true);
    dvsABtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff222222));
    dvsABtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(kDeckA).withAlpha(0.25f));
    dvsABtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(kDeckA).withAlpha(0.5f));
    dvsABtn.setColour(juce::TextButton::textColourOnId,   juce::Colour(kDeckA));
    addAndMakeVisible(dvsABtn);
    dvsABtn.onClick = [this] { deckA.setDVSEnabled(dvsABtn.getToggleState()); };

    dvsBBtn.setButtonText("DVS B");
    dvsBBtn.setClickingTogglesState(true);
    dvsBBtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff222222));
    dvsBBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(kDeckB).withAlpha(0.25f));
    dvsBBtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(kDeckB).withAlpha(0.5f));
    dvsBBtn.setColour(juce::TextButton::textColourOnId,   juce::Colour(kDeckB));
    addAndMakeVisible(dvsBBtn);
    dvsBBtn.onClick = [this] { deckB.setDVSEnabled(dvsBBtn.getToggleState()); };
}

void MixerStripComponent::setupFader(juce::Slider& s, double mn, double mx, double def)
{
    s.setSliderStyle(juce::Slider::LinearVertical);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(mn, mx);
    s.setValue(def);
    s.setColour(juce::Slider::trackColourId,      juce::Colour(0xff2a2a2a));
    s.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    addAndMakeVisible(s);
}

void MixerStripComponent::setupEqKnob(juce::Slider& s, juce::Colour c)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(-12.0, 12.0);
    s.setValue(0.0);
    s.setColour(juce::Slider::rotarySliderFillColourId, c.withAlpha(0.8f));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    addAndMakeVisible(s);
}

void MixerStripComponent::setupGainKnob(juce::Slider& s, juce::Colour c)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(0.0, 2.0);
    s.setValue(1.0);
    s.setColour(juce::Slider::rotarySliderFillColourId, c.withAlpha(0.7f));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    addAndMakeVisible(s);
}

void MixerStripComponent::paint(juce::Graphics& g)
{
    // Charcoal panel body
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

    auto drawLabel = [&](const juce::String& text, juce::Rectangle<int> area, juce::Colour col) {
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f).withStyle("Bold")));
        g.setColour(col);
        g.drawText(text, area, juce::Justification::centred);
    };

    int w = getWidth();
    const juce::Colour cA(kDeckA), cB(kDeckB), cDim(0xff606060);

    // Column labels (amber=A, blue=B)
    drawLabel("A",      { 0,       4,  w/2,  14 }, cA);
    drawLabel("B",      { w/2,     4,  w/2,  14 }, cB);
    drawLabel("GAIN",   { 0,      20,  w,    10 }, cDim);
    drawLabel("HIGH",   { 0,      72,  w,    10 }, cDim);
    drawLabel("MID",    { 0,     108,  w,    10 }, cDim);
    drawLabel("LOW",    { 0,     144,  w,    10 }, cDim);

    // Fader region divider
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(186, 6.0f, (float)(w - 6));

    drawLabel("VOL",    { 0,     188,  w,    10 }, cDim);
    drawLabel("XFADE",  { 0,     310,  w,    10 }, cDim);
    drawLabel("MASTER", { 0,     350,  w/2,  10 }, cDim);
    drawLabel("CUE",    { w/2,   350,  w/2,  10 }, cDim);
}

void MixerStripComponent::resized()
{
    int w = getWidth();
    int hw = w / 2;

    // Gain row
    gainAKnob.setBounds(4,  28, hw - 4,  36);
    gainBKnob.setBounds(hw, 28, hw - 4,  36);

    // EQ rows
    eqAHighKnob.setBounds(4,  82,       hw - 4, 30);
    eqAMidKnob .setBounds(4,  82 + 36,  hw - 4, 30);
    eqALowKnob .setBounds(4,  82 + 72,  hw - 4, 30);

    eqBHighKnob.setBounds(hw, 82,       hw - 4, 30);
    eqBMidKnob .setBounds(hw, 82 + 36,  hw - 4, 30);
    eqBLowKnob .setBounds(hw, 82 + 72,  hw - 4, 30);

    // Volume faders
    volAFader.setBounds(4,  200, hw - 4, 100);
    volBFader.setBounds(hw, 200, hw - 4, 100);

    // Crossfader
    crossfader.setBounds(6, 322, w - 12, 22);

    // Master + CUE faders
    masterFader.setBounds(4,  362, hw - 4, 100);
    cueFader   .setBounds(hw, 362, hw - 4, 100);

    // DVS buttons
    dvsABtn.setBounds(4,  470, hw - 4, 22);
    dvsBBtn.setBounds(hw, 470, hw - 4, 22);
}

void MixerStripComponent::update()
{
    // Sync sliders from processor state (MIDI may have changed values)
    gainAKnob  .setValue(deckA.getGain(),   juce::dontSendNotification);
    eqAHighKnob.setValue(deckA.getEQHigh(), juce::dontSendNotification);
    eqAMidKnob .setValue(deckA.getEQMid(),  juce::dontSendNotification);
    eqALowKnob .setValue(deckA.getEQLow(),  juce::dontSendNotification);
    volAFader  .setValue(deckA.getVolume(), juce::dontSendNotification);

    gainBKnob  .setValue(deckB.getGain(),   juce::dontSendNotification);
    eqBHighKnob.setValue(deckB.getEQHigh(), juce::dontSendNotification);
    eqBMidKnob .setValue(deckB.getEQMid(),  juce::dontSendNotification);
    eqBLowKnob .setValue(deckB.getEQLow(),  juce::dontSendNotification);
    volBFader  .setValue(deckB.getVolume(), juce::dontSendNotification);

    crossfader.setValue(proc.getCrossfader(), juce::dontSendNotification);

    dvsABtn.setToggleState(deckA.isDVSEnabled(), juce::dontSendNotification);
    dvsBBtn.setToggleState(deckB.isDVSEnabled(), juce::dontSendNotification);
}
