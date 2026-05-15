#include "SpinningPlatterComponent.h"
#include <cmath>

SpinningPlatterComponent::SpinningPlatterComponent(DeckProcessor& d, juce::Colour accent)
    : deck(d), accentColour(accent)
{
    setMouseCursor(juce::MouseCursor::UpDownLeftRightResizeCursor);
}

void SpinningPlatterComponent::update(double deltaTimeSec)
{
    bool active = deck.isPlaying() || deck.isScratching() || deck.isDVSEnabled();
    if (active)
    {
        float rpm    = kRPM;
        float speed  = deck.isDVSEnabled() ? deck.getVULevel() * 2.0f : 1.0f;
        float dAngle = (float)(2.0 * juce::MathConstants<double>::pi
                       * rpm / 60.0 * deltaTimeSec * speed);
        if (!deck.isPlaying() && deck.isDVSEnabled()) dAngle = 0.0f;
        angle = std::fmod(angle + dAngle, juce::MathConstants<float>::twoPi);
    }
    repaint();
}

void SpinningPlatterComponent::paint(juce::Graphics& g)
{
    auto b  = getLocalBounds().toFloat().reduced(4.0f);
    float cx = b.getCentreX();
    float cy = b.getCentreY();
    float r  = std::min(b.getWidth(), b.getHeight()) * 0.5f;

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - r + 2.0f, cy - r + 5.0f, r * 2.0f - 4.0f, r * 2.0f - 4.0f);

    // Disc
    {
        juce::ColourGradient disc(juce::Colour(0xff1e1e22), cx, cy - r,
                                   juce::Colour(0xff0b0b0e), cx, cy + r, false);
        g.setGradientFill(disc);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // Groove rings
    int numGrooves = 32;
    for (int i = 0; i < numGrooves; ++i)
    {
        float gr = r * (0.30f + (float)i / numGrooves * 0.62f);
        float alpha = (i % 2 == 0) ? 0.18f : 0.10f;
        g.setColour(juce::Colour(0xff606070).withAlpha(alpha));
        g.drawEllipse(cx - gr, cy - gr, gr * 2.0f, gr * 2.0f, 0.7f);
    }

    // Spinning indicator line
    if (deck.isPlaying() || deck.isDVSEnabled())
    {
        float dotR = r * 0.72f;
        float sin_a = std::sin(angle), cos_a = std::cos(angle);
        float dx = cx + sin_a * dotR;
        float dy = cy - cos_a * dotR;

        // Glow
        g.setColour(accentColour.withAlpha(0.25f));
        g.fillEllipse(dx - 7.0f, dy - 7.0f, 14.0f, 14.0f);
        // Dot
        g.setColour(accentColour.brighter(0.2f));
        g.fillEllipse(dx - 4.0f, dy - 4.0f, 8.0f, 8.0f);

        // Radius line
        g.setColour(accentColour.withAlpha(0.18f));
        g.drawLine(cx, cy, dx, dy, 1.0f);
    }

    // Tone arm pickup dot
    {
        float ar = r * 0.55f;
        float ax = cx + ar;
        float ay = cy;
        g.setColour(juce::Colour(0xffaaaacc).withAlpha(0.5f));
        g.fillEllipse(ax - 2.5f, ay - 2.5f, 5.0f, 5.0f);
    }

    // Center label
    {
        float lr = r * 0.28f;
        juce::ColourGradient label(accentColour.withAlpha(0.2f), cx, cy - lr,
                                    accentColour.darker(0.5f).withAlpha(0.05f), cx, cy + lr, false);
        g.setGradientFill(label);
        g.fillEllipse(cx - lr, cy - lr, lr * 2.0f, lr * 2.0f);
        g.setColour(accentColour.withAlpha(0.5f));
        g.drawEllipse(cx - lr, cy - lr, lr * 2.0f, lr * 2.0f, 1.2f);

        juce::String name = deck.getLoadedFileName();
        if (name.isEmpty()) name = "NO TRACK";
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.5f).withStyle("Bold")));
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawText(name.substring(0, 18),
                   (int)(cx - lr + 2), (int)(cy - 7), (int)(lr * 2 - 4), 14,
                   juce::Justification::centred, true);
    }

    // DVS indicator
    if (deck.isDVSEnabled())
    {
        g.setColour(juce::Colour(0xffffcc00));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
        g.drawText("DVS", (int)(cx - 14), (int)(cy + r * 0.3f - 6), 28, 12,
                   juce::Justification::centred);
    }

    // Outer ring
    g.setColour(juce::Colour(0xff3a3a50));
    g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);

    // Playing status ring
    if (deck.isPlaying())
    {
        g.setColour(accentColour.withAlpha(0.3f));
        g.drawEllipse(cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f, 2.0f);
    }
}

void SpinningPlatterComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!deck.hasFile()) return;
    lastMouseX   = e.position.x;
    lastDragTime = juce::Time::getMillisecondCounterHiRes();
    deck.engageScratch(0.0f);
}

void SpinningPlatterComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!deck.hasFile()) return;
    float delta = e.position.x - lastMouseX;
    lastMouseX  = e.position.x;
    double now  = juce::Time::getMillisecondCounterHiRes();
    double dt   = juce::jmax(0.008, (now - lastDragTime) / 1000.0);
    lastDragTime = now;
    float rate = (delta / (float)getWidth()) * 6.0f / (float)dt;
    angle += rate * 0.05f;
    deck.engageScratch(rate);
}

void SpinningPlatterComponent::mouseUp(const juce::MouseEvent&)
{
    deck.disengageScratch();
}
