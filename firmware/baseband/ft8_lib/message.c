#include "message.h"
#include "text.h"
#include <stdlib.h>
#include <string.h>

#define LOG_LEVEL LOG_WARN
#include "debug.h"

#define MAX22    ((uint32_t)4194304ul)
#define NTOKENS  ((uint32_t)2063592ul)
#define MAXGRID4 ((uint16_t)32400ul)

////////////////////////////////////////////////////// Static function prototypes //////////////////////////////////////////////////////////////

static void add_brackets(char* result, const char* original, int length);

/// Compute hash value for a callsign and save it in a hash table via the provided callsign hash interface.
/// @param[in] hash_if  Callsign hash interface
/// @param[in] callsign Callsign (up to 11 characters, trimmed)
/// @param[out] n22_out Pointer to store 22-bit hash value (can be NULL)
/// @param[out] n12_out Pointer to store 12-bit hash value (can be NULL)
/// @param[out] n10_out Pointer to store 10-bit hash value (can be NULL)
/// @return True on success
static bool save_callsign(const ftx_callsign_hash_interface_t* hash_if, const char* callsign, uint32_t* n22_out, uint16_t* n12_out, uint16_t* n10_out);
static bool lookup_callsign(const ftx_callsign_hash_interface_t* hash_if, ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign);

/// returns the numeric value if it matches "CQ nnn" or "CQ a[bcd]", otherwise -1
static int parse_cq_modifier(const char* string);

/// Pack a special token, a 22-bit hash code, or a valid base call into a 29-bit integer.
static int32_t pack28(const char* callsign, const ftx_callsign_hash_interface_t* hash_if, uint8_t* ip);

/// Unpack a callsign from 28+1 bit field in the payload of the standard message (type 1 or type 2).
/// @param[in] n29     29-bit integer, e.g. n29a or n29b, containing encoded callsign, plus suffix flag (1 bit) as LSB
/// @param[in] i3      Payload type (3 bits), 1 or 2
/// @param[in] hash_if Callsign hash table interface (can be NULL)
/// @param[out] result Unpacked callsign (max size: 13 characters including the terminating \0)
static int unpack28(uint32_t n28, uint8_t ip, uint8_t i3, const ftx_callsign_hash_interface_t* hash_if, char* result, ftx_field_t* field_type);

/// Pack a non-standard base call into a 28-bit integer.
static bool pack58(const ftx_callsign_hash_interface_t* hash_if, const char* callsign, uint64_t* n58);

/// Unpack a non-standard base call from a 58-bit integer.
static bool unpack58(uint64_t n58, const ftx_callsign_hash_interface_t* hash_if, char* callsign);

static uint16_t packgrid(const char* grid4);
static int unpackgrid(uint16_t igrid4, uint8_t ir, char* extra, ftx_field_t* extra_field_type);

/////////////////////////////////////////////////////////// Exported functions /////////////////////////////////////////////////////////////////

void ftx_message_init(ftx_message_t* msg)
{
    memset((void*)msg, 0, sizeof(ftx_message_t));
}

uint8_t ftx_message_get_i3(const ftx_message_t* msg)
{
    // Extract i3 (bits 74..76)
    uint8_t i3 = (msg->payload[9] >> 3) & 0x07u;
    return i3;
}

uint8_t ftx_message_get_n3(const ftx_message_t* msg)
{
    // Extract n3 (bits 71..73)
    uint8_t n3 = ((msg->payload[8] << 2) & 0x04u) | ((msg->payload[9] >> 6) & 0x03u);
    return n3;
}

ftx_message_type_t ftx_message_get_type(const ftx_message_t* msg)
{
    // Extract i3 (bits 74..76)
    uint8_t i3 = (msg->payload[9] >> 3) & 0x07u;

    switch (i3)
    {
    case 0: {
        // Extract n3 (bits 71..73)
        uint8_t n3 = ((msg->payload[8] << 2) & 0x04u) | ((msg->payload[9] >> 6) & 0x03u);

        switch (n3)
        {
        case 0:
            return FTX_MESSAGE_TYPE_FREE_TEXT;
        case 1:
            return FTX_MESSAGE_TYPE_DXPEDITION;
        case 2:
            return FTX_MESSAGE_TYPE_EU_VHF;
        case 3:
        case 4:
            return FTX_MESSAGE_TYPE_ARRL_FD;
        case 5:
            return FTX_MESSAGE_TYPE_TELEMETRY;
        default:
            return FTX_MESSAGE_TYPE_UNKNOWN;
        }
        break;
    }
    case 1:
    case 2:
        return FTX_MESSAGE_TYPE_STANDARD;
        break;
    case 3:
        return FTX_MESSAGE_TYPE_ARRL_RTTY;
        break;
    case 4:
        return FTX_MESSAGE_TYPE_NONSTD_CALL;
        break;
    case 5:
        return FTX_MESSAGE_TYPE_WWROF;
    default:
        return FTX_MESSAGE_TYPE_UNKNOWN;
    }
}

