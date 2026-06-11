/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_sector_file_ring.hpp"

#include "meteor_lrpt/meteor_deinterleaver.hpp"

#include "ff.h"

#include "meteor_lrpt/meteor_deinterleaver_reader.hpp"

#include <algorithm>
#include <utility>

namespace meteor_lrpt {

MeteorDeintFileRing::~MeteorDeintFileRing() {
    close();
}

Optional<File::Error> MeteorDeintFileRing::preallocate_ring_file() {
    if (!open_)
        return Optional<File::Error>{File::Error{FR_INVALID_OBJECT}};

    std::array<int8_t, kDeinterleaverSectorBytes> zero{};
    zero.fill(0);

    for (uint32_t base = 0; base < kDeinterleaverRingBytes; base += kDeinterleaverSectorBytes) {
        auto sk = file_.seek(base);
        if (sk.is_error())
            return Optional<File::Error>{sk.error()};
        auto wr = file_.write(zero.data(), kDeinterleaverSectorBytes);
        if (wr.is_error())
            return Optional<File::Error>{wr.error()};
        if (wr.value() != kDeinterleaverSectorBytes)
            return Optional<File::Error>{File::Error{FR_DISK_FULL}};
    }

    if (file_.size() != kDeinterleaverRingBytes)
        return Optional<File::Error>{File::Error{FR_UNEXPECTED}};

    cache_base_ = 0xffffffffu;
    dirty_ = false;
    return {};
}

Optional<File::Error> MeteorDeintFileRing::create(const std::filesystem::path& path) {
    close();
    auto err = file_.create(path);
    if (err)
        return err;
    open_ = true;
    dirty_ = false;
    cache_base_ = 0xffffffffu;
    auto pe = preallocate_ring_file();
    if (pe) {
        file_.close();
        open_ = false;
        dirty_ = false;
        cache_base_ = 0xffffffffu;
        return pe;
    }
    return {};
}

void MeteorDeintFileRing::close() {
    if (open_) {
        flush_cache();
        file_.close();
        open_ = false;
    }
    dirty_ = false;
    cache_base_ = 0xffffffffu;
}

void MeteorDeintFileRing::flush_cache() const {
    if (!open_ || !dirty_ || cache_base_ == 0xffffffffu)
        return;
    auto sk = file_.seek(cache_base_);
    if (!sk)
        return;
    (void)file_.write(cache_.data(), kDeinterleaverSectorBytes);
    dirty_ = false;
}

void MeteorDeintFileRing::load_sector(uint32_t sector_base) const {
    if (!open_)
        return;
    flush_cache();
    cache_base_ = sector_base;
    auto sk = file_.seek(sector_base);
    if (!sk) {
        cache_.fill(0);
        return;
    }
    auto rd = file_.read(cache_.data(), kDeinterleaverSectorBytes);
    if (!rd || rd.value() != kDeinterleaverSectorBytes)
        cache_.fill(0);
    dirty_ = false;
}

void MeteorDeintFileRing::write_byte(uint32_t index_mod, int8_t v) {
    if (!open_)
        return;
    const uint32_t idx = index_mod % kDeinterleaverRingBytes;
    const uint32_t sec = idx - (idx % kDeinterleaverSectorBytes);
    if (cache_base_ != sec)
        load_sector(sec);
    cache_[idx - cache_base_] = v;
    dirty_ = true;
}

int8_t MeteorDeintFileRing::read_byte(uint32_t index_mod) const {
    if (!open_)
        return 0;
    const uint32_t idx = index_mod % kDeinterleaverRingBytes;
    const uint32_t sec = idx - (idx % kDeinterleaverSectorBytes);
    if (cache_base_ != sec)
        load_sector(sec);
    return cache_[idx - cache_base_];
}

Optional<File::Error> MeteorDeintFileRing::flush_and_sync() {
    flush_cache();
    if (!open_)
        return {};
    return file_.sync();
}

Optional<File::Error> MeteorDeintFileRing::read_sector_raw(
    uint32_t sector_base,
    std::array<int8_t, kDeinterleaverSectorBytes>& out) const {
    if (!open_)
        return Optional<File::Error>{File::Error{FR_INVALID_OBJECT}};
    if ((sector_base % kDeinterleaverSectorBytes) != 0 || sector_base >= kDeinterleaverRingBytes)
        return Optional<File::Error>{File::Error{FR_INVALID_PARAMETER}};

    flush_cache();
    auto sk = file_.seek(sector_base);
    if (sk.is_error())
        return Optional<File::Error>{sk.error()};
    auto rd = file_.read(out.data(), kDeinterleaverSectorBytes);
    if (rd.is_error())
        return Optional<File::Error>{rd.error()};
    if (rd.value() != kDeinterleaverSectorBytes)
        return Optional<File::Error>{File::Error{FR_UNEXPECTED}};

    if (cache_base_ == sector_base) {
        cache_ = out;
        dirty_ = false;
    } else {
        cache_base_ = 0xffffffffu;
    }
    return {};
}

Optional<File::Error> MeteorDeintFileRing::write_sector_raw(
    uint32_t sector_base,
    const std::array<int8_t, kDeinterleaverSectorBytes>& in) {
    if (!open_)
        return Optional<File::Error>{File::Error{FR_INVALID_OBJECT}};
    if ((sector_base % kDeinterleaverSectorBytes) != 0 || sector_base >= kDeinterleaverRingBytes)
        return Optional<File::Error>{File::Error{FR_INVALID_PARAMETER}};

    flush_cache();
    auto sk = file_.seek(sector_base);
    if (sk.is_error())
        return Optional<File::Error>{sk.error()};
    auto wr = file_.write(in.data(), kDeinterleaverSectorBytes);
    if (wr.is_error())
        return Optional<File::Error>{wr.error()};
    if (wr.value() != kDeinterleaverSectorBytes)
        return Optional<File::Error>{File::Error{FR_DISK_FULL}};

    if (cache_base_ == sector_base) {
        cache_ = in;
        dirty_ = false;
    } else {
        cache_base_ = 0xffffffffu;
    }
    return {};
}

Optional<File::Error> meteor_deinterleave_file_ring(
    MeteorDeintFileRing& ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch) {
    if (len == 0 || len > meteor_lrpt::kDeinterleaverReaderMaxOutputLen)
        return Optional<File::Error>{File::Error{FR_INVALID_PARAMETER}};

    const uint32_t ringb = (uint32_t)kDeinterleaverRingBytes;
    const uint32_t read_idx_start =
        (offset + (uint32_t)kInterBranchCount * (uint32_t)kInterBranchDelay) % ringb;

    /* Chunked writes: a full 8192-pair batch is ~64 KiB and breaks M0 `ram` when placed in .bss.
     * Semantics match the monolithic path: each chunk sorts/dedups only its own indices. */
    constexpr size_t kWriteChunk = 512;
    using WPair = std::pair<uint32_t, int8_t>;
    std::array<WPair, kWriteChunk> writes{};

    uint32_t off = offset;
    int br = cur_branch;
    const int8_t* s = src;

    for (size_t chunk0 = 0; chunk0 < len; chunk0 += kWriteChunk) {
        const size_t chunk_len = std::min(kWriteChunk, len - chunk0);
        for (size_t ci = 0; ci < chunk_len; ci++) {
            if (br == 0)
                s += 8;

            const int delay = (br % kInterBranchCount) * kInterBranchDelay * kInterBranchCount;
            const uint32_t write_idx = (uint32_t)((int64_t)off - (int64_t)delay + (int64_t)ringb) % ringb;
            writes[ci] = WPair{write_idx, *s++};
            off = (off + 1u) % ringb;
            br = (br + 1) % kInterMarkerIntersamps;
        }

        std::sort(writes.begin(), writes.begin() + chunk_len, [](const WPair& a, const WPair& b) {
            return a.first < b.first;
        });

        size_t nd = 0;
        for (size_t i = 0; i < chunk_len;) {
            const uint32_t idx = writes[i].first;
            size_t j = i + 1;
            while (j < chunk_len && writes[j].first == idx)
                j++;
            writes[nd++] = writes[j - 1];
            i = j;
        }

        std::array<int8_t, kDeinterleaverSectorBytes> sector_buf{};
        size_t wi = 0;
        while (wi < nd) {
            const uint32_t base = writes[wi].first - (writes[wi].first % kDeinterleaverSectorBytes);
            auto er = ring.read_sector_raw(base, sector_buf);
            if (er)
                return er;
            while (wi < nd) {
                const auto& w = writes[wi];
                if (w.first < base || w.first >= base + kDeinterleaverSectorBytes)
                    break;
                sector_buf[w.first - base] = w.second;
                wi++;
            }
            auto ew = ring.write_sector_raw(base, sector_buf);
            if (ew)
                return ew;
        }
    }

    offset = off;
    cur_branch = br;

    uint32_t read_idx = read_idx_start;
    std::array<int8_t, kDeinterleaverSectorBytes> rsec{};
    uint32_t rbase = 0xffffffffu;
    for (size_t i = 0; i < len; i++) {
        const uint32_t idx = read_idx % ringb;
        const uint32_t sec = idx - (idx % kDeinterleaverSectorBytes);
        if (rbase != sec) {
            auto err = ring.read_sector_raw(sec, rsec);
            if (err)
                return err;
            rbase = sec;
        }
        *dst++ = rsec[idx - sec];
        read_idx = (read_idx + 1u) % ringb;
    }

    return {};
}

}  // namespace meteor_lrpt
