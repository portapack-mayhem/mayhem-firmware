#include "ui_flex_tx.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"
#include "ui_textentry.hpp"
#include "portapack_persistent_memory.hpp"
#include "portapack_shared_memory.hpp"
#include "rtc_time.hpp"

#include <cstring>

using namespace portapack;

namespace ui::external_app::flex_tx {

#define MAX_FLEX_MSG 240
#define BCH_POLY 0x769
#define DATA_MASK 0x1FFFFF

// ===== Bit reversal =====

static uint32_t reverse_bits32(uint32_t v) {
    v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
    v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
    v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
    v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
    v = (v >> 16) | (v << 16);
    return v;
}

// ===== BCH(31,21) encoder =====

static uint32_t flex_encode_word(uint32_t dw) {
    uint32_t data = dw >> 11;
    uint32_t dividend = data << 10;
    for (int i = 30; i >= 10; i--) {
        if ((dividend >> i) & 1)
            dividend ^= BCH_POLY << (i - 10);
    }
    uint32_t ecc = dividend & 0x3FF;
    uint32_t code31 = (data << 10) | ecc;
    uint32_t p = 0, tmp = code31;
    while (tmp) {
        p ^= (tmp & 1);
        tmp >>= 1;
    }
    return (code31 << 1) | p;
}

static uint32_t flex_enc(uint32_t data21) {
    return flex_encode_word(reverse_bits32(data21));
}

// ===== Checksum =====

static uint32_t flex_checksum(uint32_t d) {
    uint32_t s = (d & 0xF) + ((d >> 4) & 0xF) + ((d >> 8) & 0xF) +
                 ((d >> 12) & 0xF) + ((d >> 16) & 0xF) + ((d >> 20) & 0x1);
    return (d & ~0xFU) | ((0xF - (s & 0xF)) & 0xF);
}

// ===== Year equivalence =====

static int flex_is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int flex_jan1_dow(int y) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    return (y + y / 4 - y / 100 + y / 400 + t[0] + 1) % 7;
}

static int flex_equiv_year(int year) {
    if (year >= 1994 && year <= 2025)
        return year;
    int target_leap = flex_is_leap(year);
    int target_dow = flex_jan1_dow(year);
    for (int y = 2025; y >= 1994; y--) {
        if (flex_is_leap(y) == target_leap &&
            flex_jan1_dow(y) == target_dow)
            return y;
    }
    return 1994 + ((year - 1994) & 0x1F);
}

// ===== FIW =====

static uint32_t flex_fiw(uint32_t cycle, uint32_t frame, uint32_t roaming) {
    uint32_t d = 0;
    d |= (cycle & 0xF) << 4;
    d |= (frame & 0x7F) << 8;
    d |= (roaming & 1) << 15;
    // r=0, t=0 (single transmission, no low traffic flags)
    return flex_enc(flex_checksum(d));
}

// ===== BIW1 =====

static uint32_t flex_biw1(uint32_t astart, uint32_t vstart, uint32_t collapse) {
    uint32_t d = 0;
    d |= ((astart - 1) & 0x03) << 8;
    d |= (vstart & 0x3F) << 10;
    d |= (collapse & 0x07) << 18;
    return flex_enc(flex_checksum(d));
}

// ===== BIW Date (type 001) =====

static uint32_t flex_biw_date(uint32_t year_field, uint32_t month, uint32_t day) {
    uint32_t d = 0;
    d |= (1U) << 4;  // type = 001
    d |= (year_field & 0x1F) << 7;
    d |= (day & 0x1F) << 12;
    d |= (month & 0x0F) << 17;
    return flex_enc(flex_checksum(d));
}

// ===== BIW Time (type 010) =====

static uint32_t flex_biw_time(uint32_t hour, uint32_t minute, uint32_t second_step) {
    uint32_t d = 0;
    d |= (2U) << 4;  // type = 010
    d |= (hour & 0x1F) << 7;
    d |= (minute & 0x3F) << 12;
    d |= (second_step & 0x07) << 18;
    return flex_enc(flex_checksum(d));
}

// ===== BIW SysInfo (type 101) =====