ftx_message_rc_t ftx_message_encode(ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, const char* message_text)
{
    char call_to[12];
    char call_de[12];
    char extra[20];

    const char* parse_position = message_text;
    const bool is_cq = starts_with(message_text, "CQ ");
    if (is_cq) {
        parse_position += 3;
        memset(call_to, 0, sizeof(call_to));

        // copy the next token temporarily (for the debug message)
        copy_token(call_de, 12, parse_position);
        LOG(LOG_DEBUG, "next token after CQ: '%s' in '%s'\n", call_de, message_text);

        // see if the word after CQ matches the a[bcd] or nnn pattern, and append to call_to
        int cq_modifier_v = parse_cq_modifier(message_text);
        if (cq_modifier_v >= 0) {
            // treat "CQ nnn" or "CQ a[bcd]" as a single token:
            // copy the CQ and then the next token to call_to
            memcpy(call_to, "CQ \0", 4);
            parse_position = copy_token(call_to + 3, sizeof(call_to) - 3, parse_position);
            LOG(LOG_DEBUG, "CQ modifier encoding %d; parse_pos after CQ: %s in %s\n", cq_modifier_v, parse_position, message_text);
        } else {
            memcpy(call_to, "CQ\0", 3);
            // the next token should be call_de, which we will get below
        }
    } else {
        // else it's not a CQ: expect first token to be the "to" callsign
        parse_position = copy_token(call_to, sizeof(call_to), parse_position);
    }
    // now we are fairly sure the next word should be the "de" callsign
    parse_position = copy_token(call_de, sizeof(call_de), parse_position);
    // and the word after that may be a grid or signal report
    parse_position = copy_token(extra, sizeof(extra), parse_position);

    LOG(LOG_DEBUG, "ftx_message_encode: parsed '%s' '%s' '%s'; remaining chars '%s'\n", call_to, call_de, extra, parse_position);

    if (call_to[sizeof(call_to) - 1] != '\0')
    {
        // token too long
        return FTX_MESSAGE_RC_ERROR_CALLSIGN1;
    }
    if (call_de[sizeof(call_de) - 1] != '\0')
    {
        // token too long
        return FTX_MESSAGE_RC_ERROR_CALLSIGN2;
    }
    if (extra[sizeof(extra) - 1] != '\0')
    {
        // token too long
        return FTX_MESSAGE_RC_ERROR_GRID;
    }

    ftx_message_rc_t rc;
    if (!parse_position[0]) {
        // up to 3 tokens with no leftovers
        rc = ftx_message_encode_std(msg, hash_if, call_to, call_de, extra);
        if (rc == FTX_MESSAGE_RC_OK)
            return rc;
        LOG(LOG_DEBUG, "   ftx_message_encode_std failed: %d\n", rc);
        rc = ftx_message_encode_nonstd(msg, hash_if, call_to, call_de, extra);
        if (rc == FTX_MESSAGE_RC_OK)
            return rc;
        LOG(LOG_DEBUG, "   ftx_message_encode_nonstd failed: %d\n", rc);
    }
    rc = ftx_message_encode_free(msg, message_text);
    if (rc == FTX_MESSAGE_RC_OK)
        return rc;
    LOG(LOG_DEBUG, "   ftx_message_encode_free failed: %d\n", rc);

    return rc;
}

ftx_message_rc_t ftx_message_encode_std(ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, const char* call_to, const char* call_de, const char* extra)
{
    uint8_t ipa, ipb;

    LOG(LOG_DEBUG, "ftx_message_encode_std '%s' '%s' '%s'\n", call_to, call_de, extra);

    int32_t n28a = pack28(call_to, hash_if, &ipa);
    int32_t n28b = pack28(call_de, hash_if, &ipb);
    LOG(LOG_DEBUG, "   n29a = %d, n29b = %d\n", n28a, n28b);

    if (n28a < 0)
        return FTX_MESSAGE_RC_ERROR_CALLSIGN1;
    if (n28b < 0)
        return FTX_MESSAGE_RC_ERROR_CALLSIGN2;

    uint8_t i3 = 1; // No suffix or /R
    if (ends_with(call_to, "/P") || ends_with(call_de, "/P"))
    {
        i3 = 2; // Suffix /P for EU VHF contest
        if (ends_with(call_to, "/R") || ends_with(call_de, "/R"))
        {
            return FTX_MESSAGE_RC_ERROR_SUFFIX;
        }
    }

    char* slash_de = strchr(call_de, '/');
    uint8_t icq = (uint8_t)equals(call_to, "CQ") || starts_with(call_to, "CQ ");
    if (slash_de && (slash_de - call_de >= 2) && icq && !(equals(slash_de, "/P") || equals(slash_de, "/R")))
    {
        return FTX_MESSAGE_RC_ERROR_CALLSIGN2; // nonstandard call: need a type 4 message
    }

    uint16_t igrid4 = packgrid(extra);
    LOG(LOG_DEBUG, "igrid4 = %d\n", igrid4);

    // Shift in ipa and ipb bits into n28a and n28b
    uint32_t n29a = ((uint32_t)n28a << 1) | ipa;
    uint32_t n29b = ((uint32_t)n28b << 1) | ipb;

    // TODO: check for suffixes
    if (ends_with(call_to, "/R"))
        n29a |= 1; // ipa = 1
    else if (ends_with(call_to, "/P"))
    {
        n29a |= 1; // ipa = 1
        i3 = 2;
    }

    // Pack into (28 + 1) + (28 + 1) + (1 + 15) + 3 bits
    msg->payload[0] = (uint8_t)(n29a >> 21);
    msg->payload[1] = (uint8_t)(n29a >> 13);
    msg->payload[2] = (uint8_t)(n29a >> 5);
    msg->payload[3] = (uint8_t)(n29a << 3) | (uint8_t)(n29b >> 26);
    msg->payload[4] = (uint8_t)(n29b >> 18);
    msg->payload[5] = (uint8_t)(n29b >> 10);
    msg->payload[6] = (uint8_t)(n29b >> 2);
    msg->payload[7] = (uint8_t)(n29b << 6) | (uint8_t)(igrid4 >> 10);
    msg->payload[8] = (uint8_t)(igrid4 >> 2);
    msg->payload[9] = (uint8_t)(igrid4 << 6) | (uint8_t)(i3 << 3);

    return FTX_MESSAGE_RC_OK;
}

