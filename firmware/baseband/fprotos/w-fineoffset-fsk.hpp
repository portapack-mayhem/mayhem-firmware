
#ifndef __FPROTO_FINEOFFSET_FSK_H__
#define __FPROTO_FINEOFFSET_FSK_H__

#include "weatherbase.hpp"

/*
Fine Offset Electronics / Ecowitt / Ambient Weather / Froggit / Misol... FSK sensors.
Protocol details are from rtl_433 ( https://github.com/merbanan/rtl_433 src/devices/fineoffset*.c, ambientweather_wh31e.c ).

All of them use FSK PCM (NRZ) at ~58us / bit, a 0xAA.. preamble, a 0x2DD4 sync word, then the packet starting with a family code:
    0x24 WH24, WH65, HP1000, WS69, Misol WS2320 (outdoor unit)
    0xDx, 0xEx WH25, WH32, WH32B, WN32B (T/H/P)
    0x41, 0x42 WH0290, WH41 (PM2.5)
    0x30, 0x37 WH31E, WH31B (T/H)
    0x40 WH40 (rain)
    0x20 WN20 (rain)
    0x68 WS68 (wind, light, UV)
    0x51 WH51, WN31 (soil moisture)
    0x80 WS80 (T/H, wind, light, UV)
    0x85 WS85 (wind, rain)
    0x90 WS90 (T/H, wind, light, UV, rain, pressure)
    0x34, 0x38 WN34, WN38 (temperature probe)
    0x57 WH57, WH31L (lightning)
    0x55 WH55 (water leak)
This class only finds and validates the packets (CRC / checksum), the parsing is done in the app from the raw payload.
*/

class FProtoWeatherFineOffsetFSK : public FProtoWeatherBase {
   public:
    FProtoWeatherFineOffsetFSK() {
        sensorType = FPW_FO_WH24;
    }

    void feed(bool, uint32_t) override {}  // not an OOK protocol, see feed_fsk()

    // level / duration from the FSK detector
    void feed_fsk(bool level, uint32_t duration) {
        // the bit time is 56-60us depending on the model. Measure it on the preamble (single bit runs), long runs (up to ~40 bits) need it.
        if (!collecting && duration > te_min && duration < te_max) te_x16 += ((int32_t)(duration << 4) - (int32_t)te_x16) / 8;
        uint32_t bits = ((duration << 4) + te_x16 / 2) / te_x16;
        if (bits > 64) bits = 64;  // no valid packet has longer runs, but let it resync
        for (uint32_t i = 0; i < bits; i++) add_bit(level);
    }

    // the burst is over
    void fsk_end() {
        if (collecting) check_packet();
        reset();
    }

    const uint8_t* getPayload(uint8_t& len) override {
        len = byte_count;
        return buf;
    }

   protected:
    static constexpr uint32_t te_nominal = 58;
    static constexpr uint32_t te_min = 40;
    static constexpr uint32_t te_max = 80;
    uint32_t te_x16 = te_nominal << 4;  // measured bit time, in 1/16 us
    uint32_t shift_reg = 0;
    uint8_t byte_count = 0;
    uint8_t bit_count = 0;
    uint8_t alt_run = 0;       // current run of alternating bits
    uint8_t preamble_len = 0;  // longest run of alternating bits before the sync word
    bool collecting = false;
    bool inverted = false;
    uint8_t buf[WS_MAX_PAYLOAD]{};

    void reset() {
        collecting = false;
        te_x16 = te_nominal << 4;
        shift_reg = 0;
        byte_count = 0;
        bit_count = 0;
        alt_run = 0;
        preamble_len = 0;
    }

