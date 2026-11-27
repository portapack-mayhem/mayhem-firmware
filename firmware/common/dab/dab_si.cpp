/*
 * Copyright (C) 2026 Mayhem firmware contributors
 *
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

#include "dab_si.hpp"
#include <cstring>
#include <cstdio>
#include <vector>

namespace dab {

/* ---- Character sets ---- */

namespace {

/* EBU Latin (TS 101 756 annex C) as Unicode. 0x20-0x7A are ASCII; the rest
 * differ. 0 marks an unassigned code. */
constexpr uint16_t kEbuLow[0x20] = {
    0x0000, 0x0118, 0x012E, 0x0172, 0x0102, 0x0116, 0x010E, 0x0218,
    0x021A, 0x010A, 0x0000, 0x0000, 0x0120, 0x0139, 0x017B, 0x0143,
    0x0105, 0x0119, 0x012F, 0x0173, 0x0103, 0x0117, 0x010F, 0x0219,
    0x021B, 0x010B, 0x0147, 0x011A, 0x0121, 0x013A, 0x017C, 0x0000};

constexpr uint8_t kEbuHighFirst = 0x7B;
constexpr uint16_t kEbuHigh[0x100 - kEbuHighFirst] = {
    0x00AB, 0x016F, 0x00BB, 0x013D, 0x0126,
    0x00E1, 0x00E0, 0x00E9, 0x00E8, 0x00ED, 0x00EC, 0x00F3, 0x00F2,
    0x00FA, 0x00F9, 0x00D1, 0x00C7, 0x015E, 0x00DF, 0x00A1, 0x0178,
    0x00E2, 0x00E4, 0x00EA, 0x00EB, 0x00EE, 0x00EF, 0x00F4, 0x00F6,
    0x00FB, 0x00FC, 0x00F1, 0x00E7, 0x015F, 0x011F, 0x0131, 0x00FF,
    0x0136, 0x0145, 0x00A9, 0x0122, 0x011E, 0x011B, 0x0148, 0x0151,
    0x0150, 0x20AC, 0x00A3, 0x0024, 0x0100, 0x0112, 0x012A, 0x016A,
    0x0137, 0x0146, 0x013B, 0x0123, 0x013C, 0x0130, 0x0144, 0x0171,
    0x0170, 0x00BF, 0x013E, 0x00B0, 0x0101, 0x0113, 0x012B, 0x016B,
    0x00C1, 0x00C0, 0x00C9, 0x00C8, 0x00CD, 0x00CC, 0x00D3, 0x00D2,
    0x00DA, 0x00D9, 0x0158, 0x010C, 0x0160, 0x017D, 0x00D0, 0x013F,
    0x00C2, 0x00C4, 0x00CA, 0x00CB, 0x00CE, 0x00CF, 0x00D4, 0x00D6,
    0x00DB, 0x00DC, 0x0159, 0x010D, 0x0161, 0x017E, 0x0111, 0x0140,
    0x00C3, 0x00C5, 0x00C6, 0x0152, 0x0177, 0x00DD, 0x00D5, 0x00D8,
    0x00DE, 0x014A, 0x0154, 0x0106, 0x015A, 0x0179, 0x0164, 0x00F0,
    0x00E3, 0x00E5, 0x00E6, 0x0153, 0x0175, 0x00FD, 0x00F5, 0x00F8,
    0x00FE, 0x014B, 0x0155, 0x0107, 0x015B, 0x017A, 0x0165, 0x0127};

/* Latin Extended-A, U+0100-U+017F, folded to the base letter. */
constexpr char kLatinExtA[] =
    "AaAaAaCcCcCcCcDd"
    "DdEeEeEeEeEeGgGg"
    "GgGgHhHhIiIiIiIi"
    "IiIiJjKkkLlLlLlL"
    "lLlNnNnNnnNnOoOo"
    "OoOoRrRrRrSsSsSs"
    "SsTtTtTtUuUuUuUu"
    "UuUuWwYyYZzZzZzs";
static_assert(sizeof(kLatinExtA) == 0x80 + 1, "one letter per code point");

/* 0 means "drop". */
char display_char(uint32_t cp) {
    if (cp < 0x20 || cp == 0x7F)
        return 0;
    if (cp < 0x7F || (cp >= 0xA0 && cp <= 0xFF))
        return static_cast<char>(cp);
    if (cp >= 0x100 && cp < 0x180)
        return kLatinExtA[cp - 0x100];
    switch (cp) {
        case 0x0218:
            return 'S';
        case 0x0219:
            return 's';
        case 0x021A:
            return 'T';
        case 0x021B:
            return 't';
        case 0x2018:
        case 0x2019:
            return '\'';
        case 0x201C:
        case 0x201D:
            return '"';
        case 0x2013:
        case 0x2014:
            return '-';
        case 0x20AC:
            return 'E';
        default:
            return '?';
    }
}

uint32_t ebu_latin(uint8_t b) {
    if (b < 0x20)
        return kEbuLow[b];
    if (b < kEbuHighFirst)
        return b;
    return kEbuHigh[b - kEbuHighFirst];
}

}  // namespace