ftx_message_rc_t ftx_message_encode_nonstd(ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, const char* call_to, const char* call_de, const char* extra)
{
    uint8_t i3 = 4;

    LOG(LOG_DEBUG, "ftx_message_encode_nonstd '%s' '%s' '%s'\n", call_to, call_de, extra);

    uint8_t icq = (uint8_t)equals(call_to, "CQ") || starts_with(call_to, "CQ ");
    int len_call_to = strlen(call_to);
    int len_call_de = strlen(call_de);

    if ((icq == 0) && ((len_call_to < 3)))
        return FTX_MESSAGE_RC_ERROR_CALLSIGN1;
    if ((len_call_de < 3))
        return FTX_MESSAGE_RC_ERROR_CALLSIGN2;

    uint8_t iflip;
    uint16_t n12;
    uint64_t n58;
    uint8_t nrpt;

    const char* call58;

    if (icq == 0)
    {
        // choose which of the callsigns to encode as plain-text (58 bits) or hash (12 bits)
        iflip = 0; // call_de will be sent plain-text
        if (call_de[0] == '<' && call_de[len_call_to - 1] == '>')
        {
            iflip = 1;
        }

        const char* call12;
        call12 = (iflip == 0) ? call_to : call_de;
        call58 = (iflip == 0) ? call_de : call_to;
        if (!save_callsign(hash_if, call12, NULL, &n12, NULL))
        {
            return FTX_MESSAGE_RC_ERROR_CALLSIGN1;
        }
    }
    else
    {
        iflip = 0;
        n12 = 0;
        call58 = call_de;
        LOG(LOG_DEBUG, "CQ: 58-bit '%s'; omitting grid '%s'\n", call58, extra);
    }

    if (!pack58(hash_if, call58, &n58))
    {
        return FTX_MESSAGE_RC_ERROR_CALLSIGN2;
    }

    if (icq != 0)
        nrpt = 0;
    else if (equals(extra, "RRR"))
        nrpt = 1;
    else if (equals(extra, "RR73"))
        nrpt = 2;
    else if (equals(extra, "73"))
        nrpt = 3;
    else
        nrpt = 0;

    // Pack into 12 + 58 + 1 + 2 + 1 + 3 == 77 bits
    // write(c77,1010) n12,n58,iflip,nrpt,icq,i3
    // format(b12.12,b58.58,b1,b2.2,b1,b3.3)
    msg->payload[0] = (uint8_t)(n12 >> 4);
    msg->payload[1] = (uint8_t)(n12 << 4) | (uint8_t)(n58 >> 54);
    msg->payload[2] = (uint8_t)(n58 >> 46);
    msg->payload[3] = (uint8_t)(n58 >> 38);
    msg->payload[4] = (uint8_t)(n58 >> 30);
    msg->payload[5] = (uint8_t)(n58 >> 22);
    msg->payload[6] = (uint8_t)(n58 >> 14);
    msg->payload[7] = (uint8_t)(n58 >> 6);
    msg->payload[8] = (uint8_t)(n58 << 2) | (uint8_t)(iflip << 1) | (uint8_t)(nrpt >> 1);
    msg->payload[9] = (uint8_t)(nrpt << 7) | (uint8_t)(icq << 6) | (uint8_t)(i3 << 3);

    return FTX_MESSAGE_RC_OK;
}

ftx_message_rc_t ftx_message_encode_free(ftx_message_t* msg, const char* text)
{
    uint8_t str_len = strlen(text);
    if (str_len > 13)
    {
        // Too long text
        return FTX_MESSAGE_RC_ERROR_TYPE;
    }

    uint8_t b71[9];
    memset(b71, 0, 9);
    int8_t cid;
    char c;

    for (int idx = 0; idx < 13; idx++)
    {
        if (idx < str_len)
        {
            c = text[idx];
        }
        else
        {
            c = ' ';
        }
        cid = nchar(c, FT8_CHAR_TABLE_FULL);
        if (cid == -1)
        {
            return FTX_MESSAGE_RC_ERROR_TYPE;
        }
        uint16_t rem = cid;
        for (int i = 8; i >= 0; i--)
        {
            rem += b71[i] * 42;
            b71[i] = rem & 0xff;
            rem = rem >> 8;
        }
    }
    ftx_message_rc_t ret = ftx_message_encode_telemetry(msg, b71);
    msg->payload[9] = 0; // i3.n3 = 0.0; etc.
    return ret;
}

// TODO set byte 9? or must the caller do it?
ftx_message_rc_t ftx_message_encode_telemetry(ftx_message_t* msg, const uint8_t* telemetry)
{
    // Shift bits in telemetry left by 1 bit to right-align the data
    uint8_t carry = 0;
    for (int i = 8; i >= 0; --i)
    {
        msg->payload[i] = (telemetry[i] << 1) | (carry >> 7);
        carry = telemetry[i] & 0x80;
    }
    return FTX_MESSAGE_RC_OK;
}

