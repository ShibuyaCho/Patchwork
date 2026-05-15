#include "NILEDEditorComponent.h"
#include "PatchworkLookAndFeel.h"

// ── Pad indexed-color palette (approx screen RGB for display only) ────────────
static const juce::uint32 kPadPaletteRGB[18] = {
    0x000000,  // 0  off
    0xFF2020,  // 1  red
    0xFF7000,  // 2  orange
    0xFFAA40,  // 3  light orange
    0xFFD000,  // 4  warm yellow
    0xFFFF00,  // 5  yellow
    0x80FF00,  // 6  lime
    0x00FF00,  // 7  green
    0x00FF80,  // 8  mint
    0x00FFFF,  // 9  cyan
    0x0080FF,  // 10 turquoise
    0x0040FF,  // 11 blue
    0x8000FF,  // 12 plum
    0xA000E0,  // 13 violet
    0xC000C0,  // 14 purple
    0xFF0080,  // 15 magenta
    0xFF00FF,  // 16 fuchsia
    0xFFFFFF,  // 17 white
};

// Brightness levels for button/strip slots
static const uint8_t kBrightLevels[4] = { 0x00, 0x7C, 0x7E, 0x7F };

// Preset pad bytes: off, then all 17 colors at full brightness
static uint8_t padPreset(int n)  // n 0-17
{
    if (n == 0) return 0x00;
    return (uint8_t)((n << 2) | 3);  // colorIndex=n, brightness=3 (full)
}

// ─────────────────────────────────────────────────────────────────────────────

NILEDEditorComponent::NILEDEditorComponent(NIDeviceActivator& a)
    : activator(a), box({}, this)
{
    box.setRowHeight(22);
    box.setColour(juce::ListBox::backgroundColourId, PatchworkLookAndFeel::BG);
    box.setColour(juce::ListBox::outlineColourId,    PatchworkLookAndFeel::BORDER);
    addAndMakeVisible(box);

    // Inline label editor
    nameEd.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    nameEd.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    nameEd.setColour(juce::TextEditor::textColourId,       juce::Colours::white);
    nameEd.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xffff9500));
    nameEd.onReturnKey = [this] { commitEditLabel(); };
    nameEd.onEscapeKey = [this] { nameEd.setVisible(false); editRow = -1; };
    nameEd.onFocusLost = [this] { commitEditLabel(); };
    addChildComponent(nameEd);

    auto styleBtn = [](juce::TextButton& b, juce::uint32 col) {
        b.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(col));
        b.setColour(juce::TextButton::textColourOnId,  juce::Colour(0xffef4444));
    };

    styleBtn(scanAllBtn, 0xff22c55e);
    scanAllBtn.setClickingTogglesState(true);
    scanAllBtn.setTooltip("Probe all 80 LED slots in sequence — watch device to map slot# to physical control");
    scanAllBtn.onClick = [this] {
        if (scanAllBtn.getToggleState()) { scanSlot = 0; probeSlot(0); startTimer(850); }
        else                             { stopTimer(); scanSlot = -1; }
    };
    addAndMakeVisible(scanAllBtn);

    styleBtn(saveBtn, 0xff22c55e);
    saveBtn.onClick = [this] {
        auto f = dataFile();
        f.getParentDirectory().createDirectory();
        juce::XmlElement root("MK3LEDs");
        for (int i = 0; i < kLEDCount; ++i) {
            auto* s = root.createNewChildElement("LED");
            s->setAttribute("i",     i);
            s->setAttribute("v",     (int)ledData[i]);
            s->setAttribute("label", slotLabels[i]);
        }
        root.writeTo(f);
    };
    addAndMakeVisible(saveBtn);

    styleBtn(loadBtn, 0xffff9500);
    loadBtn.onClick = [this] {
        auto xml = juce::XmlDocument::parse(dataFile());
        if (!xml) return;
        for (int i = 0; i < xml->getNumChildElements(); ++i) {
            auto* e = xml->getChildElement(i);
            int idx = e->getIntAttribute("i", -1);
            if (idx < 0 || idx >= kLEDCount) continue;
            ledData[idx]    = (uint8_t)e->getIntAttribute("v", 0);
            slotLabels[idx] = e->getStringAttribute("label");
        }
        activator.setAllLEDs(ledData);
        box.repaint();
    };
    addAndMakeVisible(loadBtn);
}

NILEDEditorComponent::~NILEDEditorComponent() { stopTimer(); }

void NILEDEditorComponent::timerCallback()
{
    ++scanSlot;
    if (scanSlot >= kLEDCount) {
        scanSlot = -1;
        scanAllBtn.setToggleState(false, juce::dontSendNotification);
        stopTimer();
        return;
    }
    probeSlot(scanSlot);
    box.repaintRow(scanSlot - 1);
    box.repaintRow(scanSlot);
}

void NILEDEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto btns = area.removeFromBottom(26).reduced(2, 2);
    scanAllBtn.setBounds(btns.removeFromLeft(80));
    btns.removeFromLeft(4);
    saveBtn.setBounds(btns.removeFromLeft(50));
    btns.removeFromLeft(4);
    loadBtn.setBounds(btns.removeFromLeft(50));
    box.setBounds(area);
}

// ── Color helpers ─────────────────────────────────────────────────────────────