static uint32_t flex_biw_sysinfo(uint32_t a_type, uint32_t info) {
    uint32_t d = 0;
    d |= (5U) << 4;  // type = 101
    d |= (a_type & 0x0F) << 7;
    d |= (info & 0x03FF) << 11;
    return flex_enc(flex_checksum(d));
}

// ===== BIW SSID1 (type 000) =====

static uint32_t flex_biw_ssid1(uint32_t local_id, uint32_t coverage_zone) {
    uint32_t d = 0;
    // type = 000 (no bits to set)
    d |= (coverage_zone & 0x1F) << 7;
    d |= (local_id & 0x01FF) << 12;
    return flex_enc(flex_checksum(d));
}

// ===== BIW SSID2 (type 111) =====

static uint32_t flex_biw_ssid2(uint32_t country_code, uint32_t tmf) {
    uint32_t d = 0;
    d |= (7U) << 4;  // type = 111
    d |= (tmf & 0x0F) << 7;
    d |= (country_code & 0x03FF) << 11;
    return flex_enc(flex_checksum(d));
}

// ===== Short address =====

static uint32_t flex_short_addr(uint64_t capcode) {
    return flex_enc((uint32_t)(capcode + 0x8000) & DATA_MASK);
}

// ===== Long address (2 words) =====

static int flex_long_addr(uint64_t capcode, uint32_t out[2]) {
    uint64_t result;
    uint32_t w1, w2;
    if (capcode >= 2101249ULL && capcode <= 1075843072ULL) {
        result = capcode - 2068481ULL;
        w1 = (result % 32768) + 1;
        w2 = 2097151U - (result / 32768);
    } else if (capcode >= 1075843073ULL && capcode <= 3223326720ULL) {
        result = capcode - 2068481ULL;
        w1 = (result % 32768) + 1;
        w2 = (result / 32768) + 1933312U;
    } else if (capcode >= 3223326721ULL && capcode <= 4297068542ULL) {
        result = capcode - 2068479ULL;
        w1 = (result % 32768) + 2064383U;
        w2 = (result / 32768) + 1867776U;
    } else {
        return -1;
    }
    out[0] = flex_enc(w1 & DATA_MASK);
    out[1] = flex_enc(w2 & DATA_MASK);
    return 0;
}

// ===== Vector words =====

static uint32_t flex_alpha_vector(uint32_t mw_start, uint32_t mw_count) {
    uint32_t d = 0;
    d |= (5U & 0x07) << 4;
    d |= (mw_start & 0x7F) << 7;
    d |= (mw_count & 0x7F) << 14;
    return flex_enc(flex_checksum(d));
}

static uint32_t flex_numeric_vector(uint32_t type, uint32_t mw_start, uint32_t mw_count, uint32_t kbit) {
    uint32_t n_field = (mw_count > 0) ? mw_count - 1 : 0;
    uint32_t d = 0;
    d |= (type & 0x07) << 4;
    d |= (mw_start & 0x7F) << 7;
    d |= (n_field & 0x07) << 14;
    d |= (kbit & 0x0F) << 17;
    return flex_enc(flex_checksum(d));
}

// Short vector (type 2, t=00): BCD digits in vector word
// Short addr: 3 digits in d0-d11. Long addr: 3 + 5 in 2nd word.
// Tone-only: call with empty string (all digits become space 0xC).
static uint32_t flex_short_vector(int is_long, const std::string& msg, uint32_t* vy_out) {
    static const char bcd_chars[20] = "0123456789.U -][";
    auto to_bcd = [&](char c) -> uint8_t {
        for (int k = 0; k < 16; k++)
            if (bcd_chars[k] == c) return k;
        return 0xC;  // space
    };

    int max_digits = is_long ? 8 : 3;
    uint8_t digits[8];
    for (int i = 0; i < 8; i++) {
        digits[i] = (i < (int)msg.size() && i < max_digits) ? to_bcd(msg[i]) : 0xC;
    }

    uint32_t vw = 0;
    vw |= (2U & 0x07) << 4;  // type = 010
    // t1t0 = 00 (numeric) — bits 7-8 stay 0
    vw |= ((uint32_t)digits[0] & 0xF) << 9;
    vw |= ((uint32_t)digits[1] & 0xF) << 13;
    vw |= ((uint32_t)digits[2] & 0xF) << 17;

    if (is_long && vy_out) {
        uint32_t d2 = 0;
        for (int i = 0; i < 5 && (i + 3) < max_digits; i++)
            d2 |= ((uint32_t)digits[i + 3] & 0xF) << (i * 4);
        *vy_out = flex_enc(d2);
    }
    return flex_enc(flex_checksum(vw));
}

