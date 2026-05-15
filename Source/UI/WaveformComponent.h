#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modules/DeckProcessor.h"

class WaveformComponent : public juce::Component
{
public:
    enum class Mode { Scratch, CDJ };

    explicit WaveformComponent(DeckProcessor& deck, juce::Colour accent);
    void paint(juce::Graphics&) override;
    void resized() override;
    void update();

    void setShowBeatGrid(bool show) { showBeatGrid = show; }
    bool isShowingBeatGrid() const  { return showBeatGrid; }

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    DeckProcessor& deck;
    juce::Colour accentColour;

    juce::Path waveformPath;
    juce::Path rmsPath;
    bool waveformBuilt = false;
    int lastLoadVersion = -1;
    void buildWaveform();

    bool showBeatGrid = false;

    // Scratch / CDJ
    Mode mode = Mode::Scratch;
    juce::TextButton modeButton;
    float lastMouseX  = 0.0f;
    float mouseDownX  = 0.0f;
    double lastDragTime = 0.0;
};
