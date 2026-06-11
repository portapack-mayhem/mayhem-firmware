/*
 * Copyright (C) 2026
 *
 * MSU-MR CADU → Space Packet scan + light reassembly + JPEG span extraction (M0 G4).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef APPLICATION_METEOR_LRPT_G4_MSUMR_DEMUX_HPP
#define APPLICATION_METEOR_LRPT_G4_MSUMR_DEMUX_HPP

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt_g4 {

using JpegPayloadHandler = void (*)(void* ctx, uint16_t apid, const uint8_t* jpeg, size_t len);

/** MSU-MR image APIDs 64–69: one reassembly buffer each (no mod-4 aliasing). */
constexpr uint16_t kMsumrImageApidMin = 64;
constexpr uint16_t kMsumrImageApidMax = 69;
constexpr size_t kReassemblyStreams = (size_t)(kMsumrImageApidMax - kMsumrImageApidMin + 1);
/** Per-stream cap (M0 AHB SRAM budget — see `LPC43xx_M0.ld` / `SharedMemory::MeteorLrptG4Ipc`). */
constexpr size_t kReassemblyCapBytes = 760;

class MsumrDemux {
   public:
    void reset();

    /**
     * Feed one 1020-byte Mayhem CADU REC record. Invokes `handler` for each complete JPEG span.
     * `drop_bits` accumulates demux-side drops (caller-owned bitfield).
     */
    void feed_cadu_1020(
        const uint8_t* cadu1020,
        JpegPayloadHandler handler,
        void* ctx,
        uint32_t& drop_bits);

   private:
    void scan_space_packets_(
        const uint8_t* buf,
        size_t len,
        size_t start_i,
        uint8_t cadu_vcid,
        JpegPayloadHandler handler,
        void* ctx,
        uint32_t& drop_bits,
        bool& emitted_from_packets);

    struct StreamBuf {
        uint8_t buf[kReassemblyCapBytes]{};
        uint32_t len{0};
        bool active{false};
        /** LRPT VCID (low 6 bits of CADU byte 5) for the open reassembly; `0xFF` = unknown / do not check. */
        uint8_t cadu_vcid{0xFF};
    };
    StreamBuf streams_[kReassemblyStreams]{};

    static bool find_jpeg_span(const uint8_t* p, size_t len, size_t& off, size_t& span_len);
    static void or_drop(uint32_t& drop_bits, uint32_t bit);
    static size_t stream_index_for(uint16_t apid);
};

}  // namespace meteor_lrpt_g4

#endif
