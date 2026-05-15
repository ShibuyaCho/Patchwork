#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

class BroadcastEngine : public juce::Thread
{
public:
    struct Config
    {
        juce::String host     = "localhost";
        int          port     = 8000;
        juce::String mount    = "/stream";
        juce::String password = "hackme";
        juce::String name     = "Patchwork Stream";
        juce::String description = "Live DJ Mix";
        int          bitrate  = 128;  // kbps
    };

    BroadcastEngine();
    ~BroadcastEngine() override;

    // Start streaming — returns false if connection failed
    bool start(const Config& cfg, double sampleRate, int numChannels);
    void stop();
    bool isStreaming()  const { return streaming.load(); }
    juce::String getStatus() const;

    // Audio thread: feed mixed audio into the ring buffer
    void processBlock(const juce::AudioBuffer<float>& buffer);

private:
    void run() override;
    void setStatus(const juce::String& s);

    Config config;
    double sr         = 44100.0;
    int    channels   = 2;

    std::atomic<bool>    streaming { false };
    juce::String         statusMsg;
    mutable juce::CriticalSection statusLock;

    // Lock-free ring buffer (interleaved float samples)
    static constexpr int kRingSize = 131072;   // ~1.5 s at 44.1kHz stereo
    std::vector<float>   ringData;
    juce::AbstractFifo   ringFifo { kRingSize };

    // Opaque handles — types only visible in the .cpp
    void* shoutHandle = nullptr;
    void* lameHandle  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BroadcastEngine)
};
