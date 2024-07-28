/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_subghzd.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui {

void SubGhzDRecentEntryDetailView::update_data() {
    // process protocol data
    parseProtocol();
    // set text elements
    text_type.set(SubGhzDView::getSensorTypeName((FPROTO_SUBGHZD_SENSOR)entry_.sensorType));

    text_id.set("0x" + to_string_hex(serial));
    if (entry_.bits > 0) console.writeln("Bits: " + to_string_dec_uint(entry_.bits));
    if (btn != SD_NO_BTN) console.writeln("Btn: " + to_string_dec_uint(btn));
    if (cnt != SD_NO_CNT) console.writeln("Cnt: " + to_string_dec_uint(cnt));

    if (entry_.data != 0) console.writeln("Data: " + to_string_hex(entry_.data));
}

SubGhzDRecentEntryDetailView::SubGhzDRecentEntryDetailView(NavigationView& nav, const SubGhzDRecentEntry& entry)
    : nav_{nav},
      entry_{entry} {
    add_children({&button_done,
                  &text_type,
                  &text_id,
                  &console,
                  &labels});

    button_done.on_select = [&nav](const ui::Button&) {
        nav.pop();
    };
    update_data();
}

void SubGhzDRecentEntryDetailView::focus() {
    button_done.focus();
}

void SubGhzDView::focus() {
    field_frequency.focus();
}

SubGhzDView::SubGhzDView(NavigationView& nav)
    : nav_{nav} {
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &button_clear_list,
                  &recent_entries_view});

    baseband::run_image(portapack::spi_flash::image_tag_subghzd);

    button_clear_list.on_select = [this](Button&) {
        recent.clear();
        recent_entries_view.set_dirty();
    };
    field_frequency.set_step(10000);

    const Rect content_rect{0, header_height, screen_width, screen_height - header_height};
    recent_entries_view.set_parent_rect(content_rect);
    recent_entries_view.on_select = [this](const SubGhzDRecentEntry& entry) {
        nav_.push<SubGhzDRecentEntryDetailView>(entry);
    };
    baseband::set_subghzd_config(0, receiver_model.sampling_rate());  // 0=am
    receiver_model.enable();
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        on_tick_second();
    };
}

void SubGhzDView::on_tick_second() {
    for (auto& entry : recent) {
        entry.inc_age(1);
    }
    recent_entries_view.set_dirty();
}

void SubGhzDView::on_data(const SubGhzDDataMessage* data) {
    SubGhzDRecentEntry key{data->sensorType, data->data, data->bits};
    auto matching_recent = find(recent, key.key());
    if (matching_recent != std::end(recent)) {
        // Found within. Move to front of list, increment counter.
        (*matching_recent).reset_age();
        recent.push_front(*matching_recent);
        recent.erase(matching_recent);
    } else {
        recent.emplace_front(key);
        truncate_entries(recent, 64);
    }
    recent_entries_view.set_dirty();
}

SubGhzDView::~SubGhzDView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    receiver_model.disable();
    baseband::shutdown();
}

