/*
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "lrpt_decoder.hpp"

#include <cstring>

namespace ui::external_app::lrpt_rx {

namespace {

constexpr uint16_t GF_POLY = 0x187;  // x^8 + x^7 + x^2 + x + 1
constexpr int RS_N = 255;
constexpr int RS_ROOTS = 32;
constexpr int RS_FCR = 112;
constexpr int RS_PRIM = 11;
constexpr int RS_INTERLEAVE = 4;

constexpr uint8_t VCID_IMAGE = 5;
constexpr size_t VCDU_DATA = 10;  // header 6, insert zone 2, M_PDU header 2
constexpr size_t MPDU_DATA = 882;

constexpr uint8_t APID_FIRST = 64;
constexpr uint8_t APID_LAST = 69;
constexpr size_t SEGMENT_HEADER = 14;  // time 8, MCU number, QT, DC/AC, QFM 2, QF
constexpr uint8_t MCUS_PER_PACKET = 14;

// JPEG, the standard luminance tables
const uint8_t dc_counts[16] = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
const uint8_t ac_counts[16] = {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};
const uint8_t ac_values[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa};

const uint8_t quant_base[64] = {
    16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};

// position in the block of each zigzag index
const uint8_t zigzag[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

// 2048 * C(u) / 2 * cos((2x + 1) u pi / 16), [x][u]
const int16_t idct_table[64] = {
    724, 1004, 946, 851, 724, 569, 392, 200,
    724, 851, 392, -200, -724, -1004, -946, -569,
    724, 569, -392, -1004, -724, 200, 946, 851,
    724, 200, -946, -569, 724, 851, -392, -1004,
    724, -200, -946, 569, 724, -851, -392, 1004,
    724, -569, -392, 1004, -724, -200, 946, -851,
    724, -851, 392, 200, -724, 1004, -946, 569,
    724, -1004, 946, -851, 724, -569, 392, -200};

void idct(const int32_t* coef, uint8_t* out) {
    int32_t tmp[64];
    for (int y = 0; y < 8; y++) {
        const int32_t* r = &coef[y * 8];
        int32_t* t = &tmp[y * 8];
        if (!(r[1] | r[2] | r[3] | r[4] | r[5] | r[6] | r[7])) {
            const int32_t v = (r[0] * 724 + 128) >> 8;
            for (int x = 0; x < 8; x++) t[x] = v;
            continue;
        }
        for (int x = 0; x < 8; x++) {
            const int16_t* c = &idct_table[x * 8];
            int32_t sum = 0;
            for (int u = 0; u < 8; u++) sum += c[u] * r[u];
            t[x] = (sum + 128) >> 8;
        }
    }
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            const int16_t* c = &idct_table[y * 8];
            int32_t sum = 0;
            for (int v = 0; v < 8; v++) sum += c[v] * tmp[v * 8 + x];
            int32_t p = ((sum + 8192) >> 14) + 128;
            if (p < 0) p = 0;
            if (p > 255) p = 255;
            out[y * 8 + x] = (uint8_t)p;
        }
    }
}

inline int extend(int v, int n) {
    return (v < (1 << (n - 1))) ? v - (1 << n) + 1 : v;
}

}  // namespace

Decoder::Decoder(StripHandler handler, void* context)
    : handler_{handler}, context_{context} {
    uint16_t x = 1;
    for (int i = 0; i < 255; i++) {
        exp_[i] = (uint8_t)x;
        log_[x] = (uint8_t)i;
        x <<= 1;
        if (x & 0x100) x ^= GF_POLY;
    }
    exp_[255] = exp_[0];

    // CCSDS PN, x^8 + x^7 + x^5 + x^3 + 1, all ones to start
    uint8_t st = 0xFF;
    for (int i = 0; i < 255; i++) {
        uint8_t byte = 0;
        for (int k = 0; k < 8; k++) {
            byte = (byte << 1) | (st >> 7);
            const uint8_t fb = ((st >> 7) ^ (st >> 4) ^ (st >> 2) ^ st) & 1;
            st = (st << 1) | fb;
        }
        pn_[i] = byte;
    }
}

void Decoder::set_planes(uint8_t* planes) {
    planes_ = planes;
    if (planes_) memset(planes_, 0, PLANES * PLANE_BYTES);
    strip_has_data_ = false;
}

void Decoder::set_apids(uint8_t red, uint8_t green, uint8_t blue) {
    apids_[0] = red;
    apids_[1] = green;
    apids_[2] = blue;
}

void Decoder::restart() {
    have_sequence_ = false;
    have_phase_ = false;
    have_strip_ = false;
    in_sync_ = false;
    set_planes(planes_);
}

void Decoder::flush() {
    if (strip_has_data_) emit();
}

void Decoder::emit() {
    if (planes_) {
        handler_(context_);
        memset(planes_, 0, PLANES * PLANE_BYTES);
    }
    stats_.strips++;
    strip_has_data_ = false;
}

uint8_t Decoder::mul(uint8_t a, uint8_t b) const {
    if (!a || !b) return 0;
    int l = log_[a] + log_[b];
    if (l >= 255) l -= 255;
    return exp_[l];
}

// Codeword with data[0] the highest power. Returns the symbols corrected, -1 if it can't.
int Decoder::rs_decode(uint8_t* cw) const {
    uint8_t s[RS_ROOTS];
    uint8_t any = 0;
    int root = (RS_PRIM * RS_FCR) % 255;
    for (int i = 0; i < RS_ROOTS; i++) {
        uint8_t v = 0;
        for (int j = 0; j < RS_N; j++) {
            if (v) {
                int l = log_[v] + root;
                if (l >= 255) l -= 255;
                v = exp_[l];
            }
            v ^= cw[j];
        }
        s[i] = v;
        any |= v;
        root += RS_PRIM;
        if (root >= 255) root -= 255;
    }
    if (!any) return 0;

    // Berlekamp-Massey
    uint8_t lambda[RS_ROOTS + 1] = {1};
    uint8_t b[RS_ROOTS + 1] = {1};
    uint8_t t[RS_ROOTS + 1];
    int L = 0;
    int m = 1;
    uint8_t bb = 1;
    for (int n = 0; n < RS_ROOTS; n++) {
        uint8_t d = s[n];
        for (int i = 1; i <= L; i++) d ^= mul(lambda[i], s[n - i]);
        if (!d) {
            m++;
            continue;
        }
        int l = log_[d] + 255 - log_[bb];
        if (l >= 255) l -= 255;
        const uint8_t coef = exp_[l];
        const bool grow = 2 * L <= n;
        if (grow) memcpy(t, lambda, sizeof(t));
        for (int i = 0; i + m <= RS_ROOTS; i++) lambda[i + m] ^= mul(coef, b[i]);
        if (grow) {
            L = n + 1 - L;
            memcpy(b, t, sizeof(b));
            bb = d;
            m = 1;
        } else {
            m++;
        }
    }
    if (L > RS_ROOTS / 2) return -1;

    // omega = s * lambda mod x^32
    uint8_t omega[RS_ROOTS];
    for (int i = 0; i < RS_ROOTS; i++) {
        uint8_t v = 0;
        for (int j = 0; j <= i && j <= L; j++) v ^= mul(s[i - j], lambda[j]);
        omega[i] = v;
    }

    // Chien search over the positions p = 254 - j, X^-1 = alpha^(-11 p); then Forney
    int found = 0;
    int lx = 0;  // log of X^-1
    for (int p = 0; p < RS_N; p++) {
        uint8_t v = 0;
        for (int i = L; i >= 0; i--) {
            if (v) {
                int l = log_[v] + lx;
                if (l >= 255) l -= 255;
                v = exp_[l];
            }
            v ^= lambda[i];
        }
        if (!v) {
            uint8_t num = 0;
            for (int i = RS_ROOTS - 1; i >= 0; i--) {
                if (num) {
                    int l = log_[num] + lx;
                    if (l >= 255) l -= 255;
                    num = exp_[l];
                }
                num ^= omega[i];
            }
            uint8_t den = 0;
            int lx2 = lx * 2;
            if (lx2 >= 255) lx2 -= 255;
            int pw = 0;
            for (int i = 1; i <= L; i += 2) {
                if (lambda[i]) {
                    int l = log_[lambda[i]] + pw;
                    if (l >= 255) l -= 255;
                    den ^= exp_[l];
                }
                pw += lx2;
                if (pw >= 255) pw -= 255;
            }
            if (!den) return -1;
            if (num) {
                // e = X^(1 - fcr) num / den
                int l = ((RS_FCR - 1) * lx) % 255 + log_[num];
                if (l >= 255) l -= 255;
                l += 255 - log_[den];
                if (l >= 255) l -= 255;
                cw[RS_N - 1 - p] ^= exp_[l];
            }
            found++;
        }
        lx -= RS_PRIM;
        if (lx < 0) lx += 255;
    }
    return (found == L) ? found : -1;
}

void Decoder::frame(uint8_t* data) {
    stats_.frames++;

    for (size_t i = 0, k = 0; i < FRAME_BYTES; i++) {
        data[i] ^= pn_[k];
        if (++k == 255) k = 0;
    }

    bool fixed = false;
    for (int c = 0; c < RS_INTERLEAVE; c++) {
        uint8_t cw[RS_N];
        for (int j = 0; j < RS_N; j++) cw[j] = data[j * RS_INTERLEAVE + c];
        const int r = rs_decode(cw);
        if (r < 0) {
            stats_.rs_failed++;
            in_sync_ = false;
            return;
        }
        if (r > 0) {
            fixed = true;
            for (int j = 0; j < RS_N; j++) data[j * RS_INTERLEAVE + c] = cw[j];
        }
    }
    if (fixed) stats_.rs_fixed++;

    if ((data[0] >> 6) != 1) return;
    if ((data[1] & 0x3F) != VCID_IMAGE) return;

    const uint32_t counter = ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 8) | data[4];
    if (have_counter_ && counter != ((counter_ + 1) & 0xFFFFFF)) in_sync_ = false;
    have_counter_ = true;
    counter_ = counter;

    const uint16_t first_header = ((data[8] & 0x07) << 8) | data[9];
    zone(&data[VCDU_DATA], MPDU_DATA, first_header);
}

void Decoder::zone(const uint8_t* data, size_t length, uint16_t first_header) {
    size_t i = 0;
    if (!in_sync_) {
        if (first_header >= length) return;  // no packet starts here
        i = first_header;
        in_sync_ = true;
        packet_length_ = 0;
        skip_ = 0;
    }

    while (i < length) {
        if (skip_) {
            size_t k = length - i;
            if (k > skip_) k = skip_;
            skip_ -= k;
            i += k;
            continue;
        }
        packet_[packet_length_++] = data[i++];
        if (packet_length_ == 6) {
            if (packet_[0] >> 5) {  // version 0 only, lost the thread
                in_sync_ = false;
                return;
            }
            packet_total_ = 7 + (((size_t)packet_[4] << 8) | packet_[5]);
            if (packet_total_ > MAX_PACKET) {
                skip_ = packet_total_ - 6;
                packet_length_ = 0;
            }
        } else if (packet_length_ > 6 && packet_length_ == packet_total_) {
            packet(packet_, packet_total_);
            packet_length_ = 0;
        }
    }
}

void Decoder::packet(const uint8_t* p, size_t length) {
    const uint16_t apid = ((p[0] & 0x07) << 8) | p[1];
    if (apid < APID_FIRST || apid > APID_LAST) return;
    if (length <= 6 + SEGMENT_HEADER) return;
    const uint8_t* d = &p[6];
    const uint8_t mcu = d[8];
    if (d[9] != 0 || d[10] != 0 || d[11] != 0xFF || d[12] != 0xF0) return;
    if (mcu % MCUS_PER_PACKET || mcu >= STRIP_WIDTH / 8) return;

    // The sequence count runs over all the APIDs, 43 packets a strip. Unwrap it, then the
    // first packet of a channel in a strip is at u = sequence - (mcu / 14).
    const uint16_t seq = ((p[2] & 0x3F) << 8) | p[3];
    if (!have_sequence_) {
        sequence_ = 0x100000 + seq;
        have_sequence_ = true;
    } else {
        sequence_ += (uint16_t)(seq - last_sequence_) & 0x3FFF;
    }
    last_sequence_ = seq;
    const uint32_t u = sequence_ - mcu / MCUS_PER_PACKET;

    // strips start with the lowest APID
    if (!have_phase_ || apid < phase_apid_) {
        if (have_phase_ && strip_has_data_) emit();
        phase_ = u % PACKETS_PER_STRIP;
        phase_apid_ = apid;
        have_phase_ = true;
        have_strip_ = false;
    }
    const uint32_t strip = (u - phase_) / PACKETS_PER_STRIP;

    if (!have_strip_) {
        strip_ = strip;
        have_strip_ = true;
    }
    if (strip < strip_) return;
    if (strip > strip_) {
        if (strip_has_data_) {
            emit();
            uint32_t gap = strip - strip_ - 1;
            if (gap > MAX_GAP_STRIPS) gap = MAX_GAP_STRIPS;
            while (gap--) emit();
        }
        strip_ = strip;
    }

    bool wanted = false;
    for (size_t c = 0; c < PLANES; c++) wanted |= apids_[c] == apid;
    if (!wanted) return;

    const uint8_t q = d[13];
    if (q != quality_) {
        quality_ = q;
        const bool low = (q >= 20 && q < 50);
        for (int i = 0; i < 64; i++) {
            int32_t v = low ? (100 * quant_base[i] + q) / (2 * q)
                            : ((200 - 2 * (int32_t)q) * quant_base[i] + 50) / 100;
            quant_[i] = (v < 1) ? 1 : (uint16_t)v;
        }
    }

    stats_.packets++;
    segment(&d[SEGMENT_HEADER], length - 6 - SEGMENT_HEADER, mcu, apid);
}

void Decoder::segment(const uint8_t* data, size_t length, uint8_t mcu, uint8_t apid) {
    bits_ = data;
    bit_count_ = length * 8;
    bit_pos_ = 0;
    dc_ = 0;

    uint8_t px[64];
    for (uint8_t k = 0; k < MCUS_PER_PACKET; k++) {
        if (!block(px)) break;
        if (!planes_) continue;
        const size_t x = ((size_t)mcu + k) * 8;
        for (size_t c = 0; c < PLANES; c++) {
            if (apids_[c] != apid) continue;
            uint8_t* dst = planes_ + c * PLANE_BYTES + x;
            for (int y = 0; y < 8; y++) memcpy(dst + y * STRIP_WIDTH, &px[y * 8], 8);
        }
        strip_has_data_ = true;
    }
}

int Decoder::read_bits(uint8_t n) {
    if (bit_pos_ + n > bit_count_) return -1;
    int v = 0;
    for (uint8_t i = 0; i < n; i++) {
        v = (v << 1) | ((bits_[bit_pos_ >> 3] >> (7 - (bit_pos_ & 7))) & 1);
        bit_pos_++;
    }
    return v;
}

// canonical Huffman code; values nullptr means the value is the index
int Decoder::huffman(const uint8_t* counts, const uint8_t* values) {
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 0; len < 16; len++) {
        const int b = read_bits(1);
        if (b < 0) return -1;
        code |= b;
        const int count = counts[len];
        if (code - count < first) {
            const int k = index + (code - first);
            return values ? values[k] : k;
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

bool Decoder::block(uint8_t* pixels) {
    int32_t coef[64];
    memset(coef, 0, sizeof(coef));

    const int cat = huffman(dc_counts, nullptr);
    if (cat < 0 || cat > 11) return false;
    if (cat) {
        const int v = read_bits(cat);
        if (v < 0) return false;
        dc_ += extend(v, cat);
    }
    coef[0] = dc_ * quant_[0];

    for (int k = 1; k < 64;) {
        const int sym = huffman(ac_counts, ac_values);
        if (sym < 0) return false;
        const int run = sym >> 4;
        const int size = sym & 0x0F;
        if (!size) {
            if (run != 15) break;  // end of block
            k += 16;
            continue;
        }
        k += run;
        if (k > 63) return false;
        const int v = read_bits(size);
        if (v < 0) return false;
        const uint8_t z = zigzag[k];
        coef[z] = extend(v, size) * quant_[z];
        k++;
    }

    idct(coef, pixels);
    return true;
}

}  // namespace ui::external_app::lrpt_rx
