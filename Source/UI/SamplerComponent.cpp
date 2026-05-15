#include "SamplerComponent.h"
#include "PatchworkLookAndFeel.h"

// Colour groups for pads (4 groups of 4, MPC-style)
static constexpr juce::uint32 kPadGroups[4] = {
    0xffff9500,  // amber  — pads 1–4
    0xff0090d4,  // blue   — pads 5–8
    0xff22c55e,  // green  — pads 9–12
    0xffef4444,  // red    — pads 13–16
};

SamplerComponent::SamplerComponent(SamplerEngine& s) : sampler(s)
{
    for (int i = 0; i < 16; ++i)
    {
        auto& pad = pads[i];
        pad.padIndex = i;
        pad.setButtonText(juce::String(i + 1));

        const juce::Colour grp(kPadGroups[i / 4]);
        pad.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff242424));
        pad.setColour(juce::TextButton::buttonOnColourId, grp.withAlpha(0.35f));
        pad.setColour(juce::TextButton::textColourOffId,  grp.withAlpha(0.45f));
        pad.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);

        // Left-click plays
        pad.onClick = [this, i] { sampler.triggerPad(i, 1.0f); };

        // Right-click opens file chooser
        pad.onRightClick = [this](int idx) {
            padFileChooser = std::make_unique<juce::FileChooser>(
                "Load Sample — Pad " + juce::String(idx + 1),
                juce::File{},
                "*.wav;*.mp3;*.aiff;*.flac;*.ogg");

            padFileChooser->launchAsync(
                juce::FileBrowserComponent::openMode |
                juce::FileBrowserComponent::canSelectFiles,
                [this, idx](const juce::FileChooser& fc)
                {
                    const auto results = fc.getResults();
                    if (results.size() > 0)
                        sampler.loadSample(idx, results[0]);
                });
        };

        addAndMakeVisible(pad);
    }
}

void SamplerComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Background panel
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(b, 8.0f);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.0f);

    // Title bar
    auto titleBar = b.removeFromTop(26);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(titleBar, 8.0f);

    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    g.setColour(juce::Colour(0xffe2e2e2));
    g.drawText("SAMPLE PADS  —  left-click: play    right-click: load file    drag & drop",
               titleBar.toNearestInt(), juce::Justification::centred);
}

void SamplerComponent::resized()
{
    auto area = getLocalBounds().reduced(6);
    area.removeFromTop(30);  // title bar

    const int cols = 4, rows = 4;
    const int padW = area.getWidth()  / cols;
    const int padH = area.getHeight() / rows;

    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < cols; ++col)
        {
            int idx = row * cols + col;
            pads[idx].setBounds(area.getX() + col * padW + 3,
                                area.getY() + row * padH + 3,
                                padW - 6, padH - 6);
        }
}

void SamplerComponent::updatePadStates()
{
    for (int i = 0; i < 16; ++i)
    {
        const bool loaded  = sampler.isPadLoaded(i);
        const bool playing = sampler.isPadPlaying(i);
        const juce::Colour grp(kPadGroups[i / 4]);

        if (playing)
        {
            pads[i].setColour(juce::TextButton::buttonColourId, grp.withAlpha(0.85f));
            pads[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        else if (loaded)
        {
            pads[i].setColour(juce::TextButton::buttonColourId, grp.withBrightness(0.12f));
            pads[i].setColour(juce::TextButton::textColourOffId, grp.withAlpha(0.9f));
        }
        else
        {
            pads[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff242424));
            pads[i].setColour(juce::TextButton::textColourOffId, grp.withAlpha(0.35f));
        }

        // Build label: number + name (if loaded)
        juce::String label = juce::String(i + 1);
        if (loaded)
        {
            auto name = sampler.getPadName(i);
            if (name.isNotEmpty())
                label += "\n" + name.substring(0, 8);
        }
        else
        {
            label += "\n\xe2\x8c\x95";  // ⌕ — hint that something can go here
        }
        pads[i].setButtonText(label);
    }
}

bool SamplerComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void SamplerComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    if (files.isEmpty()) return;
    for (int i = 0; i < 16; ++i)
    {
        if (pads[i].getBounds().contains(x, y))
        {
            sampler.loadSample(i, juce::File(files[0]));
            return;
        }
    }
    // If not dropped on a specific pad, load into first empty one
    for (int i = 0; i < 16; ++i)
    {
        if (!sampler.isPadLoaded(i))
        {
            sampler.loadSample(i, juce::File(files[0]));
            return;
        }
    }
}
