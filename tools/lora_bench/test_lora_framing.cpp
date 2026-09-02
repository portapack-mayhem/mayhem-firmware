// Host test for firmware/common/lora_framing.hpp - the arithmetic that turns
// nibbles into chips and back.
//
//   c++ -std=c++17 -o test_lora_framing test_lora_framing.cpp && ./test_lora_framing
//
// Two halves, and the second is the one that matters.
//
// The sweep runs every spreading factor, bandwidth, coding rate and length through
// encode and back, byte for byte. It is worth having - it would have caught the
// transmitter coding its payload at full rate under LDRO the moment that was
// written - but on its own it proves only that we agree with ourselves. The coding
// rate bug would have sailed straight through it: both sides read the same wrong
// table, so both sides were wrong together and the round trip closed perfectly.
//
// So the second half decodes SYMBOLS RECORDED OFF THE AIR from a stock Heltec node
// and checks them against what that node actually sent. That is the half that can
// disagree with us.
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../firmware/common/lora_framing.hpp"

static int fails = 0;
static void check(bool ok, const char* what) {
    if (!ok) { fails++; printf("FAIL  %s\n", what); }
}

// The receiver's chain from a peak bin to a symbol for the deinterleaver, exactly as
// proc_lora.cpp runs it: undo the transmitter's +1, drop the two low chips if the
// rate is reduced (which is where the appended parity bit goes), then gray encode.
static uint16_t bin_to_symbol(uint16_t chip, int n, bool ldro) {
    uint32_t a = (static_cast<uint32_t>(chip) + n - 1u) % n;
    if (ldro) a = lora::ldro_div4(a, n);
    return static_cast<uint16_t>(lora::gray_encode(a));
}

// One block through the whole frame layer and back.
static bool block_round_trip(int sf, int cw_len, bool ldro, const uint8_t* nibbles) {
    const int words = lora::words_per_block(sf, ldro);
    const int n = 1 << sf;
    const int cr_app = cw_len - 4;

    int cw[12];
    for (int i = 0; i < words; i++) cw[i] = lora::hamming_encode(nibbles[i], cr_app);

    uint16_t chips[12];
    lora::interleave(cw, words, cw_len, ldro, sf, n, chips);

    uint16_t syms[12];
    for (int i = 0; i < cw_len; i++) syms[i] = bin_to_symbol(chips[i], n, ldro);

    uint16_t back[12];
    lora::deinterleave(syms, words, cw_len, back);
    for (int i = 0; i < words; i++)
        if (lora::hamming_top4(back[i], cw_len) != nibbles[i]) return false;
    return true;
}