ftx_message_rc_t ftx_message_decode(const ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, char* message, ftx_message_offsets_t* offsets)
{
    ftx_message_rc_t rc;

    char buf[35]; // 13 + 13 + 6 (std/nonstd) / 14 (free text) / 19 (telemetry)
    char* field1 = buf;
    char* field2 = buf + 14;
    char* field3 = buf + 14 + 14;

    message[0] = '\0';
    for (int i = 0; i < FTX_MAX_MESSAGE_FIELDS; ++i)
    {
        offsets->types[i] = FTX_FIELD_UNKNOWN;
        offsets->offsets[i] = -1;
    }

    ftx_message_type_t msg_type = ftx_message_get_type(msg);
    switch (msg_type)
    {
    case FTX_MESSAGE_TYPE_STANDARD:
        rc = ftx_message_decode_std(msg, hash_if, field1, field2, field3, offsets->types);
        break;
    case FTX_MESSAGE_TYPE_NONSTD_CALL:
        rc = ftx_message_decode_nonstd(msg, hash_if, field1, field2, field3, offsets->types);
        break;
    case FTX_MESSAGE_TYPE_FREE_TEXT:
        ftx_message_decode_free(msg, field1);
        field2 = NULL;
        field3 = NULL;
        rc = FTX_MESSAGE_RC_OK;
        break;
    case FTX_MESSAGE_TYPE_TELEMETRY:
        ftx_message_decode_telemetry_hex(msg, field1);
        field2 = NULL;
        field3 = NULL;
        rc = FTX_MESSAGE_RC_OK;
        break;
    default:
        // not handled yet
        field1 = NULL;
        rc = FTX_MESSAGE_RC_ERROR_TYPE;
        break;
    }

    if (field1 != NULL)
    {
        // TODO join fields via whitespace
        const char* message_start = message;
        message = append_string(message, field1);
        offsets->offsets[0] = 0;
        if (field2 != NULL)
        {
            message = append_string(message, " ");
            offsets->offsets[1] = message - message_start;
            message = append_string(message, field2);
            if ((field3 != NULL) && (field3[0] != 0))
            {
                message = append_string(message, " ");
                offsets->offsets[2] = message - message_start;
                message = append_string(message, field3);
            }
        }
    }

    return rc;
}

ftx_message_rc_t ftx_message_decode_std(const ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if,
    char* call_to, char* call_de, char* extra, ftx_field_t field_types[FTX_MAX_MESSAGE_FIELDS])
{
    uint32_t n29a, n29b;
    uint16_t igrid4;
    uint8_t ir;

    // Extract packed fields
    n29a = (msg->payload[0] << 21);
    n29a |= (msg->payload[1] << 13);
    n29a |= (msg->payload[2] << 5);
    n29a |= (msg->payload[3] >> 3);
    n29b = ((msg->payload[3] & 0x07u) << 26);
    n29b |= (msg->payload[4] << 18);
    n29b |= (msg->payload[5] << 10);
    n29b |= (msg->payload[6] << 2);
    n29b |= (msg->payload[7] >> 6);
    ir = ((msg->payload[7] & 0x20u) >> 5);
    igrid4 = ((msg->payload[7] & 0x1Fu) << 10);
    igrid4 |= (msg->payload[8] << 2);
    igrid4 |= (msg->payload[9] >> 6);

    // Extract i3 (bits 74..76)
    uint8_t i3 = (msg->payload[9] >> 3) & 0x07u;
    LOG(LOG_DEBUG, "decode_std() n28a=%d ipa=%d n28b=%d ipb=%d ir=%d igrid4=%d i3=%d\n", n29a >> 1, n29a & 1u, n29b >> 1, n29b & 1u, ir, igrid4, i3);

    call_to[0] = call_de[0] = extra[0] = '\0';

    // Unpack both callsigns
    if (unpack28(n29a >> 1, n29a & 1u, i3, hash_if, call_to, &field_types[0]) < 0)
    {
        return FTX_MESSAGE_RC_ERROR_CALLSIGN1;
    }
    if (unpack28(n29b >> 1, n29b & 1u, i3, hash_if, call_de, &field_types[1]) < 0)
    {
        return FTX_MESSAGE_RC_ERROR_CALLSIGN2;
    }
    if (unpackgrid(igrid4, ir, extra, &field_types[2]) < 0)
    {
        return FTX_MESSAGE_RC_ERROR_GRID;
    }

    LOG(LOG_INFO, "Decoded standard (type %d) message [%s] [%s] [%s]\n", i3, call_to, call_de, extra);
    return FTX_MESSAGE_RC_OK;
}

