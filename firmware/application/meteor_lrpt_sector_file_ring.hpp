/*
 * Copyright (C) 2026
 *
 * FatFs-backed deinterleaver ring file (512 B write-back cache), M0-side.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_SECTOR_FILE_RING_HPP
#define METEOR_LRPT_SECTOR_FILE_RING_HPP

#include "file.hpp"
#include "meteor_lrpt_ring_iface.hpp"

#include <array>
#include <cstdint>

namespace meteor_lrpt {

class MeteorDeintFileRing final : public IMeteorDeintRing {
   public:
    MeteorDeintFileRing() = default;
    ~MeteorDeintFileRing() override;

    Optional<File::Error> create(const std::filesystem::path& path);
    void close();

    void write_byte(uint32_t index_mod, int8_t v) override;
    int8_t read_byte(uint32_t index_mod) const override;

    /** Flush dirty sector cache and `f_sync` the underlying file (optional durability). */
    Optional<File::Error> flush_and_sync();

    /**
     * One FatFs read(512) at `sector_base` (must be multiple of kDeinterleaverSectorBytes,
     * `< kDeinterleaverRingBytes`). Flushes write cache first. Does not require the sector to
     * match the single-slot byte cache; invalidates that cache unless the read updates it.
     */
    Optional<File::Error> read_sector_raw(uint32_t sector_base, std::array<int8_t, kDeinterleaverSectorBytes>& out) const;

    /**
     * One FatFs write(512) at `sector_base`. Flushes write cache first. If this sector is the
     * active cache line, updates `cache_` and clears dirty.
     */
    Optional<File::Error> write_sector_raw(uint32_t sector_base, const std::array<int8_t, kDeinterleaverSectorBytes>& in);

   private:
    void flush_cache() const;
    void load_sector(uint32_t sector_base) const;
    Optional<File::Error> preallocate_ring_file();

    mutable File file_{};
    mutable bool open_{false};
    mutable bool dirty_{false};
    mutable uint32_t cache_base_{0xffffffffu};
    mutable std::array<int8_t, kDeinterleaverSectorBytes> cache_{};
};

/**
 * Same semantics as meteor_deinterleave (see firmware/baseband/meteor_lrpt/meteor_deinterleaver.cpp)
 * but uses 512-byte read_sector_raw / write_sector_raw on ring for FatFs efficiency.
 * ring must be create()d and preallocated.
 */
Optional<File::Error> meteor_deinterleave_file_ring(
    MeteorDeintFileRing& ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch);

}  // namespace meteor_lrpt

#endif