size_t to_display(const uint8_t* in, size_t length, uint8_t charset, char* out, size_t capacity) {
    size_t n = 0;
    size_t i = 0;
    while (i < length && n < capacity) {
        uint32_t cp;
        if (charset == kCharsetUtf8) {
            const uint8_t b = in[i++];
            size_t extra = 0;
            if (b < 0x80) {
                cp = b;
            } else if ((b & 0xE0) == 0xC0) {
                cp = b & 0x1F;
                extra = 1;
            } else if ((b & 0xF0) == 0xE0) {
                cp = b & 0x0F;
                extra = 2;
            } else if ((b & 0xF8) == 0xF0) {
                cp = b & 0x07;
                extra = 3;
            } else {
                cp = '?';
            }
            for (; extra > 0; --extra) {
                if (i >= length || (in[i] & 0xC0) != 0x80) {
                    cp = '?';
                    break;
                }
                cp = (cp << 6) | (in[i++] & 0x3F);
            }
        } else if (charset == kCharsetUcs2) {
            if (i + 1 >= length)
                break;
            cp = (static_cast<uint32_t>(in[i]) << 8) | in[i + 1];
            i += 2;
        } else {
            cp = ebu_latin(in[i++]);
        }
        const char c = display_char(cp);
        if (c != 0)
            out[n++] = c;
    }
    return n;
}

/* ---- Channel blocks ---- */

namespace {

/* Ascending frequency, so a sweep walks the table in order. */
constexpr Block band_iii[] = {
    {"5A", 174928000},
    {"5B", 176640000},
    {"5C", 178352000},
    {"5D", 180064000},
    {"6A", 181936000},
    {"6B", 183648000},
    {"6C", 185360000},
    {"6D", 187072000},
    {"7A", 188928000},
    {"7B", 190640000},
    {"7C", 192352000},
    {"7D", 194064000},
    {"8A", 195936000},
    {"8B", 197648000},
    {"8C", 199360000},
    {"8D", 201072000},
    {"9A", 202928000},
    {"9B", 204640000},
    {"9C", 206352000},
    {"9D", 208064000},
    {"10A", 209936000},
    {"10N", 210096000},
    {"10B", 211648000},
    {"10C", 213360000},
    {"10D", 215072000},
    {"11A", 216928000},
    {"11N", 217088000},
    {"11B", 218640000},
    {"11C", 220352000},
    {"11D", 222064000},
    {"12A", 223936000},
    {"12N", 224096000},
    {"12B", 225648000},
    {"12C", 227360000},
    {"12D", 229072000},
    {"13A", 230784000},
    {"13B", 232496000},
    {"13C", 234208000},
    {"13D", 235776000},
    {"13E", 237488000},
    {"13F", 239200000},
};

constexpr Block l_band[] = {
    {"LA", 1452960000},
    {"LB", 1454672000},
    {"LC", 1456384000},
    {"LD", 1458096000},
    {"LE", 1459808000},
    {"LF", 1461520000},
    {"LG", 1463232000},
    {"LH", 1464944000},
    {"LI", 1466656000},
    {"LJ", 1468368000},
    {"LK", 1470080000},
    {"LL", 1471792000},
    {"LM", 1473504000},
    {"LN", 1475216000},
    {"LO", 1476928000},
    {"LP", 1478640000},
    {"LQ", 1480352000},
    {"LR", 1482064000},
    {"LS", 1483776000},
    {"LT", 1485488000},
    {"LU", 1487200000},
    {"LV", 1488912000},
    {"LW", 1490624000},
};

const Block* find_in(const Block* table, size_t count, const char* name) {
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(table[i].name, name) == 0)
            return &table[i];
    }
    return nullptr;
}

}  // namespace

