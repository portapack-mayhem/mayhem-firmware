/*
 * DECT RX baseband processor for the PortaPack Mayhem DECT RX.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
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
 *
 * Decode pipeline ported from DeDECTive by SarahRose, which ports gr-dect2
 * by Pavel Yazev (GPLv3).
 */

#include "proc_dect.hpp"

#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

#include <cstring>

void DECTRxProcessor::reset_pipeline() {
    dc_blocker.reset();
    phase_diff.reset();
    receiver.reset();
    decoder.reset();
    last_parts = 0;
    last_voice_parts = 0;
}

void DECTRxProcessor::configure(const DECTRxConfigureMessage& message) {
    (void)message;
    reset_pipeline();
    configured = true;
}

void DECTRxProcessor::execute(const buffer_c8_t& buffer) {
    /* Report status even before configuration so the app can distinguish
     * "no config message" from "configured but no signal". */
    if (++status_blocks >= status_interval) {
        status_blocks = 0;
        send_status();
    }

    if (!configured) return;

    /* 4,608,000 samples/s: DC block, phase-difference demod, packet sync. */
    for (size_t i = 0; i < buffer.count; i++) {
        const auto s = buffer.p[i];

        /* Carrier is parked ~20 kHz off the LO spike, so the blocker's narrow
         * notch (and its long transient) act on the spike, not on the signal. */
        dect::cf_t x{(float)s.real(), (float)s.imag()};
        x = dc_blocker.process(x);
        const dect::phase_diff_t pd = phase_diff.process(x);
        receiver.process_sample(pd.im, pd.re);
    }

    /* Drain completed packets, then the parts that timed out. */
    dect::ReceivedPacket pkt;
    while (receiver.pop_packet(pkt)) decoder.process_packet(pkt);

    int lost;
    while ((lost = receiver.pop_lost()) >= 0) decoder.notify_lost(lost);

    publish_parts();
}

void DECTRxProcessor::send_status() {
    const DECTStatusMessage message{
        configured,
        last_parts,
        last_voice_parts};
    shared_memory.application_queue.push(message);
}

void DECTRxProcessor::publish_parts() {
    dect::PartInfo parts[dect::MAX_PARTS];
    int count = 0;
    if (!decoder.take_part_update(parts, count)) return;

    DECTPartInfoMessage message{};
    if (count > (int)(sizeof(message.parts) / sizeof(message.parts[0])))
        count = sizeof(message.parts) / sizeof(message.parts[0]);
    message.count = (uint8_t)count;

    last_parts = (uint8_t)count;
    last_voice_parts = 0;

    for (int i = 0; i < count; i++) {
        if (parts[i].voice_present) last_voice_parts++;

        message.parts[i].rx_id = parts[i].rx_id;
        message.parts[i].type = (int32_t)parts[i].type;
        message.parts[i].voice_present = parts[i].voice_present ? 1 : 0;
        message.parts[i].part_id_valid = parts[i].part_id_valid ? 1 : 0;
        message.parts[i].qt_synced = parts[i].qt_synced ? 1 : 0;
        message.parts[i].slot = parts[i].slot;
        memcpy(message.parts[i].part_id, parts[i].part_id, 5);
    }

    shared_memory.application_queue.push(message);
}

void DECTRxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::DECTRxConfigure:
            configure(*reinterpret_cast<const DECTRxConfigureMessage*>(message));
            break;

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<DECTRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