// ===== Block interleave =====

static void flex_interleave_block(uint32_t blk, uint32_t* frame_words) {
    uint32_t src[8];
    uint8_t dst[32];
    memcpy(src, frame_words + blk * 8, sizeof(src));
    for (uint32_t i = 0; i < 32; i++) {
        dst[i] = (uint8_t)((((src[0] >> (31 - i)) & 1) << 7) |
                           (((src[1] >> (31 - i)) & 1) << 6) |
                           (((src[2] >> (31 - i)) & 1) << 5) |
                           (((src[3] >> (31 - i)) & 1) << 4) |
                           (((src[4] >> (31 - i)) & 1) << 3) |
                           (((src[5] >> (31 - i)) & 1) << 2) |
                           (((src[6] >> (31 - i)) & 1) << 1) |
                           (((src[7] >> (31 - i)) & 1) << 0));
    }
    memcpy(frame_words + blk * 8, dst, sizeof(dst));
}

// ===== Alpha encoding =====

static int flex_encode_alpha(const std::string& msg, uint32_t* words, int max_words, uint32_t seq, uint32_t msg_r) {
    int wc = 0;
    uint32_t hdr = 0;
    hdr |= (3U << 11);
    hdr |= ((seq & 0x3F) << 13);
    hdr |= ((msg_r & 1U) << 19);
    words[wc++] = hdr;

    size_t ci = 0;
    uint32_t dw = 0;
    uint8_t ch;
    ch = (ci < msg.size()) ? msg[ci++] : 0x03;
    dw |= ((uint32_t)(ch & 0x7F)) << 7;
    ch = (ci < msg.size()) ? msg[ci++] : 0x03;
    dw |= ((uint32_t)(ch & 0x7F)) << 14;
    words[wc++] = dw;

    while (ci < msg.size() && wc < max_words) {
        dw = 0;
        for (int s = 0; s < 3; s++) {
            ch = (ci < msg.size()) ? msg[ci++] : 0x03;
            dw |= ((uint32_t)(ch & 0x7F)) << (s * 7);
        }
        words[wc++] = dw;
    }

    // Signature
    {
        uint32_t sig_sum = 0;
        for (int i = 1; i < wc; i++) {
            uint32_t c0 = (words[i] >> 0) & 0x7F;
            uint32_t c1 = (words[i] >> 7) & 0x7F;
            uint32_t c2 = (words[i] >> 14) & 0x7F;
            if (c0 != 0x03) sig_sum += c0;
            if (c1 != 0x03) sig_sum += c1;
            if (c2 != 0x03) sig_sum += c2;
        }
        words[1] = (words[1] & ~0x7FU) | ((~sig_sum) & 0x7F);
    }

    // K checksum
    {
        uint32_t k_sum = 0;
        for (int i = 0; i < wc; i++) {
            k_sum += words[i] & 0xFF;
            k_sum += (words[i] >> 8) & 0xFF;
            k_sum += (words[i] >> 16) & 0x1F;
        }
        words[0] |= ((~k_sum) & 0x3FF);
    }

    for (int i = 0; i < wc; i++)
        words[i] = flex_enc(words[i]);

    return wc;
}

// ===== Numeric BCD encoding =====

