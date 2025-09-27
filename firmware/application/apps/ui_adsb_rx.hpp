/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2024 Mark Thompson
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

#include "ui.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"

#include "adsb.hpp"
#include "app_settings.hpp"
#include "crc.hpp"
#include "database.hpp"
#include "file.hpp"
#include "log_file.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "recent_entries.hpp"
#include "string_format.hpp"

using namespace adsb;

namespace ui {
#define AIRCRAFT_ID_L 1  // aircraft ID message type (lowest type id)
#define AIRCRAFT_ID_H 4  // aircraft ID message type (highest type id)

#define SURFACE_POS_L 5  // surface position (lowest type id)
#define SURFACE_POS_H 8  // surface position (highest type id)

#define AIRBORNE_POS_BARO_L 9   // airborne position (lowest type id)
#define AIRBORNE_POS_BARO_H 18  // airborne position (highest type id)

#define AIRBORNE_VEL 19  // airborne velocities

#define AIRBORNE_POS_GPS_L 20  // airborne position (lowest type id)
#define AIRBORNE_POS_GPS_H 22  // airborne position (highest type id)

#define AIRBORNE_OP_STATUS 31  // Aircraft operation status

#define RESERVED_L 23  // reserved for other uses
#define RESERVED_H 31  // reserved for other uses

#define VEL_GND_SUBSONIC 1
#define VEL_GND_SUPERSONIC 2
#define VEL_AIR_SUBSONIC 3
#define VEL_AIR_SUPERSONIC 4

#define O_E_FRAME_TIMEOUT 20          // timeout between odd and even frames
#define MARKER_UPDATE_SECONDS_OSM 10  // "other" map marker redraw interval for osm
#define MARKER_UPDATE_SECONDS_BIN 5   // "other" map marker redraw interval for bin map

/* Thresholds (in seconds) that define the transition between ages. */
struct ADSBAgeLimit {
    static constexpr int Current = 10;
    static constexpr int Recent = 30;
    static constexpr int Expired = 300;
};

/* Age states used for sorting and drawing recent entries. */
enum class ADSBAgeState : uint8_t {
    Invalid,
    Current,
    Recent,
    Old,
    Expired,
};

/* Data extracted from ADSB frames. */
struct AircraftRecentEntry {
    using Key = uint32_t;

    static constexpr Key invalid_key = 0xffffffff;

    uint32_t ICAO_address{};
    uint16_t hits{0};

    ADSBAgeState state{ADSBAgeState::Invalid};
    uint32_t age{0};  // In seconds
    uint32_t amp{0};
    adsb_pos pos{false, false, 0, 0, 0};
    adsb_vel velo{false, SPD_GND, 0, 999, 0};
    ADSBFrame frame_pos_even{};
    ADSBFrame frame_pos_odd{};

    std::string icao_str{};
    std::string callsign{};
    std::string info_string{};

    uint8_t sil{0};  // Surveillance integrity level
    uint16_t sqwk{0};

    AircraftRecentEntry(const uint32_t ICAO_address)
        : ICAO_address{ICAO_address} {
        this->icao_str = to_string_hex(ICAO_address, 6);
    }

    /* RecentEntries helpers expect a "key" on every item. */
    Key key() const {
        return ICAO_address;
    }

    void set_callsign(std::string new_callsign) {
        callsign = std::move(new_callsign);
    }

    void inc_hit() {
        hits++;
    }

    void set_frame_pos(ADSBFrame& frame, uint32_t parity) {
        if (!parity)
            frame_pos_even = frame;
        else
            frame_pos_odd = frame;

        if (!frame_pos_even.empty() && !frame_pos_odd.empty()) {
            if (abs(frame_pos_even.get_rx_timestamp() - frame_pos_odd.get_rx_timestamp()) < O_E_FRAME_TIMEOUT)
                pos = decode_frame_pos(frame_pos_even, frame_pos_odd);
        }
    }

    void set_frame_velo(ADSBFrame& frame) {
        velo = decode_frame_velo(frame);
    }

    void set_info_string(std::string new_info_string) {
        info_string = std::move(new_info_string);
    }

    void reset_age() {
        age = 0;
    }

