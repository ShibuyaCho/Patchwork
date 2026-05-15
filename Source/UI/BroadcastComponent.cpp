#include "BroadcastComponent.h"

static constexpr juce::uint32 kBg   = 0xff0c0c16;
static constexpr juce::uint32 kBg2  = 0xff111122;
static constexpr juce::uint32 kText = 0xffcbd5e1;

static void styleEdit(juce::TextEditor& e, const juce::String& placeholder)
{
    e.setTextToShowWhenEmpty(placeholder, juce::Colour(0xff4a4a6a));
    e.setColour(juce::TextEditor::backgroundColourId, juce::Colour(kBg2));
    e.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xff2a2a40));
    e.setColour(juce::TextEditor::textColourId,       juce::Colour(kText));
    e.setFont(juce::Font(12.0f));
}

static void sectionLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(10.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(0xff6a6acc));
}

static void fieldLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(11.0f));
    l.setColour(juce::Label::textColourId, juce::Colour(kText));
    l.setJustificationType(juce::Justification::centredRight);
}

BroadcastComponent::BroadcastComponent(BroadcastEngine& eng, MixRecorder& rec)
    : engine(eng), recorder(rec)
{
    sectionLabel(sectionStream, "— ICECAST / SHOUTCAST —");
    addAndMakeVisible(sectionStream);

    fieldLabel(hostLabel, "Host");      styleEdit(hostEdit,    "localhost");   hostEdit.setText("localhost");
    fieldLabel(portLabel, "Port");      styleEdit(portEdit,    "8000");        portEdit.setText("8000");
    fieldLabel(mountLabel, "Mount");    styleEdit(mountEdit,   "/stream");     mountEdit.setText("/stream");
    fieldLabel(passLabel, "Password");  styleEdit(passEdit,    "hackme");      passEdit.setText("hackme");
                                        passEdit.setPasswordCharacter('*');
    fieldLabel(bitrateLabel, "Bitrate (kbps)"); styleEdit(bitrateEdit, "128"); bitrateEdit.setText("128");

    for (auto* w : {&hostEdit, &portEdit, &mountEdit, &passEdit, &bitrateEdit})
        addAndMakeVisible(w);
    for (auto* l : {&hostLabel, &portLabel, &mountLabel, &passLabel, &bitrateLabel})
        addAndMakeVisible(l);

    streamBtn.setButtonText("Start Streaming");
    streamBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff001a22));
    streamBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff003344));
    streamBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00c8f0));
    streamBtn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
    streamBtn.setClickingTogglesState(true);
    streamBtn.onClick = [this] { startOrStopStream(); };
    addAndMakeVisible(streamBtn);

    statusLabel.setFont(juce::Font(10.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a4a6a));
    addAndMakeVisible(statusLabel);

    sectionLabel(sectionRecord, "— RECORD MIX TO FILE —");
    addAndMakeVisible(sectionRecord);

    recordBtn.setButtonText("Start Recording");
    recordBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a0808));
    recordBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff550000));
    recordBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffef4444));
    recordBtn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
    recordBtn.setClickingTogglesState(true);
    recordBtn.onClick = [this] { startOrStopRecord(); };
    addAndMakeVisible(recordBtn);

    recordLabel.setText("Idle", juce::dontSendNotification);
    recordLabel.setFont(juce::Font(10.0f));
    recordLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a4a6a));
    addAndMakeVisible(recordLabel);

    startTimerHz(2);
}

BroadcastComponent::~BroadcastComponent()
{
    stopTimer();
}

void BroadcastComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kBg));
}

void BroadcastComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 8);

    sectionStream.setBounds(area.removeFromTop(18));
    area.removeFromTop(4);

    auto row = [&](juce::Label& lbl, juce::Component& edit) {
        auto r = area.removeFromTop(22);
        lbl.setBounds(r.removeFromLeft(90));
        r.removeFromLeft(4);
        edit.setBounds(r);
        area.removeFromTop(4);
    };
    row(hostLabel,    hostEdit);
    row(portLabel,    portEdit);
    row(mountLabel,   mountEdit);
    row(passLabel,    passEdit);
    row(bitrateLabel, bitrateEdit);

    auto btnRow = area.removeFromTop(28);
    streamBtn.setBounds(btnRow.removeFromLeft(130));
    btnRow.removeFromLeft(8);
    statusLabel.setBounds(btnRow);

    area.removeFromTop(14);
    sectionRecord.setBounds(area.removeFromTop(18));
    area.removeFromTop(4);

    auto recRow = area.removeFromTop(28);
    recordBtn.setBounds(recRow.removeFromLeft(130));
    recRow.removeFromLeft(8);
    recordLabel.setBounds(recRow);
}

void BroadcastComponent::timerCallback()
{
    statusLabel.setText(engine.getStatus(), juce::dontSendNotification);

    bool streamOn = engine.isStreaming();
    streamBtn.setToggleState(streamOn, juce::dontSendNotification);
    streamBtn.setButtonText(streamOn ? "Stop Streaming" : "Start Streaming");

    bool recOn = recorder.isRecording();
    recordBtn.setToggleState(recOn, juce::dontSendNotification);
    recordBtn.setButtonText(recOn ? "Stop Recording" : "Start Recording");
    if (!recOn && recorder.getLastRecordedFile().isNotEmpty())
    {
        recordLabel.setText("Saved: " + juce::File(recorder.getLastRecordedFile()).getFileName(),
                            juce::dontSendNotification);
    }
}

void BroadcastComponent::startOrStopStream()
{
    if (engine.isStreaming())
    {
        engine.stop();
        return;
    }
    BroadcastEngine::Config cfg;
    cfg.host    = hostEdit.getText().trim();
    cfg.port    = portEdit.getText().getIntValue();
    cfg.mount   = mountEdit.getText().trim();
    cfg.password = passEdit.getText();
    cfg.bitrate = bitrateEdit.getText().getIntValue();
    if (cfg.port <= 0) cfg.port = 8000;
    if (cfg.bitrate <= 0) cfg.bitrate = 128;

    if (!engine.start(cfg, 44100.0, 2))
        statusLabel.setText("Failed: " + engine.getStatus(), juce::dontSendNotification);
}

void BroadcastComponent::startOrStopRecord()
{
    if (recorder.isRecording()) { recorder.stopRecording(); return; }

    auto timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
    auto dir = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
                   .getChildFile("Patchwork Recordings");
    dir.createDirectory();

    // Prefer FLAC extension; MixRecorder decides actual format
    auto file = dir.getChildFile("mix_" + timestamp + ".flac");
    if (recorder.startRecording(file, 44100.0, 2))
        recordLabel.setText("Recording to: " + file.getFileName(), juce::dontSendNotification);
    else
        recordLabel.setText("Failed to open file", juce::dontSendNotification);
}