static int flex_encode_numeric(const std::string& msg, uint32_t* words, int max_words, uint32_t* k_out) {
    static const char bcd[20] = "0123456789.U -][";
    uint32_t mw[8] = {0};
    int bit = 2;
    int word_idx = 0;
    for (size_t i = 0; i < msg.size(); i++) {
        uint8_t nib = 0;
        for (int k = 0; k < 16; k++)
            if (bcd[k] == msg[i]) {
                nib = k;
                break;
            }
        for (int b = 0; b < 4; b++) {
            word_idx = bit / 21;
            if (word_idx >= 8) break;
            if (nib & (1 << b))
                mw[word_idx] |= (1U << (bit % 21));
            bit++;
        }
    }
    word_idx = (bit > 0) ? (bit - 1) / 21 : 0;
    {
        int end_bit = (word_idx + 1) * 21;
        while (bit + 4 <= end_bit) {
            for (int b = 0; b < 4; b++) {
                if (0x0C & (1 << b))
                    mw[bit / 21] |= (1U << (bit % 21));
                bit++;
            }
        }
    }
    uint32_t kb = 0;
    for (int i = 0; i <= word_idx; i++) {
        kb += mw[i] & 0xFF;
        kb += (mw[i] >> 8) & 0xFF;
        kb += (mw[i] >> 16) & 0x1F;
    }
    kb &= 0xFF;
    kb = (kb & 0x3F) + (kb >> 6);
    kb = ~kb;
    mw[0] |= ((kb >> 4) & 0x03);
    *k_out = kb & 0x0F;
    int wc = word_idx + 1;
    for (int i = 0; i < wc && i < max_words; i++)
        words[i] = flex_enc(mw[i]);
    return wc;
}

// ===== Sync patterns =====

static const uint8_t bs1[] = {0xAA, 0xAA, 0xAA, 0xAA};
static const uint8_t a1[] = {0x78, 0xF3, 0x59, 0x39};
static const uint8_t b_code[4] = {0x55, 0x55, 0x00, 0x00};
static const uint8_t ar[] = {0xCB, 0x20, 0x59, 0x39};

static void write_word(uint8_t*& p, uint32_t w) {
    *p++ = (w >> 24) & 0xFF;
    *p++ = (w >> 16) & 0xFF;
    *p++ = (w >> 8) & 0xFF;
    *p++ = w & 0xFF;
}

// ===== Build frame =====

