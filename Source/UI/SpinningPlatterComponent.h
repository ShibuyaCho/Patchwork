#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/DeckProcessor.h"

class SpinningPlatterComponent : public juce::Component
{
public:
    SpinningPlatterComponent(DeckProcessor& deck, juce::Colour accent);

    void paint(juce::Graphics&) override;
    void update(double deltaTimeSec);  // advance rotation

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    DeckProcessor& deck;
    juce::Colour   accentColour;
    float          angle    = 0.0f;     // radians, current spinner position
    float          lastMouseX = 0.0f;
    double         lastDragTime = 0.0;

    static constexpr float kRPM = 33.33f;
};
