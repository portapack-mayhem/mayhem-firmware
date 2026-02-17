#include "../../hackrf/firmware/common/firmware_info.h"
#include "../../hackrf/firmware/common/platform_detect.h"

// Use the same logic as official HackRF firmware
#ifdef JAWBREAKER
#define SUPPORTED_PLATFORM PLATFORM_JAWBREAKER
#elif HACKRF_ONE
#define SUPPORTED_PLATFORM (PLATFORM_HACKRF1_OG | PLATFORM_HACKRF1_R9)
#elif RAD1O
#define SUPPORTED_PLATFORM PLATFORM_RAD1O
#elif PRALINE
#define SUPPORTED_PLATFORM PLATFORM_PRALINE
#else
#define SUPPORTED_PLATFORM 0
#endif

#define DFU_MODE_VALUE 0

__attribute__((section(".firmware_info"))) const struct firmware_info_t firmware_info = {
    .magic = "HACKRFFW",
    .struct_version = 1,
    .dfu_mode = DFU_MODE_VALUE,
    .supported_platform = SUPPORTED_PLATFORM,
    .version_string = VERSION_STRING,
};