static size_t build_frame(uint8_t* buf, uint64_t capcode, int msg_type, const std::string& msg, uint32_t cycle, uint32_t frame, uint32_t msg_r, const FlexBIWParams& bp) {
    uint8_t* p = buf;

    // S1 sync
    memcpy(p, bs1, 4);
    p += 4;
    memcpy(p, a1, 4);
    p += 4;
    memcpy(p, b_code, 2);
    p += 2;
    for (int i = 0; i < 4; i++) *p++ = ~a1[i];

    // FIW
    write_word(p, flex_fiw(cycle, frame, bp.roaming));

    // S2
    {
        uint8_t s2[5];
        memset(s2, 0, 5);
        uint64_t bits = 0;
        bits |= (uint64_t)0xA << 36;
        bits |= (uint64_t)0xED84 << 20;
        bits |= (uint64_t)0x5 << 16;
        bits |= (uint64_t)0x127B;
        s2[0] = (bits >> 32) & 0xFF;
        s2[1] = (bits >> 24) & 0xFF;
        s2[2] = (bits >> 16) & 0xFF;
        s2[3] = (bits >> 8) & 0xFF;
        s2[4] = bits & 0xFF;
        memcpy(p, s2, 5);
        p += 5;
    }

    // Build extra BIW words (max 3 slots)
    uint32_t extra_biw[4];
    int extra_count = 0;

    // Slot priority: SSID1 first (always when enabled), then Time, Date, TZ, SSID2
    if (bp.send_ssid1 && extra_count < 3)
        extra_biw[extra_count++] = flex_biw_ssid1(bp.local_id, bp.coverage_zone);
    if (bp.send_time && extra_count < 3) {
        uint32_t sec_step = (uint32_t)(bp.second * 2 / 15);  // 7.5s steps
        if (sec_step > 7) sec_step = 7;
        extra_biw[extra_count++] = flex_biw_time(bp.hour, bp.minute, sec_step);
    }
    if (bp.send_date && extra_count < 3) {
        uint32_t year_field = (uint32_t)(bp.year - 1994);
        extra_biw[extra_count++] = flex_biw_date(year_field, bp.month, bp.day);
    }
    if (bp.send_tz && extra_count < 3) {
        uint32_t tz_info = (uint32_t)bp.tz_code & 0x1F;
        if (bp.send_dst)
            tz_info |= (0U << 5);  // L0=0 means DST active
        else
            tz_info |= (1U << 5);  // L0=1 means standard time
        extra_biw[extra_count++] = flex_biw_sysinfo(0x04, tz_info);
    }
    if (bp.send_ssid2 && extra_count < 3)
        extra_biw[extra_count++] = flex_biw_ssid2(bp.country_code, 0);

    uint32_t fw[88];
    uint32_t mw[84];
    int mwc = 0;
    uint32_t num_k = 0;

    if (msg_type == 0)
        mwc = flex_encode_alpha(msg, mw, 84, bp.msg_number, msg_r);
    else if (msg_type == 1)
        mwc = flex_encode_numeric(msg, mw, 84, &num_k);

    // Address encoding
    int is_long = (capcode >= 2101249ULL);
    int addr_words = is_long ? 2 : 1;
    int vec_words = is_long ? 2 : 1;
    int astart = 1 + extra_count;
    int vstart = astart + addr_words;
    int mstart = vstart + vec_words;

    fw[0] = flex_biw1(astart, vstart, 0);

    // Extra BIW words (words 1..extra_count)
    for (int i = 0; i < extra_count; i++)
        fw[1 + i] = extra_biw[i];

    // Address words
    if (is_long) {
        uint32_t la[2];
        if (flex_long_addr(capcode, la) < 0) return 0;
        fw[astart] = la[0];
        fw[astart + 1] = la[1];
    } else {
        fw[astart] = flex_short_addr(capcode);
    }

    // Vector + message
    if (msg_type == 0) {
        fw[vstart] = flex_alpha_vector(mstart, mwc);
        if (is_long && mwc > 0) {
            fw[vstart + 1] = mw[0];
            for (int i = 0; i < mwc - 1 && (mstart + i) < 88; i++)
                fw[mstart + i] = mw[i + 1];
            mwc = (mwc > 0) ? mwc - 1 : 0;
        } else {
            for (int i = 0; i < mwc && (mstart + i) < 88; i++)
                fw[mstart + i] = mw[i];
        }
    } else if (msg_type == 1) {
        fw[vstart] = flex_numeric_vector(3, mstart, mwc, num_k);
        if (is_long) {
            fw[vstart + 1] = (mwc > 0) ? mw[0] : flex_enc(0);
            for (int i = 1; i < mwc && (mstart + i - 1) < 88; i++)
                fw[mstart + i - 1] = mw[i];
        } else {
            for (int i = 0; i < mwc && (mstart + i) < 88; i++)
                fw[mstart + i] = mw[i];
        }
    } else {
        // Type 2 = short/tone (empty msg), Type 3 = short numeric (msg digits)
        std::string short_msg = (msg_type == 3) ? msg : "";
        uint32_t short_vy = 0;
        fw[vstart] = flex_short_vector(is_long, short_msg, &short_vy);
        if (is_long)
            fw[vstart + 1] = short_vy;
    }

    // Idle fill
    int mf_words = mwc;
    if (is_long && mwc > 0 && msg_type == 1)
        mf_words = mwc - 1;
    for (int i = mstart + mf_words; i < 88; i++)
        fw[i] = (i & 1) ? 0x00000000 : 0xFFFFFFFF;

    for (int blk = 0; blk < 11; blk++)
        flex_interleave_block(blk, fw);

    memcpy(p, fw, 88 * 4);
    p += 88 * 4;

    // Trailing idle
    *p++ = 0xAA;
    *p++ = 0xAA;
    *p++ = 0xAA;
    *p++ = 0xAA;

    return (size_t)(p - buf);
}

// ===== ERS =====

