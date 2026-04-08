/*
 * Copyright (C) 2024 EPIRB Decoder Implementation
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

#ifndef __UI_EPIRB_RX_H__
#define __UI_EPIRB_RX_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"

#include "event_m0.hpp"
#include "signal.hpp"
#include "message.hpp"
#include "log_file.hpp"

#include "baseband_packet.hpp"

#include "audio.hpp"

#include "beacon.hpp"

namespace ui::external_app::epirb_rx {

#define BEACON_HISTORY_SIZE 16

enum class PacketStatus : uint8_t {
    Valid = 0,
    Corrected = 1,
    Error = 2
};

/*#define BEACON_DATA_SIZE 18  // Max 144 bits => 18 bytes
class Location {
   public:
    class Angle {
       public:
        long degrees = 0;
        long minutes = 0;
        long seconds = 0;
        bool orientation = false;  // false = N/E, true = S/W

        Angle() {}
        Angle(long degrees) : degrees(degrees) {}

        inline void clear() {
            degrees = 255;
            minutes = 0;
            seconds = 0;
            orientation = false;
            floatValue = 255;
        }

        inline float getFloatValue() {
            if (floatValue >= 255) {
                floatValue = (float)degrees;
                floatValue += ((float)minutes / 60.0f);
                floatValue += ((float)seconds / 3600.0f);
                if (orientation) {
                    floatValue = -floatValue;
                }
            }
            return floatValue;
        }

        inline void toFloatString(char* angleStr) {
            std::sprintf(angleStr, "%015.6f\n", getFloatValue());
        }

       private:
        float floatValue = 255.0f;
    };

    enum class LocationFormat { DECIMAL,
                                SEXAGESIMAL,
                                MAIDENHEAD_LOCATOR };

    Angle latitude = Angle(127);
    Angle longitude = Angle(255);
    inline bool isUnknown() {
        return false;
    }
};
class Beacon {
   public:
    enum class FrameMode { NORMAL,
                           SELF_TEST,
                           UNKNOWN };
    enum class MainLocatingDevice { UNDEFINED,
                                    INTERNAL_NAV,
                                    EXTERNAL_NAV };
    enum class AuxLocatingDevice { UNDEFINED,
                                   NONE,
                                   NONE_OR_OTHER,
                                   OTHER,
                                   MHZ121_5,
                                   SART };

    class Protocol {
       public:
        enum class Type { STANDARD_LOCATION,
                          NATIONAL_LOCATION,
                          RLS_LOCATION,
                          ELT_DT_LOCATION,
                          USER,
                          SPARE,
                          UNKNOWN };
        Type type;

        Protocol(Type type) : type(type) {}

        bool isUser() const { return (type == Type::USER); };
        bool isNational() const { return (type == Type::NATIONAL_LOCATION); };
        bool isStandard() const { return (type == Type::STANDARD_LOCATION); };
        bool isRls() const { return (type == Type::RLS_LOCATION); };
        bool isRlsOrElt() const { return (type == Type::RLS_LOCATION || type == Type::ELT_DT_LOCATION); };
        bool isUnknown() const { return (type == Type::UNKNOWN); };

        inline const char* getTypeName(bool longFrame) const {
            switch (type) {
                case Type::STANDARD_LOCATION:
                    return "Standard Protocol";
                case Type::NATIONAL_LOCATION:
                    return "National Protocol";
                case Type::RLS_LOCATION:
                    return "RLS";
                case Type::ELT_DT_LOCATION:
                    return "ELT(DT)";
                case Type::USER:
                    return longFrame ? "User Location Protocol" : "User Protocol";
                case Type::SPARE:
                    return "Spare Protocol";
                default:
                    return "Unknown Protocol";
            }
        }

        // Déclarations des membres statiques (définis en bas de fichier)
        static const Protocol USER_EPIRB_MARITIME, USER_EPIRB_RADIO, USER_ELT, USER_SERIAL, USER_TEST, USER_ORB, USER_NAT, USER_2G;
        static const Protocol STD_EPIRB, STD_ELT_24, STD_ELT_SERIAL, STD_ELT_AIRCRAFT, STD_EPIRB_SERIAL, STD_PLB_SERIAL, STD_SHIP, STD_TEST;
        static const Protocol NAT_ELT, NAT_EPIRB, NAT_PLB, NAT_TEST;
        static const Protocol RLS, ELT_DT, SPARE, UNKNOWN;
    };

    bool longFrame{true};
    bool protocolFlag{false};
    FrameMode frameMode{FrameMode::UNKNOWN};
    MainLocatingDevice mainLocatingDevice{MainLocatingDevice::UNDEFINED};
    AuxLocatingDevice auxLocatingDevice{AuxLocatingDevice::UNDEFINED};
    long protocolCode{0};
    //const Protocol* protocol{&Protocol::UNKNOWN};
    //Country country{};
    uint8_t frame[18];
    Location location{};
    uint64_t identifier{0};
    rtc::RTC date{};
    bool hasAdditionalData{false};
    std::string additionalData{};
    bool hasSerialNumber{false};
    std::string serialNumber{};
    std::string hexId{};
    uint32_t bch1{};
    uint32_t computedBch1{};
    bool hasBch2{false};
    uint32_t bch2{};
    uint32_t computedBch2{};
    bool isEmpty{true};
    inline bool isFrameValid() { return true; }
    inline const char* getProtocolName() { return "Test Protocol"; }
    inline const char* getProtocolDesciption() { return "Test Protocol Descr"; }

};
//inline const Beacon::Protocol Beacon::Protocol::UNKNOWN{Type::UNKNOWN};
*/
/*
class EPIRBDecoder {
   public:
    static Beacon decode_packet(const baseband::Packet& packet);

   private:
    static uint32_t decode_country_code(const std::array<uint8_t, 16>& data);
    static std::string decode_vessel_name(const std::array<uint8_t, 16>& data);

    // BCH error correction methods
    static PacketStatus perform_bch_check(std::array<uint8_t, 16>& data, uint8_t& error_count);
    static uint32_t calculate_bch_syndrome(const std::array<uint8_t, 16>& data);
    static bool correct_single_error(std::array<uint8_t, 16>& data, uint32_t syndrome);
    static uint8_t count_bit_errors(const std::array<uint8_t, 16>& original, const std::array<uint8_t, 16>& corrected);
};
*/
class EPIRBLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(Beacon& beacon);

   private:
    LogFile log_file{};
};