juce::Colour NILEDEditorComponent::slotDisplayColor(int row) const
{
    uint8_t v = ledData[row];
    if (isPadSlot(row)) {
        int colorIdx  = v >> 2;
        int bright    = v & 0x03;
        float factor  = bright == 0 ? 0.0f : bright == 1 ? 0.3f : bright == 2 ? 0.6f : 1.0f;
        if (colorIdx == 0 || factor == 0.0f) return juce::Colours::black;
        juce::uint32 rgb = kPadPaletteRGB[colorIdx];
        return juce::Colour((uint8_t)((rgb >> 16 & 0xFF) * factor),
                            (uint8_t)((rgb >>  8 & 0xFF) * factor),
                            (uint8_t)((rgb >>  0 & 0xFF) * factor));
    }
    // Button / strip: brightness byte → shade of white
    if (v == 0x00) return juce::Colours::black;
    if (v == 0x7C) return juce::Colour(0xff404040);
    if (v == 0x7E) return juce::Colour(0xff808080);
    return juce::Colours::white;
}

juce::String NILEDEditorComponent::slotName(int row) const
{
    if (row < kLEDButtonCount)
        return juce::String(kButtonNames[row]);
    if (row < kLEDStripBase) {
        // Map LED index back to NI pad number: LED 39=PAD13, 40=PAD14...
        int ledOff = row - kLEDPadBase;
        int niPad  = (3 - ledOff / 4) * 4 + ledOff % 4 + 1;  // 1-based
        return "PAD " + juce::String(niPad);
    }
    return "STRIP " + juce::String(row - kLEDStripBase);
}

// ── List rendering ────────────────────────────────────────────────────────────

void NILEDEditorComponent::paintListBoxItem(int row, juce::Graphics& g,
                                             int w, int h, bool)
{
    if (row < 0 || row >= kLEDCount) return;
    g.fillAll(row == scanSlot ? juce::Colour(0xff1e2e1e) : PatchworkLookAndFeel::BG);

    // Index
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    g.setColour(juce::Colour(isPadSlot(row)    ? 0xff88aaff
                           : row >= kLEDStripBase ? 0xff88ffaa
                                                  : 0xff555555));
    g.drawText(juce::String(row).paddedLeft('0', 2),
               juce::Rectangle<int>(2, 0, 22, h), juce::Justification::centredRight);

    // Color swatch
    juce::Colour swatch = slotDisplayColor(row);
    auto swatchR = juce::Rectangle<int>(26, (h - 14) / 2, 30, 14);
    g.setColour(swatch == juce::Colours::black ? juce::Colour(0xff1a1a1a) : swatch);
    g.fillRoundedRectangle(swatchR.toFloat(), 2.0f);
    g.setColour(juce::Colour(0xff444444));
    g.drawRoundedRectangle(swatchR.toFloat(), 2.0f, 1.0f);

    // Device name + optional user label
    juce::String name = slotName(row);
    juce::String label = slotLabels[row];
    juce::String display = label.isEmpty() ? name : name + "  –  " + label;

    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.setColour(label.isEmpty() ? juce::Colour(0xff888888) : juce::Colour(0xffc0c0c0));
    g.drawText(display, juce::Rectangle<int>(60, 0, w - 62, h),
               juce::Justification::centredLeft, true);

    // Value hint (right side)
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
    g.setColour(juce::Colour(0xff444444));
    juce::String valHint;
    if (isPadSlot(row)) {
        int ci = ledData[row] >> 2, br = ledData[row] & 3;
        if (ci < 18) valHint = juce::String(kPadColorNames[ci]) + "/" + juce::String(br);
    } else {
        valHint = "0x" + juce::String::toHexString(ledData[row]).toUpperCase();
    }
    g.drawText(valHint, juce::Rectangle<int>(w - 80, 0, 78, h),
               juce::Justification::centredRight, true);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawHorizontalLine(h - 1, 0.0f, (float)w);
}

void NILEDEditorComponent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= kLEDCount) return;
    if (e.mods.isRightButtonDown()) cycleSlot(row);
    else                            probeSlot(row);
}

void NILEDEditorComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    beginEditLabel(row);
}

// ── Actions ───────────────────────────────────────────────────────────────────

void NILEDEditorComponent::probeSlot(int row)
{
    activator.requestProbeLED(row);
}

void NILEDEditorComponent::cycleSlot(int row)
{
    if (isPadSlot(row)) {
        cycleIdx[row] = (cycleIdx[row] + 1) % 18;
        ledData[row]  = padPreset(cycleIdx[row]);
    } else {
        cycleIdx[row] = (cycleIdx[row] + 1) % 4;
        ledData[row]  = kBrightLevels[cycleIdx[row]];
    }
    activator.setLEDByte(row, ledData[row]);
    box.repaintRow(row);
}

void NILEDEditorComponent::beginEditLabel(int row)
{
    if (editRow >= 0) commitEditLabel();
    editRow = row;
    auto rowInBox = box.getRowPosition(row, true);
    nameEd.setText(slotLabels[row], false);
    nameEd.setBounds(box.getX() + 60, box.getY() + rowInBox.getY() + 2,
                     getWidth() - box.getX() - 64, 18);
    nameEd.setVisible(true);
    nameEd.toFront(true);
    nameEd.grabKeyboardFocus();
    nameEd.selectAll();
}

void NILEDEditorComponent::commitEditLabel()
{
    if (editRow < 0) return;
    slotLabels[editRow] = nameEd.getText().trim();
    nameEd.setVisible(false);
    box.repaintRow(editRow);
    editRow = -1;
}

juce::File NILEDEditorComponent::dataFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
           .getChildFile("Patchwork").getChildFile("mk3_leds.xml");
}
