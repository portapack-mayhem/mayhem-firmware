#include "ui_wmbus_rx.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include <algorithm>

using namespace portapack;

namespace ui::external_app::wmbus_rx {

WMBusRxView::WMBusRxView(NavigationView& nav) : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&options_mode, &rssi, &channel, &field_rf_amp, &field_lna, &field_vga,
                  &field_frequency, &text_debug_st, &text_debug_fm, &console});

    field_frequency.set_step(100000);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(1000000);  // 1 MHz a tökéletes PLL-hez!
    receiver_model.set_baseband_bandwidth(500000);
    receiver_model.enable();

    options_mode.on_change = [this](size_t, int32_t v) { this->on_mode_changed(v); };
    options_mode.set_selected_index(0);
    on_mode_changed(0);
}

void WMBusRxView::on_mode_changed(int32_t mode) {
    receiver_model.set_squelch_level(mode);
    if (mode == 2) {
        receiver_model.set_target_frequency(868300000);
        console.writeln("\n-> S-Mode (868.3 MHz)");
    } else {
        receiver_model.set_target_frequency(868950000);
        console.writeln((mode == 0) ? "\n-> T-Mode (868.95 MHz)" : "\n-> C-Mode (868.95 MHz)");
    }
}

void WMBusRxView::focus() {
    options_mode.focus();
}

std::string WMBusRxView::decode_manufacturer(uint8_t byte1, uint8_t byte2) {
    uint16_t m_val = (byte2 << 8) | byte1;
    char c1 = ((m_val >> 10) & 0x1F) + 64;
    char c2 = ((m_val >> 5) & 0x1F) + 64;
    char c3 = (m_val & 0x1F) + 64;
    return std::string{c1, c2, c3};
}

// Kiszámolja, hogy az adott bájt indexe a W-MBus levegő-csomagban CRC bájt-e.
bool WMBusRxView::is_crc_byte(int idx) {
    if (idx == 10 || idx == 11) return true;  // Block 1 CRC
    if (idx > 11) {
        int block2_idx = idx - 12;
        if (block2_idx % 18 == 16 || block2_idx % 18 == 17) return true;  // Block 2...N CRC
    }
    return false;
}

void WMBusRxView::on_data_wmbus(const WMBusPacketMessage& msg) {
    if (msg.length == 0) {
        // Telemetria (Változatlan)
        uint8_t st = msg.data[0];
        uint8_t peak = msg.data[1];
        uint8_t syncs = msg.data[2];
        uint8_t errors = msg.data[3];
        uint8_t current_mode = msg.data[4];

        std::string st_str = (st == 0) ? "UNSYNC" : (st == 1 ? "LEN_H" : (st == 2 ? "LEN_L" : (st == 3 ? "DAT_H" : "DAT_L")));
        std::string mod_str = (current_mode == 0) ? "T" : (current_mode == 1 ? "C" : "S");

        text_debug_st.set("ST:" + st_str + " S:" + to_string_dec_uint(syncs) + " E:" + to_string_dec_uint(errors));
        text_debug_fm.set("FM Peak: " + to_string_dec_uint(peak) + " | Mod: " + mod_str);
        return;
    }

    if (msg.length < 13) return;  // Kell legalább a Block 1 (12 byte) + CI mező a kiíráshoz

    // RTL_433 VIZUÁLIS KLÓN FORMÁZÁS
    std::string mfr = decode_manufacturer(msg.data[2], msg.data[3]);
    std::string serial = to_string_hex(msg.data[7], 2) + to_string_hex(msg.data[6], 2) +
                         to_string_hex(msg.data[5], 2) + to_string_hex(msg.data[4], 2);

    std::string ver = to_string_dec_uint(msg.data[8]);
    uint8_t type_byte = msg.data[9];
    std::string type_str = "0x" + to_string_hex(type_byte, 2);
    if (type_byte == 0x08)
        type_str += " (Heat Cost)";
    else if (type_byte == 0x07)
        type_str += " (Water)";

    uint8_t l_field = msg.data[0];
    uint8_t c_field = msg.data[1];
    uint8_t ci_field = msg.data[12];  // A 10. és 11. bájt a levegőben a CRC, a 12. a CI mező!

    console.writeln("Mfr : " + mfr + "  ID: " + serial);
    console.writeln("Ver : " + ver + " Type: " + type_str);
    console.writeln("Ctrl: 0x" + to_string_hex(c_field, 2) + " Len: " + to_string_dec_uint(l_field + 1) + " CI: 0x" + to_string_hex(ci_field, 2));

    // Data Dump (CRC bájtok Csendes Kivágása, pontosan ahogy az rtl_433 csinálja!)
    std::string hex_dump = "Data: ";
    for (int i = 0; i < msg.length; i++) {
        if (!is_crc_byte(i)) {
            hex_dump += to_string_hex(msg.data[i], 2);
        }
    }

    // Mivel hosszú lehet a string, több sorba törjük ha kell
    if (hex_dump.length() > 30) {
        console.writeln(hex_dump.substr(0, 30));
        console.writeln(hex_dump.substr(30));
    } else {
        console.writeln(hex_dump);
    }

    console.writeln("------------------------");
}

WMBusRxView::~WMBusRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::wmbus_rx