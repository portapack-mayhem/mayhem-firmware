#ifndef __UI_FLEX_TX_H__
#define __UI_FLEX_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"

namespace ui::external_app::flex_tx {

struct FlexBIWParams {
    int32_t send_date{1};
    int32_t send_time{1};
    int32_t send_tz{1};
    int32_t send_dst{0};
    int32_t send_ssid1{0};
    int32_t send_ssid2{0};
    int32_t roaming{0};
    int32_t year{2015};
    int32_t month{1};
    int32_t day{1};
    int32_t hour{0};
    int32_t minute{0};
    int32_t second{0};
    int32_t tz_code{0};
    int32_t local_id{0};
    int32_t coverage_zone{0};
    int32_t country_code{0};
    int32_t msg_number{0};
    int32_t send_ers{1};
    int32_t ers_count{1};
};

class FlexParamsView : public View {
   public:
    FlexParamsView(NavigationView& nav, FlexBIWParams& params);

    std::string title() const override { return "FLEX Params"; };
    void focus() override;

   private:
    FlexBIWParams& params_;
    NavigationView& nav_;

    void on_roaming_changed(bool v);

    Labels labels{
        {{12 * 8, 1 * 16}, "-", Theme::getInstance()->fg_light->foreground},
        {{15 * 8, 1 * 16}, "-", Theme::getInstance()->fg_light->foreground},
        {{10 * 8, 2 * 16}, ":", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 2 * 16}, ":", Theme::getInstance()->fg_light->foreground},
    };

    Checkbox check_date{{0 * 8, 1 * 16}, 4, "Date", true};
    NumberField field_year{{8 * 8, 1 * 16}, 4, {1994, 2099}, 1, '0', true};
    NumberField field_month{{13 * 8, 1 * 16}, 2, {1, 12}, 1, '0', true};
    NumberField field_day{{16 * 8, 1 * 16}, 2, {1, 31}, 1, '0', true};

    Checkbox check_time{{0 * 8, 2 * 16}, 4, "Time", true};
    NumberField field_hour{{8 * 8, 2 * 16}, 2, {0, 23}, 1, '0', true};
    NumberField field_minute{{11 * 8, 2 * 16}, 2, {0, 59}, 1, '0', true};
    NumberField field_second{{14 * 8, 2 * 16}, 2, {0, 59}, 1, '0', true};

    Checkbox check_tz{{0 * 8, 3 * 16}, 2, "TZ", true};
    OptionsField options_tz{
        {6 * 8, 3 * 16},
        9,
        {{"UTC+0   ", 0},
         {"UTC+1   ", 1},
         {"UTC+2   ", 2},
         {"UTC+3   ", 3},
         {"UTC+4   ", 4},
         {"UTC+5   ", 5},
         {"UTC+6   ", 6},
         {"UTC+7   ", 7},
         {"UTC+8   ", 8},
         {"UTC+9   ", 9},
         {"UTC+10  ", 10},
         {"UTC+11  ", 11},
         {"UTC+12  ", 12},
         {"UTC+3:30", 13},
         {"UTC+4:30", 14},
         {"UTC+5:30", 15},
         {"UTC+5:45", 17},
         {"UTC+6:30", 18},
         {"UTC+9:30", 19},
         {"UTC-3:30", 20},
         {"UTC-11  ", 21},
         {"UTC-10  ", 22},
         {"UTC-9   ", 23},
         {"UTC-8   ", 24},
         {"UTC-7   ", 25},
         {"UTC-6   ", 26},
         {"UTC-5   ", 27},
         {"UTC-4   ", 28},
         {"UTC-3   ", 29},
         {"UTC-2   ", 30},
         {"UTC-1   ", 31}}};
    Checkbox check_dst{{18 * 8, 3 * 16}, 3, "DST", true};

    Checkbox check_ssid1{{0 * 8, 5 * 16}, 5, "LocID", true};
    NumberField field_local_id{{9 * 8, 5 * 16}, 3, {0, 511}, 1, '0'};
    Labels labels_cz{
        {{12 * 8, 5 * 16}, "CovZone", Theme::getInstance()->fg_light->foreground},
    };
    NumberField field_coverage{{20 * 8, 5 * 16}, 2, {0, 31}, 1, '0'};

