#include "ui_wpan_rx.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include <algorithm>

using namespace portapack;

namespace ui::external_app::wpan_rx {

WpanRxView::WpanRxView(NavigationView& nav) : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi, &channel, &field_rf_amp, &field_lna, &field_vga,
                  &field_frequency, &console});

    field_frequency.set_step(5000000);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(4000000);
    receiver_model.set_baseband_bandwidth(2500000);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

    receiver_model.set_target_frequency(2405000000);

    console.writeln("WPAN / Zigbee Sniffer");
}

void WpanRxView::focus() {
    field_frequency.focus();
}

void WpanRxView::on_data_wpan(const WPANPacketMessage& msg) {
    if (msg.length < 3) return;  // Túl rövid, nincs benne még a fejléc sem

    // --- 1. Frame Control Field (FCF) Dekódolása (Első 2 bájt) ---
    uint16_t fcf = (msg.data[1] << 8) | msg.data[0];

    uint8_t frame_type = fcf & 0x07;              // Csomag típusa
    bool pan_id_comp = (fcf >> 6) & 0x01;         // PAN ID tömörítés (Ugyanaz a src és dest hálózat?)
    uint8_t dest_addr_mode = (fcf >> 10) & 0x03;  // 0=Nincs, 2=16-bit, 3=64-bit MAC
    uint8_t src_addr_mode = (fcf >> 14) & 0x03;   // 0=Nincs, 2=16-bit, 3=64-bit MAC

    uint8_t seq = msg.data[2];  // 3. bájt a Sequence Number

    std::string type_str = "UNK";
    if (frame_type == 0)
        type_str = "BEA";  // Beacon
    else if (frame_type == 1)
        type_str = "DAT";  // Data
    else if (frame_type == 2)
        type_str = "ACK";  // Acknowledge
    else if (frame_type == 3)
        type_str = "CMD";  // MAC Command

    // Csomag fejléce
    std::string out = "[" + type_str + "] L:" + to_string_dec_uint(msg.length) + " Seq:" + to_string_hex(seq, 2) + "\n";

    int offset = 3;  // Mutató a payloadban (FCF + Seq után vagyunk)

    // --- 2. CÉL (Destination) PAN ID és MAC cím ---
    if (dest_addr_mode > 0 && offset + 2 <= msg.length) {
        // A Zigbee Little-Endian, ezért fordítva olvassuk ki a bájtokat
        uint16_t dest_pan = (msg.data[offset + 1] << 8) | msg.data[offset];
        out += "DPAN: " + to_string_hex(dest_pan, 4) + "\n";
        offset += 2;

        int addr_len = (dest_addr_mode == 2) ? 2 : 8;  // 16 bites vagy 64 bites cím
        if (offset + addr_len <= msg.length) {
            out += "DA: ";
            // Címek kiírása fordított sorrendben (Big-Endianként olvashatóbb embernek)
            for (int i = addr_len - 1; i >= 0; i--) {
                out += to_string_hex(msg.data[offset + i], 2);
            }
            out += "\n";
            offset += addr_len;
        }
    }

    // --- 3. FORRÁS (Source) PAN ID és MAC cím ---
    if (src_addr_mode > 0) {
        // Ha nincs tömörítve, akkor a Forrásnak külön PAN ID-ja van (pl. másik hálózat)
        if (!pan_id_comp && offset + 2 <= msg.length) {
            uint16_t src_pan = (msg.data[offset + 1] << 8) | msg.data[offset];
            out += "SPAN: " + to_string_hex(src_pan, 4) + "\n";
            offset += 2;
        }

        int addr_len = (src_addr_mode == 2) ? 2 : 8;
        if (offset + addr_len <= msg.length) {
            out += "SA: ";
            for (int i = addr_len - 1; i >= 0; i--) {
                out += to_string_hex(msg.data[offset + i], 2);
            }
            out += "\n";
            offset += addr_len;
        }
    }

    // --- 4. Maradék Adat (Payload) ---
    // Kiírunk max 6 bájtot a hasznos teherből, hogy elférjen a képernyőn
    if (offset < msg.length) {
        out += "Pld: ";
        int print_len = std::min((int)(msg.length - offset), 6);
        for (int i = 0; i < print_len; i++) {
            out += to_string_hex(msg.data[offset + i], 2) + " ";
        }
        out += "\n";
    }

    console.writeln(out + "----------------");
}

WpanRxView::~WpanRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::wpan_rx