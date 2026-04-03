/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifndef __EPIRB_TX_H__
#define __EPIRB_TX_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "portapack.hpp"
#include "message.hpp"
#include "tonesets.hpp"

#define BEACON_HEXA_SIZE 36
#define BEACON_HEXA_HALF_SIZE 18
#define BEACON_SIZE 18

#define AM_TEST_FREQUENCY 121375000
#define AM_REAL_FREQUENCY 121500000

#define BPSK_FREQUENCY_HAM 433025000
#define BPSK_FREQUENCY_B 406025000
#define BPSK_FREQUENCY_C 406028000
#define BPSK_FREQUENCY_F 406037000
#define BPSK_FREQUENCY_G 406040000
#define BPSK_FREQUENCY_J 406049000
#define BPSK_FREQUENCY_K 406052000
#define BPSK_FREQUENCY_N 406061000
#define BPSK_FREQUENCY_O 406064000

namespace ui::external_app::epirb_tx {

enum class BeaconMode {
    FILE = 0,
    MODE_MANUAL = 1
};

enum class BeaconType {
    EPIRB = 0,
    ELT = 1,
    PLB = 2
};

enum class BeaconProtocol {
    USER = 0,
    STANDARD = 1,
    NATIONAL = 2
};

enum class AmChannel {
    TEST = 0,
    REAL = 1,
    MANUAL = 2
};

enum class BpskChannel {
    HAM = 0,
    B = 1,
    C = 2,
    F = 3,
    G = 4,
    J = 5,
    K = 6,
    N = 7,
    O = 8,
    MANUAL = 10
};

struct Location {
    std::string locator;
    bool south;
    uint16_t lat_deg;
    uint8_t lat_min;
    uint8_t lat_sec;
    float latitude;
    bool west;
    uint16_t long_deg;
    uint8_t long_min;
    uint8_t long_sec;
    float longitude;
};

struct BeaconParams {
    BeaconType type;
    BeaconProtocol protocol;
    uint32_t country;
    bool is_test;
    bool is_internal;
    bool has_121_5;
    Location location;
};

class EPIRBTXAppView : public View {
   public:
    EPIRBTXAppView(NavigationView& nav);
    ~EPIRBTXAppView();

    void focus() override;

    std::string title() const override { return "EPIRB TX"; };

   private:
    void start_tx();
    void stop_tx();
    void update_config();
    void on_tx_progress(const uint32_t progress, const bool done);
    void on_timer();
    void load_beacons();
    void set_tx_button_state(bool active);
    std::string frame_to_hex_string(bool start);
    void generate_frame(BeaconParams params);
    void update_frame(bool updateConfig = true);
    void update_bpsk_frequency();
    void update_am_transmission();
    void update_mode();
    void update_location(bool updateLocatorField = true);

    struct Beacon {
        std::string title{};
        std::string description{};
        std::string frame{};
    };
    std::vector<Beacon> beacons{};
    Beacon default_beacon{"Self test", "Serial User Location Protocol", "FFFED0D6E6202820000C29FF51041775302D"};

    BeaconParams beacon_params{BeaconType::ELT, BeaconProtocol::STANDARD, 227, true, true, true, {"JN03RO", false, 0, 0, 0, 0, false, 0, 0, 0, 0}};

    // Currently selected beacon index (from BEACONS.TXT file / options_frame combo)
    uint32_t selected_beacon{0};

    // Frequency of the transmitter before starting the app (used to restore frequency when leaving)
    rf::Frequency original_frequency{0};
    // Frequency of the AM emergency signal
    rf::Frequency am_frequency{AM_TEST_FREQUENCY};
    // Frequency of the 406 MHz BPSK signal
    rf::Frequency bpsk_frequency{BPSK_FREQUENCY_HAM};
    // Selected am channel
    uint8_t am_channel{(uint8_t)AmChannel::TEST};
    // Selected bpsk channel
    uint8_t bpsk_channel{(uint8_t)BpskChannel::HAM};
    // Manual AM frequency value
    rf::Frequency manual_am_frequency{AM_TEST_FREQUENCY};
    // Manual BPSK frequency value
    rf::Frequency manual_bpsk_frequency{BPSK_FREQUENCY_HAM};