// Forward declarations of formatting functions
std::string format_beacon_type(Beacon& beacon);
std::string format_emergency_type(Beacon& beacon);
std::string format_packet_status(Beacon& beacon);
ui::Color get_packet_status_color(Beacon& beacon);
/*
class EPIRBBeaconDetailView : public ui::View {
   public:
    std::function<void(void)> on_close{};

    EPIRBBeaconDetailView(ui::NavigationView& nav);
    EPIRBBeaconDetailView(const EPIRBBeaconDetailView&) = delete;
    EPIRBBeaconDetailView& operator=(const EPIRBBeaconDetailView&) = delete;

    void set_beacon(const Beacon& beacon);
    const Beacon& beacon() const { return beacon_; }

    void focus() override;
    void paint(ui::Painter&) override;

    ui::GeoMapView* get_geomap_view() { return geomap_view; }

   private:
    Beacon beacon_{};

    ui::Button button_done{
        {125, 224, 96, 24},
        "Done"};
    ui::Button button_see_map{
        {19, 224, 96, 24},
        "See on map"};

    ui::GeoMapView* geomap_view{nullptr};

    ui::Rect draw_field(
        ui::Painter& painter,
        const ui::Rect& draw_rect,
        const ui::Style& style,
        const std::string& label,
        const std::string& value);
};
*/
class EPIRBAppView final : public ui::View {
   public:
    EPIRBAppView(ui::NavigationView& nav);
    ~EPIRBAppView();

