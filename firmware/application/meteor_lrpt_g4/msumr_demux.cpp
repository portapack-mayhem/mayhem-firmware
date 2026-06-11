/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "msumr_demux.hpp"

#include "meteor_lrpt_g4_service.hpp"
#include "meteor_lrpt_msumr.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace meteor_lrpt_g4 {

namespace {

constexpr uint32_t kDropSparse = 1u << 1;
constexpr uint8_t kSeqStandalone = 3u;
constexpr uint8_t kSeqFirst = 1u;
constexpr uint8_t kSeqCont = 0u;
constexpr uint8_t kSeqLast = 2u;

constexpr uint32_t kDropAsm = 1u << 0;
constexpr uint32_t kDropReassemblyOverflow = 1u << 3;

constexpr size_t kMaxJpegScan = 512u * 1024u;

}  // namespace

void MsumrDemux::reset() {
    for (auto& s : streams_) {
        s.len = 0;
        s.active = false;
        s.cadu_vcid = 0xFF;
        std::memset(s.buf, 0, sizeof(s.buf));
    }
}

void MsumrDemux::or_drop(uint32_t& drop_bits, const uint32_t bit) {
    drop_bits |= bit;
}

size_t MsumrDemux::stream_index_for(const uint16_t apid) {
    if (apid >= kMsumrImageApidMin && apid <= kMsumrImageApidMax)
        return (size_t)(apid - kMsumrImageApidMin);
    return SIZE_MAX;
}

bool MsumrDemux::find_jpeg_span(const uint8_t* p, const size_t len, size_t& off, size_t& span_len) {
    off = 0;
    span_len = 0;
    for (size_t i = 0; i + 1 < len; i++) {
        if (p[i] != 0xFFu)
            continue;
        if (p[i + 1] != 0xD8u)
            continue;
        off = i;
        size_t j = i + 2;
        while (j + 1 < len) {
            if (p[j] == 0xFFu && p[j + 1] == 0xD9u) {
                span_len = (j + 2) - off;
                return span_len >= 4;
            }
            j++;
        }
        return false;
    }
    return false;
}

void MsumrDemux::scan_space_packets_(
    const uint8_t* body,
    const size_t body_len,
    const size_t start_i,
    const uint8_t cadu_vcid,
    const JpegPayloadHandler handler,
    void* ctx,
    uint32_t& drop_bits,
    bool& emitted_from_packets) {
    auto emit_jpeg = [&](const uint16_t apid, const uint8_t* j, const size_t n) {
        emitted_from_packets = true;
        handler(ctx, apid, j, n);
    };

    size_t i = start_i;
    while (i + meteor_lrpt::kCcsdsSpacePacketPrimaryBytes <= body_len) {
        size_t total = 0;
        uint16_t apid = 0;
        size_t uoff = 0;
        size_t ulen = 0;
        if (!meteor_lrpt::ccsds_try_parse_space_packet(body, body_len, i, true, total, apid, uoff, ulen)) {
            i++;
            continue;
        }
        const uint8_t* user = body + uoff;
        const uint8_t seqf = meteor_lrpt::ccsds_primary_sequence_flags(body + i);
        const size_t si = stream_index_for(apid);
        StreamBuf* pst = (si == SIZE_MAX) ? nullptr : &streams_[si];

        auto flush_jpeg = [&](const uint8_t* data, size_t len) {
            size_t jo = 0, sl = 0;
            if (find_jpeg_span(data, len, jo, sl))
                emit_jpeg(apid, data + jo, sl);
        };

        if (seqf == kSeqStandalone) {
            flush_jpeg(user, ulen);
            if (pst) {
                pst->active = false;
                pst->len = 0;
                pst->cadu_vcid = 0xFF;
            }
        } else if (seqf == kSeqFirst) {
            if (!pst) {
                or_drop(drop_bits, G4_DROP_PACKET_PARSE);
            } else {
                pst->active = true;
                pst->len = 0;
                pst->cadu_vcid = cadu_vcid;
                if (ulen > kReassemblyCapBytes) {
                    or_drop(drop_bits, kDropReassemblyOverflow);
                    pst->active = false;
                    pst->cadu_vcid = 0xFF;
                } else {
                    std::memcpy(pst->buf, user, ulen);
                    pst->len = (uint32_t)ulen;
                }
            }
        } else if (seqf == kSeqCont) {
            if (!pst) {
                or_drop(drop_bits, kDropSparse);
            } else if (!pst->active) {
                or_drop(drop_bits, kDropSparse);
            } else if (cadu_vcid != 0xFFu && pst->cadu_vcid != 0xFFu && cadu_vcid != pst->cadu_vcid) {
                or_drop(drop_bits, G4_DROP_VCID_MISMATCH);
                pst->active = false;
                pst->len = 0;
                pst->cadu_vcid = 0xFF;
            } else {
                const uint32_t n = pst->len + (uint32_t)ulen;
                if (n > kReassemblyCapBytes) {
                    or_drop(drop_bits, kDropReassemblyOverflow);
                    pst->active = false;
                    pst->len = 0;
                    pst->cadu_vcid = 0xFF;
                } else {
                    std::memcpy(pst->buf + pst->len, user, ulen);
                    pst->len = n;
                }
            }
        } else if (seqf == kSeqLast) {
            if (!pst) {
                or_drop(drop_bits, kDropSparse);
            } else if (!pst->active) {
                or_drop(drop_bits, kDropSparse);
            } else if (cadu_vcid != 0xFFu && pst->cadu_vcid != 0xFFu && cadu_vcid != pst->cadu_vcid) {
                or_drop(drop_bits, G4_DROP_VCID_MISMATCH);
            } else {
                const uint32_t n = pst->len + (uint32_t)ulen;
                if (n > kReassemblyCapBytes) {
                    or_drop(drop_bits, kDropReassemblyOverflow);
                } else {
                    std::memcpy(pst->buf + pst->len, user, ulen);
                    pst->len = n;
                    flush_jpeg(pst->buf, pst->len);
                }
            }
            if (pst) {
                pst->active = false;
                pst->len = 0;
                pst->cadu_vcid = 0xFF;
            }
        } else {
            flush_jpeg(user, ulen);
        }

        i += std::max(total, (size_t)1);
    }
}