    // True when using a beacon from the BEACONS.TXT file
    bool mode_file{false};
    // True when looping on sending beacons is enabled
    bool loop_enabled{true};
    // True if AM emergency signal transmission is enabled
    bool am_enabled{true};
    // True if we want to send a new frame each time the user changes the current beacon
    bool send_on_change{true};
    // The current locator string
    std::string locator{"JN03RO"};
    // The delay between each frame when on loop mode
    uint32_t delay{50};
    uint8_t beacon_type{(uint8_t)BeaconType::EPIRB};
    // Currently selected beacon protocol
    uint8_t beacon_protocol{(uint8_t)BeaconProtocol::USER};
    // Currently selected beacon country
    uint32_t beacon_country{227};
    // Current beacon's internal state (true for internal location system)
    bool beacon_internal{true};

    TxRadioState radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        TONES_SAMPLERATE /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_epirb",
        app_settings::Mode::TX,
        {
            {"sbeacon"sv, &selected_beacon},
            {"amfreq"sv, &am_frequency},
            {"bpskfreq"sv, &bpsk_frequency},
            {"amchan"sv, &am_channel},
            {"bpskchan"sv, &bpsk_channel},
            {"loop"sv, &loop_enabled},
            {"delay"sv, &delay},
            {"file"sv, &mode_file},
            {"am"sv, &am_enabled},
            {"soc"sv, &send_on_change},
            {"type"sv, &beacon_type},
            {"proto"sv, &beacon_protocol},
            {"country"sv, &beacon_country},
            {"internal"sv, &beacon_internal},
            {"locator"sv, &locator},
        }};

    // Time of the last sent frame
    uint32_t last_frame_time{0};
    // True when transmission is enabled
    bool transmitting{false};
    // True when transmitting a BPSK frame
    bool transmitting_bpsk{false};
    // True when currently looping on sending beacons
    bool loop{false};

    // Current EPIRBTXDataMessage for baseband
    EPIRBTXDataMessage epirb_tx_message{};

