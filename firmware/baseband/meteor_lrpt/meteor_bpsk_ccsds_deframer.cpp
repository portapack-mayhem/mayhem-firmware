/*
 * SPDX-License-Identifier: MIT
 *
 * Derived from SatDump `src-core/common/codings/deframing/bpsk_ccsds_deframer.cpp`
 * (BPSK_CCSDS_Deframer). Mayhem file is GPL v2 combined work; this translation
 * remains MIT-compatible per SatDump upstream.
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#include "meteor_bpsk_ccsds_deframer.hpp"

MeteorBpskCcsdsDeframer::MeteorBpskCcsdsDeframer(int cadu_bits, uint32_t sync_word)
    : cadu_asm_(sync_word),
      cadu_asm_inv_(sync_word ^ 0xFFFFFFFFU),
      cadu_size_bits_(cadu_bits) {
}

void MeteorBpskCcsdsDeframer::reset() {
    d_state_ = kStateNosync;
    in_frame_ = false;
    shifter_ = 0;
    bit_inversion_ = false;
    bit_of_frame_ = 0;
    d_invalid_asm_ = 0;
    d_good_asm_ = 0;
    frame_buffer_.fill(0);
}

int MeteorBpskCcsdsDeframer::compare_32(uint32_t v1, uint32_t v2) {
    int cor = 0;
    uint32_t diff = v1 ^ v2;
    for (; diff; cor++)
        diff &= diff - 1U;
    return cor;
}

void MeteorBpskCcsdsDeframer::write_bit(uint8_t b) {
    frame_buffer_[bit_of_frame_ / 8] = (uint8_t)((frame_buffer_[bit_of_frame_ / 8] << 1) | b);
    bit_of_frame_++;
}

void MeteorBpskCcsdsDeframer::reset_frame() {
    frame_buffer_.fill(0);
    frame_buffer_[0] = (uint8_t)(cadu_asm_ >> 24);
    frame_buffer_[1] = (uint8_t)(cadu_asm_ >> 16);
    frame_buffer_[2] = (uint8_t)(cadu_asm_ >> 8);
    frame_buffer_[3] = (uint8_t)(cadu_asm_ >> 0);
    bit_of_frame_ = kCaduAsmBits;
}

int MeteorBpskCcsdsDeframer::work(const uint8_t* input_bits, int num_bits, uint8_t* output_bytes) {
    int frame_count = 0;
    constexpr int cadu_padding = 0;
    const int out_stride = (cadu_size_bits_ + cadu_padding) / 8;

    for (int ibit = 0; ibit < num_bits; ibit++) {
        shifter_ = (shifter_ << 1) | (input_bits[ibit] & 1U);

        if (in_frame_) {
            write_bit((uint8_t)(input_bits[ibit] ^ (bit_inversion_ ? 1U : 0U)));

            if (bit_of_frame_ == cadu_size_bits_) {
                std::memcpy(&output_bytes[frame_count * out_stride], frame_buffer_.data(), (size_t)out_stride);
                frame_count++;
            } else if (bit_of_frame_ == cadu_size_bits_ + kCaduAsmBits - 1) {
                in_frame_ = false;
            }
            continue;
        }

        if (d_state_ == kStateNosync) {
            if (shifter_ == cadu_asm_) {
                bit_inversion_ = false;
                reset_frame();
                in_frame_ = true;
                d_state_ = kStateSyncing;
                d_good_asm_ = d_invalid_asm_ = 0;
            } else if (shifter_ == cadu_asm_inv_) {
                bit_inversion_ = true;
                reset_frame();
                in_frame_ = true;
                d_state_ = kStateSyncing;
                d_good_asm_ = d_invalid_asm_ = 0;
            }
        } else if (d_state_ == kStateSyncing) {
            if (compare_32(shifter_, bit_inversion_ ? cadu_asm_inv_ : cadu_asm_) < d_state_) {
                reset_frame();
                in_frame_ = true;
                d_invalid_asm_ = 0;
                d_good_asm_++;

                if (d_good_asm_ > 10)
                    d_state_ = kStateSynced;
            } else {
                d_invalid_asm_++;
                d_good_asm_ = 0;

                if (d_invalid_asm_ > 2)
                    d_state_ = kStateNosync;
            }
        } else if (d_state_ == kStateSynced) {
            if (compare_32(shifter_, bit_inversion_ ? cadu_asm_inv_ : cadu_asm_) < d_state_) {
                reset_frame();
                in_frame_ = true;
            } else {
                d_good_asm_ = d_invalid_asm_ = 0;
                d_state_ = kStateNosync;
            }
        }
    }

    return frame_count;
}
