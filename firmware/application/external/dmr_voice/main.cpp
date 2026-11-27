/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#include "ui_dmr_voice.hpp"
#include "external_app.hpp"

namespace ui::external_app::dmr_voice {
void initialize_app(NavigationView& nav) {
    nav.push<DmrVoiceView>();
}
}  // namespace ui::external_app::dmr_voice

extern "C" {

__attribute__((section(".external_app.app_dmr_voice.application_information"), used))

application_information_t _application_information_dmr_voice = {
    nullptr,
    ui::external_app::dmr_voice::initialize_app,
    EXTENDED_HEADER_VERSION,
    VERSION_MD5,
    "DMR Voice",
    {0x00, 0x00, 0xfe, 0x7f, 0x02, 0x40, 0xfa, 0x5f,
     0x02, 0x40, 0xf2, 0x4f, 0x02, 0x40, 0xe2, 0x47,
     0x02, 0x40, 0xc2, 0x43, 0x02, 0x40, 0x82, 0x41,
     0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00},
    ui::Color::yellow().v,
    app_location_t::RX,
    -1,
    {'P', 'D', 'V', 'O'},
    0,
};

}