int main() {
    // ---- the sweep ----------------------------------------------------------
    uint32_t combos = 0;
    for (int sf = 7; sf <= 12; sf++) {
        for (int cw_len = 5; cw_len <= 8; cw_len++) {
            for (int l = 0; l < 2; l++) {
                const bool ldro = (l == 1);
                const int words = lora::words_per_block(sf, ldro);
                // Walk the nibble space with patterns that move every bit: all zeros,
                // all ones, a counter, and its complement.
                for (int pattern = 0; pattern < 4; pattern++) {
                    uint8_t nib[12];
                    for (int i = 0; i < words; i++)
                        nib[i] = static_cast<uint8_t>(
                            pattern == 0 ? 0x0 :
                            pattern == 1 ? 0xF :
                            pattern == 2 ? (i & 0xF) : (0xF - (i & 0xF)));
                    combos++;
                    char what[96];
                    snprintf(what, sizeof what, "SF%d CR4/%d %s: block round trip",
                             sf, cw_len, ldro ? "reduced" : "full");
                    check(block_round_trip(sf, cw_len, ldro, nib), what);
                }
            }
        }
    }

    // The reduced-rate rule against the symbol time it is defined by.
    struct RateCase { uint8_t sf; uint32_t bw; bool want; };
    const RateCase rates[] = {
        {7, 500000, false}, {7, 250000, false}, {7, 125000, false},
        {9, 250000, false}, {10, 250000, false}, {11, 250000, false},
        {11, 125000, true},              // 16.384 ms - LONG_MODERATE, just over
        {12, 125000, true},
        // 4096/250 kHz is 16.384 ms, the same as SF11 at 125 kHz: reduced rate, not off.
        {12, 250000, true},
        {12, 500000, false},             // 8.192 ms
    };
    for (const auto& r : rates) {
        char what[96];
        snprintf(what, sizeof what, "SF%u at %u kHz: reduced rate should be %s",
                 r.sf, r.bw / 1000, r.want ? "on" : "off");
        check(lora::ldro_needed(r.sf, r.bw) == r.want, what);
    }

    // Whitening: the published sequence, and its own inverse.
    {
        const uint8_t want[] = {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE1, 0xC2, 0x85, 0x0B};
        uint8_t st = 0xFF;
        for (size_t i = 0; i < sizeof(want); i++)
            check(lora::whiten_next(st) == want[i], "whitening sequence");
        uint8_t a = 0xFF, b = 0xFF;
        for (int i = 0; i < 64; i++) {
            const uint8_t plain = static_cast<uint8_t>(i * 37 + 11);
            check(((plain ^ lora::whiten_next(a)) ^ lora::whiten_next(b)) == plain,
                  "whitening undoes itself");
        }
    }

    // Header checksum: every length, coding rate and CRC flag it can carry.
    for (int len = 0; len < 256; len++)
        for (int cr = 1; cr <= 4; cr++)
            for (int crc = 0; crc < 2; crc++) {
                const uint8_t h0 = (len >> 4) & 0xF, h1 = len & 0xF;
                const uint8_t h2 = static_cast<uint8_t>((cr << 1) | crc);
                uint8_t c4, clo;
                lora::header_checksum(h0, h1, h2, &c4, &clo);
                check(c4 <= 1 && clo <= 0xF, "checksum fits its nibbles");
                check(lora::header_coding_rate(h2) == cr, "header states its coding rate");
                check(lora::header_has_crc(h2) == (crc != 0), "header states its CRC flag");
            }

    // ---- recorded off the air ------------------------------------------------
    // A broadcast text from a stock Heltec on LONG_MODERATE (SF11, 125 kHz), 2026-08-26:
    // the demodulator's raw peak bins, read out of the device over the console. The
    // frame these must produce is that node's own, and nothing here can talk it into
    // a different answer.
    {
        const uint16_t bins[] = {
            0x3AA,0x012,0x006,0x403,0x73E,0x106,0x252,0x15E,0x1EA,0x636,0x16A,0x066,
            0x3B7,0x1CE,0x3A3,0x612,0x44B,0x2D2,0x526,0x18E,0x136,0x483,0x3F3,0x24A,0x08E};
        const uint32_t off = 5;
        const int sf = 11, n = 1 << sf;
        const bool ldro = lora::ldro_needed(11, 125000);
        const int words = lora::words_per_block(sf, ldro);      // 9
        check(ldro, "SF11 at 125 kHz runs at reduced rate");

        // Header block: eight symbols, always reduced rate and CR 4/8, and this one
        // carries the parity worth using.
        uint16_t g[8];
        for (int i = 0; i < 8; i++) {
            uint32_t a = (bins[i] - off) % n;
            g[i] = static_cast<uint16_t>(lora::gray_encode(lora::ldro_div4(a, n)));
        }
        uint16_t hcw[12];
        lora::deinterleave(g, words, 8, hcw);
        uint8_t nib[12];
        for (int i = 0; i < words; i++) nib[i] = lora::hamming_correct84(hcw[i] & 0xFF);

        uint8_t c4, clo;
        lora::header_checksum(nib[0], nib[1], nib[2], &c4, &clo);
        check((nib[3] & 1) == c4 && (nib[4] & 0xF) == clo, "recorded header checksum");
        check(((nib[0] << 4) | nib[1]) == 24, "recorded header declares 24 bytes");

        // The coding rate comes from the header, which is the whole point: the preset
        // table said 4/8 and the air says 4/5.
        const int cw_len = 4 + lora::header_coding_rate(nib[2]);
        check(cw_len == 5, "recorded header declares CR 4/5");
        check(lora::header_has_crc(nib[2]), "recorded header declares a CRC");

        // Payload: the header block's spare nibbles first, then blocks of cw_len.
        std::vector<uint8_t> pn(nib + 5, nib + words);
        for (int bi = 8; bi + cw_len <= (int)(sizeof(bins) / sizeof(bins[0])); bi += cw_len) {
            uint16_t pg[12];
            for (int i = 0; i < cw_len; i++) {
                uint32_t a = (bins[bi + i] - off) % n;
                pg[i] = static_cast<uint16_t>(lora::gray_encode(lora::ldro_div4(a, n)));
            }
            uint16_t pcw[12];
            lora::deinterleave(pg, words, cw_len, pcw);
            for (int c = 0; c < words; c++) pn.push_back(lora::hamming_top4(pcw[c], cw_len));
        }

        uint8_t st = 0xFF;
        std::vector<uint8_t> out;
        for (size_t i = 0; i + 1 < pn.size(); i += 2)
            out.push_back(static_cast<uint8_t>(((pn[i + 1] << 4) | pn[i]) ^ lora::whiten_next(st)));

        // dest FFFFFFFF, sender 08D60476 - the Heltec's own address - channel hash 08.
        const uint8_t want[] = {0xFF,0xFF,0xFF,0xFF, 0x76,0x04,0xD6,0x08,
                                0xA4,0x85,0x47,0xFE, 0xE7,0x08};
        check(out.size() >= sizeof(want), "recorded frame is long enough to check");
        for (size_t i = 0; i < sizeof(want) && i < out.size(); i++) {
            char what[96];
            snprintf(what, sizeof what, "recorded frame byte %u: got %02X expect %02X",
                     (unsigned)i, out[i], want[i]);
            check(out[i] == want[i], what);
        }
    }

    printf("%s  %u block round trips, plus a frame recorded off the air\n",
           fails ? "FAILED" : "ok", combos);
    return fails ? 1 : 0;
}