    const size_t max_text_width = UI_POS_WIDTH_REMAINING(6) / UI_POS_DEFAULT_WIDTH;
    const size_t max_text_width_ext = UI_POS_WIDTH_REMAINING(0) / UI_POS_DEFAULT_WIDTH;

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Source:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Frame:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(10)}, "Next frame in   s.", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(12)}, "AM frequency:         MHz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(14)}, "AM   chan.:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(15)}, "BPSK chan.:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(17), UI_POS_Y(9)}, "s.", Theme::getInstance()->fg_light->foreground}};

    // For file mode
    Text text_beacon{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(7), UI_POS_DEFAULT_HEIGHT},
        "Beacon:"};
    // Beacon selection from BEACONS.TXT
    OptionsField options_frame{
        {UI_POS_X(7), UI_POS_Y(1)},
        30,
        {}};
    Text text_description_label{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(12), UI_POS_DEFAULT_HEIGHT},
        "Description:"};
    Text text_description{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH_REMAINING(0), UI_POS_DEFAULT_HEIGHT},
        ""};
    Text text_description_end{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(0), UI_POS_DEFAULT_HEIGHT},
        ""};

    // For manual mode
    Text text_beacon_type{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Type:"};
    Text text_beacon_country{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Country:"};
    Checkbox checkbox_beacon_internal{
        {UI_POS_X_RIGHT(12), UI_POS_Y(2)},
        12,
        "Internal",
        true};
    Text text_beacon_locator{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Locator:"};
    Text text_beacon_latitude{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Lat.:"};
    Text text_beacon_longitude{
        {UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Long.:"};
    Text text_beacon_latitude_value{
        {UI_POS_X(7), UI_POS_Y(4), UI_POS_WIDTH(12), UI_POS_DEFAULT_HEIGHT},
        ""};
    Text text_beacon_longitude_value{
        {UI_POS_X(7), UI_POS_Y(5), UI_POS_WIDTH(12), UI_POS_DEFAULT_HEIGHT},
        ""};
    OptionsField options_beacon_type{
        {UI_POS_X(9), UI_POS_Y(1)},
        7,
        {{"EPIRB", (uint8_t)BeaconType::EPIRB},
         {"ELT", (uint8_t)BeaconType::ELT},
         {"PLB", (uint8_t)BeaconType::PLB}}};
    OptionsField options_beacon_protocol{
        {UI_POS_X(9 + 7), UI_POS_Y(1)},
        30,
        {{"User", (uint8_t)BeaconProtocol::USER},
         {"Standard", (uint8_t)BeaconProtocol::STANDARD},
         {"National", (uint8_t)BeaconProtocol::NATIONAL}}};
    OptionsField options_beacon_country{
        {UI_POS_X(9), UI_POS_Y(2)},
        7,
        {{"France", 227},
         {"USA", 366},
         {"Germany", 211},
         {"Russia", 273},
         {"Spain", 224},
         {"Japan", 431},
         {"UK", 232}}};
    TextField text_field_beacon_locator{
        {UI_POS_X(9), UI_POS_Y(3), UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT},
        "JN03RO"};
    Button button_mangps{
        {UI_POS_X_RIGHT(9), UI_POS_Y(3), UI_POS_WIDTH(9), UI_POS_HEIGHT(2)},
        "Set pos."};

    // Mode selection (file/manual)
    OptionsField options_mode{
        {UI_POS_X(7), UI_POS_Y(0)},
        30,
        {{"File (BEACONS.TXT)", (uint8_t)BeaconMode::FILE},
         {"Manual (Editor)", (uint8_t)BeaconMode::MODE_MANUAL}}};

    // Frame content
    Text text_frame{
        {UI_POS_X(6), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(6), UI_POS_DEFAULT_HEIGHT},
        ""};
    Text text_frame_end{
        {UI_POS_X(6), UI_POS_Y(7), UI_POS_WIDTH_REMAINING(6), UI_POS_DEFAULT_HEIGHT},
        ""};

    Text text_timeout{
        {UI_POS_X(14), UI_POS_Y(10), UI_POS_WIDTH(2), UI_POS_DEFAULT_HEIGHT},
        ""};

    // Transmission settings
    Checkbox checkbox_loop{
        {UI_POS_X(0), UI_POS_Y(9)},
        10,
        "Resend every",
        true};
    NumberField field_delay{
        {UI_POS_X(15), UI_POS_Y(9)},
        2,
        {1, 99},
        1,
        ' '};
    Checkbox checkbox_am{
        {UI_POS_X(0), UI_POS_Y(11)},
        10,
        "AM signal",
        true};
    FrequencyField field_am_frequency{
        {UI_POS_X(13), UI_POS_Y(12)}};
    Checkbox checkbox_send_on_change{
        {UI_POS_X(0), UI_POS_Y(13)},
        14,
        "Send on change",
        true};
    Button button_tx{
        {UI_POS_X_RIGHT(9), UI_POS_Y(9), UI_POS_WIDTH(9), UI_POS_HEIGHT(2)},
        "START"};
    const Style& style_tx_start = *Theme::getInstance()->fg_green;
    const Style& style_tx_stop = *Theme::getInstance()->fg_red;
    OptionsField options_am_channel{
        {UI_POS_X(11), UI_POS_Y(14)},
        20,
        {{"121.375 MHz (Test)", 0},
         {"121.500 MHz /!\\Real", 1},
         {"Manual", 2}}};
    OptionsField options_bpsk_channel{
        {UI_POS_X(11), UI_POS_Y(15)},
        20,
        {{"433.025 MHz (Ham)", (uint8_t)BpskChannel::HAM},
         {"406.025 MHz (B)", (uint8_t)BpskChannel::B},
         {"406.028 MHz (C)", (uint8_t)BpskChannel::C},
         {"406.037 MHz (F)", (uint8_t)BpskChannel::F},
         {"406.040 MHz (G)", (uint8_t)BpskChannel::G},
         {"406.049 MHz (J)", (uint8_t)BpskChannel::J},
         {"406.052 MHz (K)", (uint8_t)BpskChannel::K},
         {"406.061 MHz (N)", (uint8_t)BpskChannel::N},
         {"406.064 MHz (O)", (uint8_t)BpskChannel::O},
         {"Manual", (uint8_t)BpskChannel::MANUAL}}};

    // Transmitter view
    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        10000,
        12};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        // Use frame sync for our applcation timer callback
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

}  // namespace ui::external_app::epirb_tx

#endif /*__EPIRB_TX_H__*/