// non-standard messages, code originally by KD8CEC
ftx_message_rc_t ftx_message_decode_nonstd(const ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if,
    char* call_to, char* call_de, char* extra, ftx_field_t field_types[FTX_MAX_MESSAGE_FIELDS])
{
    uint16_t n12, iflip, nrpt, icq;
    uint64_t n58;
    n12 = (msg->payload[0] << 4);  // 11 ~ 4 : 8
    n12 |= (msg->payload[1] >> 4); // 3 ~ 0  : 12

    n58 = ((uint64_t)(msg->payload[1] & 0x0Fu) << 54); // 57 ~ 54 : 4
    n58 |= ((uint64_t)msg->payload[2] << 46);          // 53 ~ 46 : 12
    n58 |= ((uint64_t)msg->payload[3] << 38);          // 45 ~ 38 : 12
    n58 |= ((uint64_t)msg->payload[4] << 30);          // 37 ~ 30 : 12
    n58 |= ((uint64_t)msg->payload[5] << 22);          // 29 ~ 22 : 12
    n58 |= ((uint64_t)msg->payload[6] << 14);          // 21 ~ 14 : 12
    n58 |= ((uint64_t)msg->payload[7] << 6);           // 13 ~ 6  : 12
    n58 |= ((uint64_t)msg->payload[8] >> 2);           // 5 ~ 0   : 765432 10

    iflip = (msg->payload[8] >> 1) & 0x01u; // 76543210
    nrpt = ((msg->payload[8] & 0x01u) << 1);
    nrpt |= (msg->payload[9] >> 7); // 76543210
    icq = ((msg->payload[9] >> 6) & 0x01u);

    // Extract i3 (bits 74..76)
    uint8_t i3 = (msg->payload[9] >> 3) & 0x07u;
    LOG(LOG_DEBUG, "decode_nonstd() n12=%04x n58=%08llx iflip=%d nrpt=%d icq=%d i3=%d\n", n12, n58, iflip, nrpt, icq, i3);

    // Decode one of the calls from 58 bit encoded string
    char call_decoded[14];
    unpack58(n58, hash_if, call_decoded);

    // Decode the other call from hash lookup table
    char call_3[14];
    lookup_callsign(hash_if, FTX_CALLSIGN_HASH_12_BITS, n12, call_3);

    // Possibly flip them around
    char* call_1 = (iflip) ? call_decoded : call_3;
    char* call_2 = (iflip) ? call_3 : call_decoded;

    if (icq == 0)
    {
        strcpy(call_to, call_1);
        field_types[0] = FTX_FIELD_CALL;
        if (nrpt == 1)
        {
            strcpy(extra, "RRR");
            field_types[2] = FTX_FIELD_TOKEN;
        }
        else if (nrpt == 2)
        {
            strcpy(extra, "RR73");
            field_types[2] = FTX_FIELD_TOKEN;
        }
        else if (nrpt == 3)
        {
            strcpy(extra, "73");
            field_types[2] = FTX_FIELD_TOKEN;
        }
        else
        {
            extra[0] = '\0';
            field_types[2] = FTX_FIELD_NONE;
        }
    }
    else
    {
        strcpy(call_to, "CQ");
        extra[0] = '\0';
        field_types[0] = FTX_FIELD_TOKEN;
        field_types[2] = FTX_FIELD_NONE;
    }
    strcpy(call_de, call_2);
    field_types[1] = FTX_FIELD_CALL;
    LOG(LOG_INFO, "Decoded non-standard (type %d) message [%s] [%s] [%s]\n", i3, call_to, call_de, extra);
    return FTX_MESSAGE_RC_OK;
}

void ftx_message_decode_free(const ftx_message_t* msg, char* text)
{
    uint8_t b71[9];

    ftx_message_decode_telemetry(msg, b71);

    char c14[14];
    c14[13] = 0;
    for (int idx = 12; idx >= 0; --idx)
    {
        // Divide the long integer in b71 by 42
        uint16_t rem = 0;
        for (int i = 0; i < 9; ++i)
        {
            rem = (rem << 8) | b71[i];
            b71[i] = rem / 42;
            rem = rem % 42;
        }
        c14[idx] = charn(rem, FT8_CHAR_TABLE_FULL);
    }

    strcpy(text, trim(c14));
}

void ftx_message_decode_telemetry_hex(const ftx_message_t* msg, char* telemetry_hex)
{
    uint8_t b71[9];

    ftx_message_decode_telemetry(msg, b71);

    // Convert b71 to hexadecimal string
    for (int i = 0; i < 9; ++i)
    {
        uint8_t nibble1 = (b71[i] >> 4);
        uint8_t nibble2 = (b71[i] & 0x0Fu);
        char c1 = (nibble1 > 9) ? (nibble1 - 10 + 'A') : nibble1 + '0';
        char c2 = (nibble2 > 9) ? (nibble2 - 10 + 'A') : nibble2 + '0';
        telemetry_hex[i * 2] = c1;
        telemetry_hex[i * 2 + 1] = c2;
    }

    telemetry_hex[18] = '\0';
}

void ftx_message_decode_telemetry(const ftx_message_t* msg, uint8_t* telemetry)
{
    // Shift bits in payload right by 1 bit to right-align the data
    uint8_t carry = 0;
    for (int i = 0; i < 9; ++i)
    {
        telemetry[i] = (carry << 7) | (msg->payload[i] >> 1);
        carry = (msg->payload[i] & 0x01u);
    }
}

#ifdef FTX_DEBUG_PRINT
#include <stdio.h>

void ftx_message_print(ftx_message_t* msg)
{
    printf("[");
    for (int i = 0; i < FTX_PAYLOAD_LENGTH_BYTES; ++i)
    {
        if (i > 0)
            printf(" ");
        printf("%02x", msg->payload[i]);
    }
    printf("]");
}
#endif

/////////////////////////////////////////////////////////// Static functions /////////////////////////////////////////////////////////////////

static void add_brackets(char* result, const char* original, int length)
{
    result[0] = '<';
    memcpy(result + 1, original, length);
    result[length + 1] = '>';
    result[length + 2] = '\0';
}

static bool save_callsign(const ftx_callsign_hash_interface_t* hash_if, const char* callsign, uint32_t* n22_out, uint16_t* n12_out, uint16_t* n10_out)
{
    uint64_t n58 = 0;
    int i = 0;
    while (callsign[i] != '\0' && i < 11)
    {
        int j = nchar(callsign[i], FT8_CHAR_TABLE_ALPHANUM_SPACE_SLASH);
        if (j < 0)
            return false; // hash error (wrong character set)
        n58 = (38 * n58) + j;
        i++;
    }
    // pretend to have trailing whitespace (with j=0, index of ' ')
    while (i < 11)
    {
        n58 = (38 * n58);
        i++;
    }

    uint32_t n22 = ((47055833459ull * n58) >> (64 - 22)) & (0x3FFFFFul);
    uint32_t n12 = n22 >> 10;
    uint32_t n10 = n22 >> 12;
    LOG(LOG_DEBUG, "save_callsign('%s') = [n22=%d, n12=%d, n10=%d]\n", callsign, n22, n12, n10);

    if (n22_out != NULL)
        *n22_out = n22;
    if (n12_out != NULL)
        *n12_out = n12;
    if (n10_out != NULL)
        *n10_out = n10;

    if (hash_if != NULL)
        hash_if->save_hash(callsign, n22);

    return true;
}