    void inc_age(int delta) {
        age += delta;

        if (age < ADSBAgeLimit::Current)
            state = pos.pos_valid ? ADSBAgeState::Current
                                  : ADSBAgeState::Invalid;

        else if (age < ADSBAgeLimit::Recent)
            state = ADSBAgeState::Recent;

        else if (age < ADSBAgeLimit::Expired)
            state = ADSBAgeState::Old;

        else
            state = ADSBAgeState::Expired;
    }
};

// NB: uses std::list underneath so assuming refs are NOT invalidated.
using AircraftRecentEntries = RecentEntries<AircraftRecentEntry>;

/* Holds data for logging. */
struct ADSBLogEntry {
    std::string raw_data{};
    std::string icao{};
    std::string callsign{};
    adsb_pos pos{};
    adsb_vel vel{};
    uint8_t vel_type{};
    uint8_t sil{};
    uint16_t sqwk{};
};

// TODO: Make logging optional.
/* Logs entries to a log file. */
class ADSBLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }
    void log(const ADSBLogEntry& log_entry);

   private:
    LogFile log_file{};
};

/* Shows detailed information about an aircraft. */
class ADSBRxAircraftDetailsView : public View {
   public:
    ADSBRxAircraftDetailsView(
        NavigationView&,
        const AircraftRecentEntry& entry);

    void focus() override;
    std::string title() const override { return "AC Details"; }

