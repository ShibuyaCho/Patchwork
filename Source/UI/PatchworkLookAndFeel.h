#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class PatchworkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PatchworkLookAndFeel();

    // Colours exposed so components can use the shared palette
    static const juce::Colour BG, PANEL, CARD, BORDER, TEXT_DIM, TEXT_SEC, TEXT_PRI;

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& bgColour,
                              bool isMouseOver, bool isButtonDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool isMouseOver, bool isButtonDown) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    int getSliderThumbRadius(juce::Slider&) override { return 8; }
};
