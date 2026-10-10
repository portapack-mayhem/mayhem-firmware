/*
 * DECT A-field decoder and part tracking for the PortaPack Mayhem DECT RX.
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
 * Ported from gr-dect2 packet_decoder_impl.cc (Pavel Yazev, GPLv3) via
 * DeDECTive.
 */

#include "dect_packet_decoder.hpp"

#include <cstring>

namespace dect {

/* RCRC-16 table (CCITT HDLC variant used in the DECT A-field). */
static const uint16_t crc_table[16] = {
    0x0000, 0x0589, 0x0b12, 0x0e9b, 0x1624, 0x13ad, 0x1d36, 0x18bf,
    0x2c48, 0x29c1, 0x275a, 0x22d3, 0x3a6c, 0x3fe5, 0x317e, 0x34f7};

void PacketDecoder::reset() noexcept {
    for (int i = 0; i < MAX_PARTS; i++) {
        parts_[i] = PartState{};
    }
    update_count_ = 0;
    update_pending_ = false;
}

bool PacketDecoder::take_part_update(PartInfo out[], int& count) noexcept {
    if (!update_pending_) return false;
    for (int i = 0; i < update_count_; i++) out[i] = update_[i];
    count = update_count_;
    update_pending_ = false;
    return true;
}

uint16_t PacketDecoder::calc_rcrc(const uint8_t* data, unsigned len) noexcept {
    uint16_t crc = 0;
    while (len--) {
        unsigned idx = (crc >> 12) ^ (*data >> 4);
        crc = crc_table[idx & 0x0F] ^ (crc << 4);
        idx = (crc >> 12) ^ (*data >> 0);
        crc = crc_table[idx & 0x0F] ^ (crc << 4);
        ++data;
    }
    return crc ^ 0x0001;
}

bool PacketDecoder::decode_afield(const uint8_t* bits, int rx_id) noexcept {
    PartState& ps = parts_[rx_id];

    uint8_t af[8] = {};
    for (int i = 0; i < 64; ++i)
        af[i >> 3] = (af[i >> 3] << 1) | (bits[i] & 1);

    const uint8_t header = af[0];
    const uint8_t ta = (header >> 5) & 0x07;
    const uint8_t ba = (header >> 1) & 0x07;

    const uint16_t rcrc_recv = ((uint16_t)af[6] << 8) | af[7];
    const uint16_t rcrc_calc = calc_rcrc(af, 6);
    if (rcrc_calc != rcrc_recv) {
        ++ps.bad_crc_cnt;
        return false;
    }

    switch (ta) {
        case 3: /* Identification: RFPI (base) or IPUI (portable). */
            for (int i = 0; i < 5; ++i) ps.part_id[i] = af[1 + i];
            ps.part_id_rcvd = true;
            break;
        case 4: /* Qt: multiframe sync / system info (frame 8 of 16). */
            if (!ps.qt_rcvd) {
                ps.qt_rcvd = true;
                ps.log_dirty = true;
            }
            break;
        default:
            break;
    }

    /* BA == 0 marks a bearer carrying voice (traffic bearer). */
    const bool voice = (ba == 0);
    if (voice != ps.voice_present) {
        ps.voice_present = voice;
        ps.log_dirty = true;
    }

    return true;
}

bool PacketDecoder::ids_equal(const uint8_t* a, const uint8_t* b) noexcept {
    for (int i = 0; i < 5; ++i)
        if (a[i] != b[i]) return false;
    return true;
}

void PacketDecoder::try_pair(int rx_id) noexcept {
    PartState& me = parts_[rx_id];
    if (me.pair_rx_id >= 0 || !me.part_id_rcvd || me.type != PartType::PP) return;

    for (int i = 0; i < MAX_PARTS; ++i) {
        if (i == rx_id) continue;
        PartState& other = parts_[i];
        if (!other.active) continue;
        if (!other.part_id_rcvd) continue;
        if (ids_equal(me.part_id, other.part_id)) {
            me.pair_rx_id = i;
            other.pair_rx_id = rx_id;
        }
    }
}

void PacketDecoder::build_update() noexcept {
    int count = 0;
    for (int i = 0; i < MAX_PARTS; ++i) {
        const PartState& ps = parts_[i];
        if (!ps.active) continue;
        PartInfo& pi = update_[count++];
        pi.rx_id = i;
        pi.type = ps.type;
        pi.voice_present = ps.voice_present;
        pi.part_id_valid = ps.part_id_rcvd;
        pi.qt_synced = ps.qt_rcvd;
        pi.slot = ps.slot;
        pi.packets_ok = ps.packet_cnt;
        pi.packets_bad_crc = ps.bad_crc_cnt;
        memcpy(pi.part_id, ps.part_id, 5);
    }
    update_count_ = count;
    update_pending_ = true;
}

void PacketDecoder::process_packet(const ReceivedPacket& pkt) noexcept {
    const int rx_id = pkt.rx_id;
    if (rx_id < 0 || rx_id >= MAX_PARTS) return;

    PartState& ps = parts_[rx_id];

    if (ps.active) {
        ps.slot = pkt.rx_slot;
        ++ps.packet_cnt;
    } else {
        ps.active = true;
        ps.type = pkt.type;
        ps.slot = pkt.rx_slot;
        ps.voice_present = false;
        ps.packet_cnt = 0;
        ps.bad_crc_cnt = 0;
        ps.part_id_rcvd = false;
        ps.qt_rcvd = false;
        ps.log_dirty = true;
        ps.pair_rx_id = -1;
        memset(ps.part_id, 0, 5);
    }

    decode_afield(pkt.bits, rx_id);

    try_pair(rx_id);

    /* An RFP that has the Qt marker lends it to its paired PP. */
    if (pkt.type == PartType::RFP && ps.qt_rcvd && ps.pair_rx_id >= 0) {
        PartState& pp = parts_[ps.pair_rx_id];
        if (!pp.qt_rcvd) {
            pp.qt_rcvd = true;
            pp.log_dirty = true;
        }
    }

    const bool periodic = (ps.packet_cnt % 100 == 0) && ps.packet_cnt > 0;
    if (ps.log_dirty || periodic) {
        build_update();
        ps.log_dirty = false;
    }
}

void PacketDecoder::notify_lost(int rx_id) noexcept {
    if (rx_id < 0 || rx_id >= MAX_PARTS) return;
    PartState& ps = parts_[rx_id];
    const bool had_id = ps.part_id_rcvd;

    if (ps.pair_rx_id >= 0) {
        PartState& pair = parts_[ps.pair_rx_id];
        if (ps.type == PartType::RFP) pair.voice_present = false;
        pair.pair_rx_id = -1;
    }

    ps = PartState{};

    if (had_id) build_update();
}

}  // namespace dect
