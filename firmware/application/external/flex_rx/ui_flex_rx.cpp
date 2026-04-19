#include "ui_flex_rx.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "memory_map.hpp"
#include "usb_serial_asyncmsg.hpp"

using namespace portapack;

namespace ui::external_app::flex_rx {

static const int flex_tz_table[] = {0, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 720,
                                    210, 270, 330, 0, 345, 390, 570, -210, -660, -600, -540, -480, -420, -360, -300, -240, -180, -120, -60};

FlexAppView::FlexAppView(NavigationView& nav)
    : nav_{nav} {
    // Load baseband image for FLEX decoding
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &text_status1,
                  &text_status2,
                  &console});

    // Restore saved frequency
    field_frequency.set_value(frequency_value);
    receiver_model.set_target_frequency(frequency_value);

    // Frequency change callback
    field_frequency.updated = [this](rf::Frequency f) {
        update_freq(f);
    };

    // Configure receiver
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();
    receiver_model.set_squelch_level(0);

    // Initialize FLEX baseband
    baseband::set_flex_config();

    text_status1.set("No signal");
    console.writeln("Ready");
}

FlexAppView::~FlexAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void FlexAppView::focus() {
    field_frequency.focus();
}

// Add message to console
void FlexAppView::log_message(const std::string& message) {
    console.writeln(message);
}

// Update frequency and save for persistence
void FlexAppView::update_freq(rf::Frequency f) {
    frequency_value = f;
    receiver_model.set_target_frequency(f);
}

// Type tag from numeric type code
static const char* flex_type_tag(uint32_t type) {
    switch (type) {
        case 0:
            return "SEC";
        case 1:
            return "INS";
        case 2:
            return "TON";
        case 3:
            return "NUM";
        case 4:
            return "SNUM";
        case 5:
            return "ALN";
        case 6:
            return "HEX";
        case 7:
            return "NNUM";
        case 8:
            return "SHORT";
        case 9:
            return "BIW";
        default:
            return "UNK";
    }
}

