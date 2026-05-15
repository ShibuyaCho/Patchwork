#include "MidiMonitorComponent.h"
#include "PatchworkLookAndFeel.h"

static constexpr juce::uint32 kAmber = 0xffff9500;
static constexpr juce::uint32 kBlue  = 0xff0090d4;

MidiMonitorComponent::MidiMonitorComponent(MidiController& mc)
    : controller(mc)
{
    auto makeTitle = [&](juce::Label& l, const juce::String& text, juce::uint32 col = 0xffe2e2e2) {
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
        l.setColour(juce::Label::textColourId, juce::Colour(col));
        addAndMakeVisible(l);
    };

    auto makeLog = [&](juce::TextEditor& e, bool editable = false) {
        e.setMultiLine(true);
        e.setReadOnly(!editable);
        e.setScrollbarsShown(true);
        e.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.5f, 0));
        e.setColour(juce::TextEditor::backgroundColourId, PatchworkLookAndFeel::BG);
        e.setColour(juce::TextEditor::textColourId,       PatchworkLookAndFeel::TEXT_SEC);
        e.setColour(juce::TextEditor::outlineColourId,    PatchworkLookAndFeel::BORDER);
        addAndMakeVisible(e);
    };

    // ── Section titles ─────────────────────────────────────────────────
    makeTitle(devicesTitle, "OPENED DEVICES",                        kAmber);
    makeTitle(blockedTitle,  "DAW-ROUTED",                           0xff9a9a9a);
    makeTitle(eventTitle,    "LIVE MIDI ACTIVITY",                   0xff9a9a9a);
    makeTitle(availTitle,    "ALL MIDI PORTS (system)",              kBlue);
    makeTitle(niTitle,       "NATIVE INSTRUMENTS — AUTO-ACTIVATE",  0xffff9500);
    makeTitle(diagTitle,     "DEVICE SETUP NOTES",                   0xff9a9a9a);
    makeTitle(learnTitle,    "MIDI LEARN",                           kAmber);

    makeLog(devicesList);
    makeLog(blockedList);
    makeLog(eventLog);
    makeLog(availList);
    makeLog(niStatusBox);
    makeLog(diagBox);

    // ── NI activate button ─────────────────────────────────────────────
    niActivateBtn.setButtonText("ACTIVATE MIDI MODE");
    niActivateBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    niActivateBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff9500));
    niActivateBtn.onClick = [this] {
        niActivateBtn.setEnabled(false);
        niActivateBtn.setButtonText("ACTIVATING...");
        niActivator.activateAsync([this](NIActivationResult r) {
            niStatusBox.setText(r.log, juce::dontSendNotification);
            niActivateBtn.setEnabled(true);
            niActivateBtn.setButtonText("ACTIVATE MIDI MODE");
            if (r.needsUdevRule)
            {
                niStatusBox.setText(r.log + "\n--- UDEV RULE NEEDED ---\n" + r.udevRuleText,
                                    juce::dontSendNotification);
            }
            refreshAvailable();
        });
    };
    addAndMakeVisible(niActivateBtn);

    niUdevBtn.setButtonText("INSTALL UDEV RULE");
    niUdevBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    niUdevBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff9a9a9a));
    niUdevBtn.onClick = [this] {
        auto err = NIDeviceActivator::writeUdevRule();
        if (err.isEmpty())
        {
            auto err2 = NIDeviceActivator::applyUdevRules();
            niStatusBox.setText(err2.isEmpty()
                ? "udev rule installed and applied — replug device and try ACTIVATE again."
                : "Rule written. Run manually:\n  sudo udevadm control --reload-rules\n  sudo udevadm trigger",
                juce::dontSendNotification);
        }
        else
        {
            // Can't write as current user — show the rule text to copy
            niStatusBox.setText("Could not write rule automatically.\n\n"
                + err + "\n\nRun this in a terminal:\n\n"
                "sudo bash -c 'cat > /etc/udev/rules.d/99-native-instruments.rules << EOF\n"
                + NIDeviceActivator::getUdevRuleText()
                + "EOF'\nsudo udevadm control --reload-rules && sudo udevadm trigger",
                juce::dontSendNotification);
        }
    };
    addAndMakeVisible(niUdevBtn);

    niMonitorBtn.setButtonText("HID MONITOR");
    niMonitorBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    niMonitorBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff22c55e));
    niMonitorBtn.onClick = [this] {
        if (niActivator.isMonitoring())
        {
            niActivator.stopHIDMonitor();
            niMonitorBtn.setButtonText("HID MONITOR");
            niMonitorBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff22c55e));
            niActivateBtn.setEnabled(true);
        }
        else
        {
            niStatusBox.clear();
            niStatusBox.setText(
                "── HID Monitor ────────────────────────────────\n"
                "Byte positions:\n"
                " 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 ...\n"
                "───────────────────────────────────────────────\n"
                "Press pads, buttons and turn knobs — watch which\n"
                "bytes change. Report the output at patchwork.dj/issues\n"
                "so we can map them to MIDI.\n\n"
                "Starting...\n",
                juce::dontSendNotification);
            niActivator.startHIDMonitor(controller);
            niMonitorBtn.setButtonText("STOP MONITOR");
            niMonitorBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffef4444));
            niActivateBtn.setEnabled(false);
        }
    };
    addAndMakeVisible(niMonitorBtn);

    refreshNIPanel();
    addAndMakeVisible(ledEditor);

    // ── Manual open combo + button ─────────────────────────────────────
    manualOpenBox.setColour(juce::ComboBox::backgroundColourId, PatchworkLookAndFeel::CARD);
    manualOpenBox.setColour(juce::ComboBox::textColourId,       PatchworkLookAndFeel::TEXT_PRI);
    manualOpenBox.setColour(juce::ComboBox::outlineColourId,    PatchworkLookAndFeel::BORDER);
    manualOpenBox.setTextWhenNothingSelected("-- select a port to force-open --");
    addAndMakeVisible(manualOpenBox);

    manualOpenBtn.setButtonText("OPEN");
    manualOpenBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    manualOpenBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kBlue));
    manualOpenBtn.onClick = [this] {
        int sel = manualOpenBox.getSelectedId();
        if (sel <= 0 || sel > cachedAvailDevices.size()) return;
        controller.forceOpenDevice(cachedAvailDevices[sel - 1].identifier);
        learnStatusLabel.setText("Opened: " + cachedAvailDevices[sel - 1].name,
                                  juce::dontSendNotification);
    };
    addAndMakeVisible(manualOpenBtn);

    // ── Learn section ──────────────────────────────────────────────────
    buildTargetCombo();
    learnTargetBox.setColour(juce::ComboBox::backgroundColourId, PatchworkLookAndFeel::CARD);
    learnTargetBox.setColour(juce::ComboBox::textColourId,       PatchworkLookAndFeel::TEXT_PRI);
    learnTargetBox.setColour(juce::ComboBox::outlineColourId,    PatchworkLookAndFeel::BORDER);
    addAndMakeVisible(learnTargetBox);

    learnStartBtn.setButtonText("START LEARN");
    learnStartBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    learnStartBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kAmber));
    learnStartBtn.onClick = [this] { startLearn(); };
    addAndMakeVisible(learnStartBtn);

    learnCancelBtn.setButtonText("CANCEL");
    learnCancelBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    learnCancelBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffef4444));
    learnCancelBtn.onClick = [this] { cancelLearn(); };
    learnCancelBtn.setEnabled(false);
    addAndMakeVisible(learnCancelBtn);

    learnStatusLabel.setText("Select a target then press Start Learn, then move a control.",
                              juce::dontSendNotification);
    learnStatusLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    learnStatusLabel.setColour(juce::Label::textColourId, PatchworkLookAndFeel::TEXT_SEC);
    addAndMakeVisible(learnStatusLabel);

    // ── Save / load ────────────────────────────────────────────────────
    saveBtn.setButtonText("SAVE MAPPINGS");
    saveBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    saveBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff22c55e));
    saveBtn.onClick = [this] {
        controller.saveCustomMappings(mappingsFile());
        learnStatusLabel.setText("Saved: " + mappingsFile().getFullPathName(),
                                  juce::dontSendNotification);
    };
    addAndMakeVisible(saveBtn);

    loadBtn.setButtonText("LOAD MAPPINGS");
    loadBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff252525));
    loadBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kAmber));
    loadBtn.onClick = [this] {
        controller.loadCustomMappings(mappingsFile());
        learnStatusLabel.setText("Loaded: " + mappingsFile().getFullPathName(),
                                  juce::dontSendNotification);
    };
    addAndMakeVisible(loadBtn);

    // ── Learn complete callback ────────────────────────────────────────
    controller.onLearnComplete = [this](MidiMapping::Target target, bool isNote,
                                        int number, const juce::String& device)
    {
        learnStatusLabel.setText("Mapped: " + MidiMapping::targetName(target)
                                 + " <-- " + (isNote ? "Note " : "CC ")
                                 + juce::String(number) + " on " + device,
                                 juce::dontSendNotification);
        learningActive = false;
        learnStartBtn.setEnabled(true);
        learnCancelBtn.setEnabled(false);
        learnStartBtn.setButtonText("START LEARN");
    };

    startTimerHz(5);
    refreshDevices();
    refreshAvailable();
}

