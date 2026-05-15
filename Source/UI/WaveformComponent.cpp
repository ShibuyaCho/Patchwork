#include "WaveformComponent.h"
#include "PatchworkLookAndFeel.h"

static const juce::uint32 kCueColours[4] = {
    0xfffbbf24, 0xff3b82f6, 0xff22c55e, 0xffef4444
};

WaveformComponent::WaveformComponent(DeckProcessor& d, juce::Colour accent)
    : deck(d), accentColour(accent)
{
    modeButton.setButtonText("SCR");
    modeButton.setClickingTogglesState(true);
    modeButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff252525));
    modeButton.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.25f));
    modeButton.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xff686868));
    modeButton.setColour(juce::TextButton::textColourOnId,   accent);
    modeButton.onClick = [this] {
        bool cdj = modeButton.getToggleState();
        mode = cdj ? Mode::CDJ : Mode::Scratch;
        modeButton.setButtonText(cdj ? "CDJ" : "SCR");
        deck.disengageScratch();
        deck.releaseNudge();
    };
    addAndMakeVisible(modeButton);
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void WaveformComponent::buildWaveform()
{
    const auto& buf = deck.getFileBuffer();
    if (buf.getNumSamples() == 0) return;

    waveformPath.clear();
    rmsPath.clear();

    const float fw         = (float)getWidth();
    const float fh         = (float)getHeight();
    const float halfH      = fh * 0.5f;
    const int   totalSamps = buf.getNumSamples();
    const int   steps      = (int)fw;
    const int   numCh      = buf.getNumChannels();

    // Peak path (outline) + RMS path (inner fill for "body" look)
    waveformPath.startNewSubPath(0.0f, halfH);
    rmsPath.startNewSubPath(0.0f, halfH);

    for (int xi = 0; xi < steps; ++xi)
    {
        int s0 = (int)((float)xi       / fw * totalSamps);
        int s1 = (int)((float)(xi + 1) / fw * totalSamps);
        s1 = juce::jmin(s1, totalSamps);

        float peak = 0.0f, rmsAcc = 0.0f;
        int   cnt  = s1 - s0;
        for (int s = s0; s < s1; ++s)
        {
            float v = 0.0f;
            for (int ch = 0; ch < numCh; ++ch) v += std::abs(buf.getSample(ch, s));
            v /= numCh;
            peak    = juce::jmax(peak, v);
            rmsAcc += v * v;
        }
        float rms = cnt > 0 ? std::sqrt(rmsAcc / cnt) : 0.0f;

        waveformPath.lineTo((float)xi, halfH - peak * halfH * 0.88f);
        rmsPath.lineTo((float)xi, halfH - rms * halfH * 0.88f);
    }

    // Mirror bottom
    for (int xi = steps - 1; xi >= 0; --xi)
    {
        int s0 = (int)((float)xi       / fw * totalSamps);
        int s1 = (int)((float)(xi + 1) / fw * totalSamps);
        s1 = juce::jmin(s1, totalSamps);

        float peak = 0.0f, rmsAcc = 0.0f;
        int   cnt  = s1 - s0;
        for (int s = s0; s < s1; ++s)
        {
            float v = 0.0f;
            for (int ch = 0; ch < numCh; ++ch) v += std::abs(buf.getSample(ch, s));
            v /= numCh;
            peak    = juce::jmax(peak, v);
            rmsAcc += v * v;
        }
        float rms = cnt > 0 ? std::sqrt(rmsAcc / cnt) : 0.0f;

        waveformPath.lineTo((float)xi, halfH + peak * halfH * 0.88f);
        rmsPath.lineTo((float)xi, halfH + rms * halfH * 0.88f);
    }

    waveformPath.closeSubPath();
    rmsPath.closeSubPath();
    waveformBuilt = true;
}

void WaveformComponent::update()
{
    int v = deck.getLoadVersion();
    if (lastLoadVersion != v) { waveformBuilt = false; lastLoadVersion = v; }
    if (!waveformBuilt && deck.hasFile()) buildWaveform();
    repaint();
}