BlockTable band_iii_blocks() {
    return {band_iii, sizeof(band_iii) / sizeof(band_iii[0])};
}

BlockTable l_band_blocks() {
    return {l_band, sizeof(l_band) / sizeof(l_band[0])};
}

const Block* find_block(const char* name) {
    if (name == nullptr || name[0] == '\0')
        return nullptr;

    const auto biii = band_iii_blocks();
    if (const auto* found = find_in(biii.items, biii.count, name))
        return found;

    const auto lb = l_band_blocks();
    return find_in(lb.items, lb.count, name);
}

/* ---- FIG parsing ---- */

namespace {

constexpr uint8_t kEndMarker = 0xFF;

uint16_t read_u16(const uint8_t* p) {
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

uint32_t read_u32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

/* Labels are space padded to 16 bytes; the padding is trimmed. */
void copy_label(const uint8_t* src, uint8_t charset, char* dst) {
    size_t n = to_display(src, kLabelBytes, charset, dst, kLabelBytes);
    while (n > 0 && dst[n - 1] == ' ')
        --n;
    dst[n] = '\0';
}

}  // namespace

void FigParser::reset_counters() {
    fibs_accepted_ = 0;
    fibs_rejected_ = 0;
    figs_unhandled_ = 0;
    figs_malformed_ = 0;
}

bool FigParser::parse_fib(const uint8_t fib[kFibBytes], FigSink& sink) {
    if (fib == nullptr)
        return false;

    if (!fib_crc_ok(fib)) {
        ++fibs_rejected_;
        return false;
    }

    ++fibs_accepted_;

    size_t pos = 0;
    while (pos < kFibPayloadBytes) {
        const uint8_t header = fib[pos];
        if (header == kEndMarker)
            break;

        const uint8_t type = static_cast<uint8_t>(header >> 5);
        const size_t length = static_cast<size_t>(header & 0x1F);
        ++pos;

        if (length == 0)
            continue;
        if (pos + length > kFibPayloadBytes) {
            ++figs_malformed_;
            break;
        }

        dispatch(type, &fib[pos], length, sink);
        pos += length;
    }

    return true;
}

void FigParser::dispatch(uint8_t type, const uint8_t* body, size_t length, FigSink& sink) {
    switch (type) {
        case 0:
            parse_type0(body, length, sink);
            break;
        case 1:
            parse_type1(body, length, sink);
            break;
        default:
            ++figs_unhandled_;
            sink.on_unhandled(type, 0);
            break;
    }
}

void FigParser::parse_type0(const uint8_t* body, size_t length, FigSink& sink) {
    if (length < 1) {
        ++figs_malformed_;
        return;
    }

    const uint8_t extension = static_cast<uint8_t>(body[0] & 0x1F);
    const bool pd_long_sid = ((body[0] >> 5) & 1) != 0;

    const uint8_t* p = &body[1];
    const size_t remaining_total = length - 1;

    switch (extension) {
        case 0: {  // Ensemble information
            if (remaining_total < 4) {
                ++figs_malformed_;
                return;
            }
            const uint16_t eid = read_u16(p);
            const bool reconfiguration = ((p[2] >> 6) & 0x03) != 0;
            const uint16_t cif_count = static_cast<uint16_t>(((p[2] & 0x1F) << 8) | p[3]);
            sink.on_ensemble(eid, cif_count, reconfiguration);
            break;
        }

        case 1: {  // Subchannel organisation
            size_t offset = 0;
            while (offset + 3 <= remaining_total) {
                const uint8_t subch_id = static_cast<uint8_t>((p[offset] >> 2) & 0x3F);
                const uint16_t start_cu = static_cast<uint16_t>(((p[offset] & 0x03) << 8) | p[offset + 1]);
                const bool long_form = ((p[offset + 2] >> 7) & 1) != 0;

                ProtectionProfile protection{};
                uint16_t size_cu = 0;

                if (!long_form) {
                    /* Short form is UEP (MP2 only): table index kept, not resolved. */
                    protection.kind = ProtectionKind::Uep;
                    protection.level = static_cast<uint8_t>(p[offset + 2] & 0x3F);
                    protection.option_b = false;
                    size_cu = 0;
                    sink.on_subchannel(subch_id, start_cu, size_cu, protection);
                    offset += 3;
                } else {
                    if (offset + 4 > remaining_total) {
                        ++figs_malformed_;
                        return;
                    }
                    protection.kind = ProtectionKind::Eep;
                    /* 000 is EEP-A, 001 EEP-B; reserved values read as B
                     * would give a plausible but wrong bit rate. */
                    const uint8_t option = static_cast<uint8_t>((p[offset + 2] >> 4) & 0x07);
                    if (option > 1) {
                        ++figs_malformed_;
                        return;
                    }
                    protection.option_b = (option == 1);
                    protection.level = static_cast<uint8_t>(((p[offset + 2] >> 2) & 0x03) + 1);
                    size_cu = static_cast<uint16_t>(((p[offset + 2] & 0x03) << 8) | p[offset + 3]);
                    sink.on_subchannel(subch_id, start_cu, size_cu, protection);
                    offset += 4;
                }
            }
            break;
        }

        case 2: {  // Service organisation
            size_t offset = 0;
            const size_t sid_bytes = pd_long_sid ? 4 : 2;

            while (offset + sid_bytes + 1 <= remaining_total) {
                const uint32_t sid = pd_long_sid ? read_u32(&p[offset]) : read_u16(&p[offset]);
                offset += sid_bytes;

                const uint8_t component_count = static_cast<uint8_t>(p[offset] & 0x0F);
                ++offset;

                for (uint8_t c = 0; c < component_count; ++c) {
                    if (offset + 2 > remaining_total) {
                        ++figs_malformed_;
                        return;
                    }

                    const uint8_t tmid = static_cast<uint8_t>((p[offset] >> 6) & 0x03);
                    const bool primary = ((p[offset + 1] >> 1) & 1) != 0;

                    if (tmid == 0 || tmid == 1) {
                        const uint8_t ascty = static_cast<uint8_t>(p[offset] & 0x3F);
                        const uint8_t subch_id = static_cast<uint8_t>((p[offset + 1] >> 2) & 0x3F);
                        const ComponentKind kind = (tmid == 0) ? ComponentKind::MscStreamAudio : ComponentKind::MscStreamData;
                        sink.on_service_component(sid, subch_id, kind, ascty, primary);
                    } else if (tmid == 3) {
                        const uint16_t scid = static_cast<uint16_t>(((p[offset] & 0x3F) << 6) | ((p[offset + 1] >> 2) & 0x3F));
                        sink.on_packet_component(sid, scid);
                    }

                    offset += 2;
                }
            }
            break;
        }

        case 8: {  // Service component global definition
            /* EN 300 401 6.3.3, per component: SId 16/32 (P/D) | Ext 1 | Rfa 3 |
             * SCIdS 4 | L/S 1, then short form MSC/FIC 1 | Id 6 (a SubChId
             * when MSC/FIC is 0) or long form Rfa 3 | SCId 12; one Rfa byte
             * follows when Ext is set. */
            size_t offset = 0;
            const size_t sid_bytes = pd_long_sid ? 4 : 2;

            while (offset + sid_bytes + 2 <= remaining_total) {
                const uint32_t sid = pd_long_sid ? read_u32(&p[offset]) : read_u16(&p[offset]);
                offset += sid_bytes;

                const bool ext_flag = ((p[offset] >> 7) & 1) != 0;
                const uint8_t scids = static_cast<uint8_t>(p[offset] & 0x0F);
                ++offset;

                const bool long_form = ((p[offset] >> 7) & 1) != 0;

                if (!long_form) {
                    const bool fic = ((p[offset] >> 6) & 1) != 0;
                    const uint8_t id = static_cast<uint8_t>(p[offset] & 0x3F);
                    ++offset;
                    /* MSC/FIC 1: the Id is a FIDCId, not a subchannel. */
                    if (!fic)
                        sink.on_component_global_definition(sid, scids, id, !pd_long_sid);
                } else {
                    if (offset + 2 > remaining_total) {
                        ++figs_malformed_;
                        return;
                    }
                    /* Packet mode SCId; its subchannel is in FIG 0/3 (not parsed). */
                    offset += 2;
                }

                if (ext_flag) {
                    if (offset + 1 > remaining_total) {
                        ++figs_malformed_;
                        return;
                    }
                    ++offset;  // Rfa
                }
            }
            break;
        }

        case 9: {  // Country, LTO and international table
            if (remaining_total < 2) {
                ++figs_malformed_;
                return;
            }
            sink.on_country(p[1]);
            break;
        }

        case 10: {  // Date and time
            if (remaining_total < 4) {
                ++figs_malformed_;
                return;
            }
            DabTime t{};
            t.mjd = static_cast<uint32_t>(((p[0] & 0x7F) << 10) | (p[1] << 2) | ((p[2] >> 6) & 0x03));
            const bool utc_long = ((p[2] >> 3) & 1) != 0;
            t.hours = static_cast<uint8_t>(((p[2] & 0x07) << 2) | ((p[3] >> 6) & 0x03));
            t.minutes = static_cast<uint8_t>(p[3] & 0x3F);
            if (utc_long && remaining_total >= 6) {
                t.seconds = static_cast<uint8_t>((p[4] >> 2) & 0x3F);
                t.has_seconds = true;
            }
            sink.on_datetime(t);
            break;
        }

        case 13: {  // User application information
            size_t offset = 0;
            const size_t sid_bytes = pd_long_sid ? 4 : 2;
            while (offset + sid_bytes + 1 <= remaining_total) {
                const uint32_t sid = pd_long_sid ? read_u32(&p[offset]) : read_u16(&p[offset]);
                offset += sid_bytes;

                const uint8_t app_count = static_cast<uint8_t>(p[offset] & 0x0F);
                ++offset;

                for (uint8_t a = 0; a < app_count; ++a) {
                    if (offset + 2 > remaining_total) {
                        ++figs_malformed_;
                        return;
                    }
                    const uint16_t app_type = static_cast<uint16_t>((p[offset] << 3) | ((p[offset + 1] >> 5) & 0x07));
                    const uint8_t data_length = static_cast<uint8_t>(p[offset + 1] & 0x1F);
                    sink.on_user_application(sid, app_type);
                    offset += 2 + data_length;
                }
            }
            break;
        }

        case 17: {  // Programme type
            size_t offset = 0;
            while (offset + 4 <= remaining_total) {
                const uint32_t sid = read_u16(&p[offset]);
                sink.on_programme_type(sid, static_cast<uint8_t>(p[offset + 3] & 0x1F));
                offset += 4;
            }
            break;
        }

        default:
            ++figs_unhandled_;
            sink.on_unhandled(0, extension);
            break;
    }
}

void FigParser::parse_type1(const uint8_t* body, size_t length, FigSink& sink) {
    if (length < 1) {
        ++figs_malformed_;
        return;
    }

    const uint8_t extension = static_cast<uint8_t>(body[0] & 0x07);
    const uint8_t charset = static_cast<uint8_t>(body[0] >> 4);
    const uint8_t* p = &body[1];
    const size_t remaining_total = length - 1;

    char label[kLabelBufferBytes];

    switch (extension) {
        case 0:    // Ensemble label
        case 1: {  // Programme service label
            if (remaining_total < 2 + kLabelBytes) {
                ++figs_malformed_;
                return;
            }
            const uint32_t id = read_u16(p);
            copy_label(&p[2], charset, label);
            sink.on_label(id, (extension == 0) ? LabelKind::Ensemble : LabelKind::ProgrammeService, label);
            break;
        }

        case 5: {  // Data service label
            if (remaining_total < 4 + kLabelBytes) {
                ++figs_malformed_;
                return;
            }
            const uint32_t id = read_u32(p);
            copy_label(&p[4], charset, label);
            sink.on_label(id, LabelKind::DataService, label);
            break;
        }

        default:
            ++figs_unhandled_;
            sink.on_unhandled(1, extension);
            break;
    }
}

/* ---- Ensemble database ---- */

void EnsembleDatabase::clear() {
    for (size_t i = 0; i < kMaxServices; ++i)
        services_[i] = DabService{};
    service_count_ = 0;

    for (size_t i = 0; i < kMaxSubchannels; ++i)
        subchannels_[i] = Subchannel{};

    ensemble_label_[0] = '\0';
    ensemble_id_ = 0;
    has_ensemble_id_ = false;
    reconfiguration_ = false;
    ecc_ = 0;
}

DabService* EnsembleDatabase::find_or_add(uint32_t sid) {
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i].sid == sid)
            return &services_[i];
    }

    if (service_count_ >= kMaxServices)
        return nullptr;

    DabService& s = services_[service_count_++];
    s = DabService{};
    s.sid = sid;
    return &s;
}

