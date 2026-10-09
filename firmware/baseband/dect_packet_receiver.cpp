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
 * DeDECTive.
 */

#include "dect_packet_receiver.hpp"

#include <cmath>
#include <cstring>

namespace dect {

void PacketReceiver::reset() noexcept {
    memset(rx_bits_buf_, 0, sizeof(rx_bits_buf_));
    memset(smpl_buf_, 0, sizeof(smpl_buf_));
    memset(smpl_re_buf_, 0, sizeof(smpl_re_buf_));
    memset(part_time_, 0, sizeof(part_time_));
    memset(part_seq_, 0, sizeof(part_seq_));
    memset(part_slot_, 0, sizeof(part_slot_));
    memset(&cur_packet_, 0, sizeof(cur_packet_));
    rx_bits_idx_ = 0;
    smpl_buf_idx_ = 0;
    sync_state_ = SyncState::WaitBegin;
    cur_part_type_ = PartType::RFP;
    begin_pos_ = 0;
    end_pos_ = 0;
    smpl_cnt_ = 0;
    out_bit_cnt_ = 0;
    inc_smpl_cnt_ = 0;
    part_activity_ = 0;
    cur_part_rx_id_ = -1;
    comp_head_ = comp_tail_ = 0;
    lost_head_ = lost_tail_ = 0;
}

bool PacketReceiver::pop_packet(ReceivedPacket& out) noexcept {
    if (comp_head_ == comp_tail_) return false;
    out = comp_q_[comp_tail_];
    comp_tail_ = (comp_tail_ + 1) % COMP_Q_LEN;
    return true;
}

int PacketReceiver::pop_lost() noexcept {
    if (lost_head_ == lost_tail_) return -1;
    const int v = lost_q_[lost_tail_];
    lost_tail_ = (lost_tail_ + 1) % LOST_Q_LEN;
    return v;
}

/* Find the sub-sample offset (0-3) with the highest cumulative energy in the
 * buffered samples, to align the bit decision point optimally. */
int PacketReceiver::find_best_smpl_point() noexcept {
    if (begin_pos_ == end_pos_) return 0;

    float max_val = 0.0f;
    int max_index = begin_pos_;
    int pos = begin_pos_;

    while (true) {
        float acc = 0.0f;
        int index = pos;
        for (int j = 0; j < 32; ++j) {
            /* Exactly DeDECTive's metric: the absolute atan2 phase of the
             * cross-product. Only evaluated once per burst (32 samples per
             * candidate phase), so atan2f here is well within budget. */
            const float im = smpl_buf_[index];
            const float re = smpl_re_buf_[index];
            acc += fabsf(atan2f(im, re));
            index = (index - 4) & (SMPL_BUF_LEN - 1);
        }
        if (acc > max_val) {
            max_val = acc;
            max_index = pos;
        }
        if (pos == end_pos_) break;
        pos = (pos + 1) & (SMPL_BUF_LEN - 1);
    }
    return (end_pos_ - max_index) & (SMPL_BUF_LEN - 1);
}

/* Assign an incoming burst to an existing part (by timing) or allocate one. */
int PacketReceiver::register_part() noexcept {
    const uint8_t slot = static_cast<uint8_t>((inc_smpl_cnt_ % INTER_FRAME_TIME) / INTER_SLOT_TIME);

    if (part_activity_) {
        uint32_t part_mask = 1;
        for (int j = 0; j < MAX_PARTS; ++j, part_mask <<= 1) {
            if (!(part_activity_ & part_mask)) continue;

            const uint64_t delta = (inc_smpl_cnt_ - part_time_[j]) % INTER_FRAME_TIME;
            uint32_t seq = 0;

            if (delta < TIME_TOL) {
                seq = static_cast<uint32_t>((inc_smpl_cnt_ - part_time_[j]) / INTER_FRAME_TIME);
            } else if (INTER_FRAME_TIME - delta <= TIME_TOL) {
                seq = 1 + static_cast<uint32_t>((inc_smpl_cnt_ - part_time_[j]) / INTER_FRAME_TIME);
            } else {
                continue;
            }

            part_time_[j] = inc_smpl_cnt_;
            part_seq_[j] = (part_seq_[j] + seq) & 0x1F;
            part_slot_[j] = slot;
            return j;
        }

        /* No existing part matched - allocate a new one. */
        part_mask = 1;
        for (int j = 0; j < MAX_PARTS; ++j, part_mask <<= 1) {
            if (!(part_activity_ & part_mask)) {
                part_activity_ |= part_mask;
                part_time_[j] = inc_smpl_cnt_;
                part_seq_[j] = 0;
                part_slot_[j] = slot;
                return j;
            }
        }
        return -1;
    } else {
        part_activity_ = 1;
        part_time_[0] = inc_smpl_cnt_;
        part_seq_[0] = 0;
        part_slot_[0] = slot;
        return 0;
    }
}

/* Queue the rx_id of any part not heard for > 16 frames. */
int PacketReceiver::check_part_activity() noexcept {
    if (!part_activity_) return -1;
    uint32_t part_mask = 1;
    for (int j = 0; j < MAX_PARTS; ++j, part_mask <<= 1) {
        if ((part_activity_ & part_mask) &&
            (inc_smpl_cnt_ - part_time_[j] > 16 * INTER_FRAME_TIME)) {
            part_activity_ &= ~part_mask;
            const int next = (lost_head_ + 1) % LOST_Q_LEN;
            if (next != lost_tail_) {
                lost_q_[lost_head_] = j;
                lost_head_ = next;
            }
            return j;
        }
    }
    return -1;
}

void PacketReceiver::process_sample(float phase_im, float phase_re) noexcept {
    /* Bit decision: positive phase -> 0, negative -> 1. */
    const uint32_t rx_bit = (phase_im >= 0.0f) ? 0u : 1u;

    rx_bits_buf_[rx_bits_idx_] = (rx_bits_buf_[rx_bits_idx_] << 1) | rx_bit;
    smpl_buf_[smpl_buf_idx_] = phase_im;
    smpl_re_buf_[smpl_buf_idx_] = phase_re;

    switch (sync_state_) {
        case SyncState::WaitBegin: {
            const bool rfp_sync = (rx_bits_buf_[rx_bits_idx_] == RFP_SYNC_FIELD);
            const bool pp_sync = (rx_bits_buf_[rx_bits_idx_] == ~RFP_SYNC_FIELD);

            if (rfp_sync || pp_sync) {
                cur_part_type_ = rfp_sync ? PartType::RFP : PartType::PP;
                begin_pos_ = smpl_buf_idx_;
                sync_state_ = SyncState::WaitEnd;
            }
            break;
        }

        case SyncState::WaitEnd: {
            const uint32_t expected = (cur_part_type_ == PartType::RFP)
                                          ? RFP_SYNC_FIELD
                                          : ~RFP_SYNC_FIELD;
            const bool still_sync = (rx_bits_buf_[rx_bits_idx_] == expected);

            if (!still_sync) {
                end_pos_ = (smpl_buf_idx_ - 1) & (SMPL_BUF_LEN - 1);

                smpl_cnt_ = (1 + find_best_smpl_point()) & 3;

                cur_part_rx_id_ = register_part();
                if (cur_part_rx_id_ < 0) {
                    sync_state_ = SyncState::WaitBegin;
                    break;
                }

                out_bit_cnt_ = 0;
                cur_packet_.rx_id = cur_part_rx_id_;
                cur_packet_.rx_seq = part_seq_[cur_part_rx_id_];
                cur_packet_.type = cur_part_type_;
                cur_packet_.rx_slot = part_slot_[cur_part_rx_id_];
                sync_state_ = SyncState::PostWait;
            }
            break;
        }

        case SyncState::PostWait:
            if (smpl_cnt_ == 0) {
                cur_packet_.bits[out_bit_cnt_] = static_cast<uint8_t>(rx_bit);
                ++out_bit_cnt_;

                if (out_bit_cnt_ == P32_D_FIELD_BITS) {
                    const int next = (comp_head_ + 1) % COMP_Q_LEN;
                    if (next != comp_tail_) {
                        comp_q_[comp_head_] = cur_packet_;
                        comp_head_ = next;
                    }
                    sync_state_ = SyncState::WaitBegin;
                }
            }
            break;
    }

    smpl_buf_idx_ = (smpl_buf_idx_ + 1) & (SMPL_BUF_LEN - 1);
    rx_bits_idx_ = (rx_bits_idx_ + 1) & 3;
    smpl_cnt_ = (smpl_cnt_ + 1) & 3;
    ++inc_smpl_cnt_;

    /* Parts time out after 16 frames (~160 ms), so checking once every 4096
     * samples (0.9 ms) is ample and keeps the per-sample path cheap. */
    if ((inc_smpl_cnt_ & 0xFFF) == 0) check_part_activity();
}

}  // namespace dect