    void add_bit(bool bit) {
        if (!collecting) {
            if (bit != (shift_reg & 1)) {
                if (alt_run < 0xFF) alt_run++;
                if (alt_run > preamble_len) preamble_len = alt_run;
            } else {
                alt_run = 0;
            }
            shift_reg = (shift_reg << 1) | bit;
            // part of the preamble + sync word. Look for inverted too, in case the spectrum is flipped.
            if ((shift_reg & 0xFFFFFF) == 0xAA2DD4) {
                inverted = false;
            } else if ((~shift_reg & 0xFFFFFF) == 0xAA2DD4) {
                inverted = true;
            } else {
                return;
            }
            collecting = true;
            byte_count = 0;
            bit_count = 0;
            return;
        }
        uint8_t b = bit ^ inverted;
        buf[byte_count] = (buf[byte_count] << 1) | b;
        if (++bit_count < 8) return;
        bit_count = 0;
        byte_count++;
        if (byte_count >= WS_MAX_PAYLOAD) {
            check_packet();
            reset();
        }
    }

    // not inlined, to save flash
    __attribute__((noinline)) uint8_t crc8(uint8_t len) {
        return FProtoGeneral::subghz_protocol_blocks_crc8(buf, len, 0x31, 0x00);
    }
    // CRC-8 (poly 0x31) of crc_len bytes, including the crc byte itself, must be 0. Then the SUM-8 of the first sum_len bytes is in the next one.
    __attribute__((noinline)) bool check_crc_sum(uint8_t crc_len, uint8_t sum_len) {
        if (byte_count < crc_len || byte_count <= sum_len || crc8(crc_len) != 0) return false;
        return sum_len == 0 || FProtoGeneral::subghz_protocol_blocks_add_bytes(buf, sum_len) == buf[sum_len];
    }

    struct Family {
        uint8_t code;
        uint8_t type;
        uint8_t crc_len;
        uint8_t sum_len;
    };

    void check_packet() {
        static constexpr Family families[] = {
            {0x41, FPW_FO_WH0290, 7, 7},
            {0x42, FPW_FO_WH0290, 7, 7},
            {0x30, FPW_FO_WH31E, 6, 6},
            {0x37, FPW_FO_WH31E, 6, 6},
            {0x40, FPW_FO_WH40, 8, 8},
            {0x20, FPW_FO_WN20, 9, 9},
            {0x68, FPW_FO_WS68, 15, 15},
            {0x51, FPW_FO_WH51, 13, 13},
            {0x80, FPW_FO_WS80, 17, 17},
            {0x85, FPW_FO_WS85, 27, 27},
            {0x90, FPW_FO_WS90, 31, 31},
            {0x34, FPW_FO_WN34, 8, 8},
            {0x38, FPW_FO_WN34, 8, 8},
            {0x57, FPW_FO_WH57, 8, 8},
            {0x55, FPW_FO_WH55, 9, 0},
        };
        uint8_t family = buf[0];
        bool ok = false;
        if (byte_count < 7) return;
        if (family == 0x24) {
            ok = check_crc_sum(16, 16);
            if (ok) {
                // WS69 has 8 more bytes with their own crc and sum.
                // WH24: ~38 bit preamble, ~3 bits postamble. WH65: ~46 bit preamble or ~12 bits postamble
                if (check_crc_sum(24, 24))
                    sensorType = FPW_FO_WS69;
                else if ((byte_count - 17) * 8 + bit_count < 8 && preamble_len < 43)
                    sensorType = FPW_FO_WH24;
                else
                    sensorType = FPW_FO_WH65;
            }
        } else if ((family & 0xF0) == 0xD0 || (family & 0xF0) == 0xE0) {
            // WH25, WH32(B): sum of 6 bytes. WH25 only: then the xor of them with swapped nibbles
            ok = FProtoGeneral::subghz_protocol_blocks_add_bytes(buf, 6) == buf[6];
            uint8_t x = 0;
            for (uint8_t i = 0; i < 6; i++) x ^= buf[i];
            x = (x << 4) | (x >> 4);
            sensorType = ((family & 0xF0) == 0xE0 && byte_count >= 8 && x == buf[7]) ? FPW_FO_WH25 : FPW_FO_WH32;
        } else {
            for (const auto& f : families) {
                if (f.code == family) {
                    sensorType = f.type;
                    ok = check_crc_sum(f.crc_len, f.sum_len);
                    break;
                }
            }
        }
        if (ok && callback) callback(this);
    }
};

#endif