void WaveformComponent::paint(juce::Graphics& g)
{
    const float fw = (float)getWidth();
    const float fh = (float)getHeight();

    // Background
    g.setColour(PatchworkLookAndFeel::BG);
    g.fillRoundedRectangle(0, 0, fw, fh, 6.0f);

    // Subtle inner border
    g.setColour(PatchworkLookAndFeel::BORDER.withAlpha(0.5f));
    g.drawRoundedRectangle(0.5f, 0.5f, fw - 1.0f, fh - 1.0f, 6.0f, 1.0f);

    if (!deck.hasFile())
    {
        g.setFont(juce::Font(11.0f));
        g.setColour(PatchworkLookAndFeel::TEXT_DIM);
        g.drawText("Drop audio here or press LOAD",
                   getLocalBounds(), juce::Justification::centred);
        return;
    }

    if (!waveformBuilt) buildWaveform();

    const double len = deck.getLengthInSeconds();
    const double pos = deck.getPosition();
    const float  pxPos = len > 0 ? (float)(pos / len) * fw : 0.0f;

    // ── Played region overlay ─────────────────────────────────────────────
    if (pxPos > 0)
    {
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRect(0.0f, 0.0f, pxPos, fh);
    }

    // ── Loop region ───────────────────────────────────────────────────────
    if (deck.isLooping() && len > 0)
    {
        float lx = (float)(deck.getLoopInSeconds()  / len) * fw;
        float rx = (float)(deck.getLoopOutSeconds() / len) * fw;
        g.setColour(accentColour.withAlpha(0.12f));
        g.fillRect(lx, 0.0f, rx - lx, fh);
        g.setColour(accentColour.withAlpha(0.5f));
        g.drawVerticalLine((int)lx, 0.0f, fh);
        g.drawVerticalLine((int)rx, 0.0f, fh);
        // Loop bracket ticks
        g.fillRect(lx, 0.0f, 3.0f, fh * 0.2f);
        g.fillRect(lx, fh * 0.8f, 3.0f, fh * 0.2f);
        g.fillRect(rx - 3.0f, 0.0f, 3.0f, fh * 0.2f);
        g.fillRect(rx - 3.0f, fh * 0.8f, 3.0f, fh * 0.2f);
    }

    // ── Beat grid overlay ─────────────────────────────────────────────────
    if (showBeatGrid)
    {
        float bpm = deck.getDetectedBPM();
        if (bpm <= 0.0f) bpm = deck.getTempo();
        if (bpm > 0.0f && len > 0.0)
        {
            double beatPeriodSec = 60.0 / (double)bpm;
            // Draw from beat 0 (start of file) forward
            int beatIdx = 0;
            for (double beatSec = 0.0; beatSec < len; beatSec += beatPeriodSec, ++beatIdx)
            {
                float bx = (float)(beatSec / len) * fw;
                bool  isBar = (beatIdx % 4 == 0);
                g.setColour(isBar
                    ? accentColour.withAlpha(0.35f)
                    : juce::Colours::white.withAlpha(0.10f));
                g.drawVerticalLine((int)bx, fh * 0.7f, fh);
                if (isBar)
                    g.drawVerticalLine((int)bx, 0.0f, fh * 0.12f);
            }
        }
    }

    // Centre line
    g.setColour(PatchworkLookAndFeel::BORDER.withAlpha(0.6f));
    g.drawHorizontalLine((int)(fh * 0.5f), 4.0f, fw - 4.0f);

    // ── Waveform ──────────────────────────────────────────────────────────
    // RMS body (softer, darker)
    {
        juce::ColourGradient grad(accentColour.withAlpha(0.25f), 0, 0,
                                   accentColour.darker(0.6f).withAlpha(0.08f), 0, fh, false);
        g.setGradientFill(grad);
        g.fillPath(rmsPath);
    }
    // Peak outline (brighter)
    {
        juce::ColourGradient grad(accentColour.withAlpha(0.7f), 0, 0,
                                   accentColour.darker(0.3f).withAlpha(0.4f), 0, fh, false);
        g.setGradientFill(grad);
        g.fillPath(waveformPath);

        g.setColour(accentColour.withAlpha(0.9f));
        g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
    }

    // ── Hot cue markers ───────────────────────────────────────────────────
    if (len > 0)
    {
        for (int i = 0; i < DeckProcessor::NUM_HOT_CUES; ++i)
        {
            if (!deck.isHotCueSet(i)) continue;
            float cx = (float)(deck.getHotCueSamples(i) / (deck.getFileBuffer().getNumSamples()))
                       * fw;
            juce::Colour cc = juce::Colour(kCueColours[i]);

            // Triangle pointing down from top
            const float tw = 8.0f, th = 10.0f;
            juce::Path tri;
            tri.startNewSubPath(cx,          0.0f);
            tri.lineTo(cx + tw * 0.5f, th);
            tri.lineTo(cx - tw * 0.5f, th);
            tri.closeSubPath();
            g.setColour(cc);
            g.fillPath(tri);

            // Thin vertical line
            g.setColour(cc.withAlpha(0.5f));
            g.drawVerticalLine((int)cx, th, fh);

            // Number label
            g.setFont(juce::Font(7.0f).boldened());
            g.setColour(juce::Colours::black);
            g.drawText(juce::String(i + 1),
                       (int)(cx - 4), 1, 8, 9,
                       juce::Justification::centred);
        }
    }

    // ── Playhead ──────────────────────────────────────────────────────────
    if (len > 0 && pxPos >= 0.0f)
    {
        // Glow column
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRect(pxPos - 3.0f, 0.0f, 7.0f, fh);

        // White line
        g.setColour(juce::Colours::white);
        g.drawLine(pxPos, 0.0f, pxPos, fh, 1.5f);

        // Accent ticks
        g.setColour(accentColour);
        g.fillRect(pxPos - 2.0f, 0.0f,   4.0f, 4.0f);
        g.fillRect(pxPos - 2.0f, fh - 4, 4.0f, 4.0f);
    }

    // VU meter (right side, vertical bar)
    {
        float vu = deck.getVULevel();
        float vuH = fh * juce::jlimit(0.0f, 1.0f, vu * 3.0f);
        float vuX = fw - 8.0f;
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(vuX, 0, 6, fh, 3.0f);
        // Level bar — color changes with level
        juce::Colour vuCol = vu < 0.5f ? accentColour : (vu < 0.8f ? juce::Colour(0xffffcc00) : juce::Colour(0xffef4444));
        g.setColour(vuCol.withAlpha(0.85f));
        g.fillRoundedRectangle(vuX, fh - vuH, 6, vuH, 3.0f);
    }

    // ── Scratch/CDJ highlight ─────────────────────────────────────────────
    if (deck.isScratching())
    {
        g.setColour(accentColour.withAlpha(0.07f));
        g.fillRoundedRectangle(0, 0, fw, fh, 6.0f);
    }

    // ── Time overlay (top-left) ───────────────────────────────────────────
    if (deck.hasFile() && len > 0)
    {
        auto fmt = [](double s) -> juce::String {
            bool neg = s < 0;
            s = std::abs(s);
            int m   = (int)(s / 60.0);
            int sec = (int)s % 60;
            int ms  = (int)((s - std::floor(s)) * 10);
            return (neg ? "-" : "") + juce::String::formatted("%d:%02d.%d", m, sec, ms);
        };

        juce::String timeStr  = fmt(pos);
        juce::String remaining = "-" + fmt(len - pos);

        g.setFont(juce::Font(10.0f).boldened());
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawText(timeStr,   4,  2, 80, 14, juce::Justification::centredLeft);
        g.setColour(PatchworkLookAndFeel::TEXT_SEC);
        g.drawText(remaining, 4, 14, 80, 12, juce::Justification::centredLeft);
    }
}

void WaveformComponent::resized()
{
    waveformBuilt = false;
    modeButton.setBounds(getWidth() - 46, 3, 43, 16);
}

void WaveformComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!deck.hasFile()) return;
    lastMouseX   = e.position.x;
    mouseDownX   = e.position.x;
    lastDragTime = juce::Time::getMillisecondCounterHiRes();
    if (mode == Mode::Scratch) deck.engageScratch(0.0f);
}

void WaveformComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!deck.hasFile()) return;
    float deltaX = e.position.x - lastMouseX;
    lastMouseX   = e.position.x;

    if (mode == Mode::Scratch)
    {
        double now = juce::Time::getMillisecondCounterHiRes();
        double dt  = juce::jmax(0.008, (now - lastDragTime) / 1000.0);
        lastDragTime = now;
        float rate = (deltaX / (float)getWidth()) / (float)dt;
        deck.engageScratch(rate);
    }
    else
    {
        float nudge = ((e.position.x - mouseDownX) / (float)getWidth()) * 0.4f;
        deck.applyNudge(nudge);
    }
}

void WaveformComponent::mouseUp(const juce::MouseEvent&)
{
    if (mode == Mode::Scratch) deck.disengageScratch();
    else                       deck.releaseNudge();
}
