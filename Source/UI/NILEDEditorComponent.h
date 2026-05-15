#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/NIDeviceActivator.h"

// Scrollable list of all 80 LED slots in report 0x80.
// Shows actual button/pad/strip names. Probe to identify, right-click to set color.
class NILEDEditorComponent : public juce::Component,
                              public juce::ListBoxModel,
                              public juce::Timer
{
public:
    explicit NILEDEditorComponent(NIDeviceActivator& activator);
    ~NILEDEditorComponent() override;

    void resized() override;
    void timerCallback() override;

    int  getNumRows() override { return kLEDCount; }
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

private:
    NIDeviceActivator& activator;

    uint8_t ledData[kLEDCount] = {};   // local mirror of LED state
    int     cycleIdx[kLEDCount] = {};  // cycle position per slot

    int scanSlot = -1;

    juce::ListBox    box;
    juce::TextEditor nameEd;
    int              editRow = -1;
    juce::String     slotLabels[kLEDCount];

    juce::TextButton scanAllBtn { "SCAN ALL" };
    juce::TextButton saveBtn    { "SAVE" };
    juce::TextButton loadBtn    { "LOAD" };

    // Returns true if row is a pad LED slot
    static bool isPadSlot(int row) { return row >= kLEDPadBase && row < kLEDStripBase; }

    juce::Colour slotDisplayColor(int row) const;
    void cycleSlot(int row);
    void probeSlot(int row);
    void beginEditLabel(int row);
    void commitEditLabel();
    juce::String slotName(int row) const;

    static juce::File dataFile();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NILEDEditorComponent)
};