MidiMonitorComponent::~MidiMonitorComponent()
{
    stopTimer();
    controller.onLearnComplete = nullptr;
    controller.cancelLearn();
}

// ── buildTargetCombo ──────────────────────────────────────────────────────────

void MidiMonitorComponent::buildTargetCombo()
{
    learnTargetBox.clear();
    int id = 1;

    auto addGroup = [&](const juce::String& header) {
        learnTargetBox.addSectionHeading(header);
    };
    auto addItem = [&](MidiMapping::Target t) {
        learnTargetBox.addItem(MidiMapping::targetName(t), id++);
    };

    addGroup("Deck A");
    addItem(MidiMapping::Target::DeckAPlay);   addItem(MidiMapping::Target::DeckASync);
    addItem(MidiMapping::Target::DeckALoop);   addItem(MidiMapping::Target::DeckALoad);
    addItem(MidiMapping::Target::DeckAVolume); addItem(MidiMapping::Target::DeckATempo);
    addItem(MidiMapping::Target::DeckAPan);
    addItem(MidiMapping::Target::DeckAEQHigh); addItem(MidiMapping::Target::DeckAEQMid);
    addItem(MidiMapping::Target::DeckAEQLow);
    addItem(MidiMapping::Target::DeckAHotCue0); addItem(MidiMapping::Target::DeckAHotCue1);
    addItem(MidiMapping::Target::DeckAHotCue2); addItem(MidiMapping::Target::DeckAHotCue3);

    addGroup("Deck B");
    addItem(MidiMapping::Target::DeckBPlay);   addItem(MidiMapping::Target::DeckBSync);
    addItem(MidiMapping::Target::DeckBLoop);   addItem(MidiMapping::Target::DeckBLoad);
    addItem(MidiMapping::Target::DeckBVolume); addItem(MidiMapping::Target::DeckBTempo);
    addItem(MidiMapping::Target::DeckBPan);
    addItem(MidiMapping::Target::DeckBEQHigh); addItem(MidiMapping::Target::DeckBEQMid);
    addItem(MidiMapping::Target::DeckBEQLow);
    addItem(MidiMapping::Target::DeckBHotCue0); addItem(MidiMapping::Target::DeckBHotCue1);
    addItem(MidiMapping::Target::DeckBHotCue2); addItem(MidiMapping::Target::DeckBHotCue3);

    addGroup("Mixer");
    addItem(MidiMapping::Target::Crossfader);

    addGroup("Effects");
    addItem(MidiMapping::Target::ReverbAmount);
    addItem(MidiMapping::Target::DelayAmount);
    addItem(MidiMapping::Target::FilterFreq);

    addGroup("Sampler Pads");
    for (int i = 0; i < 16; ++i)
        addItem(static_cast<MidiMapping::Target>(
            (int)MidiMapping::Target::SamplerPad0 + i));

    learnTargetBox.setSelectedId(1);
}