static bool lookup_callsign(const ftx_callsign_hash_interface_t* hash_if, ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign)
{
    char c11[12];

    bool found;
    if (hash_if != NULL)
        found = hash_if->lookup_hash(hash_type, hash, c11);
    else
        found = false;

    if (!found)
    {
        strcpy(callsign, "<...>");
    }
    else
    {
        add_brackets(callsign, c11, strlen(c11));
    }
    LOG(LOG_DEBUG, "lookup_callsign(n%s=%d) = '%s'\n", (hash_type == FTX_CALLSIGN_HASH_22_BITS ? "22" : (hash_type == FTX_CALLSIGN_HASH_12_BITS ? "12" : "10")), hash, callsign);
    return found;
}

int32_t pack_basecall(const char* callsign, int length)
{
    if (length > 2)
    {
        // Attempt to pack a standard callsign, if fail, revert to hashed callsign
        char c6[6] = { ' ', ' ', ' ', ' ', ' ', ' ' };

        // Copy callsign to 6 character buffer
        if (starts_with(callsign, "3DA0") && (length > 4) && (length <= 7))
        {
            // Work-around for Swaziland prefix: 3DA0XYZ -> 3D0XYZ
            memcpy(c6, "3D0", 3);
            memcpy(c6 + 3, callsign + 4, length - 4);
        }
        else if (starts_with(callsign, "3X") && is_letter(callsign[2]) && length <= 7)
        {
            // Work-around for Guinea prefixes: 3XA0XYZ -> QA0XYZ
            memcpy(c6, "Q", 1);
            memcpy(c6 + 1, callsign + 2, length - 2);
        }
        else
        {
            // Check the position of callsign digit and make a right-aligned copy into c6
            if (is_digit(callsign[2]) && length <= 6)
            {
                // AB0XYZ
                memcpy(c6, callsign, length);
            }
            else if (is_digit(callsign[1]) && length <= 5)
            {
                // A0XYZ -> " A0XYZ"
                memcpy(c6 + 1, callsign, length);
            }
        }

        // Check for standard callsign
        int i0 = nchar(c6[0], FT8_CHAR_TABLE_ALPHANUM_SPACE);
        int i1 = nchar(c6[1], FT8_CHAR_TABLE_ALPHANUM);
        int i2 = nchar(c6[2], FT8_CHAR_TABLE_NUMERIC);
        int i3 = nchar(c6[3], FT8_CHAR_TABLE_LETTERS_SPACE);
        int i4 = nchar(c6[4], FT8_CHAR_TABLE_LETTERS_SPACE);
        int i5 = nchar(c6[5], FT8_CHAR_TABLE_LETTERS_SPACE);
        if ((i0 >= 0) && (i1 >= 0) && (i2 >= 0) && (i3 >= 0) && (i4 >= 0) && (i5 >= 0))
        {
            // This is a standard callsign
            LOG(LOG_DEBUG, "Encoding basecall [%.6s]\n", c6);
            int32_t n = i0;
            n = n * 36 + i1;
            n = n * 10 + i2;
            n = n * 27 + i3;
            n = n * 27 + i4;
            n = n * 27 + i5;

            return n; // Standard callsign
        }
    }
    return -1;
}

// returns the numeric value if it matches CQ_nnn or CQ_a[bcd], otherwise -1
static int parse_cq_modifier(const char* string)
{
    int nnum = 0, nlet = 0;

    // encode CQ_nnn or CQ_a[bcd]
    int m = 0;
    for (int i = 3; i < 8; ++i) {
        if (!string[i] || is_space(string[i]))
            break;
        else if (is_digit(string[i]))
            ++nnum;
        else if (is_letter(string[i])) {
            ++nlet;
            m = 27 * m + (string[i] - 'A' + 1);
        } else {
            // non-digit non-letter characters (such as '/') are not allowed
            return -1;
        }
    }
    LOG(LOG_DEBUG, "CQ_nnn/CQ_a[bcd] '%s' %d/%d\n", string, nnum, nlet);
    if (nnum == 3 && nlet == 0) {
        LOG(LOG_DEBUG, "CQ_nnn detected: %d\n", atoi(string + 3));
        return atoi(string + 3);
    }
    else if (nnum == 0 && nlet <= 4) {
        LOG(LOG_DEBUG, "CQ_a[bcd] detected: m %d\n", m);
        return 1000 + m;
    }
    return -1; // not a special CQ
}

