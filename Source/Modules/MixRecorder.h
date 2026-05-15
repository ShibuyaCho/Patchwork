#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>

class MixRecorder
{
public:
    MixRecorder();
    ~MixRecorder();

    bool startRecording(const juce::File& file, double sampleRate, int numChannels);
    void stopRecording();
    bool isRecording() const { return recording.load(); }

    void processBlock(const juce::AudioBuffer<float>& buffer);

    juce::String getLastRecordedFile() const { return lastFile.getFullPathName(); }

private:
    juce::TimeSliceThread             backgroundThread { "MixRecorder" };
    juce::AudioFormatManager          formatManager;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    std::atomic<bool>                 recording { false };
    juce::File                        lastFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixRecorder)
};