const DabService* EnsembleDatabase::find_service(uint32_t sid) const {
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i].sid == sid)
            return &services_[i];
    }
    return nullptr;
}

void EnsembleDatabase::apply_subchannel(DabService& service) {
    if (service.subch_id >= kMaxSubchannels)
        return;

    const Subchannel& sub = subchannels_[service.subch_id];
    if (!sub.known)
        return;

    service.start_cu = sub.start_cu;
    service.size_cu = sub.size_cu;
    service.protection = sub.protection;
    service.bitrate = eep_bitrate(sub.size_cu, sub.protection);

    /* FIG 0/8 gives no ASCTy: UEP is Layer II only, so infer it from the
     * protection. A later FIG 0/2 overwrites this. */
    if (service.ascty == kAsctyUnknown && service.component_from_fig08) {
        service.ascty = (sub.protection.kind == ProtectionKind::Eep) ? kAsctyAac : kAsctyMp2;
        service.is_dab_plus = (service.ascty == kAsctyAac);
    }
}

void EnsembleDatabase::on_ensemble(uint16_t eid, uint16_t /*cif_count*/, bool reconfiguration) {
    ensemble_id_ = eid;
    has_ensemble_id_ = true;
    if (reconfiguration)
        reconfiguration_ = true;
}

void EnsembleDatabase::on_subchannel(uint8_t subch_id, uint16_t start_cu, uint16_t size_cu, const ProtectionProfile& protection) {
    if (subch_id >= kMaxSubchannels)
        return;

    Subchannel& sub = subchannels_[subch_id];
    sub.known = true;
    sub.start_cu = start_cu;
    sub.size_cu = size_cu;
    sub.protection = protection;

    /* A subchannel may arrive after the services that reference it. */
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i].subch_id == subch_id)
            apply_subchannel(services_[i]);
    }
}

