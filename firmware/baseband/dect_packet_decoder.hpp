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
 * DeDECTive. std::function/std::string removed; results are queued for the
 * processor to drain.
 */

#ifndef __DECT_PACKET_DECODER_H__
#define __DECT_PACKET_DECODER_H__

#include "dect_packet_receiver.hpp"

#include <cstdint>
#include <cstddef>

namespace dect {

/* Part information reported to the application layer. */
struct PartInfo {
    int rx_id;
    PartType type;
    uint8_t part_id[5]; /* DECT RFPI / IPUI */
    bool voice_present;
    bool part_id_valid;
    bool qt_synced;
    uint8_t slot; /* DECT timeslot 0-23 */

    uint32_t packets_ok;
    uint32_t packets_bad_crc;
};

class PacketDecoder {
   public:
    PacketDecoder() { reset(); }

    void reset() noexcept;

    /* Feed a received packet. */
    void process_packet(const ReceivedPacket& pkt) noexcept;

    /* Mark a part as lost. */
    void notify_lost(int rx_id) noexcept;

    /* Copy the pending parts snapshot; returns false if no update pending. */
    bool take_part_update(PartInfo out[], int& count) noexcept;

   private:
    static uint16_t calc_rcrc(const uint8_t* data, unsigned len) noexcept;

    bool decode_afield(const uint8_t* bits, int rx_id) noexcept;

    struct PartState {
        bool active = false;
        bool voice_present = false;
        bool part_id_rcvd = false;
        bool qt_rcvd = false;
        bool log_dirty = false;
        uint8_t slot = 0;
        uint8_t part_id[5] = {};
        PartType type = PartType::RFP;
        uint32_t packet_cnt = 0;
        uint32_t bad_crc_cnt = 0;
        int pair_rx_id = -1;
    };

    PartState parts_[MAX_PARTS];

    static bool ids_equal(const uint8_t* a, const uint8_t* b) noexcept;
    void try_pair(int rx_id) noexcept;
    void build_update() noexcept;

    PartInfo update_[MAX_PARTS];
    int update_count_;
    bool update_pending_;
};

}  // namespace dect

#endif /*__DECT_PACKET_DECODER_H__*/