const char* SubGhzDView::getSensorTypeName(FPROTO_SUBGHZD_SENSOR type) {
    switch (type) {
        case FPS_PRINCETON:
            return "Princeton";
        case FPS_BETT:
            return "Bett";
        case FPS_CAME:
            return "Came";
        case FPS_PRASTEL:
            return "Prastel";
        case FPS_AIRFORCE:
            return "Airforce";
        case FPS_CAMEATOMO:
            return "Came Atomo";
        case FPS_CAMETWEE:
            return "Came Twee";
        case FPS_CHAMBCODE:
            return "Chamb Code";
        case FPS_CLEMSA:
            return "Clemsa";
        case FPS_DOITRAND:
            return "Doitrand";
        case FPS_DOOYA:
            return "Dooya";
        case FPS_FAAC:
            return "Faac";
        case FPS_GATETX:
            return "Gate TX";
        case FPS_HOLTEK:
            return "Holtek";
        case FPS_HOLTEKHT12X:
            return "Holtek HT12X";
        case FPS_HONEYWELL:
            return "Honeywell";
        case FPS_HONEYWELLWDB:
            return "Honeywell Wdb";
        case FPS_HORMANN:
            return "Hormann";
        case FPS_IDO:
            return "Ido 11x";
        case FPS_INTERTECHNOV3:
            return "InterTehcno v3";
        case FPS_KEELOQ:
            return "KeeLoq";
        case FPS_KINGGATESSTYLO4K:
            return "Kinggate Stylo4K";
        case FPS_LINEAR:
            return "Linear";
        case FPS_LINEARDELTA3:
            return "Linear Delta3";
        case FPS_MAGELLAN:
            return "Magellan";
        case FPS_MARANTEC:
            return "Marantec";
        case FPS_MASTERCODE:
            return "Mastercode";
        case FPS_MEGACODE:
            return "Megacode";
        case FPS_NERORADIO:
            return "Nero Radio";
        case FPS_NERO_SKETCH:
            return "Nero Sketch";
        case FPS_NICEFLO:
            return "Nice Flo";
        case FPS_NICEFLORS:
            return "Nice Flor S";
        case FPS_PHOENIXV2:
            return "Phoenix V2";
        case FPS_POWERSMART:
            return "PowerSmart";
        case FPS_SECPLUSV1:
            return "SecPlus V1";
        case FPS_SECPLUSV2:
            return "SecPlus V2";
        case FPS_SMC5326:
            return "SMC5326";
        case FPS_STARLINE:
            return "Star Line";
        case FPS_X10:
            return "X10";
        case FPS_LEGRAND:
            return "Legrand";
        case FPS_SOMIFY_KEYTIS:
            return "Somify Keytis";
        case FPS_SOMIFY_TELIS:
            return "Somify Telis";

        case FPS_Invalid:
        default:
            return "Unknown";
    }
}

std::string SubGhzDView::pad_string_with_spaces(int snakes) {
    std::string paddedStr(snakes, ' ');
    return paddedStr;
}

template <>
void RecentEntriesTable<ui::SubGhzDRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line{};
    line.reserve(30);

    line = SubGhzDView::getSensorTypeName((FPROTO_SUBGHZD_SENSOR)entry.sensorType);
    line = line + " " + to_string_hex(entry.data << 32);
    if (line.length() < 19) {
        line += SubGhzDView::pad_string_with_spaces(19 - line.length());
    } else {
        line = truncate(line, 19);
    }
    std::string ageStr = to_string_dec_uint(entry.age);
    std::string bitsStr = to_string_dec_uint(entry.bits);
    line += SubGhzDView::pad_string_with_spaces(5 - bitsStr.length()) + bitsStr;
    line += SubGhzDView::pad_string_with_spaces(4 - ageStr.length()) + ageStr;

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

// decoder helper functions

void atomo_decrypt(uint8_t* buff) {
    buff[0] = (buff[0] ^ 5) & 0x7F;
    uint8_t tmpB = (-buff[0]) & 0x7F;

    uint8_t bitCnt = 8;
    while (bitCnt < 59) {
        if ((tmpB & 0x18) && (((tmpB / 8) & 3) != 3)) {
            tmpB = ((tmpB << 1) & 0xFF) | 1;
        } else {
            tmpB = (tmpB << 1) & 0xFF;
        }

        if (tmpB & 0x80) {
            buff[bitCnt / 8] ^= (0x80 >> (bitCnt & 7));
        }

        bitCnt++;
    }
}

const uint32_t came_twee_magic_numbers_xor[15] = {
    0x0E0E0E00,
    0x1D1D1D11,
    0x2C2C2C22,
    0x3B3B3B33,
    0x4A4A4A44,
    0x59595955,
    0x68686866,
    0x77777777,
    0x86868688,
    0x95959599,
    0xA4A4A4AA,
    0xB3B3B3BB,
    0xC2C2C2CC,
    0xD1D1D1DD,
    0xE0E0E0EE,
};

// to save some byte of fw space, these will be inline. unreadeable? yes. needs a tons of free space? certanly. so sorry for this.