void EnsembleDatabase::on_service_component(uint32_t sid, uint8_t subch_id, ComponentKind kind, uint8_t ascty, bool primary) {
    if (kind != ComponentKind::MscStreamAudio)
        return;

    DabService* s = find_or_add(sid);
    if (s == nullptr)
        return;

    /* Never replace a primary component with a secondary one. */
    if (s->subch_id != kNoSubchannel && !primary && s->primary)
        return;

    s->has_audio_component = true;
    s->component_from_fig08 = false;
    s->subch_id = subch_id;
    s->ascty = ascty;
    s->is_dab_plus = (ascty == kAsctyAac);
    s->primary = primary;
    apply_subchannel(*s);
}

void EnsembleDatabase::on_component_global_definition(uint32_t sid, uint8_t scids, uint8_t subch_id, bool programme_service) {
    /* A 32-bit SId is a data service (FIG 0/2 P/D flag): never audio. */
    if (!programme_service)
        return;
    if (subch_id >= kMaxSubchannels)
        return;

    DabService* s = find_or_add(sid);
    if (s == nullptr)
        return;

    /* FIG 0/2 carries the ASCTy and the primary flag; never overwrite it. */
    if (s->has_audio_component && !s->component_from_fig08)
        return;

    /* FIG 0/8 has no primary flag: keep the lowest SCIdS. */
    if (s->component_from_fig08 && scids > s->scids)
        return;

    s->has_audio_component = true;
    s->component_from_fig08 = true;
    s->scids = scids;
    s->subch_id = subch_id;
    apply_subchannel(*s);
}

