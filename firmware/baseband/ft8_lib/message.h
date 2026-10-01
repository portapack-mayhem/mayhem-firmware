#ifndef _INCLUDE_MESSAGE_H_
#define _INCLUDE_MESSAGE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FTX_PAYLOAD_LENGTH_BYTES 10 ///< number of bytes to hold 77 bits of FTx payload data
#define FTX_MAX_MESSAGE_LENGTH   35 ///< max message length = callsign[13] + space + callsign[13] + space + report[6] + terminator
#define FTX_MAX_MESSAGE_FIELDS   3  // may need to get longer for multi-part messages (DXpedition, contest etc.)

/// Structure that holds the decoded message
typedef struct
{
    uint8_t payload[FTX_PAYLOAD_LENGTH_BYTES];
    uint16_t hash; ///< Hash value to be used in hash table and quick checking for duplicates
} ftx_message_t;

// ----------------------------------------------------------------------------------
// i3.n3 Example message                    Bits           Total  Purpose
// ----------------------------------------------------------------------------------
// 0.0   FREE TEXT MSG                      71               71   Free text
// 0.1   K1ABC RR73; W9XYZ <KH1/KH7Z> -12   28 28 10 5       71   DXpedition Mode
// 0.2   PA3XYZ/P R 590003 IO91NP           28 1 1 3 12 25   70   EU VHF contest
// 0.3   WA9XYZ KA1ABC R 16A EMA            28 28 1 4 3 7    71   ARRL Field Day
// 0.4   WA9XYZ KA1ABC R 32A EMA            28 28 1 4 3 7    71   ARRL Field Day
// 0.5   123456789ABCDEF012                 71               71   Telemetry (18 hex)
// 0.6   K1ABC RR73; CQ W9XYZ EN37          28 28 15         71   Contesting
// 0.7   ... tbd
// 1     WA9XYZ/R KA1ABC/R R FN42           28 1 28 1 1 15   74   Standard msg
// 2     PA3XYZ/P GM4ABC/P R JO22           28 1 28 1 1 15   74   EU VHF contest
// 3     TU; W9XYZ K1ABC R 579 MA           1 28 28 1 3 13   74   ARRL RTTY Roundup
// 4     <WA9XYZ> PJ4/KA1ABC RR73           12 58 1 2 1      74   Nonstandard calls
// 5     TU; W9XYZ K1ABC R-07 FN            1 28 28 1 7 9    74   WWROF contest ?

typedef enum
{
    FTX_MESSAGE_TYPE_FREE_TEXT,   // 0.0   FREE TEXT MSG                      71               71   Free text
    FTX_MESSAGE_TYPE_DXPEDITION,  // 0.1   K1ABC RR73; W9XYZ <KH1/KH7Z> -12   28 28 10 5       71   DXpedition Mode
    FTX_MESSAGE_TYPE_EU_VHF,      // 0.2   PA3XYZ/P R 590003 IO91NP           28 1 1 3 12 25   70   EU VHF contest
    FTX_MESSAGE_TYPE_ARRL_FD,     // 0.3   WA9XYZ KA1ABC R 16A EMA            28 28 1 4 3 7    71   ARRL Field Day
                                  // 0.4   WA9XYZ KA1ABC R 32A EMA            28 28 1 4 3 7    71   ARRL Field Day
    FTX_MESSAGE_TYPE_TELEMETRY,   // 0.5   0123456789abcdef01                 71               71   Telemetry (18 hex)
    FTX_MESSAGE_TYPE_CONTESTING,  // 0.6   K1ABC RR73; CQ W9XYZ EN37          28 28 15         71   Contesting
    FTX_MESSAGE_TYPE_STANDARD,    // 1     WA9XYZ/R KA1ABC/R R FN42           28 1 28 1 1 15   74   Standard msg
                                  // 2     PA3XYZ/P GM4ABC/P R JO22           28 1 28 1 1 15   74   EU VHF contest
    FTX_MESSAGE_TYPE_ARRL_RTTY,   // 3     TU; W9XYZ K1ABC R 579 MA           1 28 28 1 3 13   74   ARRL RTTY Roundup
    FTX_MESSAGE_TYPE_NONSTD_CALL, // 4     <WA9XYZ> PJ4/KA1ABC RR73           12 58 1 2 1      74   Nonstandard calls
    FTX_MESSAGE_TYPE_WWROF,       // 5     TU; W9XYZ K1ABC R-07 FN            1 28 28 1 7 9    74   WWROF contest ?
    FTX_MESSAGE_TYPE_UNKNOWN      // Unknown or invalid type
} ftx_message_type_t;

typedef enum
{
    FTX_CALLSIGN_HASH_22_BITS,
    FTX_CALLSIGN_HASH_12_BITS,
    FTX_CALLSIGN_HASH_10_BITS
} ftx_callsign_hash_type_t;

typedef struct
{
    /// Called when a callsign is looked up by its 22/12/10 bit hash code
    bool (*lookup_hash)(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign);
    /// Called when a callsign should hashed and stored (by its 22, 12 and 10 bit hash codes)
    void (*save_hash)(const char* callsign, uint32_t n22);
} ftx_callsign_hash_interface_t;

