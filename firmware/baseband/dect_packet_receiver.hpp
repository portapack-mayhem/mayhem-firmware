/*
 * DECT frame/slot sync and bit recovery for the PortaPack Mayhem DECT RX.
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
 * Ported from gr-dect2 packet_receiver_impl.cc (Pavel Yazev, GPLv3) via
 * DeDECTive. Instead of std::function callbacks the completed packets and
 * lost-part events are queued for the processor to drain.
 */

#ifndef __DECT_PACKET_RECEIVER_H__
#define __DECT_PACKET_RECEIVER_H__

#include <cstdint>
#include <cstddef>

namespace dect {

/* Maximum concurrent DECT parts tracked simultaneously. */
inline constexpr int MAX_PARTS = 8;

/* DECT RFP sync word; PP uses the bitwise complement. */
inline constexpr uint32_t RFP_SYNC_FIELD = 0xAAAAE98A;

/* Payload bits captured after the sync: 64 A + 320 B + 4 X-CRC = 388. */
inline constexpr int P32_D_FIELD_BITS = 388;

/* Timing in input samples at 4,608,000 Hz (4 samples/symbol). */
inline constexpr uint64_t INTER_SLOT_TIME = 480 * 4;
inline constexpr uint64_t INTER_FRAME_TIME = INTER_SLOT_TIME * 24;
inline constexpr uint64_t TIME_TOL = 10;

enum class PartType : uint8_t {
    RFP = 0,
    PP = 1,
};

struct ReceivedPacket {
    uint8_t bits[P32_D_FIELD_BITS];
    int rx_id;
    uint32_t rx_seq;
    PartType type;
    uint8_t rx_slot;
};

class PacketReceiver {
   public:
    PacketReceiver() { reset(); }

    /* Feed one phase-difference sample (one per IQ sample). `phase_im` drives
     * the bit decision; both terms feed the symbol-timing metric. */
    void process_sample(float phase_im, float phase_re) noexcept;

    void reset() noexcept;

    /* Completed packets pending delivery, in order. */
    bool pop_packet(ReceivedPacket& out) noexcept;

    /* Part index reported lost, or -1 if none pending. */
    int pop_lost() noexcept;

   private:
    enum class SyncState : uint8_t {
        WaitBegin,
        WaitEnd,
        PostWait,
    };

    static constexpr int SMPL_BUF_LEN = 32 * 4;
    static constexpr int COMP_Q_LEN = 4;
    static constexpr int LOST_Q_LEN = 8;

    /* sync detection */
    uint32_t rx_bits_buf_[4];
    int rx_bits_idx_;

    float smpl_buf_[SMPL_BUF_LEN];
    float smpl_re_buf_[SMPL_BUF_LEN];
    int smpl_buf_idx_;

    SyncState sync_state_;
    PartType cur_part_type_;

    int begin_pos_;
    int end_pos_;
    int smpl_cnt_;
    int out_bit_cnt_;

    uint64_t inc_smpl_cnt_;

    /* part tracking */
    uint32_t part_activity_;
    uint64_t part_time_[MAX_PARTS];
    uint32_t part_seq_[MAX_PARTS];
    uint8_t part_slot_[MAX_PARTS];
    int cur_part_rx_id_;

    /* packet assembly */
    ReceivedPacket cur_packet_;

    /* output queues (single-threaded: M4 baseband execute) */
    ReceivedPacket comp_q_[COMP_Q_LEN];
    int comp_head_;
    int comp_tail_;
    int lost_q_[LOST_Q_LEN];
    int lost_head_;
    int lost_tail_;

    int find_best_smpl_point() noexcept;
    int register_part() noexcept;
    int check_part_activity() noexcept;
};

}  // namespace dect

#endif /*__DECT_PACKET_RECEIVER_H__*/
