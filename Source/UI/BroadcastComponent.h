#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/BroadcastEngine.h"
#include "../Modules/MixRecorder.h"

class BroadcastComponent : public juce::Component, private juce::Timer
{
public:
    explicit BroadcastComponent(BroadcastEngine& engine, MixRecorder& recorder);
    ~BroadcastComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void startOrStopStream();
    void startOrStopRecord();

    BroadcastEngine& engine;
    MixRecorder&     recorder;

    // Broadcast controls
    juce::Label       hostLabel, portLabel, mountLabel, passLabel, bitrateLabel;
    juce::TextEditor  hostEdit, portEdit, mountEdit, passEdit, bitrateEdit;
    juce::TextButton  streamBtn;
    juce::Label       statusLabel;

    // Recording controls
    juce::TextButton  recordBtn;
    juce::Label       recordLabel;

    juce::Label sectionStream, sectionRecord;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BroadcastComponent)
};
