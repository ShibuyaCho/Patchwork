#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/DeckComponent.h"
#include "UI/EffectsComponent.h"
#include "UI/SamplerComponent.h"
#include "UI/WaveformComponent.h"
#include "UI/PatchworkLookAndFeel.h"
#include "UI/MidiMonitorComponent.h"
#include "UI/LibraryComponent.h"
#include "UI/BroadcastComponent.h"
#include "UI/MixerStripComponent.h"

class PatchworkAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    explicit PatchworkAudioProcessorEditor(PatchworkAudioProcessor&);
    ~PatchworkAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    PatchworkAudioProcessor& audioProcessor;

    // ── Floating window helper (must be declared before unique_ptr members) ─
    struct FloatingWindow : public juce::DocumentWindow
    {
        FloatingWindow(const juce::String& name, juce::Colour bg)
            : juce::DocumentWindow(name, bg, juce::DocumentWindow::closeButton, true)
        {
            setUsingNativeTitleBar(true);
        }
        void closeButtonPressed() override { setVisible(false); }
    };

    // ── Main-window components ───────────────────────────────────────────
    DeckComponent      deckAComponent, deckBComponent;
    WaveformComponent  waveformAComponent, waveformBComponent;
    juce::Slider       masterVolumeSlider;
    juce::Label        masterVolumeLabel;
    juce::TextButton   recButton;
    juce::Label        recModeLabel;

    std::unique_ptr<MixerStripComponent> mixerStrip;

    // ── Header buttons ───────────────────────────────────────────────────
    juce::TextButton fxWindowBtn, samplerWindowBtn, midiWindowBtn, libraryWindowBtn;
    juce::Label      devicesLabel;

    juce::TextButton broadcastWindowBtn;

    // ── Master clock ─────────────────────────────────────────────────────
    juce::Label      masterBpmLabel;
    juce::TextButton masterBpmDownBtn, masterBpmUpBtn;
    juce::TextButton tapTempoBtn;
    juce::TextButton syncAllBtn;
    juce::TextButton beatGridBtn;
    juce::TextButton quantizeBtn;

    // ── Floating sub-windows (components owned here, windows borrow them) ─
    std::unique_ptr<EffectsComponent>     effectsComponent;
    std::unique_ptr<SamplerComponent>     samplerComponent;
    std::unique_ptr<MidiMonitorComponent> midiMonitorComponent;
    std::unique_ptr<LibraryComponent>     libraryComponent;
    std::unique_ptr<BroadcastComponent>   broadcastComponent;

    std::unique_ptr<FloatingWindow> fxWindow;
    std::unique_ptr<FloatingWindow> samplerWindow;
    std::unique_ptr<FloatingWindow> midiWindow;
    std::unique_ptr<FloatingWindow> libraryWindow;
    std::unique_ptr<FloatingWindow> broadcastWindow;

    // ── Look and feel ────────────────────────────────────────────────────
    PatchworkLookAndFeel lookAndFeel;

    // ── Helpers ──────────────────────────────────────────────────────────
    void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name,
                     double min, double max, double def);
    void drawBackground(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchworkAudioProcessorEditor)
};
