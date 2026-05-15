#include "PatchworkLookAndFeel.h"
#include <cmath>

// Industrial charcoal palette — road case aesthetic
const juce::Colour PatchworkLookAndFeel::BG       { 0xff1c1c1c };
const juce::Colour PatchworkLookAndFeel::PANEL     { 0xff252525 };
const juce::Colour PatchworkLookAndFeel::CARD      { 0xff2e2e2e };
const juce::Colour PatchworkLookAndFeel::BORDER    { 0xff3d3d3d };
const juce::Colour PatchworkLookAndFeel::TEXT_DIM  { 0xff525252 };
const juce::Colour PatchworkLookAndFeel::TEXT_SEC  { 0xff9a9a9a };
const juce::Colour PatchworkLookAndFeel::TEXT_PRI  { 0xffe2e2e2 };

PatchworkLookAndFeel::PatchworkLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId,          BG);
    setColour(juce::Label::textColourId,                          TEXT_PRI);
    setColour(juce::TextEditor::backgroundColourId,               CARD);
    setColour(juce::TextEditor::textColourId,                     TEXT_PRI);
    setColour(juce::ScrollBar::thumbColourId,                     BORDER);
    setColour(juce::ComboBox::backgroundColourId,                 CARD);
    setColour(juce::ComboBox::textColourId,                       TEXT_PRI);
    setColour(juce::ComboBox::outlineColourId,                    BORDER);
    setColour(juce::PopupMenu::backgroundColourId,                PANEL);
    setColour(juce::PopupMenu::textColourId,                      TEXT_PRI);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,     BORDER);
    setColour(juce::TableListBox::backgroundColourId,             BG);
    setColour(juce::ListBox::backgroundColourId,                  BG);
}

// ── Knob — brushed metal with amber/blue arc ──────────────────────────────────

void PatchworkLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                             int x, int y, int w, int h,
                                             float sliderPos,
                                             float startAngle, float endAngle,
                                             juce::Slider& slider)
{
    const float cx     = x + w * 0.5f;
    const float cy     = y + h * 0.5f;
    const float rOuter = juce::jmin(w, h) * 0.44f;
    const float rTrack = rOuter - 3.5f;
    const float rKnob  = rTrack  - 5.5f;
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    const juce::Colour accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

    // Drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.fillEllipse(cx - rOuter + 1.5f, cy - rOuter + 3.5f, rOuter * 2 - 3.0f, rOuter * 2 - 3.0f);

    // Outer ring — recessed mounting plate
    {
        juce::ColourGradient rim(juce::Colour(0xff3c3c3c), cx, cy - rOuter,
                                  juce::Colour(0xff181818), cx, cy + rOuter, false);
        g.setGradientFill(rim);
        g.fillEllipse(cx - rOuter, cy - rOuter, rOuter * 2, rOuter * 2);
        g.setColour(juce::Colour(0xff484848));
        g.drawEllipse(cx - rOuter, cy - rOuter, rOuter * 2, rOuter * 2, 1.0f);
    }

    // Track groove (recessed)
    {
        juce::Path arc;
        arc.addArc(cx - rTrack, cy - rTrack, rTrack * 2, rTrack * 2,
                   startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff0c0c0c));
        g.strokePath(arc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // Value arc — clean accent color, subtle outer halo
    {
        juce::Path arc;
        arc.addArc(cx - rTrack, cy - rTrack, rTrack * 2, rTrack * 2,
                   startAngle, angle, true);
        g.setColour(accent.withAlpha(0.18f));
        g.strokePath(arc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    // Knob body — brushed aluminum gradient
    {
        juce::ColourGradient body(juce::Colour(0xff4c4c4c), cx - rKnob * 0.4f, cy - rKnob,
                                   juce::Colour(0xff1e1e1e), cx + rKnob * 0.4f, cy + rKnob, false);
        g.setGradientFill(body);
        g.fillEllipse(cx - rKnob, cy - rKnob, rKnob * 2, rKnob * 2);

        // Specular highlight — upper-left crescent
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.fillEllipse(cx - rKnob * 0.75f, cy - rKnob * 0.9f, rKnob * 0.9f, rKnob * 0.65f);

        // Knurling ticks around rim
        g.setColour(juce::Colour(0xff0e0e0e).withAlpha(0.75f));
        for (int tick = 0; tick < 12; ++tick)
        {
            const float ta = tick * juce::MathConstants<float>::twoPi / 12.0f;
            const float r1 = rKnob * 0.80f;
            const float r2 = rKnob * 0.96f;
            g.drawLine(cx + std::sin(ta) * r1, cy - std::cos(ta) * r1,
                       cx + std::sin(ta) * r2, cy - std::cos(ta) * r2, 1.1f);
        }

        // Dark bezel ring
        g.setColour(juce::Colour(0xff0e0e0e));
        g.drawEllipse(cx - rKnob, cy - rKnob, rKnob * 2, rKnob * 2, 1.5f);
    }

    // Indicator dot — bright accent
    {
        const float dR  = rKnob * 0.12f;
        const float dCx = cx + std::sin(angle) * rKnob * 0.54f;
        const float dCy = cy - std::cos(angle) * rKnob * 0.54f;
        g.setColour(accent.brighter(0.1f));
        g.fillEllipse(dCx - dR, dCy - dR, dR * 2, dR * 2);
    }
}

// ── Button — physical hardware bevel ─────────────────────────────────────────

void PatchworkLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                 juce::Button& button,
                                                 const juce::Colour& bgColour,
                                                 bool isMouseOver,
                                                 bool isButtonDown)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    const float corner = 4.0f;
    const bool  isOn   = button.getToggleState();

    juce::Colour fill = isOn ? bgColour.brighter(0.10f) : bgColour;
    if (isButtonDown) fill = fill.darker(0.25f);
    else if (isMouseOver && !isOn) fill = fill.brighter(0.08f);

    g.setColour(fill);
    g.fillRoundedRectangle(b, corner);

    // Top-half sheen (bevel highlight)
    if (!isButtonDown)
    {
        g.setColour(juce::Colours::white.withAlpha(isOn ? 0.04f : 0.09f));
        g.fillRoundedRectangle(b.withHeight(b.getHeight() * 0.45f), corner);
    }

    // Border
    if (isOn)
    {
        const juce::Colour glow = bgColour.brighter(0.8f).withSaturation(0.8f);
        g.setColour(glow.withAlpha(0.9f));
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(0.5f), corner, 1.5f);
    }
    else
    {
        // Bevel: lighter top/left, darker bottom/right
        g.setColour(juce::Colour(0xff484848));
        g.drawLine(b.getX() + corner * 0.6f, b.getY(),
                   b.getRight() - corner * 0.6f, b.getY(), 1.0f);
        g.drawLine(b.getX(), b.getY() + corner * 0.6f,
                   b.getX(), b.getBottom() - corner * 0.6f, 1.0f);
        g.setColour(juce::Colour(0xff181818));
        g.drawLine(b.getX() + corner * 0.6f, b.getBottom(),
                   b.getRight() - corner * 0.6f, b.getBottom(), 1.0f);
        g.drawLine(b.getRight(), b.getY() + corner * 0.6f,
                   b.getRight(), b.getBottom() - corner * 0.6f, 1.0f);
    }
}