static size_t build_ers(uint8_t* buf, int cycles) {
    uint8_t* p = buf;
    uint8_t ar_inv[4];
    for (int i = 0; i < 4; i++) ar_inv[i] = ~ar[i];
    for (int c = 0; c < cycles; c++) {
        *p++ = 0xAA;
        *p++ = 0xAA;
        memcpy(p, ar, 4);
        p += 4;
        *p++ = 0x55;
        *p++ = 0x55;
        memcpy(p, ar_inv, 4);
        p += 4;
    }
    return (size_t)(p - buf);
}

// ===== FlexParamsView =====

FlexParamsView::FlexParamsView(NavigationView& nav, FlexBIWParams& params)
    : params_(params), nav_(nav) {
    add_children({&labels,
                  &check_date, &field_year, &field_month, &field_day,
                  &check_time, &field_hour, &field_minute, &field_second,
                  &check_tz, &options_tz, &check_dst,
                  &check_ssid1, &field_local_id, &labels_cz, &field_coverage,
                  &check_ssid2, &field_country, &check_roaming,
                  &labels_msg, &field_msg_number,
                  &check_ers, &field_ers_count,
                  &button_save, &button_cancel});

    // Populate from params
    check_date.set_value(params_.send_date);
    field_year.set_value(params_.year);
    field_month.set_value(params_.month);
    field_day.set_value(params_.day);

    check_time.set_value(params_.send_time);
    field_hour.set_value(params_.hour);
    field_minute.set_value(params_.minute);
    field_second.set_value(params_.second);

    check_tz.set_value(params_.send_tz);
    options_tz.set_by_value(params_.tz_code);
    check_dst.set_value(params_.send_dst);

    check_ssid1.set_value(params_.send_ssid1);
    field_local_id.set_value(params_.local_id);
    field_coverage.set_value(params_.coverage_zone);

    check_ssid2.set_value(params_.send_ssid2);
    field_country.set_value(params_.country_code);
    check_roaming.set_value(params_.roaming);

    field_msg_number.set_value(params_.msg_number);

    check_ers.set_value(params_.send_ers);
    field_ers_count.set_value(params_.ers_count);

    // Roaming implies SSID1 + SSID2
    check_roaming.on_select = [this](Checkbox&, bool v) {
        on_roaming_changed(v);
    };

    button_save.on_select = [this, &nav](Button&) {
        int32_t yr = field_year.value();
        if (yr > 2025) {
            int equiv = flex_equiv_year(yr);
            nav.display_modal(
                "Year > 2025",
                "Valid: 1994-2025\nEquiv for " + to_string_dec_uint(yr) + ": " + to_string_dec_uint(equiv));
            field_year.set_value(equiv);
            return;
        }
        params_.send_date = check_date.value();
        params_.year = yr;
        params_.month = field_month.value();
        params_.day = field_day.value();
        params_.send_time = check_time.value();
        params_.hour = field_hour.value();
        params_.minute = field_minute.value();
        params_.second = field_second.value();
        params_.send_tz = check_tz.value();
        params_.tz_code = options_tz.selected_index_value();
        params_.send_dst = check_dst.value();
        params_.send_ssid1 = check_ssid1.value();
        params_.local_id = field_local_id.value();
        params_.coverage_zone = field_coverage.value();
        params_.send_ssid2 = check_ssid2.value();
        params_.country_code = field_country.value();
        params_.roaming = check_roaming.value();
        params_.msg_number = field_msg_number.value();
        params_.send_ers = check_ers.value();
        params_.ers_count = field_ers_count.value();
        nav.pop();
    };

    button_cancel.on_select = [&nav](Button&) {
        nav.pop();
    };
}

void FlexParamsView::on_roaming_changed(bool v) {
    if (v) {
        check_ssid1.set_value(true);
        check_ssid2.set_value(true);
    }
}

void FlexParamsView::focus() {
    button_save.focus();
}

// ===== Serial message handler =====

