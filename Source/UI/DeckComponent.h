#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/DeckProcessor.h"
#include "SpinningPlatterComponent.h"

class DeckComponent : public juce::Component,
                      public juce::FileDragAndDropTarget
{
public:
    explicit DeckComponent(DeckProcessor& deck, const juce::String& deckName);

    void paint(juce::Graphics&) override;
    void resized() override;
    void update();

    // Set by the editor so SYNC knows the other deck's current BPM
    void setOtherDeckBpmGetter(std::function<float()> fn) { getOtherDeckBpm = std::move(fn); }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    DeckProcessor& deck;
    juce::String name;
    juce::Colour accentColour;

    // Sliders / knobs
    juce::Slider volumeSlider, tempoSlider, panSlider;
    juce::Label  volumeLabel, tempoLabel, panLabel;

    // Spinning platter
    SpinningPlatterComponent platter;

    // Transport
    juce::TextButton playButton, stopButton;
    juce::TextButton cueButton;             // headphone monitor select
    juce::TextButton syncButton, slipButton;
    juce::TextButton keyLockButton;
    juce::Slider     pitchSlider;
    juce::Label      pitchLabel;

    // File
    juce::TextButton loadButton;
    juce::Label  fileNameLabel;
    juce::Label  positionLabel;
    juce::Label  bpmLabel;  // shows detected BPM
    juce::Label  keyLabel;  // shows detected key

    // Hot cues — right-click to clear
    struct HotCueButton : public juce::TextButton
    {
        int idx = 0;
        std::function<void(int, bool)> onAction; // (idx, isRightClick)
        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) { if (onAction) onAction(idx, true); }
            else                            { juce::TextButton::mouseDown(e); }
        }
        void mouseUp(const juce::MouseEvent& e) override
        {
            if (!e.mods.isRightButtonDown()) juce::TextButton::mouseUp(e);
        }
    };
    std::array<HotCueButton, DeckProcessor::NUM_HOT_CUES> hotCueButtons;

    // Loop controls
    juce::TextButton loopInButton, loopToggleButton, loopOutButton;
    juce::TextButton autoLoopButtons[4]; // ½ 1 2 4 bars

    // Beat jump
    juce::TextButton beatJumpButtons[4]; // -4 -1 +1 +4

    std::function<float()> getOtherDeckBpm;

    static constexpr juce::uint32 kCueColours[4] = {
        0xfffbbf24, // yellow
        0xff3b82f6, // blue
        0xff22c55e, // green
        0xffef4444  // red
    };

    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& label,
                   double min, double max, double def);
    void setupSmallButton(juce::TextButton& btn, const juce::String& text,
                          juce::uint32 bgColour, juce::uint32 textColour);
    void updatePositionLabel();
    void updateHotCueColors();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckComponent)
};
