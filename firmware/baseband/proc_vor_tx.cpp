/*
 * Copyright (C) 2026 PortaPack Mayhem
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
 * along with this program; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "proc_vor_tx.hpp"

#include "sine_table_int8.hpp"
#include "event_m4.hpp"

void VorTxProcessor::execute(const buffer_c8_t& buffer) {
    for (size_t i = 0; i < buffer.count; i++) {
        if (!configured) {
            buffer.p[i] = {0, 0};
            continue;
        }

        // 30 Hz reference and variable tones (same frequency, offset by the radial).
        const int32_t ref_sine = sine_table_i8[(phase_30 >> 24) & 0xFF];
        const int32_t var_sine = sine_table_i8[((phase_30 + radial_offset) >> 24) & 0xFF];
        phase_30 += delta_30;

        // 9960 Hz subcarrier, FM-modulated +/- 480 Hz by the 30 Hz reference tone.
        const int32_t sub_inc = static_cast<int32_t>(delta_sub) + ((ref_sine * static_cast<int32_t>(delta_480)) >> 7);
        phase_sub += static_cast<uint32_t>(sub_inc);
        const int32_t sub_sine = sine_table_i8[(phase_sub >> 24) & 0xFF];

        // Optional 1020 Hz identification tone.
        int32_t id_sine = 0;
        if (ident_enabled) {
            id_sine = sine_table_i8[(phase_id >> 24) & 0xFF];
            phase_id += delta_1020;
        }

        // DSB-AM envelope (carrier kept at LO -> Q = 0).
        int32_t env = carrier_level + (((var_sine * var_depth) + (sub_sine * sub_depth) + (id_sine * id_depth)) >> 7);
        if (env < 0) env = 0;
        if (env > 127) env = 127;

        buffer.p[i] = {static_cast<int8_t>(env), 0};
    }
}

void VorTxProcessor::vor_tx_config(const VorTxConfigureMessage& message) {
    // TODO: Verify radial sign convention on hardware. The variable tone is offset
    // by +radial_offset relative to the 30 Hz reference; confirm this matches the
    // phase difference the VOR RX decoder (proc_vor_rx) measures, otherwise the
    // transmitted radial appears mirrored (e.g. 090 read as 270). May need to negate
    // radial_offset (use phase_period - offset) once validated against real receivers.
    radial_offset = static_cast<uint32_t>(static_cast<uint64_t>(message.radial_deg % 360) * phase_period / 360);
    ident_enabled = message.ident_enabled;
    configured = message.enabled;
}

void VorTxProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::VorTxConfigure) {
        vor_tx_config(*reinterpret_cast<const VorTxConfigureMessage*>(msg));
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<VorTxProcessor>()};
    event_dispatcher.run();
    return 0;
}