void MsumrDemux::feed_cadu_1020(
    const uint8_t* cadu1020,
    const JpegPayloadHandler handler,
    void* ctx,
    uint32_t& drop_bits) {
    if (!cadu1020 || !handler)
        return;

    bool emitted_from_packets = false;

    const bool asm_ok = std::memcmp(cadu1020, meteor_lrpt::kCcsdsCaduAsm, 4) == 0;
    if (!asm_ok)
        or_drop(drop_bits, kDropAsm);

    uint16_t fhp = meteor_lrpt::kMsumrMpduIdleFhp;
    const uint8_t* mpdu_data = nullptr;
    size_t mpdu_len = 0;
    const bool mpdu_ok = asm_ok && meteor_lrpt::msumr_lrpt_mpdu_field(
        cadu1020,
        meteor_lrpt::kMeteorCaduRecBytes,
        fhp,
        mpdu_data,
        mpdu_len);

    const uint8_t cadu_vcid = asm_ok ? meteor_lrpt::lrpt_vcid_from_cadu(cadu1020) : (uint8_t)0xFF;

    /* SatDump `Demuxer::work` skips when FHP is idle or out of range (see `demuxer.cpp`). */
    if (mpdu_ok && fhp < meteor_lrpt::kMsumrMpduIdleFhp && fhp < meteor_lrpt::kMsumrMpduDataBytes && fhp < mpdu_len)
        scan_space_packets_(mpdu_data, mpdu_len, fhp, cadu_vcid, handler, ctx, drop_bits, emitted_from_packets);

    /* Legacy: ASM + space packets at offset 4 (host fixtures / non-standard captures). */
    if (!emitted_from_packets && asm_ok) {
        const uint8_t* legacy = cadu1020 + 4;
        const size_t legacy_len = meteor_lrpt::kMeteorCaduRecBytes - 4u;
        scan_space_packets_(legacy, legacy_len, 0, cadu_vcid, handler, ctx, drop_bits, emitted_from_packets);
    }

    if (!emitted_from_packets) {
        size_t jo = 0, sl = 0;
        if (find_jpeg_span(cadu1020, meteor_lrpt::kMeteorCaduRecBytes, jo, sl) && sl <= kMaxJpegScan)
            handler(ctx, 0xFFFF, cadu1020 + jo, sl);
    }
}

}  // namespace meteor_lrpt_g4
