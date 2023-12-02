#ifndef __UI_BLESPAM_CT_H__
#define __UI_BLESPAM_CT_H__

// Port of this:
// https://github.com/Flipper-XFW/Xtreme-Apps/blob/16f4acbea335ad6cb784ced9787e2db09f815abd/ble_spam/protocols/continuity.c
// Saying thanks in the main view!

#include <cstdint>

#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x) / sizeof(x[0]))
#endif

typedef enum {
    PayloadModeRandom,
    PayloadModeValue,
    PayloadModeBruteforce,
} PayloadMode;

typedef enum {
    ContinuityTypeAirDrop = 0x05,
    ContinuityTypeProximityPair = 0x07,
    ContinuityTypeAirplayTarget = 0x09,
    ContinuityTypeHandoff = 0x0C,
    ContinuityTypeTetheringSource = 0x0E,
    ContinuityTypeNearbyAction = 0x0F,
    ContinuityTypeNearbyInfo = 0x10,

    ContinuityTypeCustomCrash,
    ContinuityTypeCOUNT
} ContinuityType;

typedef enum {
    ContinuityPpBruteforceModel,
    ContinuityPpBruteforceColor,
} ContinuityPpBruteforce;

typedef struct {
    ContinuityType type;
    union {
        struct {
            ContinuityPpBruteforce bruteforce_mode;
            uint16_t model;
            uint8_t color;
            uint8_t prefix;
        } proximity_pair;
        struct {
            uint8_t action;
            uint8_t flags;
        } nearby_action;
    } data;
} ContinuityCfg;

typedef struct {
    uint8_t value;
    const char* name;
} ContinuityColor;

static const ContinuityColor colors_white[] = {
    {0x00, "White"},
};
static const ContinuityColor colors_airpods_max[] = {
    {0x00, "White"},
    {0x02, "Red"},
    {0x03, "Blue"},
    {0x0F, "Black"},
    {0x11, "Light Green"},
};
static const ContinuityColor colors_beats_flex[] = {
    {0x00, "White"},
    {0x01, "Black"},
};
static const ContinuityColor colors_beats_solo_3[] = {
    {0x00, "White"},
    {0x01, "Black"},
    {0x6, "Gray"},
    {0x7, "Gold/White"},
    {0x8, "Rose Gold"},
    {0x09, "Black"},
    {0xE, "Violet/White"},
    {0xF, "Bright Red"},
    {0x12, "Dark Red"},
    {0x13, "Swamp Green"},
    {0x14, "Dark Gray"},
    {0x15, "Dark Blue"},
    {0x1D, "Rose Gold 2"},
    {0x20, "Blue/Green"},
    {0x21, "Purple/Orange"},
    {0x22, "Deep Blue/ Light blue"},
    {0x23, "Magenta/Light Fuchsia"},
    {0x25, "Black/Red"},
    {0x2A, "Gray / Disney LTD"},
    {0x2E, "Pinkish white"},
    {0x3D, "Red/Blue"},
    {0x3E, "Yellow/Blue"},
    {0x3F, "White/Red"},
    {0x40, "Purple/White"},
    {0x5B, "Gold"},
    {0x5C, "Silver"},
};
static const ContinuityColor colors_powerbeats_3[] = {
    {0x00, "White"},
    {0x01, "Black"},
    {0x0B, "Gray/Blue"},
    {0x0C, "Gray/Red"},
    {0x0D, "Gray/Green"},
    {0x12, "Red"},
    {0x13, "Swamp Green"},
    {0x14, "Gray"},
    {0x15, "Deep Blue"},
    {0x17, "Dark with Gold Logo"},
};
static const ContinuityColor colors_powerbeats_pro[] = {
    {0x00, "White"},
    {0x02, "Yellowish Green"},
    {0x03, "Blue"},
    {0x04, "Black"},
    {0x05, "Pink"},
    {0x06, "Red"},
    {0x0B, "Gray ?"},
    {0x0D, "Sky Blue"},
};
static const ContinuityColor colors_beats_solo_pro[] = {
    {0x00, "White"},
    {0x01, "Black"},
};
static const ContinuityColor colors_beats_studio_buds[] = {
    {0x00, "White"},
    {0x01, "Black"},
    {0x02, "Red"},
    {0x03, "Blue"},
    {0x04, "Pink"},
    {0x06, "Silver"},
};
static const ContinuityColor colors_beats_x[] = {
    {0x00, "White"},
    {0x01, "Black"},
    {0x02, "Blue"},
    {0x05, "Gray"},
    {0x1D, "Pink"},
    {0x25, "Dark/Red"},
};
static const ContinuityColor colors_beats_studio_3[] = {
    {0x00, "White"},
    {0x01, "Black"},
    {0x02, "Red"},
    {0x03, "Blue"},
    {0x18, "Shadow Gray"},
    {0x19, "Desert Sand"},
    {0x25, "Black / Red"},
    {0x26, "Midnight Black"},
    {0x27, "Desert Sand 2"},
    {0x28, "Gray"},
    {0x29, "Clear blue/ gold"},
    {0x42, "Green Forest camo"},
    {0x43, "White Camo"},
};
static const ContinuityColor colors_beats_studio_pro[] = {
    {0x00, "White"},
    {0x01, "Black"},
};
static const ContinuityColor colors_beats_fit_pro[] = {
    {0x00, "White"},
    {0x01, "Black"},
    {0x02, "Pink"},
    {0x03, "Grey/White"},
    {0x04, "Full Pink"},
    {0x05, "Neon Green"},
    {0x06, "Night Blue"},
    {0x07, "Light Pink"},
    {0x08, "Brown"},
    {0x09, "Dark Brown"},
};
static const ContinuityColor colors_beats_studio_buds_[] = {
    {0x00, "Black"},
    {0x01, "White"},
    {0x02, "Transparent"},
    {0x03, "Silver"},
    {0x04, "Pink"},
};

