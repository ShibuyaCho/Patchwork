#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/DeckProcessor.h"
#include "../PluginProcessor.h"

class MixerStripComponent : public juce::Component
{
public:
    MixerStripComponent(DeckProcessor& deckA, DeckProcessor& deckB,
                        PatchworkAudioProcessor& proc);

    void paint(juce::Graphics&) override;
    void resized() override;
    void update();   // call at 30Hz to sync from processor state

private:
    DeckProcessor&           deckA;
    DeckProcessor&           deckB;
    PatchworkAudioProcessor& proc;

    // Channel A
    juce::Slider gainAKnob;
    juce::Slider eqAHighKnob, eqAMidKnob, eqALowKnob;
    juce::Slider volAFader;

    // Channel B
    juce::Slider gainBKnob;
    juce::Slider eqBHighKnob, eqBMidKnob, eqBLowKnob;
    juce::Slider volBFader;

    // Master
    juce::Slider crossfader;
    juce::Slider masterFader, cueFader;

    // DVS enable toggles
    juce::TextButton dvsABtn, dvsBBtn;

    void setupFader   (juce::Slider& s, double min, double max, double def);
    void setupEqKnob  (juce::Slider& s, juce::Colour accent);
    void setupGainKnob(juce::Slider& s, juce::Colour accent);

    // Deck A = amber, Deck B = pioneer blue
    static constexpr juce::uint32 kDeckA = 0xffff9500;
    static constexpr juce::uint32 kDeckB = 0xff0090d4;
};
