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

#include "ui_blespam.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::blespam {

void BLESpamView::focus() {
    button_startstop.focus();
}

uint64_t BLESpamView::get_freq_by_channel_number(uint8_t channel_number) {
    uint64_t freq_hz;

    switch (channel_number) {
        case 37:
            freq_hz = 2'402'000'000ull;
            break;
        case 38:
            freq_hz = 2'426'000'000ull;
            break;
        case 39:
            freq_hz = 2'480'000'000ull;
            break;
        case 0 ... 10:
            freq_hz = 2'404'000'000ull + channel_number * 2'000'000ull;
            break;
        case 11 ... 36:
            freq_hz = 2'428'000'000ull + (channel_number - 11) * 2'000'000ull;
            break;
        default:
            freq_hz = UINT64_MAX;
    }

    return freq_hz;
}
BLESpamView::BLESpamView(NavigationView& nav)
    : nav_{nav} {
    add_children({&button_startstop,
                  &field_frequency,
                  &tx_view,
                  &chk_randdev,
                  &console});
    button_startstop.on_select = [this](Button&) {
        if (is_running) {
            is_running = false;
            stop();
            button_startstop.set_text("Start");
        } else {
            is_running = true;
            start();
            button_startstop.set_text("Stop");
        }
    };
    field_frequency.set_value(get_freq_by_channel_number(channel_number));
    chk_randdev.on_select = [this](Checkbox&, bool v) {
        randomDev = v;
    };
    createFastPairPacket();  // only at startup, and if the checkbox is ticked
    //
}

void BLESpamView::stop() {
    transmitter_model.disable();
    baseband::shutdown();
}

void BLESpamView::start() {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);
    transmitter_model.enable();

    baseband::set_btletx(channel_number, mac, advertisementData, pduType);
}
void BLESpamView::reset() {
    stop();
    start();
}

void BLESpamView::randomChn() {
    channel_number = 37 + std::rand() % (39 - 37 + 1);
    field_frequency.set_value(get_freq_by_channel_number(channel_number));
}

void BLESpamView::randomizeMac() {
    if (!randomMac) return;
    const char hexDigits[] = "0123456789ABCDEF";
    // Generate 12 random hexadecimal characters
    for (int i = 0; i < 12; ++i) {
        int randomIndex = rand() % 16;
        mac[i] = hexDigits[randomIndex];
    }
    mac[12] = '\0';  // Null-terminate the string
}

void BLESpamView::createFastPairPacket() {
    uint32_t model;
    model = fastpairModels[rand() % fastpairModels_count].value;
    memset(advertisementData, 0, sizeof(advertisementData));
    // 0         0     6
    const char* source = "03032CFE06162CFED5A59E020AB4\0";
    memcpy(advertisementData, source, 28);
    advertisementData[16] = uint_to_char((model >> 20) & 0x0F, 16);
    advertisementData[17] = uint_to_char((model >> 16) & 0x0F, 16);
    advertisementData[18] = uint_to_char((model >> 12) & 0x0F, 16);
    advertisementData[19] = uint_to_char((model >> 8) & 0x0F, 16);
    advertisementData[20] = uint_to_char((model >> 4) & 0x0F, 16);
    advertisementData[21] = uint_to_char((model >> 0) & 0x0F, 16);
    // advertisementData[8] = (model >> 0x10) & 0xFF;   // Device Model
    // advertisementData[9] = (model >> 0x08) & 0xFF;   // ...
    // advertisementData[10] = (model >> 0x00) & 0xFF;  // ...
    console.writeln(advertisementData);
}

void BLESpamView::changePacket() {
    counter++;
    if (counter >= 5) {
        // really change packet and mac
        counter = 0;
        randomizeMac();
        randomChn();
        if (randomDev) createFastPairPacket();
    }
}
// called each 1/60th of second, so 6 = 100ms
void BLESpamView::on_timer() {
    if (++timer_count == timer_period) {
        timer_count = 0;
        if (is_running) {
            changePacket();
            reset();
        }
    }
}
void BLESpamView::on_tx_progress(const bool done) {
    if (done) {
        if (is_running) {
            /*            if ((packet_counter % 10) == 0) {
                text_packets_sent.set(to_string_dec_uint(packet_counter));
            }
            */
        }
    }
}

BLESpamView::~BLESpamView() {
    stop();
}

}  // namespace ui::external_app::blespam
