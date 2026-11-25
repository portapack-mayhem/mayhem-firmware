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

#include "dab_fig.hpp"
#include "dab_crc.hpp"
#include "dab_charset.hpp"

#include <cstring>

namespace dab {

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

} /* namespace dab */
