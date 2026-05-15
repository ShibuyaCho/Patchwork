#include "BroadcastEngine.h"
#include <shout/shout.h>
#include <lame/lame.h>
#include <cstring>

BroadcastEngine::BroadcastEngine() : juce::Thread("BroadcastEngine") {}

BroadcastEngine::~BroadcastEngine()
{
    stop();
}

juce::String BroadcastEngine::getStatus() const
{
    juce::CriticalSection::ScopedLockType lk(statusLock);
    return statusMsg;
}

void BroadcastEngine::setStatus(const juce::String& s)
{
    juce::CriticalSection::ScopedLockType lk(statusLock);
    statusMsg = s;
}

bool BroadcastEngine::start(const Config& cfg, double sampleRate, int numChannels)
{
    stop();
    config   = cfg;
    sr       = sampleRate;
    channels = numChannels;

    ringData.assign(kRingSize, 0.0f);
    ringFifo.reset();

    // ── libshout setup ────────────────────────────────────────────────────
    shout_init();
    shout_t* sh = shout_new();
    if (sh == nullptr) { setStatus("shout_new failed"); return false; }

    shout_set_host    (sh, cfg.host.toRawUTF8());
    shout_set_port    (sh, (unsigned short)cfg.port);
    shout_set_mount   (sh, cfg.mount.toRawUTF8());
    shout_set_password(sh, cfg.password.toRawUTF8());
    shout_set_name    (sh, cfg.name.toRawUTF8());
    shout_set_description(sh, cfg.description.toRawUTF8());
    shout_set_format  (sh, SHOUT_FORMAT_MP3);
    shout_set_protocol(sh, SHOUT_PROTOCOL_HTTP);

    juce::String bitrateStr(cfg.bitrate);
    shout_set_audio_info(sh, SHOUT_AI_BITRATE, bitrateStr.toRawUTF8());

    if (shout_open(sh) != SHOUTERR_SUCCESS)
    {
        setStatus(juce::String("Connect failed: ") + shout_get_error(sh));
        shout_free(sh);
        shout_shutdown();
        return false;
    }
    shoutHandle = sh;

    // ── LAME setup ────────────────────────────────────────────────────────
    lame_global_flags* lame = lame_init();
    lame_set_in_samplerate(lame, (int)sr);
    lame_set_num_channels (lame, channels);
    lame_set_out_samplerate(lame, 44100);
    lame_set_brate        (lame, cfg.bitrate);
    lame_set_quality      (lame, 5);     // 2=high, 7=fast; 5 is a good balance
    if (lame_init_params(lame) < 0)
    {
        setStatus("LAME init failed");
        shout_close(sh); shout_free(sh); shout_shutdown();
        lame_close(lame);
        return false;
    }
    lameHandle = lame;

    streaming.store(true);
    setStatus("Streaming to " + cfg.host + ":" + juce::String(cfg.port) + cfg.mount);
    startThread();
    return true;
}

void BroadcastEngine::stop()
{
    if (!streaming.load()) return;
    streaming.store(false);
    stopThread(3000);

    if (lameHandle)
    {
        unsigned char flush[8192];
        lame_encode_flush(static_cast<lame_global_flags*>(lameHandle), flush, sizeof(flush));
        lame_close(static_cast<lame_global_flags*>(lameHandle));
        lameHandle = nullptr;
    }
    if (shoutHandle)
    {
        shout_close(static_cast<shout_t*>(shoutHandle));
        shout_free (static_cast<shout_t*>(shoutHandle));
        shout_shutdown();
        shoutHandle = nullptr;
    }
    setStatus("Stopped");
}

void BroadcastEngine::processBlock(const juce::AudioBuffer<float>& buffer)
{
    if (!streaming.load()) return;

    const int numFrames   = buffer.getNumSamples();
    const int interleaved = numFrames * 2;  // always write stereo
    int start1, size1, start2, size2;
    ringFifo.prepareToWrite(interleaved, start1, size1, start2, size2);

    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : L;

    // Write interleaved L/R into the ring buffer (two segments due to wrap)
    int written = 0;
    auto fill = [&](int start, int size)
    {
        for (int i = 0; i < size && written < numFrames; i += 2, ++written)
        {
            ringData[(size_t)(start + i)]     = L[written];
            ringData[(size_t)(start + i + 1)] = R[written];
        }
    };
    fill(start1, size1);
    fill(start2, size2);

    ringFifo.finishedWrite(written * 2);
}

void BroadcastEngine::run()
{
    // Encode in chunks of 1152 frames (LAME's native MP3 frame size)
    constexpr int kFrames = 1152;
    std::vector<float>         left(kFrames), right(kFrames);
    std::vector<unsigned char> mp3Buf(kFrames * 4 + 7200);

    auto* sh   = static_cast<shout_t*>(shoutHandle);
    auto* lame = static_cast<lame_global_flags*>(lameHandle);

    while (!threadShouldExit())
    {
        int avail = ringFifo.getNumReady() / 2;   // stereo frames

        if (avail < kFrames)
        {
            wait(5);   // sleep 5 ms and try again
            continue;
        }

        int start1, size1, start2, size2;
        ringFifo.prepareToRead(kFrames * 2, start1, size1, start2, size2);

        // De-interleave into L/R buffers
        int frame = 0;
        auto deInterleave = [&](int start, int size)
        {
            for (int i = 0; i < size && frame < kFrames; i += 2, ++frame)
            {
                left [frame] = ringData[(size_t)(start + i)];
                right[frame] = ringData[(size_t)(start + i + 1)];
            }
        };
        deInterleave(start1, size1);
        deInterleave(start2, size2);
        ringFifo.finishedRead(frame * 2);

        int mp3Bytes = lame_encode_buffer_ieee_float(
            lame, left.data(), right.data(), frame,
            mp3Buf.data(), (int)mp3Buf.size());

        if (mp3Bytes > 0)
        {
            if (shout_send(sh, mp3Buf.data(), (size_t)mp3Bytes) != SHOUTERR_SUCCESS)
            {
                setStatus(juce::String("Send error: ") + shout_get_error(sh));
                streaming.store(false);
                break;
            }
            shout_sync(sh);  // rate-control so we don't flood the server
        }
    }
}
