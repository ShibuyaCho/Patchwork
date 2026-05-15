#include "NIDeviceActivator.h"
#include "MidiController.h"
#include <juce_events/juce_events.h>
#include <libusb-1.0/libusb.h>
#include <sys/stat.h>
#include <unistd.h>

// ── NI vendor ID ─────────────────────────────────────────────────────────────
static constexpr uint16_t kNIVendor = 0x17cc;

// ── Known device table ────────────────────────────────────────────────────────
struct NIProfile
{
    uint16_t    pid;
    const char* name;
    bool        confirmed;          // activation confirmed by community
    // USB control transfer fields (sent to interface 0)
    uint8_t     requestType;
    uint8_t     request;
    uint16_t    value;
    uint16_t    index;
    // Optional extra data byte (0 = no data)
    uint8_t     dataByte;
    bool        hasData;
};

static const NIProfile kProfiles[] = {
    // ── Maschine Mikro MK1 ──────────────────────────────────────────────
    { 0x0808, "Maschine Mikro MK1",
      true,  0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine MK1 ────────────────────────────────────────────────────
    { 0x0839, "Maschine MK1",
      true,  0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine Mikro MK2 ──────────────────────────────────────────────
    // Activation via vendor control transfer — widely confirmed working
    { 0x1300, "Maschine Mikro MK2",
      true,  0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine MK2 ────────────────────────────────────────────────────
    { 0x1140, "Maschine MK2",
      true,  0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine Jam ────────────────────────────────────────────────────
    { 0x1500, "Maschine Jam",
      false, 0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine MK3 ────────────────────────────────────────────────────
    // Uses a vendor SET_REPORT-style request; experimental based on USB captures
    { 0x1600, "Maschine MK3",
      false, 0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine Mikro MK3 ──────────────────────────────────────────────
    { 0x1700, "Maschine Mikro MK3",
      false, 0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Maschine+ (standalone) ───────────────────────────────────────────
    { 0x1e00, "Maschine+",
      false, 0x40, 0x29, 0x0100, 0x0000, 0, false },

    // ── Komplete Kontrol A series ────────────────────────────────────────
    { 0x1610, "Komplete Kontrol A25",    true, 0x40, 0x29, 0x0100, 0, 0, false },
    { 0x1620, "Komplete Kontrol A49/61", true, 0x40, 0x29, 0x0100, 0, 0, false },

    // ── Komplete Kontrol M32 ─────────────────────────────────────────────
    { 0x1630, "Komplete Kontrol M32",    true, 0x40, 0x29, 0x0100, 0, 0, false },

    // ── Komplete Kontrol S MK2 ───────────────────────────────────────────
    { 0x1010, "Komplete Kontrol S MK2 (25/49)", true, 0x40, 0x29, 0x0100, 0, 0, false },
    { 0x1020, "Komplete Kontrol S MK2 (61/88)", true, 0x40, 0x29, 0x0100, 0, 0, false },
};

static const NIProfile* findProfile(uint16_t pid)
{
    for (auto& p : kProfiles)
        if (p.pid == pid) return &p;
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// NIDeviceActivator
// ─────────────────────────────────────────────────────────────────────────────

NIDeviceActivator::NIDeviceActivator()
    : juce::Thread("NIDeviceActivator")
{
    libusb_context* ctx = nullptr;
    if (libusb_init(&ctx) == 0)
        libusbCtx = ctx;
}

NIDeviceActivator::~NIDeviceActivator()
{
    stopThread(3000);
    if (libusbCtx)
        libusb_exit(static_cast<libusb_context*>(libusbCtx));
}

// ── Scan ─────────────────────────────────────────────────────────────────────

juce::Array<NIDeviceInfo> NIDeviceActivator::scan()
{
    juce::Array<NIDeviceInfo> result;
    if (!libusbCtx) return result;

    auto* ctx = static_cast<libusb_context*>(libusbCtx);

    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) return result;

    for (ssize_t i = 0; i < cnt; ++i)
    {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;
        if (desc.idVendor != kNIVendor) continue;

        const NIProfile* p = findProfile(desc.idProduct);

        NIDeviceInfo info;
        info.vendorId  = desc.idVendor;
        info.productId = desc.idProduct;
        info.confirmed = p ? p->confirmed : false;

        if (p)
        {
            info.name        = p->name;
            info.canActivate = true;
        }
        else
        {
            // Unknown NI product — still surface it
            info.name        = "NI USB device (0x"
                             + juce::String::toHexString(desc.idProduct).toUpperCase() + ")";
            info.canActivate = false;
        }

        result.add(info);
    }

    libusb_free_device_list(list, 1);
    return result;
}

// ── Background activation ─────────────────────────────────────────────────────

void NIDeviceActivator::activateAsync(std::function<void(NIActivationResult)> onComplete)
{
    if (isThreadRunning()) return;
    monitorMode = false;
    pendingCallback = std::move(onComplete);
    startThread(juce::Thread::Priority::low);
}

// ── HID monitor ───────────────────────────────────────────────────────────────

void NIDeviceActivator::startHIDMonitor(MidiController& controller)
{
    if (isThreadRunning()) return;
    hidController = &controller;
    monitorMode   = true;
    {
        juce::ScopedLock sl(packetLock);
        packetBuffer.clear();
        lastPacketLen = 0;
    }
    std::memset(prevButtonBytes,  0, sizeof(prevButtonBytes));
    std::memset(padActive,        0, sizeof(padActive));
    std::memset(padOverrideByte,  0, sizeof(padOverrideByte));
    buttonBaselineSet = false;
    encoderPosSet     = false;
    encoderAccum      = 64;
    prevStripVal      = 0;
    probeLEDRequest.store(-1);
    ledsDirty.store(true);   // send current ledIdle on first loop iteration
    startThread(juce::Thread::Priority::normal);
}

void NIDeviceActivator::stopHIDMonitor()
{
    monitorMode = false;
    stopThread(2000);
}

juce::Array<NIHIDPacket> NIDeviceActivator::getAndClearPackets()
{
    juce::ScopedLock sl(packetLock);
    juce::Array<NIHIDPacket> out = packetBuffer;
    packetBuffer.clear();
    return out;
}

void NIDeviceActivator::run()
{
    if (monitorMode) { runMonitor(); return; }
    runActivation();
}

void NIDeviceActivator::runActivation()
{
    NIActivationResult result;
    juce::String& log = result.log;

    if (!libusbCtx)
    {
        result.success = false;
        log = "libusb not initialised.\n";
        juce::MessageManager::callAsync([this, result] { if (pendingCallback) pendingCallback(result); });
        return;
    }

    auto* ctx = static_cast<libusb_context*>(libusbCtx);

    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0)
    {
        result.success = false;
        log = "libusb_get_device_list failed: " + juce::String(libusb_strerror((libusb_error)cnt)) + "\n";
        juce::MessageManager::callAsync([this, result] { if (pendingCallback) pendingCallback(result); });
        return;
    }

    int activated = 0;

    for (ssize_t i = 0; i < cnt; ++i)
    {
        if (threadShouldExit()) break;

        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;
        if (desc.idVendor != kNIVendor) continue;

        const NIProfile* p = findProfile(desc.idProduct);
        if (!p) continue;

        log += "Found: " + juce::String(p->name) + "\n";

        libusb_device_handle* handle = nullptr;
        int err = libusb_open(list[i], &handle);

        if (err == LIBUSB_ERROR_ACCESS)
        {
            log += "  Permission denied — udev rule needed.\n";
            result.needsUdevRule = true;
            result.udevRuleText  = getUdevRuleText();
            continue;
        }
        if (err != 0)
        {
            log += "  Open failed: " + juce::String(libusb_strerror((libusb_error)err)) + "\n";
            continue;
        }

        // Detach kernel HID driver from interface 0 so we can send control transfers
        bool detached = false;
        if (libusb_kernel_driver_active(handle, 0) == 1)
        {
            if (libusb_detach_kernel_driver(handle, 0) == 0)
                { detached = true; log += "  Detached kernel driver\n"; }
            else
                log += "  Warning: could not detach kernel driver\n";
        }

        libusb_claim_interface(handle, 0);

        const bool isMK3Family = (desc.idProduct == 0x1600 ||
                                  desc.idProduct == 0x1700 ||
                                  desc.idProduct == 0x1e00);
        bool needReset = false;

        if (isMK3Family)
        {
            // ── MK3 family: uses interrupt OUT endpoint, not control transfers ──
            log += "  MK3 family: locating interrupt OUT endpoint...\n";

            // Find interrupt OUT endpoint on interface 0
            uint8_t outEp = 0;
            libusb_config_descriptor* cfgDesc = nullptr;
            if (libusb_get_active_config_descriptor(list[i], &cfgDesc) == 0 && cfgDesc)
            {
                if (cfgDesc->bNumInterfaces > 0)
                {
                    const auto& alt = cfgDesc->interface[0].altsetting[0];
                    for (int ep = 0; ep < alt.bNumEndpoints; ++ep)
                    {
                        const auto& epDesc = alt.endpoint[ep];
                        bool isOut  = (epDesc.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
                                       == LIBUSB_ENDPOINT_OUT;
                        bool isIntr = (epDesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
                                       == LIBUSB_TRANSFER_TYPE_INTERRUPT;
                        if (isOut && isIntr) { outEp = epDesc.bEndpointAddress; break; }
                    }
                }
                libusb_free_config_descriptor(cfgDesc);
            }

            if (outEp == 0)
            {
                log += "  No interrupt OUT endpoint found on interface 0.\n";
            }
            else
            {
                log += juce::String::formatted("  Found interrupt OUT ep 0x%02x\n", outEp);

                // Try every known mode-switch payload — the exact bytes that trigger
                // MIDI class mode are firmware-dependent; send all and then reset.
                const struct { uint8_t data[29]; const char* label; } attempts[] = {
                    { {0x01, 0x00}, "01/00" },
                    { {0x01, 0x04}, "01/04" },
                    { {0x01, 0x08}, "01/08" },
                    { {0x02, 0x08}, "02/08" },
                    { {0x05, 0x01}, "05/01" },
                };

                bool anyOk = false;
                for (auto& a : attempts)
                {
                    if (threadShouldExit()) break;
                    uint8_t buf[29] = {};
                    std::memcpy(buf, a.data, sizeof(a.data));
                    int transferred = 0;
                    int ret = libusb_interrupt_transfer(handle, outEp,
                                                        buf, (int)sizeof(buf),
                                                        &transferred, 1500);
                    log += juce::String("  [") + a.label + "] "
                         + (ret >= 0 ? juce::String("OK (") + juce::String(transferred) + "b)"
                                     : juce::String(libusb_strerror((libusb_error)ret)))
                         + "\n";
                    if (ret >= 0) anyOk = true;
                }

                if (anyOk)
                {
                    // Send USB reset — device re-enumerates; may expose MIDI if
                    // any payload matched the firmware's mode-switch command.
                    needReset = true;
                    ++activated;
                    result.success = true;
                    log += "  Commands sent + USB reset — check MIDI ports list.\n"
                           "  If no Maschine port appears, the firmware needs a\n"
                           "  different command sequence. See Diagnostics panel.\n";
                }
            }

            if (!result.success)
            {
                log += "\n  Could not reach the interrupt OUT endpoint.\n"
                       "  See Diagnostics panel for working alternatives.\n";
            }
        }
        else
        {
            // ── MK1 / MK2 / Komplete Kontrol: confirmed vendor control transfer ──
            log += "  Sending MIDI mode activation ("
                 + juce::String(p->confirmed ? "confirmed" : "experimental") + ")...\n";

            uint8_t  dataArr[1] = { p->dataByte };
            uint8_t* dataPtr    = p->hasData ? dataArr : nullptr;
            int      dataLen    = p->hasData ? 1 : 0;

            int xfer = libusb_control_transfer(handle,
                                               p->requestType,
                                               p->request,
                                               p->value,
                                               p->index,
                                               dataPtr,
                                               (uint16_t)dataLen,
                                               5000);
            if (xfer >= 0)
            {
                log += "  Control transfer OK (" + juce::String(xfer) + " bytes)\n";
                ++activated;
                result.success = true;
                log += "  Device should reconnect as USB-MIDI in ~2 s\n";
            }
            else
            {
                log += "  Transfer failed: " + juce::String(libusb_strerror((libusb_error)xfer)) + "\n";
                if (xfer == LIBUSB_ERROR_PIPE && !p->confirmed)
                    log += "  (Experimental — may still activate on replug)\n";
            }
        }

        libusb_release_interface(handle, 0);
        if (detached)
            libusb_attach_kernel_driver(handle, 0);

        if (needReset)
        {
            int rr = libusb_reset_device(handle);
            log += juce::String("  USB reset: ")
                 + (rr == 0 ? "OK — device re-enumerating"
                             : juce::String(libusb_strerror((libusb_error)rr)))
                 + "\n";
        }

        libusb_close(handle);
    }

    libusb_free_device_list(list, 1);

    if (activated == 0 && !result.needsUdevRule)
        log += "No activatable NI devices found.\n";

    if (result.success)
    {
        log += "\nWaiting for device to re-enumerate...\n";
        for (int w = 0; w < 40 && !threadShouldExit(); ++w)
            juce::Thread::sleep(100);
        log += "Done — check MIDI ports list.\n";
    }

    juce::MessageManager::callAsync([this, result] { if (pendingCallback) pendingCallback(result); });
}

// ── 8×8 pixel font (A–Z only) ────────────────────────────────────────────────

static const uint8_t kFont8x8[26][8] = {
    { 0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00 }, // A
    { 0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00 }, // B
    { 0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00 }, // C
    { 0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00 }, // D
    { 0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00 }, // E
    { 0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00 }, // F
    { 0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00 }, // G
    { 0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00 }, // H
    { 0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00 }, // I
    { 0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00 }, // J
    { 0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00 }, // K
    { 0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00 }, // L
    { 0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00 }, // M
    { 0x63,0x73,0x7B,0x6F,0x67,0x63,0x63,0x00 }, // N
    { 0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00 }, // O
    { 0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00 }, // P
    { 0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00 }, // Q
    { 0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00 }, // R
    { 0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00 }, // S
    { 0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00 }, // T
    { 0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00 }, // U
    { 0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00 }, // V
    { 0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00 }, // W
    { 0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00 }, // X
    { 0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00 }, // Y
    { 0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00 }, // Z
};

// Render uppercase ASCII text into a 128×64 row-major 1-bpp bitmap (16 bytes/row).
static void renderText8x8(uint8_t* bm, int x, int y, const char* text)
{
    for (int ci = 0; text[ci]; ++ci, x += 8)
    {
        char c = (char)(text[ci] & 0xDF);   // to uppercase
        if (c < 'A' || c > 'Z') continue;
        const uint8_t* g = kFont8x8[c - 'A'];
        for (int row = 0; row < 8; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                if (!(g[row] & (0x80u >> col))) continue;
                int px = x + col, py = y + row;
                if (px < 0 || px >= 128 || py < 0 || py >= 64) continue;
                bm[py * 16 + px / 8] |= (uint8_t)(0x80u >> (px % 8));
            }
        }
    }
}

// ── Diagnostic log ────────────────────────────────────────────────────────────

void NIDeviceActivator::appendMonitorLog(const juce::String& msg)
{
    juce::ScopedLock sl(monitorLogLock);
    monitorLogStr += msg;
}

juce::String NIDeviceActivator::getAndClearMonitorLog()
{
    juce::ScopedLock sl(monitorLogLock);
    juce::String out = monitorLogStr;
    monitorLogStr.clear();
    return out;
}

// ── LED output ────────────────────────────────────────────────────────────────

void NIDeviceActivator::setLEDByte(int idx, uint8_t val)
{
    if (idx < 0 || idx >= kLEDCount) return;
    juce::ScopedLock sl(ledLock);
    ledIdle[idx] = val;
    ledsDirty.store(true);
}

void NIDeviceActivator::setAllLEDs(const uint8_t data[80])
{
    juce::ScopedLock sl(ledLock);
    std::memcpy(ledIdle, data, kLEDCount);
    ledsDirty.store(true);
}

void NIDeviceActivator::requestProbeLED(int idx)
{
    probeLEDRequest.store(idx);
}

// Sends the current LED state (ledIdle + active pad overrides) as an 81-byte report.
void NIDeviceActivator::sendLEDReport(void* handle)
{
    auto* h = static_cast<libusb_device_handle*>(handle);
    ledsDirty.store(false);

    uint8_t buf[81];
    buf[0] = 0x80;
    {
        juce::ScopedLock sl(ledLock);
        std::memcpy(buf + 1, ledIdle, 80);
    }
    // Apply pad-press brightness overrides
    for (int i = 0; i < 16; ++i)
    {
        if (padActive[i])
            buf[1 + padInputToLEDIdx(i)] = padOverrideByte[i];
    }
    int t = 0;
    libusb_interrupt_transfer(h, 0x01, buf, 81, &t, 150);
}

// ── Display output ────────────────────────────────────────────────────────────

// Display format (confirmed from open-source MK3 drivers):
// Two 265-byte packets split at the 32-row midpoint.
// Header: [0xE0, 0x00, 0x00, ROW_OFFSET, 0x00, 0x80, 0x00, 0x02, 0x00]
//   ROW_OFFSET = 0x00 for top half, 0x02 for bottom half.
// Data: 256 bytes encoding 128 columns × 32 rows in band-column order.
//   band = localRow/8, bit = localRow%8
//   byteIndex = band*128 + col
//   Clear bit (AND ~mask) = white pixel; set bit (OR mask) = black pixel.
//   Initialize to 0xFF = black background; clear bits to draw white.
void NIDeviceActivator::sendDisplay(void* handle)
{
    auto* h = static_cast<libusb_device_handle*>(handle);

    // 512-byte pixel buffer: [0..255]=top half, [256..511]=bottom half, 0xFF=black bg
    uint8_t pix[512];
    std::memset(pix, 0xFF, sizeof(pix));

    // Helper: set a white pixel at (col, row) in the 128×64 display
    auto setWhite = [&](int col, int row) {
        if (col < 0 || col >= 128 || row < 0 || row >= 64) return;
        uint8_t* half = pix + (row >= 32 ? 256 : 0);
        int localRow = row % 32;
        int band = localRow / 8;
        int bit  = localRow % 8;
        half[band * 128 + col] &= ~(uint8_t)(1u << bit);
    };

    // Render "PATCHWORK" centred
    const char* text = "PATCHWORK";
    int textW = (int)std::strlen(text) * 8;
    int startX = (128 - textW) / 2;
    int startY = (64 - 8) / 2;

    for (int ci = 0; text[ci]; ++ci)
    {
        char c = (char)(text[ci] & ~0x20);
        if (c < 'A' || c > 'Z') continue;
        const uint8_t* g = kFont8x8[c - 'A'];
        for (int row = 0; row < 8; ++row)
            for (int col = 0; col < 8; ++col)
                if (g[row] & (0x80u >> col))
                    setWhite(startX + ci * 8 + col, startY + row);
    }

    // Horizontal rules above and below text
    for (int col = 12; col < 116; ++col)
    {
        setWhite(col, startY - 4);
        setWhite(col, startY + 12);
    }

    // Send top half then bottom half
    uint8_t pkt[265];
    static const uint8_t kHdrBase[9] = { 0xE0, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x02, 0x00 };
    std::memcpy(pkt, kHdrBase, 9);
    int t = 0;

    pkt[3] = 0x00;
    std::memcpy(pkt + 9, pix, 256);
    int r1 = libusb_interrupt_transfer(h, 0x01, pkt, 265, &t, 1000);

    pkt[3] = 0x02;
    std::memcpy(pkt + 9, pix + 256, 256);
    int r2 = libusb_interrupt_transfer(h, 0x01, pkt, 265, &t, 1000);

    appendMonitorLog(juce::String("Display: top=")
        + (r1 == 0 ? "OK" : libusb_strerror((libusb_error)r1))
        + " bot=" + (r2 == 0 ? "OK" : libusb_strerror((libusb_error)r2)) + "\n");
}

// ── HID → MIDI translation (called on monitor thread, immediately per packet) ─

static constexpr int kPadBaseNote    = 36;   // pad 0 → note 36 … pad 15 → note 51
static constexpr int kButtonBaseNote = 80;   // button 0 → note 80 … button 38 → note 118
static constexpr int kEncoderCC      = 14;   // CC 14 for rotary encoder
static constexpr int kStripCC        = 15;   // CC 15 for touch strip
static constexpr int kEncoderNotePress = 119; // encoder press → note 119

void NIDeviceActivator::translatePacket(const NIHIDPacket& pkt)
{
    if (!hidController || pkt.len < 2) return;
    const uint8_t* d = pkt.data;

    // ── Report 0x01: buttons / encoder / touch strip ──────────────────────────
    if (d[0] == 0x01 && pkt.len >= 8)
    {
        // First packet: record baseline so we don't fire spurious events at startup
        if (!buttonBaselineSet)
        {
            for (int b = 0; b < 5; ++b)
                prevButtonBytes[b] = d[1 + b];
            prevEncoderPos = d[7] & 0x0F;
            encoderPosSet  = true;
            buttonBaselineSet = true;
            return;
        }

        // ── Buttons: bytes 1-5, 39 named buttons + encoder press at bit 39 ──
        for (int b = 0; b < 5; ++b)
        {
            uint8_t prev = prevButtonBytes[b];
            uint8_t curr = d[1 + b];
            uint8_t diff = prev ^ curr;
            if (!diff) continue;

            for (int bit = 0; bit < 8; ++bit)
            {
                if (!(diff & (1u << bit))) continue;
                bool on = (curr & (1u << bit)) != 0;
                int  idx = b * 8 + bit;

                if (idx < 39)  // named button
                {
                    hidController->injectHIDMessage(
                        on ? juce::MidiMessage::noteOn (10, kButtonBaseNote + idx, (uint8_t)100)
                           : juce::MidiMessage::noteOff(10, kButtonBaseNote + idx),
                        "Maschine Mikro MK3");
                }
                else if (idx == 39)  // encoder press
                {
                    hidController->injectHIDMessage(
                        on ? juce::MidiMessage::noteOn (10, kEncoderNotePress, (uint8_t)100)
                           : juce::MidiMessage::noteOff(10, kEncoderNotePress),
                        "Maschine Mikro MK3");
                }
            }
            prevButtonBytes[b] = curr;
        }

        // ── Encoder: byte 7 low nibble, absolute 0-15 position, wraps ────────
        if (pkt.len >= 8)
        {
            uint8_t pos = d[7] & 0x0F;
            if (!encoderPosSet)
            {
                prevEncoderPos = pos;
                encoderPosSet  = true;
            }
            else if (pos != prevEncoderPos)
            {
                int delta = (int)pos - (int)prevEncoderPos;
                if (delta >  7) delta -= 16;
                if (delta < -7) delta += 16;
                encoderAccum = juce::jlimit(0, 127, encoderAccum + delta * 4);
                hidController->injectHIDMessage(
                    juce::MidiMessage::controllerEvent(10, kEncoderCC, encoderAccum),
                    "Maschine Mikro MK3");
                prevEncoderPos = pos;
            }
        }

        // ── Touch strip: bytes 10-11, 16-bit raw (0 = not touched) ───────────
        if (pkt.len >= 12)
        {
            uint16_t stripVal = (uint16_t)d[10] | ((uint16_t)d[11] << 8);
            if (stripVal != prevStripVal)
            {
                prevStripVal = stripVal;
                if (stripVal > 0)
                {
                    int cc = juce::jlimit(0, 127, (int)(stripVal * 127 / 200));
                    hidController->injectHIDMessage(
                        juce::MidiMessage::controllerEvent(10, kStripCC, cc),
                        "Maschine Mikro MK3");
                }
            }
        }
        return;
    }

    // ── Report 0x02: pad pressure ─────────────────────────────────────────────
    // byte 1 = pad index (0-15), bytes 2-3 = big-endian 16-bit pressure
    // 0x4000 = rest, > 0x4000 = initial hit, < 0x4000 = sustained hold
    if (d[0] == 0x02 && pkt.len >= 4)
    {
        int padIdx = d[1];
        if (padIdx < 0 || padIdx > 15) return;

        uint16_t raw  = ((uint16_t)d[2] << 8) | d[3];
        int      note = kPadBaseNote + padIdx;
        int      ledIdx = padInputToLEDIdx(padIdx);

        if (!padActive[padIdx])
        {
            if (raw > 0x4000)
            {
                int32_t excess = (int32_t)raw - 0x4000;
                int vel = juce::jlimit(1, 127, (int)(excess * 127L / 4096L));
                hidController->injectHIDMessage(
                    juce::MidiMessage::noteOn(10, note, (uint8_t)vel),
                    "Maschine Mikro MK3");
                padActive[padIdx] = true;

                // Build press override: same color, full brightness
                uint8_t idle;
                { juce::ScopedLock sl(ledLock); idle = ledIdle[ledIdx]; }
                uint8_t colorIdx = idle >> 2;
                if (colorIdx == 0) colorIdx = 17;  // if idle=off, flash white
                padOverrideByte[padIdx] = (uint8_t)((colorIdx << 2) | 3);
                ledsDirty.store(true);
            }
        }
        else
        {
            if (raw <= 0x4000)
            {
                hidController->injectHIDMessage(
                    juce::MidiMessage::noteOff(10, note),
                    "Maschine Mikro MK3");
                padActive[padIdx] = false;
                ledsDirty.store(true);
            }
        }
    }
}

// ── HID monitor thread ────────────────────────────────────────────────────────

void NIDeviceActivator::runMonitor()
{
    auto* ctx = static_cast<libusb_context*>(libusbCtx);
    if (!ctx) return;

    // Find and open the first known NI device
    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) return;

    libusb_device_handle* handle = nullptr;
    bool detached = false;

    for (ssize_t i = 0; i < cnt && !handle; ++i)
    {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0) continue;
        if (desc.idVendor != kNIVendor || !findProfile(desc.idProduct)) continue;

        if (libusb_open(list[i], &handle) != 0) { handle = nullptr; continue; }

        if (libusb_kernel_driver_active(handle, 0) == 1)
        {
            if (libusb_detach_kernel_driver(handle, 0) == 0)
                detached = true;
            else
                { libusb_close(handle); handle = nullptr; continue; }
        }

        if (libusb_claim_interface(handle, 0) != 0)
            { libusb_close(handle); handle = nullptr; continue; }
    }
    libusb_free_device_list(list, 1);

    if (!handle) return;

    // Push a sentinel packet so the UI knows monitoring started
    {
        NIHIDPacket sentinel;
        sentinel.len = 0;   // len==0 signals "started"
        juce::ScopedLock sl(packetLock);
        packetBuffer.add(sentinel);
    }

    // ── Read HID report descriptor to find valid output report IDs ──────────
    {
        uint8_t rdesc[512] = {};
        int rlen = libusb_control_transfer(handle,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE,
            LIBUSB_REQUEST_GET_DESCRIPTOR,
            (uint16_t)(0x22 << 8),   // HID Report Descriptor, index 0
            0,                       // interface 0
            rdesc, (uint16_t)sizeof(rdesc), 2000);

        if (rlen > 0)
        {
            juce::String ids = "HID report IDs found:";
            for (int i = 0; i < rlen - 1; ++i)
                if (rdesc[i] == 0x85)   // REPORT_ID global tag
                    ids += " 0x" + juce::String::toHexString(rdesc[i + 1]).toUpperCase();
            appendMonitorLog(ids + "\n");
        }
        else
        {
            appendMonitorLog("HID descriptor read failed (err "
                + juce::String(rlen) + ")\n");
        }
    }

    // ── Try 0xD0 as host-mode init (puts device under host LED/display control) ─
    {
        auto tryInit = [&](uint8_t b1, const char* label) {
            uint8_t buf2[2] = { 0xD0, b1 };
            int t = 0;
            int r = libusb_interrupt_transfer(handle, 0x01, buf2, 2, &t, 200);
            appendMonitorLog(juce::String("Init 0xD0/") + label
                + ": " + (r == 0 ? "OK" : libusb_strerror((libusb_error)r)) + "\n");
            juce::Thread::sleep(50);
        };
        tryInit(0x00, "0x00");
        tryInit(0x01, "0x01");
        tryInit(0x04, "0x04");
    }

    // ── Send initial LED state ────────────────────────────────────────────────
    sendLEDReport(handle);

    // ── Send display ──────────────────────────────────────────────────────────
    sendDisplay(handle);

    uint8_t buf[64] = {};
    while (!threadShouldExit())
    {
        // ── Probe request: flash one LED so user can identify it ─────────────
        int req = probeLEDRequest.exchange(-1);
        if (req >= 0 && req < kLEDCount)
        {
            uint8_t probeBuf[81];
            probeBuf[0] = 0x80;
            { juce::ScopedLock sl(ledLock); std::memcpy(probeBuf + 1, ledIdle, 80); }
            // Max brightness: pad LEDs use white+bright (0x47), others use 0x7F
            probeBuf[1 + req] = (req >= kLEDPadBase && req < kLEDStripBase)
                                ? (uint8_t)((17 << 2) | 3)  // white full-bright
                                : 0x7F;
            int t = 0;
            libusb_interrupt_transfer(handle, 0x01, probeBuf, 81, &t, 150);
            for (int ms = 0; ms < 700 && !threadShouldExit(); ms += 10)
                juce::Thread::sleep(10);
            sendLEDReport(handle);
        }

        // ── Push LED state if it changed ──────────────────────────────────────
        if (ledsDirty.load())
            sendLEDReport(handle);

        // ── Read next HID packet ──────────────────────────────────────────────
        int transferred = 0;
        int ret = libusb_interrupt_transfer(handle, 0x81,
                                            buf, (int)sizeof(buf),
                                            &transferred, 100);

        if (ret != 0 || transferred <= 0) continue;

        // Deduplicate
        bool changed = (transferred != lastPacketLen)
                    || (std::memcmp(buf, lastPacketData, (size_t)transferred) != 0);
        if (!changed) continue;

        std::memcpy(lastPacketData, buf, (size_t)transferred);
        lastPacketLen = transferred;

        NIHIDPacket pkt;
        pkt.len = transferred;
        std::memcpy(pkt.data, buf, (size_t)transferred);

        // Translate immediately — sets ledsDirty if pad state changed
        translatePacket(pkt);

        {
            juce::ScopedLock sl(packetLock);
            packetBuffer.add(pkt);
            while (packetBuffer.size() > 200)
                packetBuffer.remove(0);
        }
    }

    libusb_release_interface(handle, 0);
    if (detached) libusb_attach_kernel_driver(handle, 0);
    libusb_close(handle);
}

// ── udev rule helpers ─────────────────────────────────────────────────────────

juce::String NIDeviceActivator::getUdevRuleText()
{
    return
        "# Native Instruments — grant non-root USB + HID access\n"
        "# Save this file to /etc/udev/rules.d/99-native-instruments.rules\n"
        "# then run:  sudo udevadm control --reload && sudo udevadm trigger\n"
        "\n"
        "SUBSYSTEM==\"usb\",    ATTRS{idVendor}==\"17cc\", MODE=\"0666\", GROUP=\"plugdev\"\n"
        "SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"17cc\", MODE=\"0666\", GROUP=\"plugdev\"\n";
}

juce::String NIDeviceActivator::writeUdevRule()
{
    juce::File ruleDir("/etc/udev/rules.d");
    if (!ruleDir.isDirectory())
        return "udev rules directory not found (/etc/udev/rules.d)";

    juce::File ruleFile = ruleDir.getChildFile("99-native-instruments.rules");
    if (!ruleFile.replaceWithText(getUdevRuleText()))
        return "Could not write " + ruleFile.getFullPathName()
             + " — try: sudo tee " + ruleFile.getFullPathName();

    return {};  // success
}

juce::String NIDeviceActivator::applyUdevRules()
{
    int r1 = system("udevadm control --reload-rules 2>/dev/null");
    int r2 = system("udevadm trigger             2>/dev/null");
    if (r1 != 0 || r2 != 0)
        return "udevadm returned non-zero — try running the commands manually as root";
    return {};
}
