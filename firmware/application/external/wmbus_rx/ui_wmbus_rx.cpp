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
                  &field_frequency, &text_debug_err, &console});

    field_frequency.set_step(100000);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(1000000);
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

std::string WMBusRxView::decode_device_type(uint8_t type_byte) {
    switch (type_byte) {
        case 0x00:
            return "Other";
        case 0x01:
            return "Oil";
        case 0x02:
            return "Electricity";
        case 0x03:
            return "Gas";
        case 0x04:
            return "Heat (out)";
        case 0x05:
            return "Steam";
        case 0x06:
            return "Hot water";
        case 0x07:
            return "Water";
        case 0x08:
            return "Heat cost allocator";
        case 0x09:
            return "Compressed air";
        case 0x0A:
            return "Cooling (out)";
        case 0x0B:
            return "Cooling (in)";
        case 0x0C:
            return "Heat (in)";
        case 0x0D:
            return "Heat/cooling";
        case 0x0E:
            return "Bus/system";
        case 0x0F:
            return "Unknown medium";
        case 0x15:
            return "Hot water";
        case 0x16:
            return "Cold water";
        case 0x17:
            return "Dual water";
        case 0x18:
            return "Pressure";
        case 0x19:
            return "A/D converter";
        case 0x1A:
            return "Smoke detector";
        case 0x1B:
            return "Room sensor";
        case 0x1C:
            return "Gas detector";
        case 0x20:
            return "Breaker (electricity)";
        case 0x21:
            return "Valve (gas/water)";
        case 0x25:
            return "Customer unit (display)";
        case 0x28:
            return "Waste water";
        case 0x29:
            return "Garbage";
        case 0x37:
            return "Radio converter";
        case 0x62:
            return "Heat (volumetric)";
        case 0x72:
            return "Heat (compact)";
        case 0x80:
            return "Electricity";
        default:
            return "Unknown";
    }
}

bool WMBusRxView::is_crc_byte(int idx) {
    if (idx == 10 || idx == 11) return true;
    if (idx > 11) {
        int block2_idx = idx - 12;
        if (block2_idx % 18 == 16 || block2_idx % 18 == 17) return true;
    }
    return false;
}

void WMBusRxView::on_data_wmbus(const WMBusPacketMessage& msg) {
    if (msg.length == 0) {
        uint8_t err_reason = msg.data[5];
        uint8_t err_data = msg.data[6];

        if (err_reason != 0) {
            std::string err_str = "None";
            if (err_reason == 1)
                err_str = "3v6(0x" + to_string_hex(err_data, 2) + ")";
            else if (err_reason == 2)
                err_str = "Len<9(" + to_string_dec_uint(err_data) + ")";
            else if (err_reason == 3)
                err_str = "Len>250(" + to_string_dec_uint(err_data) + ")";
            else if (err_reason == 4)
                err_str = "Manch(0x" + to_string_hex(err_data, 2) + ")";

            text_debug_err.set("Last Err: " + err_str);
        }
        return;
    }

    text_debug_err.set("Last Err: None");

    if (msg.length < 13) return;

    std::string mfr = decode_manufacturer(msg.data[2], msg.data[3]);
    std::string serial = to_string_hex(msg.data[7], 2) + to_string_hex(msg.data[6], 2) +
                         to_string_hex(msg.data[5], 2) + to_string_hex(msg.data[4], 2);

    std::string ver = to_string_dec_uint(msg.data[8]);
    uint8_t type_byte = msg.data[9];
    std::string type_str = "0x" + to_string_hex(type_byte, 2) + " (" + decode_device_type(type_byte) + ")";

    uint8_t l_field = msg.data[0];
    uint8_t c_field = msg.data[1];
    uint8_t ci_field = msg.data[12];

    console.writeln("Mfr : " + mfr + "  ID: " + serial);
    console.writeln("Ver : " + ver + " Type: " + type_str);
    console.writeln("Ctrl: 0x" + to_string_hex(c_field, 2) + " Len: " + to_string_dec_uint(l_field + 1) + " CI: 0x" + to_string_hex(ci_field, 2));

    std::string hex_dump = "Data: ";
    for (int i = 0; i < msg.length; i++) {
        if (!is_crc_byte(i)) {
            hex_dump += to_string_hex(msg.data[i], 2);
        }
    }

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