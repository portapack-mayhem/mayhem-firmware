#include "ui_flex_rx.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "memory_map.hpp"
#include "usb_serial_asyncmsg.hpp"

using namespace portapack;

namespace ui::external_app::flex_rx {

FlexAppView::FlexAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &console});

    field_frequency.set_value(receiver_model.target_frequency());
    field_frequency.updated = [this](rf::Frequency f) {
        update_freq(f);
    };

    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();
    receiver_model.set_squelch_level(0);

    baseband::set_flex_config();
}

FlexAppView::~FlexAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void FlexAppView::focus() {
    field_frequency.focus();
}

void FlexAppView::update_freq(rf::Frequency f) {
    receiver_model.set_target_frequency(f);
}

void FlexAppView::on_packet(const FlexPacketMessage* message) {
    std::string text = "";

    text += "FLEX ";
    text += to_string_dec_uint(message->packet.bitrate);
    text += " ";
    text += to_string_dec_uint(message->packet.capcode);
    text += ": ";
    text += message->packet.message;

    console.writeln(text);
}

void FlexAppView::on_stats(const FlexStatsMessage* /* message */) {
}

void FlexAppView::on_debug(const FlexDebugMessage* message) {
    std::string text = "DBG: ";
    text += message->text;
    text += " " + to_string_hex(message->val1, 8);
    text += " " + to_string_hex(message->val2, 8);
    console.writeln(text);
}

}  // namespace ui::external_app::flex_rx