static MidiMapping::Target targetAtComboIndex(int zeroBasedIndex)
{
    static const MidiMapping::Target order[] = {
        MidiMapping::Target::DeckAPlay, MidiMapping::Target::DeckASync,
        MidiMapping::Target::DeckALoop, MidiMapping::Target::DeckALoad,
        MidiMapping::Target::DeckAVolume, MidiMapping::Target::DeckATempo,
        MidiMapping::Target::DeckAPan,
        MidiMapping::Target::DeckAEQHigh, MidiMapping::Target::DeckAEQMid,
        MidiMapping::Target::DeckAEQLow,
        MidiMapping::Target::DeckAHotCue0, MidiMapping::Target::DeckAHotCue1,
        MidiMapping::Target::DeckAHotCue2, MidiMapping::Target::DeckAHotCue3,
        MidiMapping::Target::DeckBPlay, MidiMapping::Target::DeckBSync,
        MidiMapping::Target::DeckBLoop, MidiMapping::Target::DeckBLoad,
        MidiMapping::Target::DeckBVolume, MidiMapping::Target::DeckBTempo,
        MidiMapping::Target::DeckBPan,
        MidiMapping::Target::DeckBEQHigh, MidiMapping::Target::DeckBEQMid,
        MidiMapping::Target::DeckBEQLow,
        MidiMapping::Target::DeckBHotCue0, MidiMapping::Target::DeckBHotCue1,
        MidiMapping::Target::DeckBHotCue2, MidiMapping::Target::DeckBHotCue3,
        MidiMapping::Target::Crossfader,
        MidiMapping::Target::ReverbAmount, MidiMapping::Target::DelayAmount,
        MidiMapping::Target::FilterFreq,
    };
    constexpr int nFixed = (int)(sizeof(order) / sizeof(order[0]));
    if (zeroBasedIndex < nFixed)
        return order[zeroBasedIndex];
    int padIdx = zeroBasedIndex - nFixed;
    if (padIdx >= 0 && padIdx < 16)
        return static_cast<MidiMapping::Target>((int)MidiMapping::Target::SamplerPad0 + padIdx);
    return MidiMapping::Target::Unknown;
}

