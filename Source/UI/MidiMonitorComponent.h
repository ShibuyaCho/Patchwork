#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/MidiController.h"
#include "../Modules/NIDeviceActivator.h"
#include "NILEDEditorComponent.h"

class MidiMonitorComponent : public juce::Component,
                              public juce::Timer
{
public:
    explicit MidiMonitorComponent(MidiController& mc);
    ~MidiMonitorComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiController& controller;

    // ── Connected / blocked / events ─────────────────────────────────────
    juce::Label      devicesTitle, blockedTitle, eventTitle, learnTitle;
    juce::TextEditor devicesList, blockedList, eventLog;

    // ── All available ports panel ─────────────────────────────────────────
    juce::Label      availTitle;
    juce::TextEditor availList;
    juce::ComboBox   manualOpenBox;
    juce::TextButton manualOpenBtn;

    // ── NI device panel ───────────────────────────────────────────────────
    juce::Label      niTitle;
    juce::TextEditor niStatusBox;
    juce::TextButton niActivateBtn, niUdevBtn, niMonitorBtn;

    // ── Diagnostics ───────────────────────────────────────────────────────
    juce::Label      diagTitle;
    juce::TextEditor diagBox;

    // ── MIDI Learn ────────────────────────────────────────────────────────
    juce::ComboBox   learnTargetBox;
    juce::TextButton learnStartBtn, learnCancelBtn;
    juce::Label      learnStatusLabel;

    // ── Save / load ───────────────────────────────────────────────────────
    juce::TextButton saveBtn, loadBtn;

    bool learningActive = false;

    // cached device list for manualOpenBox
    juce::Array<juce::MidiDeviceInfo> cachedAvailDevices;

    NIDeviceActivator niActivator;
    NILEDEditorComponent ledEditor { niActivator };

    void buildTargetCombo();
    void startLearn();
    void cancelLearn();
    void refreshDevices();
    void refreshEvents();
    void refreshAvailable();
    void refreshNIPanel();

    static juce::File mappingsFile();
};
