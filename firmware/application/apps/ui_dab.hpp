/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* DAB/DAB+ receiver: tunes a Band III or L-Band block, decodes the FIC, lists
 * the ensemble's services and plays a picked DAB+ service. The M4 does
 * acquisition, FFT, Viterbi, CRC and audio decoding; this view is protocol
 * and presentation. */

#ifndef __UI_DAB_H__
#define __UI_DAB_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"

#include "app_settings.hpp"
#include "file_path.hpp"
#include "log_file.hpp"
#include "radio_state.hpp"

#include "dab/dab_blocks.hpp"
#include "dab/dab_dls.hpp"
#include "dab/dab_engage.hpp"
#include "dab/dab_ensemble.hpp"
#include "dab/dab_fig.hpp"
#include "dab/dab_xpad.hpp"

#include <cstdint>
#include <string>

namespace ui {

class DabView : public View, private dab::XpadSink {
   public:
    DabView(NavigationView& nav);
    ~DabView();

    DabView(const DabView& other) = delete;
    DabView& operator=(const DabView& other) = delete;

    void focus() override;

    std::string title() const override { return "DAB+ RX"; }

   private:
    static constexpr size_t kServiceRows = 12;

    void tune_block(size_t table_index);
    void on_fib(const uint8_t* fib, bool crc_ok);
    void on_soft(uint16_t offset, const uint8_t* data, uint8_t length);
    void on_pad(const uint8_t* data, size_t length);
    void on_status(const DabStatusMessage& m);
    void refresh();
    void set_message(const std::string& text);
    void render_message();
    void refresh_service_options();
    void select_service(size_t index);
    void stop_audio();
    void refuse(const char* why);
    static bool fits_image(const dab::DabService& service);
    static bool is_data_service(const dab::DabService& service);
    void set_logging(bool enabled);
    void engage_target();
    void release_target();
    void manage_engagement(bool locked, uint16_t constellation_r4);
    void retry_if_silent(uint16_t msc_state, uint16_t decoder_counts);

    /* MOT slideshow groups are not taken: reassembly needs 32 KB of the M0's
     * 64 KB RAM, and this view cannot draw the image anyway. */
    void on_dls_group(const uint8_t* data, size_t length) override;

    NavigationView& nav_;

    /* A placeholder until field_block tunes a block. */
    RxRadioState radio_state_{
        225648000 /* frequency */,
        1750000 /* bandwidth, the MAX2831 floor and above DAB's 1.536 MHz */,
        dab::kSampleRate /* 2.048 Msps, DAB mode I native */
    };

    /* Persisted: declared before settings_, which binds their addresses. */
    bool logging_{false};
    /* By name, so a change to the band tables cannot shift it. */
    std::string block_name_{"11C"};
    /* The last station engaged, started again once a tuned block lists it. */
    uint32_t station_sid_{0};
    std::unique_ptr<LogFile> logger_{};

    app_settings::SettingsManager settings_{
        "rx_dab",
        app_settings::Mode::RX,
        {
            {"logging"sv, &logging_},
            {"block"sv, &block_name_},
            {"station"sv, &station_sid_},
        }};

    dab::FigParser parser_{};
    dab::EnsembleDatabase database_{};
    dab::XpadParser xpad_{};
    dab::DlsAssembler dls_{};

    uint8_t fib_error_rate_{0};

    /* Share of a 96 ms frame the baseband used; audio decoder allocation. */
    uint8_t cpu_percent_{0};
    int16_t frequency_error_hz_{0};
    uint32_t decoder_bytes_{0};

    /* The Rx line: null depth as an SNR and, while a station plays, the share
     * of audio concealed over the last ~10 s (the baseband's dry-transfer
     * count, 144 transfers a frame). Counting starts after 1 s of flowing audio
     * (the deinterleaver fill is not a loss) or after 15 s regardless. */
    void account_quality(bool locked, uint16_t null_dip_ratio, uint16_t msc_state, uint16_t pcm_report);
    static constexpr size_t kLostBuckets = 11;
    static constexpr uint16_t kLostBucketFrames = 10;
    static constexpr uint32_t kTransfersPerFrame = 144;
    static constexpr uint8_t kWarmFlowFrames = 10;
    static constexpr uint16_t kWarmMaxFrames = 156;
    uint16_t lost_dry_[kLostBuckets]{};
    uint8_t lost_bucket_{0};
    uint8_t lost_full_{0};
    uint16_t lost_frames_{0};
    uint16_t lost_state_{0};
    uint16_t warm_frames_{0};
    uint8_t warm_flow_{0};
    bool warm_{false};
    bool locked_{false};
    uint16_t snr16_{0};

    /* A subchannel can come back from a lost lock demodulating noise while the
     * FIC is fine; only a reconfigure clears it. A healthy start decodes its
     * first unit within ~40 reports. */
    static constexpr uint8_t kSilentRetryFrames = 48;
    uint8_t silent_frames_{0};