// ── Learn actions ─────────────────────────────────────────────────────────────

void MidiMonitorComponent::startLearn()
{
    int selId = learnTargetBox.getSelectedId();
    if (selId <= 0) return;
    MidiMapping::Target t = targetAtComboIndex(selId - 1);
    if (t == MidiMapping::Target::Unknown) return;

    controller.beginLearn(t);
    learningActive = true;
    learnStartBtn.setEnabled(false);
    learnCancelBtn.setEnabled(true);
    learnStartBtn.setButtonText("WAITING...");
    learnStatusLabel.setText("Move a control on your MIDI device now...",
                              juce::dontSendNotification);
}

void MidiMonitorComponent::cancelLearn()
{
    controller.cancelLearn();
    learningActive = false;
    learnStartBtn.setEnabled(true);
    learnCancelBtn.setEnabled(false);
    learnStartBtn.setButtonText("START LEARN");
    learnStatusLabel.setText("Cancelled.", juce::dontSendNotification);
}

// ── Refresh ───────────────────────────────────────────────────────────────────

void MidiMonitorComponent::refreshDevices()
{
    auto names = controller.getConnectedDeviceNames();
    devicesList.setText(names.isEmpty()
        ? "(none)"
        : names.joinIntoString("\n"),
        juce::dontSendNotification);

    auto blocked = controller.getBlockedDeviceNames();
    blockedList.setText(blocked.isEmpty()
        ? "(none — route these through Reaper instead)"
        : blocked.joinIntoString("\n"),
        juce::dontSendNotification);
}

void MidiMonitorComponent::refreshNIPanel()
{
    auto devices = niActivator.scan();
    if (devices.isEmpty())
    {
        niStatusBox.setText("No NI USB devices detected.\nPlug in your device and wait a moment.",
                            juce::dontSendNotification);
        niActivateBtn.setEnabled(false);
    }
    else
    {
        juce::String txt;
        for (auto& d : devices)
        {
            txt += d.name;
            txt += d.confirmed ? "  [activation confirmed]" : "  [experimental]";
            txt += "\n";
        }
        txt += "\nClick ACTIVATE MIDI MODE to switch device to standard USB-MIDI.\n"
               "If it fails with 'permission denied', click INSTALL UDEV RULE first.";
        niStatusBox.setText(txt, juce::dontSendNotification);
        niActivateBtn.setEnabled(!niActivator.isActivating());
    }
}

