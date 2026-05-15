#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/SamplerEngine.h"

class SamplerComponent : public juce::Component,
                         public juce::FileDragAndDropTarget
{
public:
    explicit SamplerComponent(SamplerEngine& sampler);
    void paint(juce::Graphics&) override;
    void resized() override;
    void updatePadStates();

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    SamplerEngine& sampler;

    struct PadButton : public juce::TextButton
    {
        int padIndex = 0;
        std::function<void(int)> onRightClick;

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) { if (onRightClick) onRightClick(padIndex); }
            else                            { juce::TextButton::mouseDown(e); }
        }
        void mouseUp(const juce::MouseEvent& e) override
        {
            if (!e.mods.isRightButtonDown()) juce::TextButton::mouseUp(e);
        }
    };

    std::array<PadButton, 16> pads;
    std::unique_ptr<juce::FileChooser> padFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerComponent)
};
