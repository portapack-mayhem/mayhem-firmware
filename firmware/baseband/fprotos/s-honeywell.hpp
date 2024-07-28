
#ifndef __FPROTO_HONEYWELL_H__
#define __FPROTO_HONEYWELL_H__

#include "subghzdbase.hpp"

class FProtoSubGhzDHoneywell : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDHoneywell() {
        sensorType = FPS_HONEYWELL;
        te_short = 280;
        te_long = 143;
        te_delta = 51;
        min_count_bit_for_found = 62;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        if (!level) {
            if (DURATION_DIFF(duration, te_short) < te_delta) {
                event = ManchesterEventShortLow;
            } else if (
                DURATION_DIFF(duration, te_long) < te_delta * 2) {
                event = ManchesterEventLongLow;
            }
        } else {
            if (DURATION_DIFF(duration, te_short) < te_delta) {
                event = ManchesterEventShortHigh;
            } else if (
                DURATION_DIFF(duration, te_long) < te_delta * 2) {
                event = ManchesterEventLongHigh;
            }
        }
        if (event != ManchesterEventReset) {
            bool bit;
            bool data_ok = FProtoGeneral::manchester_advance(
                manchester_saved_state, event, &manchester_saved_state, &bit);
            if (data_ok) {
                subghz_protocol_decoder_honeywell_addbit(bit);
            }
        } else {
            decode_data = 0;
            decode_count_bit = 0;
        }
    }

   protected:
    ManchesterState manchester_saved_state = ManchesterStateMid1;

    void subghz_protocol_decoder_honeywell_addbit(bool bit) {
        decode_data = (decode_data << 1) | bit;
        decode_count_bit++;

        uint16_t preamble = (decode_data >> 48) & 0xFFFF;
        // can be multiple, since flipper can't read it well..
        if (preamble == 0b0011111111111110 || preamble == 0b0111111111111110 ||
            preamble == 0b1111111111111110) {
            uint8_t datatocrc[4];
            datatocrc[0] = (decode_data >> 40) & 0xFFFF;
            datatocrc[1] = (decode_data >> 32) & 0xFFFF;
            datatocrc[2] = (decode_data >> 24) & 0xFFFF;
            datatocrc[3] = (decode_data >> 16) & 0xFFFF;
            uint8_t channel = (decode_data >> 44) & 0xF;
            uint16_t crc_calc = 0;
            if (channel == 0x2 || channel == 0x4 || channel == 0xA) {
                // 2GIG brand
                crc_calc = subghz_protocol_honeywell_crc16(datatocrc, 4, 0x8050, 0);
            } else {  // channel == 0x8
                crc_calc = subghz_protocol_honeywell_crc16(datatocrc, 4, 0x8005, 0);
            }
            uint16_t crc = decode_data & 0xFFFF;
            if (crc == crc_calc) {
                // the data is good. process it.
                data = decode_data;
                data_count_bit = decode_count_bit;  // maybe set it to 64, and hack the first 2 bits to 1! will see if replay needs it
                if (callback) callback(this);
                decode_data = 0;
                decode_count_bit = 0;
            } else {
                return;
            }
        }
    }
    uint16_t subghz_protocol_honeywell_crc16(
        uint8_t const message[],
        unsigned nBytes,
        uint16_t polynomial,
        uint16_t init) {
        uint16_t remainder = init;
        unsigned byte, bit;

        for (byte = 0; byte < nBytes; ++byte) {
            remainder ^= message[byte] << 8;
            for (bit = 0; bit < 8; ++bit) {
                if (remainder & 0x8000) {
                    remainder = (remainder << 1) ^ polynomial;
                } else {
                    remainder = (remainder << 1);
                }
            }
        }
        return remainder;
    }
};

#endif
