#include "LibraryComponent.h"
#include <algorithm>

enum { COL_NAME = 1, COL_DUR, COL_BPM, COL_KEY };

static constexpr juce::uint32 kBg0  = 0xff0c0c16;
static constexpr juce::uint32 kBg1  = 0xff0f0f1a;
static constexpr juce::uint32 kSel  = 0xff1a2a3a;
static constexpr juce::uint32 kText = 0xffcbd5e1;

LibraryComponent::LibraryComponent()
{
    formatManager.registerBasicFormats();

    searchBox.setTextToShowWhenEmpty("Search tracks…", juce::Colour(0xff5a5a7a));
    searchBox.setColour(juce::TextEditor::backgroundColourId,   juce::Colour(0xff111122));
    searchBox.setColour(juce::TextEditor::outlineColourId,      juce::Colour(0xff2a2a40));
    searchBox.setColour(juce::TextEditor::textColourId,         juce::Colour(kText));
    searchBox.onTextChange = [this] { applyFilter(); };
    addAndMakeVisible(searchBox);

    auto styleBtn = [this](juce::TextButton& b, const juce::String& text,
                           juce::uint32 bg, juce::uint32 fg)
    {
        b.setButtonText(text);
        b.setColour(juce::TextButton::buttonColourId,  juce::Colour(bg));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(fg));
        addAndMakeVisible(b);
    };

    styleBtn(addFolderBtn, "+ Folder", 0xff111133, 0xff7a7aee);
    addFolderBtn.onClick = [this] { addFolder(); };

    styleBtn(rescanBtn, "Rescan", 0xff111133, 0xff7a7aee);
    rescanBtn.onClick = [this]
    {
        allTracks.clear();
        for (auto& f : watchedFolders) scanFolder(f);
        applyFilter();
    };

    styleBtn(loadABtn, "→ A", 0xff001a22, 0xff00c8f0);
    loadABtn.onClick = [this] { loadSelected(false); };

    styleBtn(loadBBtn, "→ B", 0xff22001a, 0xffff2277);
    loadBBtn.onClick = [this] { loadSelected(true); };

    trackTable.setModel(this);
    trackTable.setColour(juce::ListBox::backgroundColourId, juce::Colour(kBg0));
    trackTable.setColour(juce::ListBox::outlineColourId,    juce::Colour(0xff1e1e30));
    trackTable.setOutlineThickness(1);
    trackTable.setRowHeight(20);

    auto& hdr = trackTable.getHeader();
    hdr.addColumn("Name",     COL_NAME, 280, 80, 1000, juce::TableHeaderComponent::defaultFlags);
    hdr.addColumn("Duration", COL_DUR,   60, 40,  100, juce::TableHeaderComponent::defaultFlags);
    hdr.addColumn("BPM",      COL_BPM,   60, 40,   80, juce::TableHeaderComponent::defaultFlags);
    hdr.addColumn("Key",      COL_KEY,   60, 40,   80, juce::TableHeaderComponent::defaultFlags);
    hdr.setStretchToFitActive(true);
    hdr.setColour(juce::TableHeaderComponent::backgroundColourId, juce::Colour(0xff111122));
    hdr.setColour(juce::TableHeaderComponent::textColourId,       juce::Colour(kText));
    hdr.setColour(juce::TableHeaderComponent::outlineColourId,    juce::Colour(0xff1e1e30));
    addAndMakeVisible(trackTable);

    loadWatchedFolders();
    applyFilter();
}

LibraryComponent::~LibraryComponent() = default;

void LibraryComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kBg0));
}

void LibraryComponent::resized()
{
    auto area = getLocalBounds().reduced(4);
    auto bar  = area.removeFromTop(28);

    // Buttons right-aligned: → B | → A | Rescan | + Folder
    loadBBtn   .setBounds(bar.removeFromRight(44));  bar.removeFromRight(3);
    loadABtn   .setBounds(bar.removeFromRight(44));  bar.removeFromRight(3);
    rescanBtn  .setBounds(bar.removeFromRight(56));  bar.removeFromRight(3);
    addFolderBtn.setBounds(bar.removeFromRight(68)); bar.removeFromRight(3);
    searchBox  .setBounds(bar);

    area.removeFromTop(4);
    trackTable.setBounds(area);
}

// ── TableListBoxModel ─────────────────────────────────────────────────────────

int LibraryComponent::getNumRows() { return (int)filteredIndices.size(); }

void LibraryComponent::paintRowBackground(juce::Graphics& g, int row, int w, int h, bool sel)
{
    if (sel)                g.fillAll(juce::Colour(kSel));
    else if (row % 2 == 1) g.fillAll(juce::Colour(kBg1));
    else                   g.fillAll(juce::Colour(kBg0));
}

