#include "ui_wpan_rx.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include <algorithm>

using namespace portapack;

namespace ui::external_app::wpan_rx {

WpanRxView::WpanRxView(NavigationView& nav) : nav_{nav} {
    // Betöltjük a bázissávi DSP kódunkat az M4 processzorra
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi, &channel, &field_rf_amp, &field_lna, &field_vga,
                  &field_frequency, &console});

    // 5 MHz-es lépésköz, mert a WPAN csatornák így helyezkednek el (2405, 2410, 2415...)
    field_frequency.set_step(5000000);

    // Rádiós Hardver beállítása a WPAN szabványra
    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(4000000);       // 4 MHz kell a 2 Mchip/s feldolgozásához
    receiver_model.set_baseband_bandwidth(2500000);  // 2.5 MHz szűrő nyitás
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

    // Ráhangolás a WPAN "Channel 11"-re (2405 MHz), ami a leggyakoribb frekvencia
    receiver_model.set_target_frequency(2405000000);

    console.writeln("WPAN Sniffer Ready...");
    console.writeln("Ch 11: 2405MHz -> Ch 26: 2480MHz\n");
}

void WpanRxView::focus() {
    field_frequency.focus();
}

void WpanRxView::on_data_wpan(const WPANPacketMessage& msg) {
    if (msg.length < 2) return;  // Hibás, túl rövid csomag

    // --- MAC Frame Control Field (FCF) Értelmezése ---
    // Az FCF az első két bájt. Benne van a csomag típusa.
    uint16_t fcf = (msg.data[1] << 8) | msg.data[0];
    uint8_t frame_type = fcf & 0x07;  // A legalsó 3 bit a típus
    uint8_t seq = msg.data[2];        // A 3. bájt mindig a sorszám (Sequence Number)

    std::string type_str = "UNK";
    if (frame_type == 0)
        type_str = "BEA";  // Beacon (Hálózatkeresés / Azonosítás)
    else if (frame_type == 1)
        type_str = "DAT";  // Data (Tényleges adattovábbítás)
    else if (frame_type == 2)
        type_str = "ACK";  // Acknowledgment (Nyugtázás)
    else if (frame_type == 3)
        type_str = "CMD";  // MAC Command (Csatlakozás/Lecsatlakozás)

    // Formázott fejléc a konzolra
    std::string out = "[" + type_str + "] L:" + to_string_dec_uint(msg.length) + " S:" + to_string_hex(seq, 2) + "\n";

    // Nyers hexadecimális payload (Maximum az első 12 bájtot írjuk ki, hogy ráférjen a kijelzőre)
    int print_len = std::min((int)msg.length, 12);
    for (int i = 0; i < print_len; i++) {
        out += to_string_hex(msg.data[i], 2) + " ";
    }

    console.writeln(out);
}

WpanRxView::~WpanRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::wpan_rx