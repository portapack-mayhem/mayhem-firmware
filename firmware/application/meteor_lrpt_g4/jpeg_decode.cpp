/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "jpeg_decode.hpp"

#include "bmpfile.hpp"
#include "file.hpp"

extern "C" {
#include "tjpgd.h"
}

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>

namespace meteor_lrpt_g4 {

namespace {

constexpr uint32_t kDropJpegPrepare = 1u << 4;
constexpr uint32_t kDropJpegDecomp = 1u << 5;
constexpr uint32_t kDropProgressive = 1u << 7;
constexpr uint32_t kDropBmp = 1u << 6;

/* Pool passed to jd_prepare; must be >= TJPGD_WORKSPACE_SIZE from tjpgdcnf.h (3100 with JD_FASTDECODE==0). */
constexpr size_t kJpegWorkpoolBytes = 3120;
std::array<uint8_t, kJpegWorkpoolBytes> g_jpeg_pool{};

struct JpegDecodeSession {
    const uint8_t* jpg_base{};
    size_t jpg_len{};
    size_t jpg_pos{};
    BMPFile* bmp{};
    File* ppm{};
    uint32_t ppm_stride{0};
    /** Byte offset in `ppm` where raster data begins (after P6 ASCII header). */
    uint32_t ppm_data_start{0};
};

size_t tjpg_infunc(JDEC* jd, uint8_t* buf, size_t n) {
    auto* s = static_cast<JpegDecodeSession*>(jd->device);
    if (!s || !s->jpg_base)
        return 0;
    if (n == 0)
        return 0;
    const size_t rem = s->jpg_len > s->jpg_pos ? s->jpg_len - s->jpg_pos : 0;
    const size_t m = std::min(n, rem);
    if (m == 0)
        return 0;
    /* ChaN: `buf == nullptr` skips segment payload without copying (still must advance). */
    if (buf == nullptr) {
        s->jpg_pos += m;
        return m;
    }
    std::memcpy(buf, s->jpg_base + s->jpg_pos, m);
    s->jpg_pos += m;
    return m;
}

int tjpg_outfunc(JDEC* jd, void* bitmap, JRECT* rect) {
    auto* s = static_cast<JpegDecodeSession*>(jd->device);
    if (!s || !bitmap || !rect)
        return 0;
    auto* rgb = static_cast<const uint8_t*>(bitmap);
    const unsigned left = rect->left;
    const unsigned top = rect->top;
    const unsigned right = rect->right;
    const unsigned bottom = rect->bottom;
    const unsigned w = right - left + 1;
    const unsigned h = bottom - top + 1;
    if (s->bmp) {
        BMPFile& bmp = *s->bmp;
        const unsigned need_h = bottom + 1u;
        if (bmp.get_real_height() < need_h) {
            if (!bmp.expand_y(need_h))
                return 0;
        }
        for (unsigned row = 0; row < h; row++) {
            const unsigned y = top + row;
            if (!bmp.seek(left, y))
                return 0;
            const uint8_t* p = rgb + (size_t)row * (size_t)w * 3u;
            for (unsigned x = 0; x < w; x++) {
                ui::Color px{p[(size_t)x * 3u + 2], p[(size_t)x * 3u + 1], p[(size_t)x * 3u + 0]};
                if (!bmp.write_next_px(px))
                    return 0;
            }
        }
    }
    if (s->ppm && s->ppm_stride > 0) {
        File& ppm = *s->ppm;
        for (unsigned row = 0; row < h; row++) {
            const unsigned y = top + row;
            const uint8_t* p = rgb + (size_t)row * (size_t)w * 3u;
            for (unsigned x = 0; x < w; x++) {
                const unsigned px = left + x;
                const uint8_t tri[3] = {p[(size_t)x * 3u + 2], p[(size_t)x * 3u + 1], p[(size_t)x * 3u + 0]};
                const uint64_t off =
                    (uint64_t)s->ppm_data_start +
                    (((uint64_t)y * (uint64_t)s->ppm_stride + (uint64_t)px) * 3u);
                if (!ppm.seek(off))
                    return 0;
                const auto wr = ppm.write(tri, 3);
                if (!wr.is_ok() || wr.value() != 3)
                    return 0;
            }
        }
    }
    return 1;
}

}  // namespace

bool jpeg_buffer_has_progressive_sof(const uint8_t* jpg, const size_t len) {
    if (!jpg || len < 4)
        return false;
    for (size_t i = 0; i + 1 < len; i++) {
        if (jpg[i] == 0xFFu && jpg[i + 1] == 0xC2u)
            return true;
    }
    return false;
}

bool decode_jpeg_to_new_bmp_file(
    const uint8_t* jpg,
    const size_t len,
    const std::filesystem::path& path,
    uint32_t& drop_bits,
    uint8_t& jresult_out,
    const std::filesystem::path* ppm_path) {
    jresult_out = (uint8_t)JDR_PAR;
    if (!jpg || len < 4)
        return false;

    if (jpeg_buffer_has_progressive_sof(jpg, len)) {
        drop_bits |= kDropProgressive;
        jresult_out = (uint8_t)JDR_FMT3;
        return false;
    }

    JDEC jd{};
    JpegDecodeSession session{};
    session.jpg_base = jpg;
    session.jpg_len = len;
    session.jpg_pos = 0;
    session.bmp = nullptr;

    jd.device = &session;
    jd.swap = 0;

    JRESULT pr = jd_prepare(&jd, tjpg_infunc, g_jpeg_pool.data(), g_jpeg_pool.size(), &session);
    jresult_out = (uint8_t)pr;
    if (pr != JDR_OK) {
        drop_bits |= kDropJpegPrepare;
        return false;
    }

    const uint32_t iw = jd.width;
    const uint32_t ih = jd.height;
    if (iw == 0 || ih == 0 || iw > kG4MaxJpegWidth || ih > kG4MaxJpegHeight) {
        drop_bits |= kDropJpegPrepare;
        jresult_out = (uint8_t)JDR_PAR;
        return false;
    }

    File ppm_file{};
    bool ppm_active = false;
    if (ppm_path && !ppm_path->empty()) {
        const auto cre = ppm_file.create(*ppm_path);
        if (!cre.is_valid()) {
            char hdr[72];
            const int hn = std::snprintf(hdr, sizeof hdr, "P6\n%u %u\n255\n", (unsigned)iw, (unsigned)ih);
            if (hn > 0 && (size_t)hn < sizeof hdr) {
                const auto hw = ppm_file.write(hdr, (File::Size)hn);
                if (hw.is_ok() && hw.value() == (File::Size)hn)
                    ppm_active = true;
            }
            if (!ppm_active)
                ppm_file.close();
        }
    }

    const uint32_t ppm_data_start =
        ppm_active ? (uint32_t)ppm_file.tell() : 0u;

    BMPFile bmp{};
    /* Row-streaming file growth: start at 1 scanline; `tjpg_outfunc` expands as MCUs advance. */
    if (!bmp.create(path, iw, 1)) {
        ppm_file.close();
        drop_bits |= kDropBmp;
        jresult_out = (uint8_t)JDR_PAR;
        return false;
    }

    session.jpg_pos = 0;
    session.bmp = &bmp;
    session.ppm = ppm_active ? &ppm_file : nullptr;
    session.ppm_stride = ppm_active ? iw : 0;
    session.ppm_data_start = ppm_data_start;
    jd.device = &session;
    jd.swap = 0;

    pr = jd_prepare(&jd, tjpg_infunc, g_jpeg_pool.data(), g_jpeg_pool.size(), &session);
    jresult_out = (uint8_t)pr;
    if (pr != JDR_OK) {
        drop_bits |= kDropJpegPrepare;
        bmp.close();
        ppm_file.close();
        return false;
    }

    const JRESULT dr = jd_decomp(&jd, tjpg_outfunc, 0);
    jresult_out = (uint8_t)dr;
    bmp.close();
    ppm_file.close();
    if (dr != JDR_OK) {
        drop_bits |= kDropJpegDecomp;
        return false;
    }
    return true;
}

}  // namespace meteor_lrpt_g4
