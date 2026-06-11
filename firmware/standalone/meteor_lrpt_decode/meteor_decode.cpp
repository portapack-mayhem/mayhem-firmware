/*
 * Copyright (C) 2026
 *
 * Offline soft .C8 → CADU .C8 (Meteor LRPT Decode standalone).
 */
#include "ff.h"

#include "meteor_decode.hpp"
#include "meteor_standalone_ui.hpp"
#include "standalone_app.hpp"
#include "meteor_offline_fec.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <new>

namespace {
const standalone_application_api_t* api{nullptr};
uint16_t screen_w{240};
uint16_t screen_h{320};
char status_line[64]{};
uint8_t pm_flags{meteor_lrpt::kPmFlagM2x};

/* FatFs objects must match host FIL/FILINFO size; keep off UI thread stack (~368 B free). */
static FIL s_fil_in{};
static FIL s_fil_out{};
static FIL s_fil_ini{};
static FILINFO s_fno{};
static DIR s_dir{};
static TCHAR s_path[32]{};

alignas(meteor_lrpt::OfflineFecDecoder) static uint8_t dec_storage[sizeof(meteor_lrpt::OfflineFecDecoder)];
static meteor_lrpt::OfflineFecDecoder* dec_inst{nullptr};
static std::array<int8_t, meteor_lrpt::OfflineFecDecoder::kSoftBlock16384> g_soft{};
static std::array<uint8_t, meteor_lrpt::OfflineFecDecoder::kCaduRecBytes> g_cadu{};

void set_status_ascii(const char* msg) {
    size_t i = 0;
    for (; msg[i] && i + 1 < sizeof(status_line); i++)
        status_line[i] = msg[i];
    status_line[i] = '\0';
}

void draw_ui() {
    meteor_standalone_ui::draw_app_ui(
        screen_w,
        screen_h,
        status_line,
        "In:  /LRPT/SOFT*.C8",
        "Out: /LRPT/dec_cadu.C8",
        "SELECT: run decode");
}

void ensure_decoder() {
    if (dec_inst)
        return;
    dec_inst = new (dec_storage) meteor_lrpt::OfflineFecDecoder(pm_flags);
}

bool load_pm_flags() {
    if (f_open(&s_fil_ini, (const TCHAR*)u"SETTINGS/rx_meteor_lrpt.ini", FA_READ) != FR_OK)
        return false;
    char buf[96]{};
    UINT br = 0;
    if (f_read(&s_fil_ini, buf, sizeof(buf) - 1, &br) != FR_OK) {
        f_close(&s_fil_ini);
        return false;
    }
    f_close(&s_fil_ini);
    buf[br] = '\0';
    constexpr char kKey[] = "lrpt_flags=";
    const char* p = std::strstr(buf, kKey);
    if (!p)
        return false;
    p += sizeof(kKey) - 1;
    unsigned v = 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10u + static_cast<unsigned>(*p - '0');
        ++p;
    }
    pm_flags = static_cast<uint8_t>(v & 0xFFu);
    return true;
}

static bool build_lrpt_path(const TCHAR* fname) {
    size_t o = 0;
    for (const TCHAR* p = (const TCHAR*)u"/LRPT/"; *p && o + 1 < sizeof(s_path) / sizeof(s_path[0]); ++p)
        s_path[o++] = *p;
    for (const TCHAR* p = fname; *p && o + 1 < sizeof(s_path) / sizeof(s_path[0]); ++p)
        s_path[o++] = *p;
    s_path[o] = 0;
    return o > 6;
}

bool decode_file(const TCHAR* in_path) {
    set_status_ascii("Decoding...");
    draw_ui();
    api->set_dirty();

    ensure_decoder();
    if (!dec_inst) {
        set_status_ascii("Decoder init fail");
        return false;
    }

    if (f_open(&s_fil_in, in_path, FA_READ) != FR_OK) {
        set_status_ascii("Open fail");
        return false;
    }

    if (f_open(&s_fil_out, (const TCHAR*)u"/LRPT/dec_cadu.C8", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        f_close(&s_fil_in);
        set_status_ascii("Out fail");
        return false;
    }

    const size_t block = dec_inst->soft_block_bytes();
    uint32_t blocks = 0;
    uint32_t cadus = 0;

    while (true) {
        UINT br = 0;
        if (f_read(&s_fil_in, g_soft.data(), static_cast<UINT>(block), &br) != FR_OK || br == 0)
            break;
        if (br < block)
            break;
        blocks++;
        if (dec_inst->decode_soft_block(g_soft.data(), br, g_cadu)) {
            UINT bw = 0;
            if (f_write(&s_fil_out, g_cadu.data(), meteor_lrpt::OfflineFecDecoder::kCaduRecBytes, &bw) == FR_OK &&
                bw == meteor_lrpt::OfflineFecDecoder::kCaduRecBytes)
                cadus++;
        }
    }

    f_close(&s_fil_in);
    f_sync(&s_fil_out);
    f_close(&s_fil_out);

    char tmp[64];
    std::snprintf(tmp, sizeof(tmp), "blk:%u CADU:%u", (unsigned)blocks, (unsigned)cadus);
    set_status_ascii(tmp);
    return cadus > 0;
}

bool decode_first_soft_capture() {
    if (f_findfirst(&s_dir, &s_fno, (const TCHAR*)u"/LRPT", (const TCHAR*)u"SOFT*.C8") != FR_OK ||
        s_fno.fname[0] == 0) {
        set_status_ascii("No soft .C8 file");
        return false;
    }
    if (!build_lrpt_path(s_fno.fname))
        return false;
    f_closedir(&s_dir);
    return decode_file(s_path);
}

}  // namespace

extern const standalone_application_api_t* _api;

void meteor_decode_initialize(const standalone_application_api_t& a) {
    _api = &a;
    api = &a;
    meteor_standalone_ui::bind(a);
    screen_w = *(api->screen_width);
    screen_h = *(api->screen_height);
    (void)load_pm_flags();
    set_status_ascii("MeteorDx ready");
    draw_ui();
}

void meteor_decode_on_event(const uint32_t&) {}

void meteor_decode_shutdown() {
    if (dec_inst) {
        dec_inst->~OfflineFecDecoder();
        dec_inst = nullptr;
    }
}

void meteor_decode_paint() {
    draw_ui();
}

bool meteor_decode_on_key(uint8_t key) {
    if (meteor_standalone_ui::key_exit_to_menu(key))
        return true;
    if (key != meteor_standalone_ui::kKeySelect)
        return false;
    ensure_decoder();
    (void)decode_first_soft_capture();
    draw_ui();
    api->set_dirty();
    return true;
}
