/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_msumr.hpp"
#include "meteor_lrpt_g4_service.hpp"
#include "file.hpp"
#include "jpeg_decode.hpp"
#include "meteor_lrpt_g4_preview.hpp"
#include "msumr_demux.hpp"
#include "portapack_shared_memory.hpp"

#include "rtc_time.hpp"
#include "string_format.hpp"

#include "hal.h"
#include "ch.h"

#include <array>
#include <cstring>
#include <string>

namespace {

meteor_lrpt_g4::MsumrDemux g_demux{};

static void copy_path_utf8_volatile(volatile char* dst, const size_t dst_max, const std::filesystem::path& path) {
    if (!dst || dst_max == 0)
        return;
    const std::string s = path.string();
    size_t i = 0;
    for (; i + 1 < dst_max && i < s.size(); ++i)
        dst[i] = s[i];
    for (; i < dst_max; ++i)
        dst[i] = '\0';
    __DMB();
}

std::filesystem::path g4_resolved_cadu_path() {
    char tmp[SharedMemory::MeteorLrptG4Ipc::kInputPathUtf8Max];
    auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    for (size_t i = 0; i < sizeof(tmp); i++) {
        const char c = g4.input_path_utf8[i];
        tmp[i] = c;
        if (c == '\0')
            break;
    }
    tmp[sizeof(tmp) - 1] = '\0';
    if (tmp[0] == '\0')
        /* Matches `RecordView` CADU raw extension (see `ui_record_view.cpp`, RawS8 → `.C8`). */
        return std::filesystem::path{u"/LRPT/g4_cadu.C8"};
    return std::filesystem::path{std::string_view{tmp}};
}

/** M0 consumer: sole writer of `ring_pop` (SPSC with `meteor_lrpt_g4_ring_push_post_rs` on M4). */
bool g4_ring_pop(std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& out) {
    auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    __DMB();
    const uint32_t w = g4.ring_push;
    const uint32_t r = g4.ring_pop;
    if (w == r)
        return false;
    const uint32_t slot = r % SharedMemory::MeteorLrptG4Ipc::kLiveRingSlots;
    std::memcpy(out.data(), g4.live_ring_slots[slot], meteor_lrpt::kMeteorCaduRecBytes);
    g4.ring_pop = r + 1;
    __DMB();
    return true;
}

void g4_jpeg_handler(void* /*ctx*/, const uint16_t apid, const uint8_t* jpeg, const size_t len) {
    auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    if (!jpeg || len < 4)
        return;

    (void)ensure_directory(u"/LRPT");

    const std::string ap = to_string_dec_uint((apid == 0xFFFFu) ? 0u : (uint32_t)apid, (apid > 999u) ? 4 : 3);
    const std::string name = "msumr_" + ap + "_" + to_string_timestamp(rtc_time::now()) + ".bmp";
    const auto path = std::filesystem::path{u"/LRPT"} / name;

    std::filesystem::path ppm_sidecar{};
    const std::filesystem::path* ppm_arg = nullptr;
    if ((g4.debug_flags & 1u) != 0) {
        std::string ppm_name = name;
        if (ppm_name.size() > 4 && ppm_name.compare(ppm_name.size() - 4, 4, ".bmp") == 0)
            ppm_name.replace(ppm_name.size() - 4, 4, ".ppm");
        else
            ppm_name.append(".ppm");
        ppm_sidecar = std::filesystem::path{u"/LRPT"} / ppm_name;
        ppm_arg = &ppm_sidecar;
    }

    uint32_t local_drops = 0;
    uint8_t jr = 0;
    const bool ok = meteor_lrpt_g4::decode_jpeg_to_new_bmp_file(jpeg, len, path, local_drops, jr, ppm_arg);
    (void)jr;
    g4.drop_bits |= local_drops;
    if (ok) {
        g4.jpeg_ok_count++;
        g4.bmp_write_count++;
        copy_path_utf8_volatile(g4.last_bmp_utf8, sizeof(g4.last_bmp_utf8), path);
        (void)meteor_lrpt_g4::publish_decoded_bmp_preview(g4, path);
    }
    g4.last_jresult = jr;
}

static WORKING_AREA(wa_g4_msumr, 1024);

static msg_t g4_worker_thd(void* /*arg*/) {
    chRegSetThreadName("g4_msumr");
    File cadu_file{};
    uint64_t cadu_off{0};
    bool cadu_open = false;
    uint16_t last_sd_deint{0};
    uint8_t sd_backoff_count{0};

    for (;;) {
        chThdSleepMilliseconds(120);
        auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
        auto& ipc = shared_memory.meteor_lrpt_ipc;
        g4.magic = SharedMemory::MeteorLrptG4Ipc::kMagic;

        if (!g4.enabled) {
            if (cadu_open) {
                cadu_file.close();
                cadu_open = false;
            }
            cadu_off = 0;
            g_demux.reset();
            sd_backoff_count = 0;
            continue;
        }

        const uint16_t sd_now = ipc.sd_deint_errors;
        if (sd_now >= 8u && sd_now > last_sd_deint && (uint16_t)(sd_now - last_sd_deint) >= 3u)
            sd_backoff_count = 8u;
        last_sd_deint = sd_now;
        if (sd_backoff_count > 0) {
            sd_backoff_count--;
            chThdSleepMilliseconds(250);
            continue;
        }

        std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes> cadu{};
        bool got = false;
        if (g4.live_ring_enable != 0)
            got = g4_ring_pop(cadu);

        if (!got) {
            if (!cadu_open) {
                const auto path = g4_resolved_cadu_path();
                const auto oe = cadu_file.open(path, true, false);
                if (oe.is_valid()) {
                    cadu_open = false;
                    continue;
                }
                cadu_open = true;
                cadu_off = 0;
                g_demux.reset();
            }

            const auto sz = cadu_file.size();
            if (cadu_off > sz) {
                cadu_off = 0;
                g_demux.reset();
            }
            if (sz < cadu_off + meteor_lrpt::kMeteorCaduRecBytes) {
                continue;
            }

            const auto sk = cadu_file.seek(cadu_off);
            if (sk.is_error()) {
                g4.drop_bits |= G4_DROP_CADU_IO;
                cadu_file.close();
                cadu_open = false;
                continue;
            }

            const auto rd = cadu_file.read(cadu.data(), cadu.size());
            if (rd.is_error() || rd.value() < cadu.size()) {
                g4.drop_bits |= G4_DROP_CADU_IO;
                cadu_file.close();
                cadu_open = false;
                continue;
            }
            cadu_off += meteor_lrpt::kMeteorCaduRecBytes;
            got = true;
        }

        if (!got)
            continue;

        g4.cadus_processed++;

        uint32_t drop = 0;
        g_demux.feed_cadu_1020(cadu.data(), g4_jpeg_handler, nullptr, drop);
        g4.drop_bits |= drop;
    }
    return 0;
}

}  // namespace