void FlexTXView::on_serial_msg(const FlexTosendMessage data) {
    field_capcode.set_value(data.capcode);
    capcode_value = data.capcode;
    options_type.set_selected_index(data.type);
    message = std::string((char*)data.msg, data.msglen);
    buffer = message;
    text_message.set(message);
    if (message.length() > 30 && message.length() <= 60)
        text_message_l2.set(message.substr(29));
    else if (message.length() > 60)
        text_message_l2.set(message.substr(29, 27) + "...");
    else
        text_message_l2.set("");
    field_capcode.dirty();
    options_type.dirty();
    text_message.dirty();
    text_message_l2.dirty();
    tx_view.focus();
    if (start_tx()) tx_view.set_transmitting(true);
}

// ===== TX =====

void FlexTXView::on_tx_progress(const uint32_t progress, const bool done) {
    if (done) {
        if (tx_phase_ == 0) return;  // stopped by user
        int ers_n = tx_steps_total_ - 2;
        if (tx_phase_ == 1 && ers_remaining_ > 0) {
            tx_step_++;
            text_capinfo.set("[" + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(tx_steps_total_) +
                             "] ERS " + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(ers_n));
            uint8_t* data = shared_memory.bb_data.data;
            size_t total = build_ers(data, 42);
            ers_remaining_--;
            uint32_t total_bits = total * 8;
            progressbar.set_max(total_bits / 64);
            baseband::set_fsk_data(total_bits, 2280000 / 1600, 4500, 64);
        } else if (tx_phase_ == 1) {
            tx_phase_ = 2;
            tx_step_++;
            text_capinfo.set("[" + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(tx_steps_total_) +
                             "] Frame 1/2 (new)");
            send_frame(1);
        } else if (tx_phase_ == 2) {
            tx_phase_ = 3;
            tx_step_++;
            text_capinfo.set("[" + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(tx_steps_total_) +
                             "] Frame 2/2 (dup)");
            send_frame(0);
        } else {
            biw_params_.msg_number = (biw_params_.msg_number + 1) & 63;
            transmitter_model.disable();
            progressbar.set_value(0);
            tx_view.set_transmitting(false);
            tx_phase_ = 0;
            set_dirty();  // repaint capcode info
        }
    } else {
        if (tx_phase_ != 0)
            progressbar.set_value(progress);
    }
}

bool FlexTXView::start_tx() {
    uint64_t capcode = field_capcode.to_integer();
    capcode_value = capcode;
    if (capcode < 1 || capcode > 4297068542ULL) {
        nav_.display_modal("Bad capcode", "Capcode: 1-4297068542");
        return false;
    }

    int msg_type = options_type.selected_index();
    if (msg_type == 1) {
        if (message.find_first_not_of("0123456789.U -][") != std::string::npos) {
            nav_.display_modal("Bad message", "Numeric: 0-9 . U - ] [ space");
            return false;
        }
    }

    transmitter_model.set_sampling_rate(2280000);
    transmitter_model.set_baseband_bandwidth(1'750'000);
    transmitter_model.enable();

    int ers_n = (biw_params_.send_ers && biw_params_.ers_count > 0) ? biw_params_.ers_count : 0;
    tx_steps_total_ = ers_n + 2;
    tx_step_ = 0;

    if (ers_n > 0) {
        tx_phase_ = 1;
        ers_remaining_ = ers_n;
        tx_step_++;
        text_capinfo.set("[" + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(tx_steps_total_) +
                         "] ERS " + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(ers_n));
        uint8_t* data = shared_memory.bb_data.data;
        size_t total = build_ers(data, 42);
        ers_remaining_--;
        uint32_t total_bits = total * 8;
        progressbar.set_max(total_bits / 64);
        baseband::set_fsk_data(total_bits, 2280000 / 1600, 4500, 64);
    } else {
        tx_phase_ = 2;
        tx_step_++;
        text_capinfo.set("[" + to_string_dec_uint(tx_step_) + "/" + to_string_dec_uint(tx_steps_total_) +
                         "] Frame 1/2 (new)");
        send_frame(1);
    }

    return true;
}

void FlexTXView::send_frame(uint32_t msg_r) {
    uint64_t capcode = field_capcode.to_integer();
    int msg_type = options_type.selected_index();

    uint8_t* data = shared_memory.bb_data.data;
    size_t total = 0;

    total += build_ers(data + total, 8);
    total += build_frame(data + total, capcode, msg_type, message, 0, 0, msg_r, biw_params_);

    uint32_t total_bits = total * 8;
    progressbar.set_max(total_bits / 64);
    baseband::set_fsk_data(total_bits, 2280000 / 1600, 4500, 64);
}

