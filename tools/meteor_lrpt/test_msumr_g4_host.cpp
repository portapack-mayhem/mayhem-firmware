/*
 * Copyright (C) 2026
 *
 * Host smoke: build one 1020-byte CADU with ASM + standalone CCSDS packet carrying a small
 * baseline JPEG, feed `meteor_lrpt_g4::MsumrDemux`, assert the JPEG callback runs.
 * Extended: multi-segment reassembly (>256 B) and parallel APID 64 vs 68 (old mod-4 collision).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_g4/msumr_demux.hpp"
#include "meteor_lrpt_msumr.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr size_t kLegacyFirstPacketPos = 10;

bool read_all(const char* path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.seekg(0, std::ios::end);
    const auto n = f.tellg();
    if (n <= 0 || n > 65536)
        return false;
    f.seekg(0);
    out.resize((size_t)n);
    f.read(reinterpret_cast<char*>(out.data()), n);
    return (bool)f;
}

bool read_cadu1020(const char* path, std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& cadu) {
    std::vector<uint8_t> raw;
    if (!read_all(path, raw) || raw.size() != meteor_lrpt::kMeteorCaduRecBytes)
        return false;
    std::memcpy(cadu.data(), raw.data(), meteor_lrpt::kMeteorCaduRecBytes);
    return true;
}

bool build_cadu_from_jpeg(const std::vector<uint8_t>& jpeg, std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& cadu) {
    if (jpeg.size() < 4 || jpeg.size() + 6u + kLegacyFirstPacketPos > meteor_lrpt::kMeteorCaduRecBytes - 4u)
        return false;
    cadu.fill(0);
    std::memcpy(cadu.data(), meteor_lrpt::kCcsdsCaduAsm, sizeof(meteor_lrpt::kCcsdsCaduAsm));
    for (size_t i = 0; i < kLegacyFirstPacketPos; i++)
        cadu[4 + i] = 0xFF;
    /* Idle first-header-pointer so `msumr_lrpt_mpdu_field` path does not scan random LRPT zone bytes. */
    cadu[12] = 7;
    cadu[13] = 0xFF;
    uint8_t* const p = cadu.data() + 4 + kLegacyFirstPacketPos;
    /* Standalone space packet, APID 68, no secondary header, user field = JPEG */
    constexpr uint16_t apid = 68;
    p[0] = (uint8_t)((apid >> 8) & 7u);
    p[1] = (uint8_t)(apid & 0xFFu);
    p[2] = 0xC0;
    p[3] = 0x00;
    const uint32_t dlen_field = (uint32_t)jpeg.size() - 1u;
    p[4] = (uint8_t)((dlen_field >> 8) & 0xFFu);
    p[5] = (uint8_t)(dlen_field & 0xFFu);
    std::memcpy(p + 6, jpeg.data(), jpeg.size());
    for (size_t j = 4 + kLegacyFirstPacketPos + 6u + jpeg.size(); j < meteor_lrpt::kMeteorCaduRecBytes; j++)
        cadu[j] = 0xFF;
    return true;
}

void cb(void* ctx, uint16_t /*apid*/, const uint8_t* j, size_t n) {
    auto* c = static_cast<int*>(ctx);
    (*c)++;
    if (n < 4 || j[0] != 0xFF || j[1] != 0xD8)
        *c = -1000;
}

/* Append one CCSDS space packet (no secondary header) into legacy CADU body at offset 4. */
size_t append_space_packet_legacy(
    std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& cadu,
    size_t pos,
    uint16_t apid,
    uint8_t seq_flags,
    const uint8_t* user,
    size_t ulen) {
    constexpr size_t kLegacyCap = meteor_lrpt::kMeteorCaduRecBytes - 4u;
    if (ulen == 0 || pos + 6u + ulen > kLegacyCap)
        return 0;
    uint8_t* p = cadu.data() + 4 + pos;
    p[0] = (uint8_t)((apid >> 8) & 7u);
    p[1] = (uint8_t)(apid & 0xFFu);
    const uint16_t wf = (uint16_t)(((uint16_t)(seq_flags & 3u)) << 14);
    p[2] = (uint8_t)(wf >> 8);
    p[3] = (uint8_t)(wf & 0xFFu);
    const uint32_t dlen_field = (uint32_t)ulen - 1u;
    p[4] = (uint8_t)((dlen_field >> 8) & 0xFFu);
    p[5] = (uint8_t)(dlen_field & 0xFFu);
    std::memcpy(p + 6, user, ulen);
    return pos + 6u + ulen;
}

void fill_asm(std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& cadu) {
    cadu.fill(0);
    std::memcpy(cadu.data(), meteor_lrpt::kCcsdsCaduAsm, sizeof(meteor_lrpt::kCcsdsCaduAsm));
}

/** ASM + idle MPDU FHP; prime legacy prefix so `scan_space_packets_` does not mis-parse zeros as CCSDS. */
void fill_asm_idle_mpdu(std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& cadu) {
    fill_asm(cadu);
    for (size_t i = 0; i < kLegacyFirstPacketPos; i++)
        cadu[4 + i] = 0xFF;
    cadu[12] = 7;
    cadu[13] = 0xFF;
}

/** Avoid `scan_space_packets_` walking into post-packet zeros as bogus CCSDS. */
void pad_cadu_tail_ff(std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes>& cadu, const size_t next_abs) {
    for (size_t j = next_abs; j < meteor_lrpt::kMeteorCaduRecBytes; j++)
        cadu[j] = 0xFF;
}