void meteor_lrpt_g4_init() {
    static bool started = false;
    if (started)
        return;
    started = true;
    chThdCreateStatic(wa_g4_msumr, sizeof(wa_g4_msumr), LOWPRIO - 2, g4_worker_thd, nullptr);
}

void meteor_lrpt_g4_configure(const uint8_t lrpt_flags) {
    auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    g4.lrpt_flags_snapshot = lrpt_flags;
    const bool on = (lrpt_flags & kMeteorLrptFlagG4Decode) != 0;
    g4.enabled = on ? 1u : 0u;
    g4.live_ring_enable = (on && (lrpt_flags & kMeteorLrptFlagG4LiveRing) != 0) ? 1u : 0u;
    if (!on) {
        g4.ring_push = 0;
        g4.ring_pop = 0;
    }
    g4.magic = SharedMemory::MeteorLrptG4Ipc::kMagic;
}

void meteor_lrpt_g4_set_input_path_utf8(const char* path_utf8) {
    auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    if (!path_utf8 || path_utf8[0] == '\0') {
        for (size_t i = 0; i < sizeof(g4.input_path_utf8); i++)
            g4.input_path_utf8[i] = '\0';
        __DMB();
        return;
    }
    size_t i = 0;
    for (; i + 1 < sizeof(g4.input_path_utf8) && path_utf8[i]; i++)
        g4.input_path_utf8[i] = path_utf8[i];
    for (; i < sizeof(g4.input_path_utf8); i++)
        g4.input_path_utf8[i] = '\0';
    __DMB();
}

void meteor_lrpt_g4_set_trace_flags(const uint8_t trace_flags) {
    shared_memory.meteor_lrpt_g4_ipc.debug_flags = trace_flags;
    __DMB();
}
