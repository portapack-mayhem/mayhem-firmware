/*
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __UI_VDL2_RX_H__
#define __UI_VDL2_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "recent_entries.hpp"
#include "log_file.hpp"
#include "message.hpp"

#include "vdl2_frame.hpp"

namespace ui::external_app::vdl2_rx {

// One aircraft by its 24-bit ICAO address, or a ground station (GROUND_FLAG set), with what
// it and the ground said.
struct VDL2Entry {
    using Key = uint32_t;
    static constexpr Key invalid_key = 0xFFFFFFFF;
    static constexpr Key GROUND_FLAG = 0x1000000;
    // Kept small for the M0 heap (about 15 KiB free on a HackRF One).
    static constexpr size_t MAX_ENTRIES = 16;

    Key id;
    std::string reg{};
    std::string flight{};
    std::string model{};  // from the ADS-B aircraft database
    std::string operator_{};
    std::string label{};
    uint32_t frames{0};
    uint32_t link{0};  // link management (XID, RR, ...)
    uint32_t data{0};  // ATN / CPDLC
    uint32_t acars{0};
    rtc::RTC last_seen{};
    // OOOI: gate out, wheels off, wheels on, gate in; when each report came in
    rtc::RTC oooi[4]{};
    bool has_oooi[4]{};

    VDL2Entry()
        : VDL2Entry{0} {}
    VDL2Entry(const Key key)
        : id{key} {}

    Key key() const { return id; }
    bool ground() const { return id & GROUND_FLAG; }
    uint32_t address() const { return id & 0xFFFFFF; }
    void update(const Frame& frame);
};

using VDL2Entries = RecentEntries<VDL2Entry>;
using VDL2EntriesView = RecentEntriesView<VDL2Entries>;

// ACARS messages with text, of all aircraft, newest first. One store for the app, bounded in
// count and in bytes for the M0 heap; the oldest go first. The log keeps everything.
struct VDL2Message {
    VDL2Entry::Key key;
    rtc::RTC time;
    std::string head;  // "> H1" from the aircraft, "< H1" to it
    std::string text;
};

using VDL2Messages = std::list<VDL2Message>;
constexpr size_t MAX_MESSAGES = 32;
constexpr size_t MAX_MESSAGE_BYTES = 4096;

// All messages of one aircraft in full, wrapped, to scroll through.
class VDL2MessagesView : public View {
   public:
    VDL2MessagesView(NavigationView& nav, const VDL2Messages& messages, const VDL2Entry::Key key, const std::string& title, const Style* style);

    std::string title() const override { return title_; };
    void focus() override;
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;

   private:
    NavigationView& nav_;
    const VDL2Messages& messages_;
    const VDL2Entry::Key key_;
    const std::string title_;
    int32_t scroll_{0};  // first line shown

    // Calls line(n, text) for every display line, n counting from 0; returns how many.
    template <typename F>
    size_t for_each_line(const size_t columns, F&& line) const;
    void scroll(const int32_t delta);
};

// Options in the Config screen, kept in the app settings.
struct VDL2Settings {
    bool enable_log = false;  // ACARS messages to LOGS/VDL2.TXT
    bool small_font = false;  // list and detail view in the 5x8 font
};

class VDL2SettingsView : public View {
   public:
    VDL2SettingsView(NavigationView& nav, VDL2Settings& settings);

    std::string title() const override { return "VDL2 Config"; };
    void focus() override { button_save.focus(); }

   private:
    VDL2Settings& settings_;

    Checkbox check_log{
        {UI_POS_X(2), UI_POS_Y(1)},
        10,
        "Enable Log"};
    Checkbox check_small_font{
        {UI_POS_X(2), UI_POS_Y(3)},
        14,
        "Use Small Font"};
    Button button_save{
        {UI_POS_X_CENTER(10), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)},
        "Save"};
};

class VDL2DetailView : public View {
   public:
    std::function<void(void)> on_close{};
    std::function<void(void)> on_messages{};

    VDL2DetailView(const VDL2Messages& messages);

    void set_entry(const VDL2Entry& entry);
    const VDL2Entry& entry() const { return entry_; }

    void focus() override;
    void paint(Painter& painter) override;

   private:
    const VDL2Messages& messages_;
    VDL2Entry entry_{};

    // All messages of this aircraft, and back to the list.
    Button button_messages{
        {UI_POS_X_CENTER(26), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)},
        "Messages"};
    Button button_done{
        {UI_POS_X_CENTER(26) + UI_POS_WIDTH(14), UI_POS_Y_BOTTOM(5), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)},
        "Back"};
};

class VDL2RxView : public View {
   public:
    VDL2RxView(NavigationView& nav);
    ~VDL2RxView();

    void set_parent_rect(const Rect new_parent_rect) override;
    void focus() override;

    std::string title() const override { return "VDL2 RX"; };

   private:
    static constexpr Dim header_height = 2 * 16;

    NavigationView& nav_;
    RxRadioState radio_state_{136975000 /* frequency */, 1750000 /* bandwidth */, 3072000 /* sampling rate */};
    VDL2Settings vdl2_settings_{};
    app_settings::SettingsManager settings_{
        "rx_vdl2",
        app_settings::Mode::RX,
        {
            {"log"sv, &vdl2_settings_.enable_log},
            {"small_font"sv, &vdl2_settings_.small_font},
        }};

    LogFile log_file_{};
    VDL2Entries recent_{};
    uint32_t frames_{0};

    // The VDL2 channels; 136.975 is the common signalling channel, the others are used in
    // Europe and North America.
    OptionsField options_channel{
        {UI_POS_X(0), UI_POS_Y(0)},
        7,
        {
            {"136.975", 136975000},
            {"136.650", 136650000},
            {"136.700", 136700000},
            {"136.725", 136725000},
            {"136.775", 136775000},
            {"136.800", 136800000},
            {"136.825", 136825000},
            {"136.875", 136875000},
            {"136.900", 136900000},
        }};
    Text label_mhz{
        {UI_POS_X(8), UI_POS_Y(0), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "MHz"};
    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(21), 4}};
    Channel channel{
        {UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(21), 4}};

    // Row 1: frames received and what Config has switched on, and the Config button (as in FLEX RX).
    Text text_frames{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)},
        ""};
    Button button_config{
        {UI_POS_X_RIGHT(8), UI_POS_Y(1), UI_POS_WIDTH(8), UI_POS_HEIGHT(1)},
        "Config"};

    RecentEntriesColumns columns{{
        {"ICAO", 6},
        {"Reg", 7},
        {"Flight", 7},
        {"#", 0},
    }};
    VDL2EntriesView recent_view{columns, recent_};
    VDL2Messages messages_{};
    size_t message_bytes_{0};
    VDL2DetailView detail_view{messages_};

    void on_frame(const VDL2FrameMessage& message);
    void refresh_ui();
    void update_entry(const VDL2Entry::Key key, const Frame& frame);
    void add_message(const VDL2Entry::Key key, const Frame& frame);
    void show_list();
    void show_detail(const VDL2Entry& entry);

    MessageHandlerRegistration message_handler_frame{
        Message::ID::VDL2Frame,
        [this](Message* const p) {
            this->on_frame(*static_cast<const VDL2FrameMessage*>(p));
        }};
};

}  // namespace ui::external_app::vdl2_rx

#endif /*__UI_VDL2_RX_H__*/
