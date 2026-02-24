/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 *
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

#ifndef __TPMS_APP_H__
#define __TPMS_APP_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "event_m0.hpp"

#include "log_file.hpp"

#include "recent_entries.hpp"

#include "tpms_packet.hpp"

namespace ui::external_app::tpmsrx {

namespace format {

static uint8_t pressure_unit{PRESSURE_UNIT_KPA};
static uint8_t temp_unit{TEMP_UNIT_CELSIUS};
}  // namespace format

struct TPMSRecentEntry {
    using Key = std::pair<tpms::Reading::Type, tpms::TransponderID>;

    static const Key invalid_key;

    tpms::Reading::Type type{invalid_key.first};
    tpms::TransponderID id{invalid_key.second};
    tpms::SignalType signal_type{tpms::SignalType::OOK_8k192_Schrader};

    size_t received_count{0};

    Optional<Pressure> last_pressure{};
    Optional<Temperature> last_temperature{};
    Optional<tpms::Flags> last_flags{};

    TPMSRecentEntry(
        const Key& key)
        : type{key.first},
          id{key.second} {
    }

    Key key() const {
        return {type, id};
    }

    void update(const tpms::Reading& reading);
};

using TPMSRecentEntries = RecentEntries<TPMSRecentEntry>;

class TPMSLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(const tpms::Packet& packet, const uint32_t target_frequency);

   private:
    LogFile log_file{};
};

using TPMSRecentEntriesView = RecentEntriesView<TPMSRecentEntries>;

class TPMSRecentEntryDetailView : public View {
   public:
    TPMSRecentEntryDetailView(NavigationView& nav, const TPMSRecentEntry& entry);

    void set_entry(const TPMSRecentEntry& entry);
    const TPMSRecentEntry& entry() const { return entry_; }

    void focus() override;

   private:
    NavigationView& nav_;
    TPMSRecentEntry entry_;

    void on_save();
    bool save_file(const std::filesystem::path& path);

    Labels labels{
        {{0 * 8, 1 * 16}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "ID:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Pressure:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 7 * 16}, "Temperature:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 9 * 16}, "Flags:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 11 * 16}, "Count:", Theme::getInstance()->fg_light->foreground},
    };

    Text text_type{
        {8 * 8, 1 * 16, 20 * 8, 16},
        ""};

    Text text_id{
        {8 * 8, 3 * 16, 20 * 8, 16},
        ""};

    Text text_pressure{
        {12 * 8, 5 * 16, 16 * 8, 16},
        ""};

    Text text_temperature{
        {14 * 8, 7 * 16, 14 * 8, 16},
        ""};

    Text text_flags{
        {8 * 8, 9 * 16, 20 * 8, 16},
        ""};

    Text text_count{
        {8 * 8, 11 * 16, 20 * 8, 16},
        ""};

    Button button_save{
        {0 * 8, 13 * 16, 14 * 8, 32},
        "Save"};

    Button button_done{
        {16 * 8, 13 * 16, 14 * 8, 32},
        "Done"};
};

class TPMSAppView : public View {
   public:
    TPMSAppView(NavigationView& nav);
    ~TPMSAppView();

    void set_parent_rect(const Rect new_parent_rect) override;

    // Prevent painting of region covered entirely by a child.
    // TODO: Add flag to View that specifies view does not need to be cleared before painting.
    void paint(Painter&) override {};

    void focus() override;

    std::string title() const override { return "TPMS RX"; };

   private:
    NavigationView& nav_;

    RxRadioState radio_state_{
        314900000 /* frequency*/
        ,
        1750000 /* bandwidth */,
        2457600 /* sampling rate */
    };

    app_settings::SettingsManager settings_{
        "rx_tpms",
        app_settings::Mode::RX,
        {
            {"pressure_unit"sv, &format::pressure_unit},
            {"temp_unit"sv, &format::temp_unit},
        }};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::TPMSPacket,
        [this](Message* const p) {
            const auto message = static_cast<const TPMSPacketMessage*>(p);
            const tpms::Packet packet{message->packet, message->signal_type};
            this->on_packet(packet);
        }};

    static constexpr ui::Dim header_height = 1 * 16;

    ui::Rect view_normal_rect{};

    RSSI rssi{
        {UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4},
    };
    Channel channel{
        {UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4},
    };
    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    // "315 MHz" TPMS sensors transmit at either 314.9 or 315 MHz but we should pick up either
    OptionsField options_band{
        {UI_POS_X(0), UI_POS_Y(0)},
        5,
        {
            {"314.9", 314900000},
            {"315.0", 315000000},
            {"433.9", 433920000},
        }};

    OptionsField options_pressure{
        {6 * 8, UI_POS_Y(0)},
        4,
        {{"kPa", PRESSURE_UNIT_KPA},
         {"PSI", PRESSURE_UNIT_PSI},
         {"BAR", PRESSURE_UNIT_BAR}}};

    OptionsField options_temperature{
        {10 * 8, UI_POS_Y(0)},
        2,
        {{STR_DEGREES_C, TEMP_UNIT_CELSIUS},
         {STR_DEGREES_F, TEMP_UNIT_FAHRENHEIT}}};

    RFAmpField field_rf_amp{
        {13 * 8, UI_POS_Y(0)}};

    LNAGainField field_lna{
        {15 * 8, UI_POS_Y(0)}};

    VGAGainField field_vga{
        {18 * 8, UI_POS_Y(0)}};

    TPMSRecentEntries recent{};
    std::unique_ptr<TPMSLogger> logger{};

    RecentEntriesColumns columns{{
        {"Tp", 2},
        {"ID", 0},
        {"Pres", 4},
        {"Temp", 4},
        {"Cnt", 3},
        {"Fl", 2},
    }};
    TPMSRecentEntriesView recent_entries_view{columns, recent};

    void on_packet(const tpms::Packet& packet);
    void on_show_list();
    void on_show_detail(const TPMSRecentEntry& entry);
    void update_view();
};

}  // namespace ui::external_app::tpmsrx

#endif /*__TPMS_APP_H__*/
