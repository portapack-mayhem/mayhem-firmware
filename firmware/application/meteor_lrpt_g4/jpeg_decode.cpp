/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "jpeg_decode.hpp"

#include "bmpfile.hpp"

#ifdef METEOR_STANDALONE_NO_CH
#include <cstdlib>
#define chHeapAlloc(a, s) std::malloc(s)
#define chHeapFree(p) std::free(p)
#define __DMB() ((void)0)
namespace SharedMemory {
struct MeteorLrptG4Ipc {
    uint16_t preview_width{};
    uint16_t preview_height{};
    uint32_t preview_seq{};
    static constexpr unsigned kPreviewWidth = 16;
    static constexpr unsigned kPreviewHeight = 12;
    uint8_t preview_rgb[kPreviewWidth * kPreviewHeight * 3]{};
};
}  // namespace SharedMemory
#else
#include "portapack_shared_memory.hpp"
#include "ch.h"
#endif

extern "C" {
#include "tjpgd.h"
}

#include <algorithm>
#include <cstring>

namespace meteor_lrpt_g4 {

namespace {

constexpr uint32_t kDropJpegPrepare = 1u << 4;
constexpr uint32_t kDropJpegDecomp = 1u << 5;
constexpr uint32_t kDropProgressive = 1u << 7;
constexpr uint32_t kDropBmp = 1u << 6;

/* Pool passed to jd_prepare; must be >= TJPGD_WORKSPACE_SIZE from tjpgdcnf.h (3100 with JD_FASTDECODE==0). */
constexpr size_t kJpegWorkpoolBytes = 3120;

struct JpegDecodeSession {
    const uint8_t* jpg_base{};
    size_t jpg_len{};
    size_t jpg_pos{};
    BMPFile* bmp{};
    SharedMemory::MeteorLrptG4Ipc* g4_ipc{};
    uint32_t img_w{};
    uint32_t img_h{};
};

void preview_store_rgb(
    JpegDecodeSession* s,
    unsigned px,
    unsigned y,
    uint8_t r,
    uint8_t g,
    uint8_t b) {
    if (!s || !s->g4_ipc || s->img_w < 2 || s->img_h < 2)
        return;
    constexpr uint16_t pw = (uint16_t)SharedMemory::MeteorLrptG4Ipc::kPreviewWidth;
    constexpr uint16_t ph = (uint16_t)SharedMemory::MeteorLrptG4Ipc::kPreviewHeight;
    const unsigned pxi = (px * (pw - 1u)) / (s->img_w - 1u);
    const unsigned pyi = (y * (ph - 1u)) / (s->img_h - 1u);
    if (pxi >= pw || pyi >= ph)
        return;
    const size_t o = ((size_t)pyi * (size_t)pw + (size_t)pxi) * 3u;
    s->g4_ipc->preview_rgb[o + 0] = r;
    s->g4_ipc->preview_rgb[o + 1] = g;
    s->g4_ipc->preview_rgb[o + 2] = b;
}

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
                const uint8_t r = p[(size_t)x * 3u + 2];
                const uint8_t g = p[(size_t)x * 3u + 1];
                const uint8_t b = p[(size_t)x * 3u + 0];
                ui::Color px{r, g, b};
                if (!bmp.write_next_px(px))
                    return 0;
                preview_store_rgb(s, left + x, y, r, g, b);
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
    SharedMemory::MeteorLrptG4Ipc* g4_ipc) {
    jresult_out = (uint8_t)JDR_PAR;
    if (!jpg || len < 4)
        return false;

    if (jpeg_buffer_has_progressive_sof(jpg, len)) {
        drop_bits |= kDropProgressive;
        jresult_out = (uint8_t)JDR_FMT3;
        return false;
    }

    void* const pool_mem = chHeapAlloc(nullptr, kJpegWorkpoolBytes);
    if (!pool_mem) {
        drop_bits |= kDropJpegPrepare;
        jresult_out = (uint8_t)JDR_PAR;
        return false;
    }
    uint8_t* const work_pool = static_cast<uint8_t*>(pool_mem);

    const auto free_pool = [&]() {
        chHeapFree(pool_mem);
    };

    JDEC jd{};
    JpegDecodeSession session{};
    session.jpg_base = jpg;
    session.jpg_len = len;
    session.jpg_pos = 0;
    session.g4_ipc = g4_ipc;

    jd.device = &session;
    jd.swap = 0;

    JRESULT pr = jd_prepare(&jd, tjpg_infunc, work_pool, kJpegWorkpoolBytes, &session);
    jresult_out = (uint8_t)pr;
    if (pr != JDR_OK) {
        drop_bits |= kDropJpegPrepare;
        free_pool();
        return false;
    }

    const uint32_t iw = jd.width;
    const uint32_t ih = jd.height;
    if (iw == 0 || ih == 0 || iw > kG4MaxJpegWidth || ih > kG4MaxJpegHeight) {
        drop_bits |= kDropJpegPrepare;
        jresult_out = (uint8_t)JDR_PAR;
        free_pool();
        return false;
    }

    BMPFile bmp{};
    if (!bmp.create(path, iw, 1)) {
        drop_bits |= kDropBmp;
        jresult_out = (uint8_t)JDR_PAR;
        free_pool();
        return false;
    }

    session.jpg_pos = 0;
    session.bmp = &bmp;
    session.img_w = iw;
    session.img_h = ih;
    if (g4_ipc) {
        g4_ipc->preview_width = (uint16_t)SharedMemory::MeteorLrptG4Ipc::kPreviewWidth;
        g4_ipc->preview_height = (uint16_t)SharedMemory::MeteorLrptG4Ipc::kPreviewHeight;
    }
    jd.device = &session;

    const JRESULT dr = jd_decomp(&jd, tjpg_outfunc, 0);
    jresult_out = (uint8_t)dr;
    bmp.close();
    free_pool();
    if (dr != JDR_OK) {
        drop_bits |= kDropJpegDecomp;
        return false;
    }
    if (g4_ipc) {
        __DMB();
        g4_ipc->preview_seq = g4_ipc->preview_seq + 1u;
        __DMB();
    }
    return true;
}

}  // namespace meteor_lrpt_g4