    Checkbox check_ssid2{{0 * 8, 6 * 16}, 11, "CountryCode", true};
    NumberField field_country{{15 * 8, 6 * 16}, 4, {0, 1023}, 1, '0'};
    Checkbox check_roaming{{0 * 8, 7 * 16}, 4, "Roam", true};

    Labels labels_msg{
        {{0 * 8, 9 * 16}, "Msg#:", Theme::getInstance()->fg_light->foreground},
    };
    NumberField field_msg_number{{5 * 8, 9 * 16}, 2, {0, 63}, 1, '0'};

    Checkbox check_ers{{0 * 8, 10 * 16}, 3, "ERS", true};
    NumberField field_ers_count{{8 * 8, 10 * 16}, 2, {1, 10}, 1, ' '};

    Button button_save{{1 * 8, 12 * 16, 12 * 8, 32}, "Save"};
    Button button_cancel{{16 * 8, 12 * 16, 12 * 8, 32}, "Cancel"};
};

class FlexTXView : public View {
   public:
    FlexTXView(NavigationView& nav);
    ~FlexTXView();

    FlexTXView(const FlexTXView&) = delete;
    FlexTXView& operator=(const FlexTXView&) = delete;

    void focus() override;
    void paint(Painter&) override;
    std::string title() const override { return "FLEX TX"; };

    FlexBIWParams biw_params_{};

   private:
    std::string buffer{"FLEX TEST"};
    std::string message{};
    NavigationView& nav_;
    int tx_phase_{0};
    int ers_remaining_{0};
    int tx_step_{0};
    int tx_steps_total_{0};

    int64_t capcode_value{1000};

    TxRadioState radio_state_{
        931740000 /* frequency */,
        1750000 /* bandwidth */,
        2280000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_flex",
        app_settings::Mode::TX,
        {{"capcode", &capcode_value},
         {"biw_date", &biw_params_.send_date},
         {"biw_time", &biw_params_.send_time},
         {"biw_tz", &biw_params_.send_tz},
         {"biw_ssid1", &biw_params_.send_ssid1},
         {"biw_ssid2", &biw_params_.send_ssid2},
         {"biw_roam", &biw_params_.roaming},
         {"biw_tzc", &biw_params_.tz_code},
         {"biw_lid", &biw_params_.local_id},
         {"biw_cz", &biw_params_.coverage_zone},
         {"biw_cc", &biw_params_.country_code},
         {"biw_ers", &biw_params_.send_ers},
         {"biw_ersc", &biw_params_.ers_count}}};

    void on_set_text(NavigationView& nav);
    void on_tx_progress(const uint32_t progress, const bool done);
    void on_serial_msg(const FlexTosendMessage data);
    bool start_tx();
    void send_frame(uint32_t msg_r);

    Labels labels{
        {{1 * 8, 4 * 8}, "Capcode:", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, 6 * 8}, "Speed:", Theme::getInstance()->fg_light->foreground},
        {{4 * 8, 8 * 8}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 12 * 8}, "Message:", Theme::getInstance()->fg_light->foreground}};

    Text text_capinfo{
        {UI_POS_X(0), 2 * 8, screen_width, 16},
        ""};

    SymField field_capcode{
        {10 * 8, 4 * 8},
        10};

    OptionsField options_speed{
        {10 * 8, 6 * 8},
        10,
        {{"1600/2FSK ", 0}}};

    OptionsField options_type{
        {10 * 8, 8 * 8},
        14,
        {{"Alphanumeric ", 0},
         {"Numeric      ", 1},
         {"Short/tone   ", 2},
         {"Short numeric", 3}}};

    Text text_message{
        {UI_POS_X(0), 14 * 8, screen_width, 16},
        ""};
    Text text_message_l2{
        {UI_POS_X(0), 16 * 8, screen_width, 16},
        ""};

    Button button_message{
        {UI_POS_X(0), 18 * 8, 13 * 8, 32},
        "Set message"};

    Button button_params{
        {14 * 8, 18 * 8, 13 * 8, 32},
        "Set params"};

    ProgressBar progressbar{
        {16, 210, UI_POS_WIDTH_REMAINING(4), 16}};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        10000,
        9};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message =
                *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};

    MessageHandlerRegistration message_handler_flex_tosend{
        Message::ID::FlexTosend,
        [this](const Message* const p) {
            const auto message =
                *reinterpret_cast<const FlexTosendMessage*>(p);
            this->on_serial_msg(message);
        }};
};

}  // namespace ui::external_app::flex_tx

#endif
