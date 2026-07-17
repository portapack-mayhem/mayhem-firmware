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
}

void WMBusRxView::on_mode_changed(int32_t mode) {
    // receiver_model.set_squelch_level(mode); //todo send down the mode
    if (mode == 2) {
        receiver_model.set_target_frequency(868300000);
        // console.writeln("\n-> S-Mode (868.3 MHz)");
    } else {
        receiver_model.set_target_frequency(868950000);
        // console.writeln((mode == 0) ? "\n-> T-Mode (868.95 MHz)" : "\n-> C-Mode (868.95 MHz)");
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
    console.writeln("------------------------");
    console.writeln("Mfr : " + mfr + "  ID: " + serial);
    console.writeln("Ver : " + ver + " Type: " + type_str);

    auto get_byte = [&msg](int index) -> uint8_t {
        if (index < 10) return msg.data[index];
        int block = (index - 10) / 16;
        int rem = (index - 10) % 16;
        int phys_idx = 12 + block * 18 + rem;
        if (phys_idx < msg.length) return msg.data[phys_idx];
        return 0;
    };

    uint8_t ci_field = get_byte(10);
    std::string enc_str = "Enc: Unknown";
    std::string val_str = "";

    int ptr = 11;
    bool has_cw = false;
    uint8_t enc_mode = 0;

    if (ci_field == 0x72 || ci_field == 0x82) {
        ptr += 10;
        if (ptr + 1 <= l_field) {
            enc_mode = get_byte(ptr + 1) & 0x0F;
            has_cw = true;
        }
        ptr += 2;  // Data szakasz
    } else if (ci_field == 0x7A || ci_field == 0x8A) {
        ptr += 2;  // Access(1)+Status(1)
        if (ptr + 1 <= l_field) {
            enc_mode = get_byte(ptr + 1) & 0x0F;
            has_cw = true;
        }
        ptr += 2;  // Data szakasz
    } else if (ci_field == 0x78 || ci_field == 0x88) {
        has_cw = false;
    }

    if (has_cw && enc_mode > 0) {
        if (enc_mode == 5)
            enc_str = "Enc: AES-128";
        else if (enc_mode == 7)
            enc_str = "Enc: DES";
        else
            enc_str = "Enc: Mode " + to_string_dec_uint(enc_mode);
    } else {
        enc_str = "Enc: None";
        int values_found = 0;

        while (ptr <= l_field && values_found < 3) {
            uint8_t dif0 = get_byte(ptr++);
            if (dif0 == 0x0F || dif0 == 0x1F || dif0 == 0x2F) break;

            uint8_t data_len = 0;
            bool is_bcd = false;

            switch (dif0 & 0x0F) {
                case 0x00:
                    data_len = 0;
                    break;
                case 0x01:
                    data_len = 1;
                    break;
                case 0x02:
                    data_len = 2;
                    break;
                case 0x03:
                    data_len = 3;
                    break;
                case 0x04:
                    data_len = 4;
                    break;
                case 0x05:
                    data_len = 4;
                    break;  // 4 byte REAL
                case 0x06:
                    data_len = 6;
                    break;
                case 0x07:
                    data_len = 8;
                    break;
                case 0x08:
                    data_len = 0;
                    break;
                case 0x09:
                    data_len = 1;
                    is_bcd = true;
                    break;
                case 0x0A:
                    data_len = 2;
                    is_bcd = true;
                    break;
                case 0x0B:
                    data_len = 3;
                    is_bcd = true;
                    break;
                case 0x0C:
                    data_len = 4;
                    is_bcd = true;
                    break;
                case 0x0D:
                    data_len = 0;
                    break;  // Variable
                case 0x0E:
                    data_len = 6;
                    is_bcd = true;
                    break;
                case 0x0F:
                    data_len = 0;
                    break;
            }

            uint8_t dif_ext = dif0;
            while ((dif_ext & 0x80) && ptr <= l_field) {
                dif_ext = get_byte(ptr++);
            }

            if ((dif0 & 0x0F) == 0x0D) {
                if (ptr <= l_field) {
                    uint8_t lvar = get_byte(ptr++);
                    data_len = lvar & 0x7F;
                }
            }

            if (ptr <= l_field) {
                uint8_t vif = get_byte(ptr++);
                while ((vif & 0x80) && ptr <= l_field) {
                    vif = get_byte(ptr++);
                }
            }

            if (data_len > 0) {
                if (ptr + data_len <= l_field + 1) {
                    if (data_len <= 4) {
                        uint32_t val = 0;
                        for (int i = 0; i < data_len; i++) {
                            val |= (get_byte(ptr + i) << (i * 8));
                        }
                        if (is_bcd)
                            val_str += " V:" + to_string_hex(val, data_len * 2);
                        else
                            val_str += " V:" + to_string_dec_uint(val);
                        values_found++;
                    }
                    ptr += data_len;
                } else {
                    break;
                }
            }
        }
    }

    console.writeln("CI: 0x" + to_string_hex(ci_field, 2) + " " + enc_str);
    if (val_str.length() > 0) {
        console.writeln(val_str);
    }

    std::string hex_dump = "Hex: ";
    for (int i = 0; i <= l_field; i++) {
        hex_dump += to_string_hex(get_byte(i), 2);
    }

    if (hex_dump.length() > 30) {
        console.writeln(hex_dump.substr(0, 30));
        console.writeln(hex_dump.substr(30));
    } else {
        console.writeln(hex_dump);
    }
}

WMBusRxView::~WMBusRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::wmbus_rx