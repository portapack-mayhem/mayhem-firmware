/*
 * main.cpp — Sentinel labkit PortaPack external app: registration + entry point.
 *
 * GPLv2-or-later. This app is a derivative work of the PortaPack-Mayhem firmware
 * (GPLv2+); the app source under portapack/sentinel_labkit/ is distributed under the
 * same terms. See portapack/README.md for the licensing note. (The rest of the
 * drone-sentinel-lab repo is RESTRICTED and separately licensed.)
 *
 * Structure and the application_information_t block mirror the in-tree Signal Gen
 * external app (firmware/application/external/siggen/main.cpp). This app is a
 * safety-hardened signal generator: it reuses the siggen baseband, but every real
 * emission passes the ported labkit gate (GNSS emitted-footprint block + operator
 * attestation) before the transmitter is enabled.
 */

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

#include "ui_sentinel_labkit.hpp"

namespace ui::external_app::sentinel_labkit {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SentinelLabkitView>();
}
}  // namespace ui::external_app::sentinel_labkit

extern "C" {

__attribute__((section(".external_app.app_sentinel_labkit.application_information"), used))
application_information_t _application_information_sentinel_labkit = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::sentinel_labkit::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Sentinel Lab",
    /*.bitmap_data = */ {
        // 16x16 1bpp icon — a shielded antenna. Replace with a branded glyph if desired.
        0x00, 0x00,
        0x80, 0x01,
        0x80, 0x01,
        0xC0, 0x03,
        0xC0, 0x03,
        0x60, 0x06,
        0x30, 0x0C,
        0x18, 0x18,
        0x18, 0x18,
        0x3C, 0x3C,
        0x66, 0x66,
        0xC3, 0xC3,
        0x81, 0x81,
        0x00, 0x00,
        0x18, 0x18,
        0x00, 0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,

    // M4 baseband image tag. This app reuses the in-tree Signal Gen baseband
    // (proc_siggen, built as image tag 'PSIG' via DeclareTargets(PSIG siggen)) and
    // drives it with the standard baseband::set_siggen_* API. The external-app export
    // tool (export_external_apps.py) reads firmware/baseband/PSIG.bin for this tag and
    // bundles it into sentinel_labkit.ppma. Reusing an existing tag (as level/fpv_detect
    // reuse 'PWFM') avoids building a second copy of an identical baseband image.
    /*.m4_app_tag = portapack::spi_flash::image_tag_siggen */ {'P', 'S', 'I', 'G'},
    /*.m4_app_offset = */ 0x00000000,  // filled at compile time
};
}