void MidiMonitorComponent::refreshAvailable()
{
    cachedAvailDevices = controller.getAllAvailableDevices();

    juce::String text;
    manualOpenBox.clear(juce::dontSendNotification);
    int id = 1;
    for (auto& dev : cachedAvailDevices)
    {
        text += dev.name + "\n";
        manualOpenBox.addItem(dev.name, id++);
    }
    if (text.isEmpty()) text = "(no MIDI ports visible to the system)\n";
    availList.setText(text, juce::dontSendNotification);

    // Diagnostic note
    juce::String diag;
    diag += "Maschine Mikro MK3 (17cc:1700) — MIDI activation status\n"
            "─────────────────────────────────────────────────────────\n"
            "The MK3 uses a proprietary NI protocol. Its firmware requires\n"
            "a specific handshake sequence before accepting a mode switch.\n"
            "The exact command bytes are not public — Patchwork has not yet\n"
            "cracked them. All interrupt writes succeed (device responds)\n"
            "but no mode change occurs without the correct payload.\n\n"
            "── Working options ──────────────────────────────────────\n\n"
            "Option A  NI Maschine software (easiest, confirmed working)\n"
            "  1. Download NI Maschine from native-instruments.com (free)\n"
            "  2. Open it and go to:  File > Preferences > MIDI\n"
            "  3. Enable 'Send MIDI to computer'\n"
            "  4. A virtual port appears in aconnect / this MIDI list\n"
            "  5. Use OPEN button above to connect it to Patchwork\n"
            "  Maschine can run minimised; you only need it for the port.\n\n"
            "Option B  USB traffic capture (helps improve Patchwork)\n"
            "  Boot Windows, install USBPcap + Wireshark, open NI Maschine,\n"
            "  capture the USB frames for vendor 17cc on startup.\n"
            "  Share the .pcapng at github.com/patchwork.dj/issues\n"
            "  and we can implement the real activation byte sequence.\n\n"
            "Option C  a2jmidid (if using JACK/PipeWire in JACK mode)\n"
            "  a2jmidid -e &\n"
            "  Then route Maschine MIDI → Patchwork via Catia or qjackctl.\n\n"
            "Other devices listed above that are not auto-opened can be\n"
            "connected manually with the OPEN button.";
    diagBox.setText(diag, juce::dontSendNotification);
}

void MidiMonitorComponent::refreshEvents()
{
    auto events = controller.getRecentEvents();
    juce::String text;
    for (int i = events.size() - 1; i >= 0; --i)
    {
        auto& ev = events.getReference(i);
        text += "[" + ev.device.substring(0, 14).paddedRight(' ', 14) + "]  " + ev.text + "\n";
    }
    eventLog.setText(text, juce::dontSendNotification);
}

void MidiMonitorComponent::timerCallback()
{
    refreshDevices();
    refreshEvents();

    if (niActivator.isMonitoring())
    {
        // Show diagnostic log from monitor thread (HID report IDs, write results)
        juce::String diagLog = niActivator.getAndClearMonitorLog();
        if (diagLog.isNotEmpty())
        {
            juce::String cur = niStatusBox.getText();
            niStatusBox.setText(cur + diagLog, juce::dontSendNotification);
            niStatusBox.moveCaretToEnd();
        }

        // Drain the HID packet buffer and append to status box
        auto packets = niActivator.getAndClearPackets();
        if (packets.isEmpty()) return;

        juce::String append;
        for (auto& pkt : packets)
        {
            if (pkt.len == 0) continue;   // sentinel — already showed "Starting..."

            // Format hex for display: "xx xx xx xx xx xx xx xx  ..."
            juce::String line;
            for (int b = 0; b < pkt.len; ++b)
            {
                if (b > 0 && b % 8 == 0) line += "  ";
                line += juce::String::toHexString(pkt.data[b]).paddedLeft('0', 2) + " ";
            }
            append += line.trimEnd() + "\n";
        }

        // Keep last 40 lines so the box doesn't grow unbounded
        juce::String current = niStatusBox.getText();
        juce::StringArray lines;
        lines.addLines(current + append);
        while (lines.size() > 40) lines.remove(0);
        niStatusBox.setText(lines.joinIntoString("\n"), juce::dontSendNotification);
        niStatusBox.moveCaretToEnd();
        return;
    }

    static int tick = 0;
    if (++tick >= 10) { tick = 0; refreshAvailable(); refreshNIPanel(); }
}

// ── Paint / resized ───────────────────────────────────────────────────────────

