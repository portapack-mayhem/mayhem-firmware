#ifndef __UI_BLESPAM_CT_H__
#define __UI_BLESPAM_CT_H__

#include <cstdint>

#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x) / sizeof(x[0]))
#endif

static const struct {
    uint16_t value;
    const char* name;
    uint8_t count;
} iosModels[] = {
    {0x0E20, "AirPods Pro", 1},
    {0x0A20, "AirPods Max", 1},
    {0x0055, "Airtag", 1},
    {0x0030, "Hermes Airtag", 1},
    {0x0220, "AirPods", 1},
    {0x0F20, "AirPods 2nd Gen", 1},
    {0x1320, "AirPods 3rd Gen", 1},
    {0x1420, "AirPods Pro 2nd Gen", 1},
    {0x1020, "Beats Flex", 2},
    {0x0620, "Beats Solo 3", 2},
    {0x0320, "Powerbeats 3", 2},
    {0x0B20, "Powerbeats Pro", 6},
    {0x0C20, "Beats Solo Pro", 2},
    {0x1120, "Beats Studio Buds", 2},
    {0x0520, "Beats X", 2},
    {0x0920, "Beats Studio 3", 4},
    {0x1720, "Beats Studio Pro", 2},
    {0x1220, "Beats Fit Pro", 2},
    {0x1620, "Beats Studio Buds+", 2},
};
static const uint16_t iosModels_count = COUNT_OF(iosModels);

#endif