bool test_reassembly_and_dual_apid() {
    std::vector<uint8_t> jpeg;
    const char* jpath = "tools/meteor_lrpt/data/g4_tiny_baseline.jpg";
    if (!read_all(jpath, jpeg) || jpeg.size() < 80)
        return false;

    /* --- Multi-segment single APID (64): FIRST + LAST across two CADUs (cross-CADU state). --- */
    {
        const size_t n1 = jpeg.size() / 2;
        const size_t n2 = jpeg.size() - n1;
        std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes> cadu1{};
        std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes> cadu2{};
        fill_asm_idle_mpdu(cadu1);
        fill_asm_idle_mpdu(cadu2);
        size_t pos = kLegacyFirstPacketPos;
        pos = append_space_packet_legacy(cadu1, pos, 64, 1, jpeg.data(), n1);
        if (pos == 0)
            return false;
        pad_cadu_tail_ff(cadu1, 4u + pos);
        pos = append_space_packet_legacy(cadu2, kLegacyFirstPacketPos, 64, 2, jpeg.data() + n1, n2);
        if (pos == 0)
            return false;
        pad_cadu_tail_ff(cadu2, 4u + pos);

        meteor_lrpt_g4::MsumrDemux demux;
        int hits = 0;
        uint32_t drops = 0;
        demux.feed_cadu_1020(cadu1.data(), cb, &hits, drops);
        demux.feed_cadu_1020(cadu2.data(), cb, &hits, drops);
        if (hits != 1 || drops != 0) {
            std::cerr << "reassembly test: expected hits=1 drops=0 got hits=" << hits << " drops=0x" << std::hex << drops << "\n";
            return false;
        }
    }

    /* --- APID 64 and 68 interleaved (both map to slot 0 under old apid&3) --- */
    {
        std::vector<uint8_t> j2 = jpeg;

        const size_t a1 = jpeg.size() > 40 ? jpeg.size() - 20 : jpeg.size() / 2;
        const size_t a2 = jpeg.size() - a1;
        const size_t b1 = j2.size() > 40 ? j2.size() - 20 : j2.size() / 2;
        const size_t b2 = j2.size() - b1;
        if (a2 == 0 || b2 == 0)
            return false;

        meteor_lrpt_g4::MsumrDemux demux;
        int hits = 0;
        uint32_t drops = 0;

        auto feed = [&](uint16_t apid, uint8_t seq, const uint8_t* u, size_t n) {
            std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes> cadu{};
            fill_asm_idle_mpdu(cadu);
            const size_t pos = append_space_packet_legacy(cadu, kLegacyFirstPacketPos, apid, seq, u, n);
            if (pos == 0)
                return false;
            pad_cadu_tail_ff(cadu, 4u + pos);
            demux.feed_cadu_1020(cadu.data(), cb, &hits, drops);
            return true;
        };

        if (!feed(64, 1, jpeg.data(), a1))
            return false;
        if (!feed(68, 1, j2.data(), b1))
            return false;
        if (!feed(64, 2, jpeg.data() + a1, a2))
            return false;
        if (!feed(68, 2, j2.data() + b1, b2))
            return false;

        if (hits != 2 || drops != 0) {
            std::cerr << "dual-apid test: expected hits=2 drops=0 got hits=" << hits << " drops=0x" << std::hex << drops << "\n";
            return false;
        }
    }

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes> cadu{};

    if (argc >= 2) {
        const std::string arg1{argv[1]};
        if (arg1.size() >= 5 && arg1.compare(arg1.size() - 5, 5, ".cadu") == 0) {
            if (!read_cadu1020(argv[1], cadu)) {
                std::cerr << "read_cadu1020 fail (need exactly " << meteor_lrpt::kMeteorCaduRecBytes << " bytes): " << argv[1] << "\n";
                return 2;
            }
        } else {
            std::vector<uint8_t> jpeg;
            if (!read_all(argv[1], jpeg)) {
                std::cerr << "read fail: " << argv[1] << "\n";
                return 2;
            }
            if (!build_cadu_from_jpeg(jpeg, cadu)) {
                std::cerr << "build_cadu_from_jpeg fail\n";
                return 2;
            }
        }
    } else {
        const char* jpath = "tools/meteor_lrpt/data/g4_tiny_baseline.jpg";
        std::vector<uint8_t> jpeg;
        if (!read_all(jpath, jpeg)) {
            std::cerr << "read fail: " << jpath << "\n";
            return 2;
        }
        if (!build_cadu_from_jpeg(jpeg, cadu)) {
            std::cerr << "build_cadu_from_jpeg fail\n";
            return 2;
        }
    }

    meteor_lrpt_g4::MsumrDemux demux;
    int hits = 0;
    uint32_t drops = 0;
    demux.feed_cadu_1020(cadu.data(), cb, &hits, drops);
    if (hits != 1) {
        std::cerr << "expected 1 jpeg callback, got " << hits << " drops=0x" << std::hex << drops << "\n";
        return 1;
    }

    if (argc < 2 && !test_reassembly_and_dual_apid()) {
        std::cerr << "extended demux regression failed\n";
        return 1;
    }

    std::cout << "msumr_g4_host_ok\n";
    return 0;
}
