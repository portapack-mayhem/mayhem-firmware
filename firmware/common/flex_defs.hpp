#ifndef __FLEX_DEFS_H__
#define __FLEX_DEFS_H__

#include <cstdint>
#include <array>
#include "baseband.hpp"

namespace flex {

enum class FlexMode : uint8_t {
    FLEX_1600_2FSK,
    FLEX_3200_2FSK,
    FLEX_3200_4FSK,
    FLEX_6400_4FSK
};

struct FlexStats {
    uint32_t symbols_processed;
    uint32_t total_frames;
    uint32_t correct_frames;
};

struct FlexPacket {
    uint32_t bitrate;     // 1600, 3200, 6400
    uint64_t capcode;     // supports long addresses (up to 4,297,068,542)
    uint32_t function;    // 0-3 (or BIW word index for type=9)
    uint32_t type;        // 0=SEC 1=INS 2=TON 3=NUM 4=SNUM 5=ALN 6=HEX 7=NNUM 8=SHORT 9=BIW
    char message[256];    // Decoded message text (not used for BIW)
    uint32_t status;      // 0=OK, other=Errors
    uint8_t cycle;        // FIW cycle (0-14)
    uint8_t frame;        // FIW frame (0-127)
    char phase;           // 'A','B','C','D'
    uint8_t is_inverted;  // 1=inverted polarity
    uint8_t addr_type;    // 0=short 1=long 2=temp 3=oper 4=net 5=info 6=rsvd 7=unk

    // Fragment flags (ALN/SEC/HEX)
    uint8_t frag;         // F field: 3=first, 0/1/2=continuation
    uint8_t more_frag;    // C bit
    uint8_t seq;          // N field (0-63)
    uint8_t is_new;       // R bit
    uint8_t maildrop;     // M bit
    uint8_t sig;          // 7-bit signature
    uint8_t has_flags;    // 1=fragment flags valid
    uint8_t sec_enc;      // secure encoding (0-3)
    uint8_t nnum_s;       // NNUM S flag
    uint8_t fiw_roaming;  // FIW n bit: 1=roaming supported
    uint8_t is_priority;  // 1=priority address (in BIW1 P section)

    // BIW raw values (type=9 only). biw_field identifies the content.
    // 0=SSID1 1=DATE 2=TIME 5=SYSINFO 7=SSID2
    uint8_t biw_field;  // BIW type field (0-7)
    uint16_t biw_v1;    // field-dependent value 1
    uint16_t biw_v2;    // field-dependent value 2
    uint16_t biw_v3;    // field-dependent value 3
    uint16_t biw_v4;    // field-dependent value 4
};

} /* namespace flex */

#endif /*__FLEX_DEFS_H__*/