typedef enum
{
    FTX_MESSAGE_RC_OK,
    FTX_MESSAGE_RC_ERROR_CALLSIGN1,
    FTX_MESSAGE_RC_ERROR_CALLSIGN2,
    FTX_MESSAGE_RC_ERROR_SUFFIX,
    FTX_MESSAGE_RC_ERROR_GRID,
    FTX_MESSAGE_RC_ERROR_TYPE
} ftx_message_rc_t;

typedef enum
{
    FTX_FIELD_UNKNOWN,
    FTX_FIELD_NONE,
    FTX_FIELD_TOKEN,          // RRR, RR73, 73, DE, QRZ, CQ, ...
    FTX_FIELD_TOKEN_WITH_ARG, // CQ nnn, CQ abcd
    FTX_FIELD_CALL,
    FTX_FIELD_GRID,
    FTX_FIELD_RST
} ftx_field_t;

typedef struct
{
    // parallel arrays:
    // e.g. "CQ POTA W9XYZ AB12" generates
    // types { FTX_FIELD_TOKEN_WITH_ARG, FTX_FIELD_CALL, FTX_FIELD_CALL_GRID" }
    // offsets { 0, 8, 14 }
    // Both arrays end where offsets[i] < 0
    ftx_field_t types[FTX_MAX_MESSAGE_FIELDS];
    int16_t offsets[FTX_MAX_MESSAGE_FIELDS];
} ftx_message_offsets_t;

// Callsign types and sizes:
// * Std. call (basecall) - 1-2 letter/digit prefix (at least one letter), 1 digit area code, 1-3 letter suffix,
//                          total 3-6 chars (exception: 7 character calls with prefixes 3DA0- and 3XA..3XZ-)
// * Ext. std. call - basecall followed by /R or /P
// * Nonstd. call - all the rest, limited to 3-11 characters either alphanumeric or stroke (/)
// In case a call is looked up from its hash value, the call is enclosed in angular brackets (<CA0LL>).

void ftx_message_init(ftx_message_t* msg);

uint8_t ftx_message_get_i3(const ftx_message_t* msg);
uint8_t ftx_message_get_n3(const ftx_message_t* msg);
ftx_message_type_t ftx_message_get_type(const ftx_message_t* msg);

// bool ftx_message_check_recipient(const ftx_message_t* msg, const char* callsign);

/// Pack (encode) a callsign in the standard way, and return the numeric representation.
/// Returns -1 if \a callsign cannot be encoded in the standard way.
/// This function can be used to decide whether to call ftx_message_encode_std() or ftx_message_decode_nonstd().
/// Alternatively, ftx_message_encode_std() itself fails when one of the callsigns cannot be packed this way.
int32_t pack_basecall(const char* callsign, int length);

/// Pack (encode) a text message, guessing which message type to use and falling back on failure:
/// if there are 3 or fewer tokens, try ftx_message_encode_std first,
/// then ftx_message_encode_nonstd if that fails because of a non-standard callsign;
/// otherwise fall back to ftx_message_encode_free.
/// If you already know which type to use, you can call one of those functions directly.
ftx_message_rc_t ftx_message_encode(ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, const char* message_text);

/// Pack Type 1 (Standard 77-bit message) or Type 2 (ditto, with a "/P" call) message
/// Rules of callsign validity:
/// - call_to can be 'DE', 'CQ', 'QRZ', 'CQ_nnn' (three digits), or 'CQ_abcd' (four letters)
/// - nonstandard calls within <> brackets are allowed, if they don't contain '/'
ftx_message_rc_t ftx_message_encode_std(ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, const char* call_to, const char* call_de, const char* extra);

/// Pack Type 4 (One nonstandard call and one hashed call) message
ftx_message_rc_t ftx_message_encode_nonstd(ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, const char* call_to, const char* call_de, const char* extra);

/// Pack plain text, up to 13 characters
ftx_message_rc_t ftx_message_encode_free(ftx_message_t* msg, const char* text);
ftx_message_rc_t ftx_message_encode_telemetry(ftx_message_t* msg, const uint8_t* telemetry);

ftx_message_rc_t ftx_message_decode(const ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, char* message, ftx_message_offsets_t* offsets);
ftx_message_rc_t ftx_message_decode_std(const ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, char* call_to, char* call_de, char* extra, ftx_field_t field_types[FTX_MAX_MESSAGE_FIELDS]);
ftx_message_rc_t ftx_message_decode_nonstd(const ftx_message_t* msg, ftx_callsign_hash_interface_t* hash_if, char* call_to, char* call_de, char* extra, ftx_field_t field_types[FTX_MAX_MESSAGE_FIELDS]);
void ftx_message_decode_free(const ftx_message_t* msg, char* text);
void ftx_message_decode_telemetry_hex(const ftx_message_t* msg, char* telemetry_hex);
void ftx_message_decode_telemetry(const ftx_message_t* msg, uint8_t* telemetry);

#ifdef FTX_DEBUG_PRINT
void ftx_message_print(ftx_message_t* msg);
#endif

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_MESSAGE_H_
