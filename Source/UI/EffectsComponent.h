#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/EffectsChain.h"

class EffectsComponent : public juce::Component
{
public:
    explicit EffectsComponent(EffectsChain& fx);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    EffectsChain& fx;

    // Each effect is a self-contained module component
    struct Module : public juce::Component
    {
        juce::String  title;
        juce::Colour  accent;
        juce::TextButton onButton;       // bypass toggle
        juce::TextButton typeButton;     // optional (filter type cycle)
        bool          hasTypeBtn = false;
        juce::Slider  knob1, knob2;
        juce::Label   label1, label2;

        void init(const juce::String& name, juce::Colour col, bool withType = false);
        void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text,
                       double min, double max, double def, bool logScale = false);
        void paint(juce::Graphics& g) override;
        void resized() override;
    };

    Module filterMod, compMod, distMod, crushMod;
    Module flangerMod, chorusMod, phaserMod, tremoloMod;
    Module delayMod, reverbMod;

    // Filter type cycle state
    EffectsChain::FilterType filterTypeState = EffectsChain::FilterType::LowPass;
    void cycleFilterType();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsComponent)
};