void LibraryComponent::paintCell(juce::Graphics& g, int row, int col, int w, int h, bool sel)
{
    if (row >= (int)filteredIndices.size()) return;
    auto& t = allTracks[filteredIndices[row]];

    g.setColour(sel ? juce::Colours::white : juce::Colour(kText));
    g.setFont(juce::Font(12.0f));

    juce::String text;
    switch (col)
    {
        case COL_NAME: text = t.name;     break;
        case COL_DUR:  text = t.duration; break;
        case COL_BPM:  text = t.bpm;      break;
        case COL_KEY:  text = t.key;      break;
        default: break;
    }
    g.drawText(text, 4, 0, w - 6, h, juce::Justification::centredLeft, true);
}

void LibraryComponent::cellDoubleClicked(int row, int, const juce::MouseEvent&)
{
    loadSelected(false);
    juce::ignoreUnused(row);
}

juce::String LibraryComponent::getCellTooltip(int row, int)
{
    if (row < (int)filteredIndices.size())
        return allTracks[filteredIndices[row]].file.getFullPathName();
    return {};
}

// ── Internals ─────────────────────────────────────────────────────────────────

void LibraryComponent::loadSelected(bool deckB)
{
    int row = trackTable.getSelectedRow();
    if (row >= 0 && row < (int)filteredIndices.size() && onLoadTrack)
        onLoadTrack(allTracks[filteredIndices[row]].file, deckB);
}

void LibraryComponent::scanFolder(const juce::File& dir)
{
    juce::Array<juce::File> found;
    dir.findChildFiles(found, juce::File::findFiles, true,
                       "*.mp3;*.wav;*.aiff;*.aif;*.flac;*.ogg;*.m4a");

    allTracks.reserve(allTracks.size() + (size_t)found.size());
    for (auto& f : found)
    {
        // Skip duplicates
        bool dup = false;
        for (auto& t : allTracks) if (t.file == f) { dup = true; break; }
        if (dup) continue;

        LibraryTrack t;
        t.file = f;
        t.name = f.getFileNameWithoutExtension();
        t.bpm  = "--";
        t.key  = "--";

        if (auto* reader = formatManager.createReaderFor(f))
        {
            std::unique_ptr<juce::AudioFormatReader> r(reader);
            double secs = (double)r->lengthInSamples / r->sampleRate;
            int m = (int)(secs / 60.0), s = (int)secs % 60;
            t.duration = juce::String(m) + ":" + juce::String(s).paddedLeft('0', 2);
        }

        allTracks.push_back(std::move(t));
    }
}

void LibraryComponent::applyFilter()
{
    juce::String q = searchBox.getText().trim().toLowerCase();
    filteredIndices.clear();
    for (size_t i = 0; i < allTracks.size(); ++i)
    {
        auto& t = allTracks[i];
        if (q.isEmpty()
            || t.name.toLowerCase().contains(q)
            || t.bpm.contains(q)
            || t.key.toLowerCase().contains(q))
        {
            filteredIndices.push_back(i);
        }
    }
    trackTable.updateContent();
}

void LibraryComponent::updateTrackAnalysis(const juce::File& file,
                                           const juce::String& bpm,
                                           const juce::String& key)
{
    for (auto& t : allTracks)
    {
        if (t.file == file) { t.bpm = bpm; t.key = key; break; }
    }
    trackTable.repaint();
}

// ── Persistence ───────────────────────────────────────────────────────────────

juce::File LibraryComponent::getSettingsFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("Patchwork")
               .getChildFile("library.xml");
}

void LibraryComponent::saveWatchedFolders()
{
    juce::XmlElement root("PatchworkLibrary");
    for (auto& f : watchedFolders)
        root.createNewChildElement("Folder")->setAttribute("path", f.getFullPathName());
    auto file = getSettingsFile();
    file.getParentDirectory().createDirectory();
    root.writeTo(file);
}

void LibraryComponent::loadWatchedFolders()
{
    auto file = getSettingsFile();
    if (!file.existsAsFile()) return;
    if (auto xml = juce::XmlDocument::parse(file))
    {
        for (auto* child : xml->getChildIterator())
        {
            if (child->hasTagName("Folder"))
            {
                juce::File dir(child->getStringAttribute("path"));
                if (dir.isDirectory() && std::find(watchedFolders.begin(), watchedFolders.end(), dir) == watchedFolders.end())
                {
                    watchedFolders.push_back(dir);
                    scanFolder(dir);
                }
            }
        }
    }
}

void LibraryComponent::addFolder()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select music folder",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory));

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;
            auto dir = results[0];
            if (std::find(watchedFolders.begin(), watchedFolders.end(), dir) == watchedFolders.end())
            {
                watchedFolders.push_back(dir);
                scanFolder(dir);
                applyFilter();
                saveWatchedFolders();
            }
        });
}