void EnsembleDatabase::on_label(uint32_t id, LabelKind kind, const char* text) {
    if (text == nullptr)
        return;

    if (kind == LabelKind::Ensemble) {
        std::strncpy(ensemble_label_, text, kLabelBytes);
        ensemble_label_[kLabelBytes] = '\0';
        return;
    }

    DabService* s = find_or_add(id);
    if (s == nullptr)
        return;

    std::strncpy(s->label, text, kLabelBytes);
    s->label[kLabelBytes] = '\0';
}

void EnsembleDatabase::on_programme_type(uint32_t sid, uint8_t pty) {
    DabService* s = find_or_add(sid);
    if (s != nullptr)
        s->pty = pty;
}

void EnsembleDatabase::on_country(uint8_t ecc) {
    ecc_ = ecc;
}

void EnsembleDatabase::on_user_application(uint32_t sid, uint16_t app_type) {
    if (app_type != kUserAppSlideshow)
        return;

    DabService* s = find_or_add(sid);
    if (s != nullptr)
        s->has_slideshow = true;
}

bool EnsembleDatabase::is_complete() const {
    if (ensemble_label_[0] == '\0' || !has_ensemble_id_)
        return false;
    if (service_count_ == 0)
        return false;

    size_t audio_services = 0;
    for (size_t i = 0; i < service_count_; ++i) {
        if (!services_[i].has_audio_component)
            continue;

        ++audio_services;
        if (!services_[i].is_resolved())
            return false;
    }

    return audio_services > 0;
}