static int32_t pack28(const char* callsign, const ftx_callsign_hash_interface_t* hash_if, uint8_t* ip)
{
    LOG(LOG_DEBUG, "pack28() callsign [%s]\n", callsign);
    *ip = 0;

    // Check for special tokens first
    if (equals(callsign, "DE"))
        return 0;
    if (equals(callsign, "QRZ"))
        return 1;
    if (equals(callsign, "CQ"))
        return 2;

    int length = strlen(callsign);
    LOG(LOG_DEBUG, "Callsign length = %d\n", length);

    if (starts_with(callsign, "CQ ") && length < 8)
    {
        int v = parse_cq_modifier(callsign);
        if (v < 0) {
            LOG(LOG_WARN, "CQ_nnn/CQ_a[bcd] '%s' not allowed\n", callsign);
            return -1;
        }
        return 3 + v;
    }

    // Detect /R and /P suffix for basecall check
    int length_base = length;
    if (ends_with(callsign, "/P") || ends_with(callsign, "/R"))
    {
        LOG(LOG_DEBUG, "Suffix /P or /R detected\n");
        *ip = 1;
        length_base = length - 2;
    }

    int32_t n28 = pack_basecall(callsign, length_base);
    if (n28 >= 0)
    {
        // Callsign can be encoded as a standard basecall with optional /P or /R suffix
        if (!save_callsign(hash_if, callsign, NULL, NULL, NULL))
            return -1;                          // Error (some problem with callsign contents)
        return NTOKENS + MAX22 + (uint32_t)n28; // Standard callsign
    }

    if ((length >= 3) && (length <= 11))
    {
        // Treat this as a nonstandard callsign: compute its 22-bit hash
        LOG(LOG_DEBUG, "Encoding '%s' as non-standard callsign\n", callsign);
        uint32_t n22;
        if (!save_callsign(hash_if, callsign, &n22, NULL, NULL)) {
            LOG(LOG_DEBUG, "   Failed to save '%s'\n", callsign);
            return -1; // Error (some problem with callsign contents)
        }
        *ip = 0;
        return NTOKENS + n22; // 22-bit hashed callsign
    }

    return -1; // Error
}

static int unpack28(uint32_t n28, uint8_t ip, uint8_t i3, const ftx_callsign_hash_interface_t* hash_if, char* result, ftx_field_t* field_type)
{
    LOG(LOG_DEBUG, "unpack28() n28=%d i3=%d\n", n28, i3);
    // Check for special tokens DE, QRZ, CQ, CQ nnn, CQ a[bcd]
    if (n28 < NTOKENS)
    {
        if (n28 <= 2u)
        {
            if (n28 == 0)
            {
                strcpy(result, "DE");
                *field_type = FTX_FIELD_TOKEN;
            }
            else if (n28 == 1)
            {
                strcpy(result, "QRZ");
                *field_type = FTX_FIELD_TOKEN;
            }
            else
            { /* if (n28 == 2) */
                strcpy(result, "CQ");
                *field_type = FTX_FIELD_TOKEN;
            }
            return 0; // Success
        }
        if (n28 <= 1002u)
        {
            // CQ nnn with 3 digits
            strcpy(result, "CQ ");
            int_to_dd(result + 3, n28 - 3, 3, false);
            *field_type = FTX_FIELD_TOKEN_WITH_ARG;
            return 0; // Success
        }
        if (n28 <= 532443ul)
        {
            // CQ ABCD with 4 alphanumeric symbols
            uint32_t n = n28 - 1003u;
            char aaaa[5];

            aaaa[4] = '\0';
            for (int i = 3; /* no condition */; --i)
            {
                aaaa[i] = charn(n % 27u, FT8_CHAR_TABLE_LETTERS_SPACE);
                if (i == 0)
                    break;
                n /= 27u;
            }

            strcpy(result, "CQ ");
            strcat(result, trim_front(aaaa, ' '));
            *field_type = FTX_FIELD_TOKEN_WITH_ARG;
            return 0; // Success
        }
        // unspecified
        return -1;
    }

    n28 = n28 - NTOKENS;
    if (n28 < MAX22)
    {
        // This is a 22-bit hash of a result
        lookup_callsign(hash_if, FTX_CALLSIGN_HASH_22_BITS, n28, result);
        *field_type = FTX_FIELD_CALL;
        return 0; // Success
    }

    // Standard callsign
    uint32_t n = n28 - MAX22;

    char callsign[7];
    callsign[6] = '\0';
    callsign[5] = charn(n % 27, FT8_CHAR_TABLE_LETTERS_SPACE);
    n /= 27;
    callsign[4] = charn(n % 27, FT8_CHAR_TABLE_LETTERS_SPACE);
    n /= 27;
    callsign[3] = charn(n % 27, FT8_CHAR_TABLE_LETTERS_SPACE);
    n /= 27;
    callsign[2] = charn(n % 10, FT8_CHAR_TABLE_NUMERIC);
    n /= 10;
    callsign[1] = charn(n % 36, FT8_CHAR_TABLE_ALPHANUM);
    n /= 36;
    callsign[0] = charn(n % 37, FT8_CHAR_TABLE_ALPHANUM_SPACE);

    // Copy callsign to 6 character buffer
    if (starts_with(callsign, "3D0") && !is_space(callsign[3]))
    {
        // Work-around for Swaziland prefix: 3D0XYZ -> 3DA0XYZ
        memcpy(result, "3DA0", 4);
        trim_copy(result + 4, callsign + 3);
    }
    else if ((callsign[0] == 'Q') && is_letter(callsign[1]))
    {
        // Work-around for Guinea prefixes: QA0XYZ -> 3XA0XYZ
        memcpy(result, "3X", 2);
        trim_copy(result + 2, callsign + 1);
    }
    else
    {
        // Skip trailing and leading whitespace in case of a short callsign
        trim_copy(result, callsign);
    }

    int length = strlen(result);
    if (length < 3)
        return -1; // Callsign too short

    // Check if we should append /R or /P suffix
    if (ip != 0)
    {
        if (i3 == 1)
            strcat(result, "/R");
        else if (i3 == 2)
            strcat(result, "/P");
        else
            return -2;
    }

    // Save the result to hash table
    save_callsign(hash_if, result, NULL, NULL, NULL);

    *field_type = FTX_FIELD_CALL;
    return 0; // Success
}

