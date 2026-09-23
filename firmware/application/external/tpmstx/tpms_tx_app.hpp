/*
 * Copyright (C) 2026
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

#ifndef __TPMS_TX_APP_H__
#define __TPMS_TX_APP_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"
#include "message.hpp"
#include "tpms_packet.hpp"
#include "file_path.hpp"
#include "string_format.hpp"
#include "units.hpp"

namespace ui::external_app::tpmstx {

namespace format {
static uint8_t pressure_unit{PRESSURE_UNIT_PSI};
static uint8_t temp_unit{TEMP_UNIT_CELSIUS};
}  // namespace format

class TPMSTXView : public View {
   public:
    TPMSTXView(NavigationView& nav);
    ~TPMSTXView();

    void focus() override;

    std::string title() const override { return "TPMS TX"; };

   private:
    NavigationView& nav_;

    TxRadioState radio_state_{
        314900000, /* frequency */
        1750000,   /* bandwidth */
        2457600    /* sampling rate */
    };

    app_settings::SettingsManager settings_{
        "tx_tpms",
        app_settings::Mode::TX,
        {
            {"pressure_unit"sv, &format::pressure_unit},
            {"temp_unit"sv, &format::temp_unit},
        }};

    // TPMS packet data
    tpms::Reading::Type packet_type_{tpms::Reading::Type::Schrader};
    uint32_t transponder_id_{0x12345678};
    uint16_t pressure_kpa_{240};  // Default ~35 PSI
    int16_t temperature_c_{25};   // Default 25°C
    uint8_t flags_{0x00};         // 3-bit function code (0-7), checksum is auto-calculated
    tpms::SignalType signal_type_{tpms::SignalType::FSK_19k2_Schrader};

    uint8_t repeat_count_{5};
    uint32_t pause_duration_{50};  // ms between repeats

    bool is_transmitting_{false};

    // FSK-specific repeat tracking (OOK repeats are handled by baseband)
    uint8_t fsk_repeat_counter_{0};
    uint32_t fsk_repeat_timer_id_{0};

    void update_signal_type_from_packet();
    void switch_baseband();

    void start_tx();
    void stop_tx();
    void send_packet();
    void encode_and_transmit();
    void update_packet_display();
    void on_pressure_unit_change();
    void on_temperature_unit_change();
    void update_field_visibility();

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            progressbar.set_value(message.progress);
            if (message.done) {
                handle_tx_complete();
            }
        }};

    void handle_tx_complete();

    Labels labels{
        {{0 * 8, 0 * 16}, "Freq:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 1 * 16}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "ID:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "Pres:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Rpt:", Theme::getInstance()->fg_light->foreground},
    };

    Text label_temperature{
        {0 * 8, 4 * 16, 5 * 8, 16},
        "Temp:"};

    Text label_flags{
        {0 * 8, 4 * 16, 5 * 8, 16},
        "Func:"};

    OptionsField options_frequency{
        {6 * 8, 0 * 16},
        5,
        {
            {"314.9", 314900000},
            {"315.0", 315000000},
            {"433.9", 433920000},
        }};

    OptionsField options_packet_type{
        {6 * 8, 1 * 16},
        10,
        {
            {"Schrader", (int32_t)tpms::Reading::Type::Schrader},
            {"FLM_64", (int32_t)tpms::Reading::Type::FLM_64},
            {"FLM_72", (int32_t)tpms::Reading::Type::FLM_72},
            {"FLM_80", (int32_t)tpms::Reading::Type::FLM_80},
            {"GMC_96", (int32_t)tpms::Reading::Type::GMC_96},
        }};

    OptionsField options_pressure{
        {11 * 8, 3 * 16},
        4,
        {{"kPa", PRESSURE_UNIT_KPA},
         {"PSI", PRESSURE_UNIT_PSI},
         {"BAR", PRESSURE_UNIT_BAR}}};

    OptionsField options_temperature{
        {11 * 8, 4 * 16},
        2,
        {{STR_DEGREES_C, TEMP_UNIT_CELSIUS},
         {STR_DEGREES_F, TEMP_UNIT_FAHRENHEIT}}};

    SymField field_transponder_id_24{
        {6 * 8, 2 * 16},
        6,
        SymField::Type::Hex};

    SymField field_transponder_id_32{
        {6 * 8, 2 * 16},
        8,
        SymField::Type::Hex};

    NumberField field_pressure{
        {6 * 8, 3 * 16},
        4,
        {0, 9999},
        1,
        ' '};

    NumberField field_temperature{
        {6 * 8, 4 * 16},
        4,
        {-99, 999},
        1,
        ' '};

    NumberField field_flags{
        {6 * 8, 4 * 16},
        1,
        {0, 7},
        1,
        ' '};

    NumberField field_repeat{
        {6 * 8, 5 * 16},
        3,
        {1, 100},
        1,
        ' '};

    Button button_load{
        {0 * 8, 8 * 16 + 4, 7 * 8, 24},
        "Load"};

    Button button_save{
        {8 * 8, 8 * 16 + 4, 7 * 8, 24},
        "Save"};

    TransmitterView2 tx_view{
        {16 * 8, 0 * 16},
        true  // Short UI format
    };

    Button button_transmit{
        {0 * 8, 11 * 16, 15 * 8, 32},
        "START TX"};

    Text text_status{
        {0 * 8, 13 * 16 + 4, 30 * 8, 16},
        "Ready"};

    ProgressBar progressbar{
        {0 * 8, 14 * 16 + 8, 30 * 8, 16}};
};

}  // namespace ui::external_app::tpmstx

#endif /*__TPMS_TX_APP_H__*/