static const struct {
    uint16_t value;
    const char* name;
    const ContinuityColor* colors;
    const uint8_t colors_count;
} pp_models[] = {
    {0x0E20, "AirPods Pro", colors_white, COUNT_OF(colors_white)},
    {0x0A20, "AirPods Max", colors_airpods_max, COUNT_OF(colors_airpods_max)},
    {0x0055, "Airtag", colors_white, COUNT_OF(colors_white)},
    {0x0030, "Hermes Airtag", colors_white, COUNT_OF(colors_white)},
    {0x0220, "AirPods", colors_white, COUNT_OF(colors_white)},
    {0x0F20, "AirPods 2nd Gen", colors_white, COUNT_OF(colors_white)},
    {0x1320, "AirPods 3rd Gen", colors_white, COUNT_OF(colors_white)},
    {0x1420, "AirPods Pro 2nd Gen", colors_white, COUNT_OF(colors_white)},
    {0x1020, "Beats Flex", colors_beats_flex, COUNT_OF(colors_beats_flex)},
    {0x0620, "Beats Solo 3", colors_beats_solo_3, COUNT_OF(colors_beats_solo_3)},
    {0x0320, "Powerbeats 3", colors_powerbeats_3, COUNT_OF(colors_powerbeats_3)},
    {0x0B20, "Powerbeats Pro", colors_powerbeats_pro, COUNT_OF(colors_powerbeats_pro)},
    {0x0C20, "Beats Solo Pro", colors_beats_solo_pro, COUNT_OF(colors_beats_solo_pro)},
    {0x1120, "Beats Studio Buds", colors_beats_studio_buds, COUNT_OF(colors_beats_studio_buds)},
    {0x0520, "Beats X", colors_beats_x, COUNT_OF(colors_beats_x)},
    {0x0920, "Beats Studio 3", colors_beats_studio_3, COUNT_OF(colors_beats_studio_3)},
    {0x1720, "Beats Studio Pro", colors_beats_studio_pro, COUNT_OF(colors_beats_studio_pro)},
    {0x1220, "Beats Fit Pro", colors_beats_fit_pro, COUNT_OF(colors_beats_fit_pro)},
    {0x1620, "Beats Studio Buds+", colors_beats_studio_buds_, COUNT_OF(colors_beats_studio_buds_)},
};
static const uint8_t pp_models_count = COUNT_OF(pp_models);

static const struct {
    uint8_t value;
    const char* name;
} pp_prefixes[] = {
    {0x07, "New Device"},
    {0x01, "Not Your Device"},
    {0x05, "New Airtag"},
};
static const uint8_t pp_prefixes_count = COUNT_OF(pp_prefixes);

static const struct {
    uint8_t value;
    const char* name;
} na_actions[] = {
    {0x13, "AppleTV AutoFill"},
    {0x27, "AppleTV Connecting..."},
    {0x20, "Join This AppleTV?"},
    {0x19, "AppleTV Audio Sync"},
    {0x1E, "AppleTV Color Balance"},
    {0x09, "Setup New iPhone"},
    {0x02, "Transfer Phone Number"},
    {0x0B, "HomePod Setup"},
    {0x01, "Setup New AppleTV"},
    {0x06, "Pair AppleTV"},
    {0x0D, "HomeKit AppleTV Setup"},
    {0x2B, "AppleID for AppleTV?"},
};
static const uint8_t na_actions_count = COUNT_OF(na_actions);

static const char* type_names[ContinuityTypeCOUNT] = {
    [ContinuityTypeAirDrop] = "AirDrop",
    [ContinuityTypeProximityPair] = "Continuity Pair",
    [ContinuityTypeAirplayTarget] = "Airplay Target",
    [ContinuityTypeHandoff] = "Handoff",
    [ContinuityTypeTetheringSource] = "Tethering Source",
    [ContinuityTypeNearbyAction] = "Continuity Action",
    [ContinuityTypeNearbyInfo] = "Nearby Info",
    [ContinuityTypeCustomCrash] = "Continuity Custom",
};

#define HEADER_LEN (6)  // 1 Size + 1 AD Type + 2 Company ID + 1 Continuity Type + 1 Continuity Size
static uint8_t packet_sizes[ContinuityTypeCOUNT] = {
    [ContinuityTypeAirDrop] = HEADER_LEN + 18,
    [ContinuityTypeProximityPair] = HEADER_LEN + 25,
    [ContinuityTypeAirplayTarget] = HEADER_LEN + 6,
    [ContinuityTypeHandoff] = HEADER_LEN + 14,
    [ContinuityTypeTetheringSource] = HEADER_LEN + 6,
    [ContinuityTypeNearbyAction] = HEADER_LEN + 5,
    [ContinuityTypeNearbyInfo] = HEADER_LEN + 5,
    [ContinuityTypeCustomCrash] = HEADER_LEN + 11,
};

static void make_packet(uint8_t* _size, uint8_t** _packet) {
}

#endif