static bool pack58(const ftx_callsign_hash_interface_t* hash_if, const char* callsign, uint64_t* n58)
{
    // Decode one of the calls from 58 bit encoded string
    const char* src = callsign;
    if (*src == '<')
        src++;
    int length = 0;
    uint64_t result = 0;
    char c11[12];
    while (*src != '\0' && *src != '<' && (length < 11))
    {
        c11[length] = *src;
        int j = nchar(*src, FT8_CHAR_TABLE_ALPHANUM_SPACE_SLASH);
        if (j < 0)
            return false;
        result = (result * 38) + j;
        src++;
        length++;
    }
    c11[length] = '\0';

    if (!save_callsign(hash_if, c11, NULL, NULL, NULL))
        return false;

    *n58 = result;
    LOG(LOG_DEBUG, "pack58('%s')=%016llx\n", callsign, *n58);
    return true;
}

static bool unpack58(uint64_t n58, const ftx_callsign_hash_interface_t* hash_if, char* callsign)
{
    // Decode one of the calls from 58 bit encoded string
    char c11[12];
    c11[11] = '\0';
    uint64_t n58_backup = n58;
    for (int i = 10; /* no condition */; --i)
    {
        c11[i] = charn(n58 % 38, FT8_CHAR_TABLE_ALPHANUM_SPACE_SLASH);
        if (i == 0)
            break;
        n58 /= 38;
    }
    // The decoded string will be right-aligned, so trim all whitespace (also from back just in case)
    trim_copy(callsign, c11);

    LOG(LOG_DEBUG, "unpack58(%016llx)=%s\n", n58_backup, callsign);

    // Save the decoded call in a hash table for later
    if (strlen(callsign) >= 3)
        return save_callsign(hash_if, callsign, NULL, NULL, NULL);

    return false;
}

static uint16_t packgrid(const char* grid4)
{
    if (grid4 == 0 || grid4[0] == '\0')
    {
        // Two callsigns only, no report/grid
        return MAXGRID4 + 1;
    }

    // Take care of special cases
    if (equals(grid4, "RRR"))
        return MAXGRID4 + 2;
    if (equals(grid4, "RR73"))
        return MAXGRID4 + 3;
    if (equals(grid4, "73"))
        return MAXGRID4 + 4;

    // TODO: Check for "R " prefix before a 4 letter grid

    // Check for standard 4 letter grid
    if (in_range(grid4[0], 'A', 'R') && in_range(grid4[1], 'A', 'R') && is_digit(grid4[2]) && is_digit(grid4[3]))
    {
        uint16_t igrid4 = (grid4[0] - 'A');
        igrid4 = igrid4 * 18 + (grid4[1] - 'A');
        igrid4 = igrid4 * 10 + (grid4[2] - '0');
        igrid4 = igrid4 * 10 + (grid4[3] - '0');
        return igrid4;
    }

    // Parse report: +dd / -dd / R+dd / R-dd
    // TODO: check the range of dd
    if (grid4[0] == 'R')
    {
        int dd = dd_to_int(grid4 + 1, 3);
        uint16_t irpt = 35 + dd;
        return (MAXGRID4 + irpt) | 0x8000; // ir = 1
    }
    else
    {
        int dd = dd_to_int(grid4, 3);
        uint16_t irpt = 35 + dd;
        return (MAXGRID4 + irpt); // ir = 0
    }

    return MAXGRID4 + 1;
}

static int unpackgrid(uint16_t igrid4, uint8_t ir, char* extra, ftx_field_t* extra_field_type)
{
    char* dst = extra;

    if (igrid4 <= MAXGRID4)
    {
        // Extract 4 symbol grid locator
        if (ir > 0)
        {
            // In case of ir=1 add an "R " before grid
            dst = stpcpy(dst, "R ");
        }

        uint16_t n = igrid4;
        dst[4] = '\0';
        dst[3] = '0' + (n % 10); // 0..9
        n /= 10;
        dst[2] = '0' + (n % 10); // 0..9
        n /= 10;
        dst[1] = 'A' + (n % 18); // A..R
        n /= 18;
        dst[0] = 'A' + (n % 18); // A..R
                                 // if (ir > 0 && strncmp(call_to, "CQ", 2) == 0) return -1;
        *extra_field_type = FTX_FIELD_GRID;
    }
    else
    {
        // Extract report
        int irpt = igrid4 - MAXGRID4;

        // Check special cases first (irpt > 0 always)
        switch (irpt)
        {
        case 1:
            dst[0] = '\0';
            *extra_field_type = FTX_FIELD_NONE;
            break;
        case 2:
            strcpy(dst, "RRR");
            *extra_field_type = FTX_FIELD_TOKEN;
            break;
        case 3:
            strcpy(dst, "RR73");
            *extra_field_type = FTX_FIELD_TOKEN;
            break;
        case 4:
            strcpy(dst, "73");
            *extra_field_type = FTX_FIELD_TOKEN;
            break;
        default:
            // Extract signal report as a two digit number with a + or - sign
            if (ir > 0)
                *dst++ = 'R'; // Add "R" before report
            int_to_dd(dst, irpt - 35, 2, true);
            *extra_field_type = FTX_FIELD_RST;
            break;
        }
        // if (irpt >= 2 && strncmp(call_to, "CQ", 2) == 0) return -1;
    }

    return 0;
}