void PatchworkLookAndFeel::drawButtonText(juce::Graphics& g,
                                           juce::TextButton& button,
                                           bool /*isMouseOver*/,
                                           bool isButtonDown)
{
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));

    const bool isOn = button.getToggleState();
    juce::Colour col = isOn ? button.findColour(juce::TextButton::textColourOnId)
                            : button.findColour(juce::TextButton::textColourOffId);
    if (isButtonDown) col = col.brighter(0.3f);
    g.setColour(col);

    g.drawText(button.getButtonText(),
               button.getLocalBounds().reduced(2, 1),
               juce::Justification::centred, false);
}

// ── Linear slider — rubber-grip fader thumb ───────────────────────────────────

void PatchworkLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                             int x, int y, int w, int h,
                                             float sliderPos,
                                             float /*minPos*/, float /*maxPos*/,
                                             juce::Slider::SliderStyle style,
                                             juce::Slider& slider)
{
    const bool isVert = (style == juce::Slider::LinearVertical);

    // Track groove
    const float trackThick = 4.0f;
    juce::Path track;
    if (isVert)
        track.addRoundedRectangle(x + w * 0.5f - trackThick * 0.5f, (float)y,
                                   trackThick, (float)h, trackThick * 0.5f);
    else
        track.addRoundedRectangle((float)x, y + h * 0.5f - trackThick * 0.5f,
                                   (float)w, trackThick, trackThick * 0.5f);

    g.setColour(juce::Colour(0xff0c0c0c));
    g.fillPath(track);
    g.setColour(juce::Colour(0xff3e3e3e));
    g.strokePath(track, juce::PathStrokeType(0.5f));

    // Thumb dimensions
    const float tW = isVert ? (float)w * 0.78f : 16.0f;
    const float tH = isVert ? 18.0f             : (float)h * 0.78f;
    const float tx = isVert ? x + (w - tW) * 0.5f : sliderPos - tW * 0.5f;
    const float ty = isVert ? sliderPos - tH * 0.5f : y + (h - tH) * 0.5f;

    // Thumb body
    juce::ColourGradient thumbGrad(juce::Colour(0xff545454), tx, ty,
                                    juce::Colour(0xff1e1e1e), tx, ty + tH, false);
    g.setGradientFill(thumbGrad);
    g.fillRoundedRectangle(tx, ty, tW, tH, 4.0f);

    // Grip ribs (rubber texture)
    const float mx = tx + tW * 0.5f;
    const float my = ty + tH * 0.5f;
    for (int ri = -1; ri <= 1; ++ri)
    {
        g.setColour(juce::Colour(0xff0c0c0c).withAlpha(0.9f));
        if (isVert)
            g.drawLine(tx + 3.0f, my + ri * 3.5f, tx + tW - 3.0f, my + ri * 3.5f, 1.2f);
        else
            g.drawLine(mx + ri * 3.5f, ty + 3.0f, mx + ri * 3.5f, ty + tH - 3.0f, 1.2f);

        g.setColour(juce::Colour(0xff5c5c5c).withAlpha(0.35f));
        if (isVert)
            g.drawLine(tx + 3.0f, my + ri * 3.5f + 1.0f, tx + tW - 3.0f, my + ri * 3.5f + 1.0f, 0.8f);
        else
            g.drawLine(mx + ri * 3.5f + 1.0f, ty + 3.0f, mx + ri * 3.5f + 1.0f, ty + tH - 3.0f, 0.8f);
    }

    // Top specular
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(tx + 1.5f, ty + 1.5f, tW - 3.0f, tH * 0.42f, 3.0f);

    // Border
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawRoundedRectangle(tx, ty, tW, tH, 4.0f, 1.0f);

    (void)slider;
}
