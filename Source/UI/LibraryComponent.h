#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <functional>

struct LibraryTrack
{
    juce::File   file;
    juce::String name;
    juce::String duration;   // "mm:ss"
    juce::String bpm  = "--";
    juce::String key  = "--";
};

class LibraryComponent : public juce::Component,
                         private juce::TableListBoxModel
{
public:
    // Called when user loads a track; bool = true → Deck B, false → Deck A
    std::function<void(const juce::File&, bool deckB)> onLoadTrack;

    LibraryComponent();
    ~LibraryComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Update BPM/key for a file after it has been analysed (call from editor timer)
    void updateTrackAnalysis(const juce::File& file,
                             const juce::String& bpm,
                             const juce::String& key);

private:
    // TableListBoxModel
    int  getNumRows() override;
    void paintRowBackground(juce::Graphics&, int row, int w, int h, bool selected) override;
    void paintCell(juce::Graphics&, int row, int col, int w, int h, bool selected) override;
    void cellDoubleClicked(int row, int col, const juce::MouseEvent&) override;
    juce::String getCellTooltip(int row, int col) override;

    void scanFolder(const juce::File& dir);
    void applyFilter();
    void addFolder();
    void loadSelected(bool deckB);
    void saveWatchedFolders();
    void loadWatchedFolders();
    juce::File getSettingsFile() const;

    juce::TextEditor   searchBox;
    juce::TableListBox trackTable;
    juce::TextButton   addFolderBtn, rescanBtn, loadABtn, loadBBtn;

    juce::AudioFormatManager formatManager;

    std::vector<juce::File>   watchedFolders;
    std::vector<LibraryTrack> allTracks;
    std::vector<size_t>       filteredIndices;  // indices into allTracks

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryComponent)
};