    /* Reports since the block was tuned, for the empty list's status line. */
    static constexpr uint16_t kAntennaHintFrames = 160;
    uint16_t search_frames_{0};

    size_t service_options_count_{0};
    uint32_t service_label_signature_{0};
    uint32_t selected_sid_{0};
    bool audio_selected_{false};
    /* A pick refused while its service was still arriving; engaged once it can play. */
    uint32_t pending_sid_{0};

    /* The pick, held to engage once locked and again after a lost lock.
     * audio_selected_: the user wants it; engage_.engaged(): the baseband is
     * decoding it. */
    struct Target {
        uint16_t start_cu{0};
        uint16_t size_cu{0};
        uint8_t level{0};
        bool option_b{false};
        bool dab_plus{false};

        bool operator==(const Target& o) const {
            return start_cu == o.start_cu && size_cu == o.size_cu && level == o.level &&
                   option_b == o.option_b && dab_plus == o.dab_plus;
        }
    };
    Target target_{};
    dab::Engagement engage_{dab::kEngageDefaults};

    /* Rebuilding field_service fires on_change; without this guard every new
     * service would reconfigure the baseband and restart the audio. */
    bool rebuilding_services_{false};

    /* First service shown; the list scrolls to keep the picked one in view. */
    size_t list_top_{0};
    /* field_service may be empty, and reading its value then indexes past
     * its options. */
    size_t picker_count_{0};

    std::string message_{};
    size_t message_offset_{0};
    uint8_t message_tick_{0};
    uint8_t message_hold_{0};
    static constexpr uint8_t kMessageTicksPerChar = 3;
    static constexpr uint8_t kMessageHold = 20;

    /* The startup image load's log record, held as the logger opens later. */
    std::string image_log_{};

    /* At twelve FIBs a frame either kind would swamp the card, so both are capped. */
    uint16_t raw_fibs_logged_{0};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(2)}, "BLK:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(3)}, "Rx:", Theme::getInstance()->fg_light->foreground},
    };

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};

    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};

    /* In the two columns the RSSI meter leaves free, as in other receivers:
     * this view starts the audio output, so it needs the control too. */
    AudioVolumeField field_volume{
        {screen_width - 2 * 8, UI_POS_Y(0)}};

    OptionsField field_block{
        {UI_POS_X(5), UI_POS_Y(2)},
        4,
        {}};

    OptionsField field_service{
        {UI_POS_X(11), UI_POS_Y(2)},
        19,
        {}};

    Text text_quality{
        {UI_POS_X(4), UI_POS_Y(3), UI_POS_WIDTH_REMAINING(4), 16},
        "-"};

    /* Station text (DLS) and status messages; see render_message(). */
    Text text_dls{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    /* ui::Widget is non-copyable and non-movable, so the rows are named
     * members rather than an array; service_row() gives indexed access. */
    Text text_service_0{
        {UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_1{
        {UI_POS_X(0), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_2{
        {UI_POS_X(0), UI_POS_Y(7), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_3{
        {UI_POS_X(0), UI_POS_Y(8), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_4{
        {UI_POS_X(0), UI_POS_Y(9), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_5{
        {UI_POS_X(0), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_6{
        {UI_POS_X(0), UI_POS_Y(11), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_7{
        {UI_POS_X(0), UI_POS_Y(12), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_8{
        {UI_POS_X(0), UI_POS_Y(13), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_9{
        {UI_POS_X(0), UI_POS_Y(14), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_10{
        {UI_POS_X(0), UI_POS_Y(15), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text text_service_11{
        {UI_POS_X(0), UI_POS_Y(16), UI_POS_WIDTH_REMAINING(0), 16},
        ""};

    Text* service_row(size_t index);

    /* Flush with the bottom edge: the view is 19 rows and the box 24 px. */
    Checkbox check_log{
        {UI_POS_X(23), UI_POS_Y(19) - 24},
        3,
        "Log"};

    MessageHandlerRegistration message_handler_fib{
        Message::ID::DabFib,
        [this](Message* const p) {
            const auto message = static_cast<const DabFibMessage*>(p);
            this->on_fib(message->payload.data(), message->crc_ok);
        }};

    MessageHandlerRegistration message_handler_soft{
        Message::ID::DabSoft,
        [this](Message* const p) {
            const auto message = static_cast<const DabSoftMessage*>(p);
            this->on_soft(message->offset, message->payload.data(), message->length);
        }};

    MessageHandlerRegistration message_handler_pad{
        Message::ID::DabPad,
        [this](Message* const p) {
            const auto message = static_cast<const DabPadMessage*>(p);
            this->on_pad(message->payload.data(), message->length);
        }};

    MessageHandlerRegistration message_handler_status{
        Message::ID::DabStatus,
        [this](Message* const p) {
            this->on_status(*static_cast<const DabStatusMessage*>(p));
        }};
};

} /* namespace ui */

#endif /*__UI_DAB_H__*/
