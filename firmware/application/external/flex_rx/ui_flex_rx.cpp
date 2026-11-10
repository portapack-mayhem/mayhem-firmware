#include "ui_flex_rx.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "memory_map.hpp"
#include "usb_serial_asyncmsg.hpp"
#include "file_path.hpp"

using namespace portapack;

namespace ui::external_app::flex_rx {

static const int flex_tz_table[] = {0, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 720,
                                    210, 270, 330, 0, 345, 390, 570, -210, -660, -600, -540, -480, -420, -360, -300, -240, -180, -120, -60};

FlexSettingsView::FlexSettingsView(
    NavigationView& nav,
    FlexSettings& settings)
    : settings_{settings} {
    add_children(
        {&labels,
         &check_log,
         &check_log_raw,
         &check_small_font,
         &check_hide_tone,
         &opt_filter_mode,
         &field_filter_capcode,
         &button_save});

    check_log.set_value(settings_.enable_log);
    check_log_raw.set_value(settings_.log_raw);
    check_small_font.set_value(settings_.small_font);
    check_hide_tone.set_value(settings_.hide_tone_only);
    opt_filter_mode.set_by_value(settings_.filter_mode);
    field_filter_capcode.set_value(settings_.filter_capcode);

    button_save.on_select = [this, &nav](Button&) {
        settings_.enable_log = check_log.value();
        settings_.log_raw = check_log_raw.value();
        settings_.small_font = check_small_font.value();
        settings_.hide_tone_only = check_hide_tone.value();
        settings_.filter_mode = opt_filter_mode.selected_index_value();
        settings_.filter_capcode = field_filter_capcode.to_integer();
        nav.pop();
    };
}

FlexAppView::FlexAppView(NavigationView& nav)
    : nav_{nav} {
    // Load baseband image for FLEX decoding
    baseband::run_image(portapack::spi_flash::image_tag_flex);

    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &text_status1,
                  &button_config,
                  &text_time,
                  &text_config,
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
    button_config.on_select = [this](Button&) {
        nav_.push<FlexSettingsView>(flex_settings_);
        nav_.set_on_pop([this]() { refresh_ui(); });
    };

    log_file_.append(logs_dir / u"FLEX.TXT");
    refresh_ui();

    console.writeln("Ready");
}

FlexAppView::~FlexAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void FlexAppView::focus() {
    field_frequency.focus();
}

// Apply the Config screen's choices: console font and the summary on the bottom row.
void FlexAppView::refresh_ui() {
    console.set_style(flex_settings_.small_font ? Theme::getInstance()->bg_darkest_small
                                                : Theme::getInstance()->bg_darkest);

    // Fits the 8 columns next to the time: "RAW KEEP" at most.
    std::string s;
    if (flex_settings_.enable_log)
        s = flex_settings_.log_raw ? "RAW" : "LOG";
    if (flex_settings_.filter_mode != FLEX_FILTER_NONE) {
        if (!s.empty()) s += ' ';
        s += flex_settings_.filter_mode == FLEX_FILTER_KEEP ? "KEEP" : "DROP";
    }
    text_config.set(s);
}

// Capcode filter from the Config screen; applies to pages, not to system info (BIW).
bool FlexAppView::passes_filter(const flex::FlexPacket& pkt) const {
    switch (flex_settings_.filter_mode) {
        case FLEX_FILTER_KEEP:
            return pkt.capcode == flex_settings_.filter_capcode;
        case FLEX_FILTER_DROP:
            return pkt.capcode != flex_settings_.filter_capcode;
        default:
            return true;
    }
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

    // Status row 1: C/F speed polarity
    text_status1.set("C" + to_string_dec_uint(pkt.cycle) +
                     "/F" + to_string_dec_uint(pkt.frame) +
                     " " + to_string_dec_uint(pkt.bitrate) +
                     " " + pol);

    // Update status row 2 from BIW data
    if (pkt.type == 9) {
        switch (pkt.biw_field) {
            case 0:  // SSID1
                status_lid_ = pkt.biw_v1;
                status_cz_ = pkt.biw_v2;
                break;
            case 2: {  // Time
                // BIW TIME encodes the time at Frame 0 of the cycle (standard
                // mode, ARIB STD-43A §3.7.2).  Add the frame offset and the
                // extended-seconds trim (from SysInfo) to recover real time.
                //   coarse second: v3 * 7.5s   = v3 * 15 half-seconds
                //   frame offset:  frame * 1.875s = frame * 15/4 half-seconds
                //   ext_sec:       N * 0.9375s  = N * 15/8 half-seconds
                uint32_t half = pkt.biw_v3 * 15 +
                                (uint32_t)pkt.frame * 15 / 4 +
                                (uint32_t)status_extsec_ * 15 / 8;
                uint32_t total_sec = pkt.biw_v1 * 3600u + pkt.biw_v2 * 60u + half / 2u;
                total_sec %= 86400u;
                auto h = to_string_dec_uint(total_sec / 3600u, 2, '0');
                auto m = to_string_dec_uint((total_sec / 60u) % 60u, 2, '0');
                auto sec = to_string_dec_uint(total_sec % 60u, 2, '0');
                auto ts = h + ":" + m + ":" + sec;
                memcpy(status_time_, ts.c_str(), ts.size() + 1);
                break;
            }
            case 5: {  // SysInfo (timezone + ext_sec)
                if (pkt.biw_v1 == 4 || pkt.biw_v1 == 5) {
                    uint16_t zone = pkt.biw_v2 & 0x1F;
                    int ofs = (zone < 32) ? flex_tz_table[zone] : 0;
                    int hrs = ofs / 60;
                    int mins = (ofs < 0 ? -ofs : ofs) % 60;
                    auto tzs = std::string("UTC") + (ofs >= 0 ? "+" : "") +
                               to_string_dec_int(hrs);
                    if (mins != 0)
                        tzs += ":" + to_string_dec_int(mins, 2, '0');
                    // I7-I9: extended seconds (0.9375s steps per step)
                    status_extsec_ = (pkt.biw_v2 >> 7) & 0x07;
                    if (status_extsec_)
                        tzs += " es=" + to_string_dec_uint(status_extsec_);
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

        // Time row: network time and zone from the BIW
        std::string st = status_time_;
        if (status_tz_[0]) {
            if (!st.empty()) st += ' ';
            st += status_tz_;
        }
        text_time.set(st);
    }

    // Console: skip BIW, tone-only (V=010) unless asked for, SHORT reserved (t=3),
    // and pages the capcode filter leaves out.
    bool skip_gui = (pkt.type == 9) || (pkt.type == 2 && flex_settings_.hide_tone_only);
    if (pkt.type == 8 && pkt.function == 3) skip_gui = true;
    if (pkt.type != 9 && !passes_filter(pkt)) skip_gui = true;
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
        if (flex_settings_.enable_log)
            log_file_.write_entry(line);
    }

    // Serial, and the raw log: pipe-delimited, every field. Like POCSAG's raw log, it
    // records every packet; the capcode filter only applies to the pages shown above.
    const bool to_serial = portapack::usb_serial.serial_connected();
    const bool to_raw_log = flex_settings_.enable_log && flex_settings_.log_raw;
    if (to_serial || to_raw_log) {
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
        if (to_serial)
            UsbSerialAsyncmsg::asyncmsg(s);
        if (to_raw_log)
            log_file_.write_entry(s);
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