void SubGhzDRecentEntryDetailView::parseProtocol() {
    btn = SD_NO_BTN;
    cnt = SD_NO_CNT;
    serial = 0;

    if (entry_.sensorType == FPS_Invalid) return;

    if (entry_.sensorType == FPS_BETT) {
        return;  // needs dip pattern output.
    }

    if (entry_.sensorType == FPS_AIRFORCE || entry_.sensorType == FPS_PRASTEL || entry_.sensorType == FPS_CAME) {
        return;  // nothing
    }

    if (entry_.sensorType == FPS_CAMEATOMO) {
        entry_.data ^= 0xFFFFFFFFFFFFFFFF;
        entry_.data <<= 4;
        uint8_t pack[8] = {};
        pack[0] = (entry_.data >> 56);
        pack[1] = ((entry_.data >> 48) & 0xFF);
        pack[2] = ((entry_.data >> 40) & 0xFF);
        pack[3] = ((entry_.data >> 32) & 0xFF);
        pack[4] = ((entry_.data >> 24) & 0xFF);
        pack[5] = ((entry_.data >> 16) & 0xFF);
        pack[6] = ((entry_.data >> 8) & 0xFF);
        pack[7] = (entry_.data & 0xFF);

        atomo_decrypt(pack);

        // cnt_2 = pack[0];
        cnt = (uint16_t)pack[1] << 8 | pack[2];
        serial = (uint32_t)(pack[3]) << 24 | pack[4] << 16 | pack[5] << 8 | pack[6];

        uint8_t btn_decode = (pack[7] >> 4);
        if (btn_decode == 0x0) {
            btn = 0x1;
        } else if (btn_decode == 0x2) {
            btn = 0x2;
        } else if (btn_decode == 0x4) {
            btn = 0x3;
        } else if (btn_decode == 0x6) {
            btn = 0x4;
        }
        return;
    }

    if (entry_.sensorType == FPS_CAMETWEE) {
        uint8_t cnt_parcel = (uint8_t)(entry_.data & 0xF);
        uint32_t data = (uint32_t)(entry_.data & 0x0FFFFFFFF);

        data = (data ^ came_twee_magic_numbers_xor[cnt_parcel]);
        serial = data;
        data /= 4;
        btn = (data >> 4) & 0x0F;
        data >>= 16;
        data = (uint16_t)FProtoGeneral::subghz_protocol_blocks_reverse_key(data, 16);
        cnt = data >> 6;
        return;
    }

    if (entry_.sensorType == FPS_CHAMBCODE) {
        return;  // nothing
    }

    if (entry_.sensorType == FPS_CLEMSA) {
        serial = (entry_.data >> 2) & 0xFFFF;
        btn = (entry_.data & 0x03);
        return;
    }

    if (entry_.sensorType == FPS_DOITRAND) {
        cnt = (entry_.data >> 24) | ((entry_.data >> 15) & 0x1);
        btn = ((entry_.data >> 18) & 0x3);
        return;
    }

    if (entry_.sensorType == FPS_DOOYA) {
        serial = (entry_.data >> 16);
        if ((entry_.data >> 12) & 0x0F) {
            cnt = (entry_.data >> 8) & 0x0F;
        } else {
            cnt = 0xff;
        }
        btn = entry_.data & 0xFF;
        return;
    }

    if (entry_.sensorType == FPS_FAAC) {  // stripped down a lot.
        uint32_t code_fix = entry_.data >> 32;
        uint32_t code_hop = entry_.data & 0xFFFFFFFF;
        // uint32_t decrypt = 0;
        // uint64_t man;

        uint8_t data_tmp = 0;
        uint8_t data_prg[8];
        data_prg[0] = (code_hop & 0xFF);
        data_prg[1] = ((code_hop >> 8) & 0xFF);
        data_prg[2] = ((code_hop >> 16) & 0xFF);
        data_prg[3] = (code_hop >> 24);
        data_prg[4] = (code_fix & 0xFF);
        data_prg[5] = ((code_fix >> 8) & 0xFF);
        data_prg[6] = ((code_fix >> 16) & 0xFF);
        data_prg[7] = (code_fix >> 24);

        if (((data_prg[7] == 0x52) && (data_prg[6] == 0x0F) && (data_prg[0] == 0x00))) {
            // ProgMode ON
            for (uint8_t i = data_prg[1] & 0xF; i != 0; i--) {
                data_tmp = data_prg[2];

                data_prg[2] = data_prg[2] >> 1 | (data_prg[3] & 1) << 7;
                data_prg[3] = data_prg[3] >> 1 | (data_prg[4] & 1) << 7;
                data_prg[4] = data_prg[4] >> 1 | (data_prg[5] & 1) << 7;
                data_prg[5] = data_prg[5] >> 1 | (data_tmp & 1) << 7;
            }
            data_prg[2] ^= data_prg[1];
            data_prg[3] ^= data_prg[1];
            data_prg[4] ^= data_prg[1];
            data_prg[5] ^= data_prg[1];
            seed = data_prg[5] << 24 | data_prg[4] << 16 | data_prg[3] << 8 | data_prg[2];
            // uint32_t dec_prg_1 = data_prg[7] << 24 | data_prg[6] << 16 | data_prg[5] << 8 |                                 data_prg[4];
            // uint32_t dec_prg_2 = data_prg[3] << 24 | data_prg[2] << 16 | data_prg[1] << 8 |                                 data_prg[0];
            // entry_.data_2 = (uint64_t)dec_prg_1 << 32 | dec_prg_2;
            cnt = data_prg[1];
        } else {
            if (code_fix != 0x0) {
                serial = code_fix >> 4;
                btn = code_fix & 0xF;
            }
        }
        return;
    }

    if (entry_.sensorType == FPS_GATETX) {
        uint32_t code_found_reverse = FProtoGeneral::subghz_protocol_blocks_reverse_key(entry_.data, entry_.bits);
        serial = (code_found_reverse & 0xFF) << 12 | ((code_found_reverse >> 8) & 0xFF) << 4 | ((code_found_reverse >> 20) & 0x0F);
        btn = ((code_found_reverse >> 16) & 0x0F);
        return;
    }

    if (entry_.sensorType == FPS_HOLTEK) {
        if ((entry_.data & 0xF000000000) == 0x5000000000) {
            serial = FProtoGeneral::subghz_protocol_blocks_reverse_key((entry_.data >> 16) & 0xFFFFF, 20);
            uint16_t btn_ = entry_.data & 0xFFFF;
            if ((btn_ & 0xf) != 0xA) {
                btn = 0x1 << 4 | (btn_ & 0xF);
            } else if (((btn_ >> 4) & 0xF) != 0xA) {
                btn = 0x2 << 4 | ((btn_ >> 4) & 0xF);
            } else if (((btn_ >> 8) & 0xF) != 0xA) {
                btn = 0x3 << 4 | ((btn_ >> 8) & 0xF);
            } else if (((btn_ >> 12) & 0xF) != 0xA) {
                btn = 0x4 << 4 | ((btn_ >> 12) & 0xF);
            } else {
                btn = 0;
            }
        } else {
            serial = 0;
            btn = 0;
            cnt = 0;
        }
        return;
    }

    if (entry_.sensorType == FPS_HOLTEKHT12X) {
        btn = entry_.data & 0x0F;
        cnt = (entry_.data >> 4) & 0xFF;
        return;
    }

    if (entry_.sensorType == FPS_HONEYWELL) {
        serial = (entry_.data >> 24) & 0xFFFFF;
        btn = (entry_.data >> 16) & 0xFF;  // not exactly button, but can contain btn data too.
        cnt = (entry_.data >> 44) & 0xF;
        /*
        uint8_t contact = (entry_.databtn & 0x80) >> 7;
        uint8_t tamper = (entry_.databtn & 0x40) >> 6;
        uint8_t reed = (entry_.databtn & 0x20) >> 5;
        uint8_t alarm = (entry_.databtn & 0x10) >> 4;
        uint8_t battery_low = (entry_.databtn & 0x08) >> 3;
        uint8_t heartbeat = (entry_.databtn & 0x04) >> 2;
        */
        return;
    }

    if (entry_.sensorType == FPS_HONEYWELLWDB) {
        serial = (entry_.data >> 28) & 0xFFFFF;
        // enabled, when we'll have extra fields and free fw space
        /* switch ((entry_.data >> 20) & 0x3) {
            case 0x02:
                device_type = "Doorbell";
                break;
            case 0x01:
                device_type = "PIR-Motion";
                break;
            default:
                device_type = "Unknown";
                break;
        }

        switch ((entry_.data >> 16) & 0x3) {
            case 0x00:
                alert = "Normal";
                break;
            case 0x01:
            case 0x02:
                alert = "High";
                break;
            case 0x03:
                alert = "Full";
                break;
            default:
                alert = "Unknown";
                break;
        }

        secret_knock = (uint8_t)((entry_.data >> 4) & 0x1);
        relay = (uint8_t)((entry_.data >> 3) & 0x1);
        lowbat = (uint8_t)((entry_.data >> 1) & 0x1);*/
        return;
    }

    if (entry_.sensorType == FPS_HORMANN) {
        btn = (entry_.data >> 8) & 0xF;
        return;
    }

    /* if (entry_.sensorType == FPS_HORMANNBISECURE) { //fm not implemented
        serial = 0;

        for (uint8_t i = 1; i < 5; i++) {
            serial = serial << 8 | ((uint8_t*)(&entry_.data))[i];
        }
    } */

    if (entry_.sensorType == FPS_IDO) {
        uint64_t code_found_reverse = FProtoGeneral::subghz_protocol_blocks_reverse_key(entry_.data, entry_.bits);
        uint32_t code_fix = code_found_reverse & 0xFFFFFF;
        serial = code_fix & 0xFFFFF;
        btn = (code_fix >> 20) & 0x0F;
        return;
    }

    if (entry_.sensorType == FPS_INTERTECHNOV3) {
        if (entry_.bits == 32) {
            serial = (entry_.data >> 6) & 0x3FFFFFF;
            if ((entry_.data >> 5) & 0x1) {
                cnt = 1 << 5;
            } else {
                cnt = (~entry_.data & 0xF);
            }
            btn = (entry_.data >> 4) & 0x1;
        } else if (entry_.bits == 36) {
            serial = (entry_.data >> 10) & 0x3FFFFFF;
            if ((entry_.data >> 9) & 0x1) {
                cnt = 1 << 5;
            } else {
                cnt = (~(entry_.data >> 4) & 0xF);
            }
            btn = (entry_.data) & 0xF;
        } else {
            serial = 0;
            cnt = 0;
            btn = 0;
        }
        return;
    }

    if (entry_.sensorType == FPS_KEELOQ) {
        // too many sub protocol versions, skipping. maybe in future when we'll have much more fw space
        return;
    }

    /* fm not  implemented
    if (entry_.sensorType == FPS_KIA) {
        serial = (uint32_t)((entry_.data >> 12) & 0x0FFFFFFF);
        btn = (entry_.data >> 8) & 0x0F;
        cnt = (entry_.data >> 40) & 0xFFFF;
        return;
    }
    */

    if (entry_.sensorType == FPS_KINGGATESSTYLO4K) {
        uint64_t fix = FProtoGeneral::subghz_protocol_blocks_reverse_key(entry_.data, 53);
        btn = (fix >> 17) & 0x0F;
        serial = ((fix >> 5) & 0xFFFF0000) | (fix & 0xFFFF);
        return;
    }

    if (entry_.sensorType == FPS_LEGRAND) {
        return;  // nothing
    }

    if (entry_.sensorType == FPS_LINEAR || entry_.sensorType == FPS_LINEARDELTA3) {
        return;  // nothing
    }

    if (entry_.sensorType == FPS_MAGELLAN) {
        uint64_t data_rev = FProtoGeneral::subghz_protocol_blocks_reverse_key(entry_.data >> 8, 24);
        serial = data_rev & 0xFFFF;
        btn = (data_rev >> 16) & 0xFF;
        return;
    }

    if (entry_.sensorType == FPS_MARANTEC) {
        btn = (entry_.data >> 16) & 0xF;
        serial = ((entry_.data >> 12) & 0xFFFFFF00) | ((entry_.data >> 8) & 0xFF);
        return;
    }

    if (entry_.sensorType == FPS_MASTERCODE) {
        serial = (entry_.data >> 4) & 0xFFFF;
        btn = (entry_.data >> 2 & 0x03);
        return;
    }

    if (entry_.sensorType == FPS_MEGACODE) {
        if ((entry_.data >> 23) == 1) {
            serial = (entry_.data >> 3) & 0xFFFF;
            btn = entry_.data & 0b111;
            cnt = (entry_.data >> 19) & 0b1111;
        } else {
            serial = 0;
            btn = 0;
            cnt = 0;
        }
        return;
    }

    if (entry_.sensorType == FPS_NERORADIO) {
        return;  // nothing
    }

    if (entry_.sensorType == FPS_NERO_SKETCH) {
        return;  // nothing
    }

    if (entry_.sensorType == FPS_NICEFLO || entry_.sensorType == FPS_NICEFLORS) {
        return;  // nothing, and can't
    }

    if (entry_.sensorType == FPS_PHOENIXV2) {
        uint64_t data_rev = FProtoGeneral::subghz_protocol_blocks_reverse_key(entry_.data, entry_.bits + 4);
        serial = data_rev & 0xFFFFFFFF;
        cnt = (data_rev >> 40) & 0xFFFF;
        btn = (data_rev >> 32) & 0xF;
        return;
    }

    if (entry_.sensorType == FPS_POWERSMART) {
        btn = ((entry_.data >> 54) & 0x02) | ((entry_.data >> 40) & 0x1);
        serial = ((entry_.data >> 33) & 0x3FFF00) | ((entry_.data >> 32) & 0xFF);
        cnt = ((entry_.data >> 49) & 0x3F);
        return;
    }

    if (entry_.sensorType == FPS_PRINCETON) {
        serial = entry_.data >> 4;
        btn = entry_.data & 0xF;
        return;
    }
    if (entry_.sensorType == FPS_SECPLUSV1) {
        uint32_t fixed = (entry_.data >> 32) & 0xFFFFFFFF;
        cnt = entry_.data & 0xFFFFFFFF;
        btn = fixed % 3;
        // uint8_t id0 = (fixed / 3) % 3;
        uint8_t id1 = (fixed / 9) % 3;
        // uint16_t pin = 0;
        if (id1 == 0) {
            // (fixed // 3**3) % (3**7)    3^3=27  3^73=72187

            serial = (fixed / 27) % 2187;
            // pin = (fixed // 3**10) % (3**9)  3^10=59049 3^9=19683
            // pin = (fixed / 59049) % 19683;

            /* if (pin <= 9999) {
                 furi_string_cat_printf(output, " pin:%d", pin);
             } else if (pin <= 11029) {
                 furi_string_cat_printf(output, " pin:enter");
             } */
        } else {
            // id = fixed / 27;
            serial = fixed / 27;
        }
    }

    if (entry_.sensorType == FPS_SECPLUSV2) {
        return;  // fw space saver
    }

    if (entry_.sensorType == FPS_SMC5326) {
        return;  // dip pattern output needed. skipping
    }

    if (entry_.sensorType == FPS_STARLINE) {
        uint64_t key = FProtoGeneral::subghz_protocol_blocks_reverse_key(entry_.data, entry_.bits);
        uint32_t key_fix = key >> 32;
        serial = key_fix & 0x00FFFFFF;
        btn = key_fix >> 24;
        return;
    }

    if (entry_.sensorType == FPS_X10) {
        serial = (entry_.data & 0xF0000000) >> (24 + 4);
        btn = (((entry_.data & 0x07000000) >> 24) | ((entry_.data & 0xF800) >> 8));
        return;
    }

    if (entry_.sensorType == FPS_SOMIFY_KEYTIS) {
        uint64_t dataa = entry_.data ^ (entry_.data >> 8);
        btn = (dataa >> 48) & 0xF;
        cnt = (dataa >> 24) & 0xFFFF;
        serial = dataa & 0xFFFFFF;
        return;
    }

    if (entry_.sensorType == FPS_SOMIFY_TELIS) {
        uint64_t dataa = entry_.data ^ (entry_.data >> 8);
        btn = (dataa >> 44) & 0xF;     // ctrl
        cnt = (dataa >> 24) & 0xFFFF;  // rolling code
        serial = dataa & 0xFFFFFF;     // address}
        return;
    }
}
}  // namespace ui