// Handle decoded FLEX packet from baseband
void FlexAppView::on_packet(const FlexPacketMessage* message) {
    const auto& pkt = message->packet;
    const char* type = flex_type_tag(pkt.type);
    const char* pol = pkt.is_inverted ? "I" : "N";

    // Update status row 1: C/F speed polarity time timezone
    {
        std::string s1 = "C" + to_string_dec_uint(pkt.cycle) +
                         "/F" + to_string_dec_uint(pkt.frame) +
                         " " + to_string_dec_uint(pkt.bitrate) +
                         " " + pol;
        if (status_time_[0]) {
            s1 += "  ";
            s1 += status_time_;
        }
        if (status_tz_[0]) {
            s1 += " ";
            s1 += status_tz_;
        }
        text_status1.set(s1);
    }

    // Update status row 2 from BIW data
    if (pkt.type == 9) {
        switch (pkt.biw_field) {
            case 0:  // SSID1
                status_lid_ = pkt.biw_v1;
                status_cz_ = pkt.biw_v2;
                break;
            case 2: {  // Time
                uint32_t si = (pkt.biw_v3 * 75) / 10;
                auto h = to_string_dec_uint(pkt.biw_v1, 2, '0');
                auto m = to_string_dec_uint(pkt.biw_v2, 2, '0');
                auto sec = to_string_dec_uint(si, 2, '0');
                // Store for row 1
                auto ts = h + ":" + m + ":" + sec;
                memcpy(status_time_, ts.c_str(), ts.size() + 1);
                break;
            }
            case 5: {  // SysInfo (timezone)
                if (pkt.biw_v1 == 4 || pkt.biw_v1 == 5) {
                    uint16_t zone = pkt.biw_v2 & 0x1F;
                    int ofs = (zone < 32) ? flex_tz_table[zone] : 0;
                    int hrs = ofs / 60;
                    int mins = (ofs < 0 ? -ofs : ofs) % 60;
                    auto tzs = std::string("UTC") + (ofs >= 0 ? "+" : "") +
                               to_string_dec_int(hrs);
                    if (mins != 0)
                        tzs += ":" + to_string_dec_int(mins, 2, '0');
                    memcpy(status_tz_, tzs.c_str(), tzs.size() + 1);
                }
                break;
            }
            case 7:  // SSID2
                status_cc_ = pkt.biw_v1;
                break;
        }
        // Rebuild row 2
        std::string s2;
        if (status_lid_) s2 += "LID:" + to_string_dec_uint(status_lid_);
        if (status_cz_) {
            s2 += " CZ:";
            s2 += to_string_dec_uint(status_cz_);
        }
        if (status_cc_) {
            s2 += " CC:";
            s2 += to_string_dec_uint(status_cc_);
        }
        if (pkt.fiw_roaming) s2 += " R";
        text_status2.set(s2);
    }

    // Console: skip BIW, tone-only (V=010), SHORT reserved (t=3)
    bool skip_gui = (pkt.type == 9 || pkt.type == 2);
    if (pkt.type == 8 && pkt.function == 3) skip_gui = true;
    if (!skip_gui) {
        auto cf = to_string_dec_uint(pkt.cycle) + "/" + to_string_dec_uint(pkt.frame);
        std::string line = cf + " " + to_string_dec_uint(pkt.bitrate) +
                           " " + pol + " " + std::string(1, pkt.phase) + " ";

        if (pkt.type == 1 && pkt.message[0] == 'i' && pkt.message[2] == 't') {
            // INS temp group: "1234567 +GRP5@F42"
            line += to_string_dec_uint(pkt.capcode);
            line += " +TG";
            line += to_string_dec_uint(pkt.biw_v1);
            line += "@F";
            line += to_string_dec_uint(pkt.biw_v2);
        } else if (pkt.type == 1) {
            // Other INS types
            line += to_string_dec_uint(pkt.capcode);
            line += " INS ";
            line += pkt.message;
        } else if (pkt.addr_type == 2) {
            // Temp group delivery: "GRP5 ALN message"
            uint32_t slot = (uint32_t)(pkt.capcode + 0x8000 - 0x1F7800) & 0x0F;
            line += "TG";
            line += to_string_dec_uint(slot);
            line += " ";
            line += type;
            if (pkt.message[0]) {
                line += " ";
                line += pkt.message;
            }
        } else {
            line += to_string_dec_uint(pkt.capcode);
            if (pkt.is_priority) line += " P";
            line += " ";
            line += type;
            if (pkt.message[0]) {
                line += " ";
                line += pkt.message;
            }
        }
        log_message(line);
    }

    // Serial: pipe-delimited
    if (portapack::usb_serial.serial_connected()) {
        std::string s;
        s = "FLEX|";
        s += to_string_dec_uint(pkt.cycle);
        s += '/';
        s += to_string_dec_uint(pkt.frame);
        s += '|';
        s += to_string_dec_uint(pkt.bitrate);
        s += '|';
        s += pol;
        s += '|';
        s += pkt.phase;

        if (pkt.type == 9) {
            s += "|BIW|w=";
            s += to_string_dec_uint(pkt.function);
            s += "|t=";
            s += to_string_dec_uint(pkt.biw_field);
            s += '|';
            s += to_string_dec_uint(pkt.biw_v1);
            s += '|';
            s += to_string_dec_uint(pkt.biw_v2);
            s += '|';
            s += to_string_dec_uint(pkt.biw_v3);
        } else {
            s += '|';
            s += type;
            s += "|cap=";
            s += to_string_dec_uint(pkt.capcode);
            if (pkt.is_priority) s += "|pri=1";
            if (pkt.addr_type == 2) {
                // Temporary address: show slot number
                // capcode = aw - 0x8000, aw = capcode + 0x8000
                // slot = (aw - 0x1F7800) & 0x0F = (capcode + 0x8000 - 0x1F7800) & 0x0F
                uint32_t slot = (uint32_t)(pkt.capcode + 0x8000 - 0x1F7800) & 0x0F;
                s += "|slot=";
                s += to_string_dec_uint(slot);
            }
            if (pkt.has_flags) {
                if (pkt.type == 7) {
                    s += "|seq=";
                    s += to_string_dec_uint(pkt.seq);
                    s += "|new=";
                    s += to_string_dec_uint(pkt.is_new);
                    s += "|fmt=";
                    s += pkt.nnum_s ? "idrom" : "std";
                } else {
                    s += "|frag=";
                    s += (pkt.frag == 3) ? "first" : to_string_dec_uint(pkt.frag).c_str();
                    s += "|mf=";
                    s += to_string_dec_uint(pkt.more_frag);
                    s += "|seq=";
                    s += to_string_dec_uint(pkt.seq);
                    if (pkt.frag == 3) {
                        s += "|new=";
                        s += to_string_dec_uint(pkt.is_new);
                        s += "|md=";
                        s += to_string_dec_uint(pkt.maildrop);
                        s += "|sig=";
                        s += to_string_hex(pkt.sig, 2);
                    }
                    if (pkt.type == 0) {
                        static const char* enc[] = {"alpha", "sep", "bin", "rsvd"};
                        s += "|enc=";
                        s += enc[pkt.sec_enc & 3];
                    }
                    if (pkt.type == 6 && pkt.frag == 3) {
                        uint8_t b = pkt.function & 0x0F;
                        s += "|bb=";
                        s += to_string_dec_uint(b == 0 ? 16 : b);
                        if (pkt.function & 0x10) s += "|rtl=1";
                    }
                }
            }
            if (pkt.message[0]) {
                s += "|\"";
                s += pkt.message;
                s += '"';
            }
        }
        UsbSerialAsyncmsg::asyncmsg(s);
    }
}

// Handle stats message (currently unused)
void FlexAppView::on_stats(const FlexStatsMessage*) {
}

// Debug handler - uncomment to see baseband debug messages
void FlexAppView::on_debug(const FlexDebugMessage* message) {
    if (portapack::usb_serial.serial_connected()) {
        std::string s = "DBG|";
        s += message->text;
        s += "|";
        s += to_string_dec_int(message->val1);
        s += "|";
        s += to_string_dec_int(message->val2);
        UsbSerialAsyncmsg::asyncmsg(s);
    }
}

}  // namespace ui::external_app::flex_rx