    void set_parent_rect(const ui::Rect new_parent_rect) override;
    void paint(ui::Painter&) override;
    void focus() override;

    std::string title() const override { return "EPIRB RX"; }

   private:
    app_settings::SettingsManager settings_{
        "rx_epirb", app_settings::Mode::RX};

    ui::NavigationView& nav_;

    Beacon recent_beacons[BEACON_HISTORY_SIZE];
    int8_t recent_beacon_pos{0};
    bool recent_beacon_full{false};
    // std::unique_ptr<EPIRBLogger> logger{};

    // EPIRBBeaconDetailView beacon_detail_view{nav_};

    static constexpr auto header_height = 4 * 16;

    ui::Text label_frequency{
        {UI_POS_X(0), UI_POS_Y(0), 4 * 8, 1 * 16},
        "Freq"};

    ui::OptionsField options_frequency{
        {5 * 8, UI_POS_Y(0)},
        7,
        {
            {"406.028", 406028000},
            {"406.025", 406025000},
            {"406.037", 406037000},
            {"433.025", 433025000},
            {"144.875", 144875000},
        }};

    ui::RFAmpField field_rf_amp{
        {13 * 8, UI_POS_Y(0)}};

    ui::LNAGainField field_lna{
        {15 * 8, UI_POS_Y(0)}};

    ui::VGAGainField field_vga{
        {18 * 8, UI_POS_Y(0)}};

    ui::RSSI rssi{
        {UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};

    ui::Channel channel{
        {UI_POS_X(21), 5, UI_POS_WIDTH_REMAINING(24), 4}};

    ui::AudioVolumeField field_volume{
        {screen_width - 2 * 8, UI_POS_Y(0)}};

    // Status display
    ui::Text label_status{
        {UI_POS_X(0), 1 * 16, 15 * 8, 1 * 16},
        "Listening..."};

    ui::Text label_beacons_count{
        {16 * 8, 1 * 16, 14 * 8, 1 * 16},
        "Beacons: 0"};

    ui::Text label_packet_stats{
        {UI_POS_X(0), 3 * 16, 29 * 8, 1 * 16},
        ""};

    // Latest beacon info display
    ui::Text label_latest{
        {UI_POS_X(0), 2 * 16, 8 * 8, 1 * 16},
        "Latest:"};

    ui::Text text_latest_info{
        {8 * 8, 2 * 16, 22 * 8, 1 * 16},
        ""};

    // Current EPIRBTXDataMessage for baseband
    EPIRBTXDataMessage epirb_tx_message{};

    ui::OptionsField options_algo{
        {5 * 8, UI_POS_Y(4)},
        7,
        {
            {"1", 1},
            {"2", 2},
            {"3", 3},
            {"4", 4},
            {"5", 5},
        }};

    // Beacon list
    ui::Console console{
        {0, 4 * 16, 240, 152}};

    ui::Button button_map{
        {0, 224, 60, 24},
        "Map"};

    ui::Button button_clear{
        {64, 224, 60, 24},
        "Clear"};

    ui::Button button_log{
        {128, 224, 60, 24},
        "Log"};

    SignalToken signal_token_tick_second{};
    uint32_t beacons_received = 0;
    uint32_t packets_valid = 0;
    uint32_t packets_corrected = 0;
    uint32_t packets_error = 0;

    MessageHandlerRegistration message_handler_packet{
        Message::ID::EPIRBPacket,
        [this](Message* const p) {
            const auto message = static_cast<const EPIRBPacketMessage*>(p);
            this->on_packet(message->packet);
        }};

    static Beacon decode_packet(const baseband::Packet& packet);
    void on_packet(const baseband::Packet& packet);
    void on_beacon_decoded(Beacon& beacon);
    void on_show_map();
    void on_clear_beacons();
    void on_toggle_log();
    void on_tick_second();

    void update_display();
    std::string format_beacon_summary(Beacon& beacon);
    std::string format_location(Location& location);
    std::string beacon_to_hex_string(const baseband::Packet& packet);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __UI_EPIRB_RX_H__