// ===== UI =====

void FlexTXView::focus() {
    field_capcode.focus();
}

FlexTXView::~FlexTXView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void FlexTXView::paint(Painter&) {
    message = buffer;
    text_message.set(message);
    if (message.length() > 30 && message.length() <= 60)
        text_message_l2.set(message.substr(29));
    else if (message.length() > 60)
        text_message_l2.set(message.substr(29, 27) + "...");
    else
        text_message_l2.set("");

    // Capcode info line
    uint64_t cap = field_capcode.to_integer();
    std::string info;
    if (cap == 0) {
        info = "Invalid";
    } else if (cap <= 1933312ULL) {
        info = "Short addr";
    } else if (cap >= 2062336ULL && cap <= 2062351ULL) {
        info = "Temp grp #" + to_string_dec_uint(cap - 2062336ULL);
    } else if (cap >= 2062352ULL && cap <= 2062367ULL) {
        info = "Operator msg";
    } else if (cap >= 2058240ULL && cap <= 2062335ULL) {
        info = "Network addr";
    } else if (cap >= 2041856ULL && cap <= 2058239ULL) {
        info = "Info service";
    } else if (cap > 1933312ULL && cap < 2101249ULL) {
        info = "Reserved";
    } else if (cap >= 2101249ULL && cap <= 4297068542ULL) {
        info = "Long addr";
    } else {
        info = "Invalid";
    }
    // Computed frame/phase for all valid addresses
    if (cap >= 1 && cap <= 4297068542ULL) {
        static const char ph[] = "ABCD";
        int frame = (int)((cap / 16) % 128);
        int phase = (int)((cap / 4) % 4);
        info += ", F" + to_string_dec_uint(frame) +
                " " + std::string(1, ph[phase]);
    }
    text_capinfo.set(info);
}

void FlexTXView::on_set_text(NavigationView& nav) {
    text_prompt(nav, buffer, MAX_FLEX_MSG, ENTER_KEYBOARD_MODE_ALPHA);
}

FlexTXView::FlexTXView(NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    // Init random msg number from RTC
    {
        rtc::RTC datetime;
        rtc_time::now(datetime);
        int seed = datetime.second() + datetime.minute() * 7 + datetime.hour();
        biw_params_.msg_number = seed & 63;
        if (biw_params_.msg_number == 0)
            biw_params_.msg_number = 1;
    }

    // Always populate date/time/dst from RTC
    {
        rtc::RTC datetime;
        rtc_time::now(datetime);
        int real_year = datetime.year();
        biw_params_.year = flex_equiv_year(real_year);
        biw_params_.month = datetime.month();
        biw_params_.day = datetime.day();
        biw_params_.hour = datetime.hour();
        biw_params_.minute = datetime.minute();
        biw_params_.second = datetime.second();
        biw_params_.send_dst = portapack::persistent_memory::dst_enabled() ? 1 : 0;
    }

    add_children({&labels, &text_capinfo, &field_capcode, &options_speed, &options_type,
                  &text_message, &text_message_l2,
                  &button_message, &button_params, &progressbar, &tx_view});

    options_speed.set_selected_index(0);
    options_type.set_selected_index(0);
    field_capcode.set_value(capcode_value);

    field_capcode.on_change = [this](SymField&) {
        set_dirty();  // trigger repaint to update capcode info text
    };

    button_message.on_select = [this, &nav](Button&) {
        this->on_set_text(nav);
    };

    button_params.on_select = [this, &nav](Button&) {
        nav.push<FlexParamsView>(biw_params_);
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view =
            nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        if (start_tx()) tx_view.set_transmitting(true);
    };

    tx_view.on_stop = [this]() {
        tx_phase_ = 0;
        tx_view.set_transmitting(false);
        transmitter_model.disable();
        progressbar.set_value(0);
        set_dirty();  // repaint capcode info
    };
}

}  // namespace ui::external_app::flex_tx