void MidiMonitorComponent::paint(juce::Graphics& g)
{
    g.fillAll(PatchworkLookAndFeel::BG);

    g.setColour(juce::Colour(kAmber).withAlpha(0.7f));
    g.fillRect(0, 0, getWidth(), 2);

    g.setColour(PatchworkLookAndFeel::BORDER);
    g.drawHorizontalLine(getHeight() - 64, 8.0f, (float)getWidth() - 8.0f);
}

void MidiMonitorComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(4);

    // Bottom row: save/load + learn status
    auto bottomRow = area.removeFromBottom(28);
    area.removeFromBottom(4);
    saveBtn.setBounds(bottomRow.removeFromLeft(130));
    bottomRow.removeFromLeft(6);
    loadBtn.setBounds(bottomRow.removeFromLeft(130));
    bottomRow.removeFromLeft(10);
    learnStatusLabel.setBounds(bottomRow);

    // Learn section
    auto learnArea = area.removeFromBottom(56);
    learnTitle.setBounds(learnArea.removeFromTop(14));
    auto learnRow = learnArea.removeFromTop(26);
    learnTargetBox.setBounds(learnRow.removeFromLeft(220));
    learnRow.removeFromLeft(6);
    learnStartBtn.setBounds(learnRow.removeFromLeft(110));
    learnRow.removeFromLeft(4);
    learnCancelBtn.setBounds(learnRow.removeFromLeft(70));

    area.removeFromBottom(6);

    // Manual open row (above the three panes)
    auto manualRow = area.removeFromTop(26);
    availTitle.setBounds(manualRow.removeFromLeft(170));
    manualRow.removeFromLeft(6);
    manualOpenBox.setBounds(manualRow.removeFromLeft(280));
    manualRow.removeFromLeft(6);
    manualOpenBtn.setBounds(manualRow.removeFromLeft(60));

    area.removeFromTop(2);

    // Four panes: opened | daw-routed | all-available | diagnostics
    // Top half: opened/blocked on the left, events on the right
    // Bottom half: all-available on the left, diagnostics on the right

    auto topHalf    = area.removeFromTop(area.getHeight() / 2 - 2);
    area.removeFromTop(4);
    auto bottomHalf = area;

    // Top half: opened | blocked | events
    {
        int pW = (topHalf.getWidth() - 16) / 3;
        auto dev = topHalf.removeFromLeft(pW);
        devicesTitle.setBounds(dev.removeFromTop(14));
        devicesList.setBounds(dev);

        topHalf.removeFromLeft(8);

        auto blk = topHalf.removeFromLeft(pW);
        blockedTitle.setBounds(blk.removeFromTop(14));
        blockedList.setBounds(blk);

        topHalf.removeFromLeft(8);

        eventTitle.setBounds(topHalf.removeFromTop(14));
        eventLog.setBounds(topHalf);
    }

    // Bottom half: available list | NI panel | diagnostics
    {
        int pW = (bottomHalf.getWidth() - 16) / 3;

        // Available list
        auto avl = bottomHalf.removeFromLeft(pW);
        avl.removeFromTop(2);
        availList.setBounds(avl);

        bottomHalf.removeFromLeft(8);

        // NI panel
        auto ni = bottomHalf.removeFromLeft(pW);
        niTitle.setBounds(ni.removeFromTop(14));
        ni.removeFromTop(2);
        auto niBtns = ni.removeFromBottom(26);
        niActivateBtn.setBounds(niBtns.removeFromLeft(120));
        niBtns.removeFromLeft(4);
        niUdevBtn.setBounds(niBtns.removeFromLeft(100));
        niBtns.removeFromLeft(4);
        niMonitorBtn.setBounds(niBtns.removeFromLeft(100));
        ni.removeFromBottom(4);
        // LED slot editor below a reduced status box
        ledEditor.setBounds(ni.removeFromBottom(110));
        ni.removeFromBottom(2);
        niStatusBox.setBounds(ni);

        bottomHalf.removeFromLeft(8);

        // Diagnostics
        diagTitle.setBounds(bottomHalf.removeFromTop(14));
        diagBox.setBounds(bottomHalf);
    }
}

juce::File MidiMonitorComponent::mappingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
           .getChildFile("Patchwork").getChildFile("midi_mappings.xml");
}