/* ---- Station cache ---- */

namespace {

/* Splits on unescaped commas. */
bool split_fields(const char* line, std::vector<std::string>& fields) {
    fields.clear();
    std::string current;
    bool escaped = false;

    for (const char* p = line; *p != '\0'; ++p) {
        if (*p == '\r' || *p == '\n')
            break;

        if (escaped) {
            current.push_back(*p);
            escaped = false;
            continue;
        }

        if (*p == '\\') {
            current.push_back(*p);
            escaped = true;
            continue;
        }

        if (*p == ',') {
            fields.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(*p);
    }

    if (escaped)
        return false;  // trailing backslash: malformed

    fields.push_back(current);
    return true;
}

bool parse_u32(const std::string& s, uint32_t& out) {
    /* Ten digits is the widest a uint32 can be. */
    if (s.empty() || s.size() > 10)
        return false;
    for (char c : s) {
        if (c < '0' || c > '9')
            return false;
    }
    /* Not strtoul: on the 32-bit target it saturates at ULONG_MAX, so overflow
     * would go unnoticed. */
    uint64_t v = 0;
    for (char c : s) {
        v = v * 10 + static_cast<uint64_t>(c - '0');
        if (v > 0xFFFFFFFFull)
            return false;
    }

    out = static_cast<uint32_t>(v);
    return true;
}

}  // namespace

std::string escape_field(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '\\' || c == ',')
            out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

std::string unescape_field(const std::string& value) {
    std::string out;
    bool escaped = false;
    for (char c : value) {
        if (escaped) {
            out.push_back(c);
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            out.push_back(c);
        }
    }
    return out;
}

size_t serialise_ensemble(const EnsembleDatabase& db,
                          const char* block,
                          uint32_t timestamp,
                          std::string& out) {
    if (block == nullptr)
        return 0;

    size_t written = 0;
    char numbuf[64];

    for (size_t i = 0; i < db.service_count(); ++i) {
        const DabService& s = db.services()[i];
        if (!s.is_playable())
            continue;

        out += escape_field(block);
        out += ',';

        std::snprintf(numbuf, sizeof(numbuf), "%u", static_cast<unsigned>(db.ensemble_id()));
        out += numbuf;
        out += ',';

        out += escape_field(db.ensemble_label());
        out += ',';

        std::snprintf(numbuf, sizeof(numbuf), "%u", static_cast<unsigned>(s.sid));
        out += numbuf;
        out += ',';

        out += escape_field(s.label);
        out += ',';

        std::snprintf(numbuf, sizeof(numbuf), "%u,%u,%u,%u,%u,%u,%u,%u",
                      static_cast<unsigned>(s.subch_id),
                      static_cast<unsigned>(s.start_cu),
                      static_cast<unsigned>(s.size_cu),
                      static_cast<unsigned>(s.protection.level),
                      static_cast<unsigned>(s.protection.option_b ? 1 : 0),
                      static_cast<unsigned>(s.bitrate),
                      static_cast<unsigned>(s.is_dab_plus ? 1 : 0),
                      static_cast<unsigned>(timestamp));
        out += numbuf;
        out += '\n';

        ++written;
    }

    return written;
}

bool parse_cache_line(const char* line, CachedService& out) {
    if (line == nullptr || line[0] == '\0' || line[0] == '#')
        return false;

    std::vector<std::string> f;
    if (!split_fields(line, f))
        return false;
    if (f.size() != 13)
        return false;

    CachedService parsed;
    parsed.block = unescape_field(f[0]);
    if (parsed.block.empty())
        return false;

    uint32_t v = 0;

    if (!parse_u32(f[1], v) || v > 0xFFFF)
        return false;
    parsed.eid = static_cast<uint16_t>(v);

    parsed.ensemble_label = unescape_field(f[2]);

    if (!parse_u32(f[3], v))
        return false;
    parsed.sid = v;

    parsed.service_label = unescape_field(f[4]);
    if (parsed.service_label.empty())
        return false;

    if (!parse_u32(f[5], v) || v > 63)
        return false;
    parsed.subch_id = static_cast<uint8_t>(v);

    /* A CIF is 864 CUs. */
    if (!parse_u32(f[6], v) || v > 863)
        return false;
    parsed.start_cu = static_cast<uint16_t>(v);

    if (!parse_u32(f[7], v) || v == 0 || v > 864)
        return false;
    parsed.size_cu = static_cast<uint16_t>(v);

    if (!parse_u32(f[8], v) || v < 1 || v > 4)
        return false;
    parsed.protection.kind = ProtectionKind::Eep;
    parsed.protection.level = static_cast<uint8_t>(v);

    if (!parse_u32(f[9], v) || v > 1)
        return false;
    parsed.protection.option_b = (v == 1);

    if (!parse_u32(f[10], v))
        return false;
    parsed.bitrate = v;

    if (!parse_u32(f[11], v) || v > 1)
        return false;
    parsed.is_dab_plus = (v == 1);

    if (!parse_u32(f[12], v))
        return false;
    parsed.last_seen = v;

    out = parsed;
    return true;
}

} /* namespace dab */