   private:
    Labels labels{
        {{UI_POS_X(0), 1 * 16}, "ICAO:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 2 * 16}, "Registration:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 3 * 16}, "Manufacturer:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 5 * 16}, "Model:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 7 * 16}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 8 * 16}, "Number of engines:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 9 * 16}, "Engine type:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 11 * 16}, "Owner:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 13 * 16}, "Operator:", Theme::getInstance()->fg_light->foreground}};

    Text text_icao_address{
        {5 * 8, 1 * 16, 6 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_registration{
        {13 * 8, 2 * 16, 8 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_manufacturer{
        {UI_POS_X(0), 4 * 16, 19 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_model{
        {UI_POS_X(0), 6 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_type{
        {5 * 8, 7 * 16, 22 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_number_of_engines{
        {18 * 8, 8 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_engine_type{
        {UI_POS_X(0), 10 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_owner{
        {UI_POS_X(0), 12 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_operator{
        {UI_POS_X(0), 14 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Button button_close{
        {UI_POS_X_CENTER(12), UI_POS_Y(16), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)},
        "Back"};
};

/* Shows detailed information about an aircraft's flight. */
class ADSBRxDetailsView : public View {
   public:
    ADSBRxDetailsView(NavigationView&, const AircraftRecentEntry& entry);

    ADSBRxDetailsView(const ADSBRxDetailsView&) = delete;
    ADSBRxDetailsView& operator=(const ADSBRxDetailsView&) = delete;

    void focus() override;
    void update(const AircraftRecentEntry& entry);

    /* Calls forwarded to map view if shown. */
    bool map_active() const { return geomap_view_; }
    void clear_map_markers();
    /* Adds a marker for the entry to the map. Returns true on success. */
    bool add_map_marker(const AircraftRecentEntry& entry);

    std::string title() const override { return "Details"; }

    MapType get_map_type() {
        if (geomap_view_)
            return geomap_view_->get_map_type();
        else
            return MapType::MAP_TYPE_BIN;  // default
    }

   private:
    void refresh_ui();
    void on_gps(const GPSPosDataMessage* msg);
    void on_orientation(const OrientationDataMessage* msg);

    GeoMapView* geomap_view_{nullptr};
    ADSBRxAircraftDetailsView* aircraft_details_view_{nullptr};

    // NB: Keeping a copy so that it doesn't end up dangling
    // if removed from the recent entries list.
    AircraftRecentEntry entry_{AircraftRecentEntry::invalid_key};
    bool airline_checked{false};

    Labels labels{
        {{UI_POS_X(0), 1 * 16}, "ICAO:", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 1 * 16}, "Callsign:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 2 * 16}, "Last seen:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 3 * 16}, "Airline:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 5 * 16}, "Country:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 13 * 16}, "Even position frame:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 15 * 16}, "Odd position frame:", Theme::getInstance()->fg_light->foreground}};

    Text text_icao_address{
        {5 * 8, 1 * 16, 6 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_callsign{
        {22 * 8, 1 * 16, 8 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_last_seen{
        {11 * 8, 2 * 16, 19 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_airline{
        {UI_POS_X(0), 4 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_country{
        {8 * 8, 5 * 16, 22 * 8, UI_POS_HEIGHT(1)},
        "-"};

    Text text_infos{
        {UI_POS_X(0), 6 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_info2{
        {UI_POS_X(0), 7 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Text text_frame_pos_even{
        {UI_POS_X(0), 14 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};
    Text text_frame_pos_odd{
        {UI_POS_X(0), 16 * 16, screen_width, UI_POS_HEIGHT(1)},
        "-"};

    Button button_aircraft_details{
        {UI_POS_X_CENTER(12) - UI_POS_X(8), UI_POS_Y(9), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)},
        "A/C details"};

    Button button_see_map{
        {UI_POS_X_CENTER(12) + UI_POS_X(8), UI_POS_Y(9), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)},
        "See on map"};

    MessageHandlerRegistration message_handler_gps{
        Message::ID::GPSPosData,
        [this](Message* const p) {
            const auto message = static_cast<const GPSPosDataMessage*>(p);
            this->on_gps(message);
        }};
    MessageHandlerRegistration message_handler_orientation{
        Message::ID::OrientationData,
        [this](Message* const p) {
            const auto message = static_cast<const OrientationDataMessage*>(p);
            this->on_orientation(message);
        }};
};

/* Main ADSB application view and message dispatch. */
class ADSBRxView : public View {
   public:
    ADSBRxView(NavigationView& nav);
    ~ADSBRxView();

    ADSBRxView(const ADSBRxView&) = delete;
    ADSBRxView(ADSBRxView&&) = delete;
    ADSBRxView& operator=(const ADSBRxView&) = delete;
    ADSBRxView& operator=(ADSBRxView&&) = delete;

    void focus() override;
    std::string title() const override { return "ADS-B RX"; };

   private:
    RxRadioState radio_state_{
        1'090'000'000 /* frequency */,
        2'500'000 /* bandwidth */,
        2'000'000 /* sampling rate */,
        ReceiverModel::Mode::SpectrumAnalysis};
    app_settings::SettingsManager settings_{
        "rx_adsb", app_settings::Mode::RX};

    std::unique_ptr<ADSBLogger> logger{};

    /* Event Handlers */
    void on_frame(const ADSBFrameMessage* message);
    void on_tick_second();
    void refresh_ui();

    SignalToken signal_token_tick_second{};
    uint8_t tick_count = 0;
    uint16_t ticks_since_marker_refresh{MARKER_UPDATE_SECONDS_OSM};

    /* Max number of entries that can be updated in a single pass.
     * 16 is one screen of recent entries. */
    static constexpr uint8_t max_update_entries = 16;

    /* Recent Entries */
    RecentEntriesColumns columns{
        {{"ICAO/Call", 0},
         {"Lvl", 3},
         {"Spd", 3},
         {"Amp", 3},
         {"Hit", 3},
         {"Age", 4}}};
    AircraftRecentEntries recent{};
    RecentEntriesView<AircraftRecentEntries> recent_entries_view{columns, recent};

    /* Entry Management */
    void update_recent_entries(int age_delta);
    AircraftRecentEntry& find_or_create_entry(uint32_t ICAO_address);
    void sort_entries_by_state();
    void remove_expired_entries();

    /* The key of the entry in the details view if shown. */
    AircraftRecentEntry::Key detail_key{AircraftRecentEntry::invalid_key};
    ADSBRxDetailsView* details_view{nullptr};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "LNA:   VGA:   AMP:", Theme::getInstance()->fg_light->foreground}};

    LNAGainField field_lna{
        {UI_POS_X(4), UI_POS_Y(0)}};

    VGAGainField field_vga{
        {UI_POS_X(11), UI_POS_Y(0)}};

    RFAmpField field_rf_amp{
        {UI_POS_X(18), UI_POS_Y(0)}};

    RSSI rssi{
        {UI_POS_X(20), 4, UI_POS_WIDTH_REMAINING(23), 8},
    };

    ActivityDot status_frame{
        {UI_POS_X_RIGHT(3) + 2, 5, 2, 2},
        Theme::getInstance()->bg_darkest->foreground,
    };

    ActivityDot status_good_frame{
        {UI_POS_X_RIGHT(3) + 2, 9, 2, 2},
        Theme::getInstance()->fg_green->foreground,
    };

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    MessageHandlerRegistration message_handler_frame{
        Message::ID::ADSBFrame,
        [this](Message* const p) {
            const auto message = static_cast<const ADSBFrameMessage*>(p);
            this->on_frame(message);
        }};
};

} /* namespace ui */
