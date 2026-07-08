#include "ui_rds_rx.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include <cstring>

using namespace portapack;

namespace ui::external_app::rds_rx {

static const char* pty_names[32] = {
    "None", "News", "Affairs", "Info", "Sport", "Educate", "Drama", "Culture",
    "Science", "Varied", "Pop", "Rock", "Easy", "Light", "Classics", "Other M",
    "Weather", "Finance", "Children", "Social", "Religion", "Phone In", "Travel", "Leisure",
    "Jazz", "Country", "Nation M", "Oldies", "Folk M", "Document", "Test", "Alarm"};

RdsRxView::RdsRxView(NavigationView& nav) : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi, &channel, &field_rf_amp, &field_lna, &field_vga,
                  &field_frequency, &text_pi, &text_tp, &text_pty,
                  &text_ps_label, &text_ps_name,
                  &text_rt_label, &text_rt_1, &text_rt_2, &text_rt_3, &console});

    field_frequency.set_step(100000);

    receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

    memset(ps_name, ' ', 8);
    ps_name[8] = '\0';
    memset(radio_text, ' ', 64);
    radio_text[64] = '\0';
}

void RdsRxView::focus() {
    field_frequency.focus();
}

void RdsRxView::on_data_rds(const RDSGroupMessage& msg) {
    uint16_t block_a = msg.block_a;
    uint16_t block_b = msg.block_b;
    uint16_t block_c = msg.block_c;
    uint16_t block_d = msg.block_d;

    int valid_blocks = 4;

    if (msg.is_debug) {
        if (msg.debug_1 == 777)
            valid_blocks = 2;  // A+B
        else if (msg.debug_1 == 888)
            valid_blocks = 3;  // A+B+C
        else
            return;  // debug (bits counted)
    }

    // Blokk A: Program Identifier
    text_pi.set("PI: " + to_string_hex(block_a, 4));

    uint8_t group_type = 0;
    uint8_t group_version = 0;

    // Blokk B: Metadata
    if (valid_blocks >= 2) {
        group_type = (block_b >> 12) & 0x0F;
        group_version = (block_b >> 11) & 0x01;  // 0 = A, 1 = B
        uint8_t tp = (block_b >> 10) & 0x01;
        uint8_t pty = (block_b >> 5) & 0x1F;

        text_tp.set(tp ? "TP: Yes" : "TP: No ");
        text_pty.set("PTY: " + to_string_dec_uint(pty) + " (" + pty_names[pty] + ")");
    }

    // (A, B, C, D) Full payload  ---
    if (valid_blocks == 4) {
        // PS (name) - 0A  0B
        if (group_type == 0) {
            uint8_t segment = block_b & 0x03;
            char c1 = (block_d >> 8) & 0xFF;
            char c2 = block_d & 0xFF;

            if (c1 >= 32 && c1 <= 126) ps_name[segment * 2] = c1;
            if (c2 >= 32 && c2 <= 126) ps_name[segment * 2 + 1] = c2;
            text_ps_name.set(ps_name);
        }
        // RT (txt) - 2A  2B
        else if (group_type == 2) {
            uint8_t segment = block_b & 0x0F;

            if (group_version == 0) {  // 2A group
                if (!msg.is_c_prime) {
                    char c1 = (block_c >> 8) & 0xFF;
                    char c2 = block_c & 0xFF;
                    char c3 = (block_d >> 8) & 0xFF;
                    char c4 = block_d & 0xFF;

                    if (c1 >= 32 && c1 <= 126) radio_text[segment * 4] = c1;
                    if (c2 >= 32 && c2 <= 126) radio_text[segment * 4 + 1] = c2;
                    if (c3 >= 32 && c3 <= 126) radio_text[segment * 4 + 2] = c3;
                    if (c4 >= 32 && c4 <= 126) radio_text[segment * 4 + 3] = c4;
                }
            } else {  // 2B group
                char c1 = (block_d >> 8) & 0xFF;
                char c2 = block_d & 0xFF;

                if (c1 >= 32 && c1 <= 126) radio_text[segment * 2] = c1;
                if (c2 >= 32 && c2 <= 126) radio_text[segment * 2 + 1] = c2;
            }

            // RRadio text split into 3 lines on the display
            std::string rt_str(radio_text, 64);
            text_rt_1.set(rt_str.substr(0, 30));
            text_rt_2.set(rt_str.substr(30, 30));
            text_rt_3.set(rt_str.substr(60, 4));
        }
    }

    // --- 4. Diagnostic Console Output (Always runs if at least A+B are valid) ---
    // Shows which blocks were corrupted (-), and what the group type was
    std::string diag = "[" + to_string_hex(block_a, 4) + " ";
    diag += to_string_hex(block_b, 4) + " ";
    diag += (valid_blocks >= 3) ? to_string_hex(block_c, 4) + " " : "---- ";
    diag += (valid_blocks == 4) ? to_string_hex(block_d, 4) : "----";
    diag += "] G:" + to_string_dec_uint(group_type) + (group_version ? "B" : "A");

    console.writeln(diag);
}

RdsRxView::~RdsRxView() {
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::rds_rx