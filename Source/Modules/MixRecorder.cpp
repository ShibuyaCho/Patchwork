#include "MixRecorder.h"

MixRecorder::MixRecorder()
{
    formatManager.registerBasicFormats();
    backgroundThread.startThread();
}

MixRecorder::~MixRecorder()
{
    stopRecording();
    backgroundThread.stopThread(2000);
}

bool MixRecorder::startRecording(const juce::File& file, double sampleRate, int numChannels)
{
    stopRecording();

    // Prefer FLAC (lossless, ~50% smaller than WAV); fall back to WAV
    juce::AudioFormat* fmt = formatManager.findFormatForFileExtension("flac");
    if (fmt == nullptr) fmt = formatManager.findFormatForFileExtension("wav");
    if (fmt == nullptr) return false;

    auto stream = std::make_unique<juce::FileOutputStream>(file);
    if (stream->failedToOpen()) return false;

    auto* rawWriter = fmt->createWriterFor(
        stream.release(),           // format takes ownership of stream
        sampleRate,
        (unsigned int)numChannels,
        24,                         // 24-bit
        {},
        0);
    if (rawWriter == nullptr) return false;

    writer = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        rawWriter, backgroundThread, 65536);

    lastFile = file;
    recording.store(true);
    return true;
}

void MixRecorder::stopRecording()
{
    if (!recording.exchange(false)) return;
    writer.reset();   // ThreadedWriter destructor flushes and waits
}

void MixRecorder::processBlock(const juce::AudioBuffer<float>& buffer)
{
    if (recording.load() && writer)
        writer->write(buffer.getArrayOfReadPointers(), buffer.getNumSamples());
}
