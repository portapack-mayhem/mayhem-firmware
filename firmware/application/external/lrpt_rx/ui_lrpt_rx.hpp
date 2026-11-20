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

#ifndef __UI_LRPT_RX_H__
#define __UI_LRPT_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "bmpfile.hpp"
#include "file.hpp"
#include "message.hpp"
#include "lrpt_shared.hpp"

#include "lrpt_decoder.hpp"
#include "lrpt_deinterleaver.hpp"

namespace ui::external_app::lrpt_rx {

// The demodulator's last points, I and Q of the soft symbols.
class Constellation : public Widget {
   public:
    Constellation(Rect parent_rect)
        : Widget{parent_rect} {}

    void update(const int8_t* points);
    void paint(Painter& painter) override;

   private:
    int8_t points_[lrpt::CONSTELLATION_POINTS][2]{};
};

class LrptRxView : public View {
   public:
    LrptRxView(NavigationView& nav);
    ~LrptRxView();
    LrptRxView(const LrptRxView&) = delete;
    LrptRxView& operator=(const LrptRxView&) = delete;

    void focus() override;
    std::string title() const override { return "LRPT RX"; };

   private:
    static constexpr size_t PREVIEW_MAX_WIDTH = 320;
    static constexpr size_t CHUNK_PIXELS = lrpt::STRIP_WIDTH / 4;
    static constexpr uint32_t COMMIT_STRIPS = 16;

    NavigationView& nav_;
    RxRadioState radio_state_{137900000 /* frequency */, 1750000 /* bandwidth */, 2304000 /* sampling rate */};
    uint32_t composite_{0};
    app_settings::SettingsManager settings_{
        "rx_lrpt",
        app_settings::Mode::RX,
        {
            {"composite"sv, &composite_},
        }};

    bool stopping_{false};
    uint8_t flags_{0xFF};
    lrpt::Shared* shared_{nullptr};
    Decoder decoder_{on_strip, this};

    // 72k at each start: the 80k warning comes each time 80k is chosen in a run of the app
    lrpt::Mode mode_{lrpt::Mode::M72K};
    bool warned_80k_{false};
    File deint_file_{};
    bool deint_open_{false};
    Deinterleaver deint_{deint_write, deint_read, this};

    BMPFile bmp_{};
    bool recording_{false};
    std::string file_name_{};
    uint32_t rows_written_{0};
    uint32_t strips_uncommitted_{0};
    uint8_t chunk_[CHUNK_PIXELS * 3]{};

    uint32_t preview_rows_{0};  // image rows shown so far
    Color line_[PREVIEW_MAX_WIDTH]{};

    // Meteor-M N2-3 and N2-4
    OptionsField options_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        7,
        {
            {"137.900", 137900000},
            {"137.100", 137100000},
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

    Button button_record{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(7), UI_POS_HEIGHT(2)},
        "Start"};
    // carrier, code (Viterbi) and frame lock
    Text text_carrier{
        {UI_POS_X(8), UI_POS_Y(1), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "CAR"};
    Text text_code{
        {UI_POS_X(12), UI_POS_Y(1), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "VIT"};
    Text text_sync{
        {UI_POS_X(16), UI_POS_Y(1), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "SYN"};
    // 80k: the interleaver markers
    Text text_marker{
        {UI_POS_X(20), UI_POS_Y(1), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "MRK"};
    Text text_offset{
        {UI_POS_X(8), UI_POS_Y(2), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)},
        ""};
    Text text_frames{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(23), UI_POS_HEIGHT(1)},
        ""};
    Constellation constellation{
        {UI_POS_X_RIGHT(6), UI_POS_Y(1), 48, 48}};

    // which channels go to red, green and blue
    OptionsField options_composite{
        {UI_POS_X(0), UI_POS_Y(4)},
        4,
        {
            {"221", 0},
            {"321", 1},
            {"123", 2},
            {"Ch 1", 3},
            {"Ch 2", 4},
            {"Ch 3", 5},
            {"Ch 4", 6},
            {"Ch 5", 7},
            {"Ch 6", 8},
        }};
    OptionsField options_mode{
        {UI_POS_X(5), UI_POS_Y(4)},
        3,
        {
            {"72k", (int32_t)lrpt::Mode::M72K},
            {"80k", (int32_t)lrpt::Mode::M80K},
        }};
    Text text_record{
        {UI_POS_X(9), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)},
        ""};

    static void on_strip(void* context);
    static bool deint_write(void* context, uint32_t offset, const uint8_t* data, size_t length);
    static bool deint_read(void* context, uint32_t offset, uint8_t* data, size_t length);
    void set_mode(lrpt::Mode mode);
    void service();
    void on_status(const LrptRxStatusMessage& message);
    void on_frame(const LrptRxFrameMessage& message);
    void set_shared(lrpt::Shared* shared);
    void set_composite(uint32_t composite);
    void preview_strip();
    bool record_strip();
    void start();
    // partial: also the strip being decoded
    void stop(bool partial = true, bool ok = true);
    void show_record();
    Coord preview_top();
    Dim preview_height();

    MessageHandlerRegistration message_handler_status{
        Message::ID::LrptRxStatus,
        [this](Message* const p) {
            this->on_status(*static_cast<const LrptRxStatusMessage*>(p));
        }};
    MessageHandlerRegistration message_handler_block{
        Message::ID::LrptRxBlock,
        [this](Message* const p) {
            if (stopping_) return;
            set_shared(static_cast<const LrptRxBlockMessage*>(p)->shared);
            service();
        }};
    MessageHandlerRegistration message_handler_frame{
        Message::ID::LrptRxFrame,
        [this](Message* const p) {
            this->on_frame(*static_cast<const LrptRxFrameMessage*>(p));
        }};
};

}  // namespace ui::external_app::lrpt_rx

#endif /*__UI_LRPT_RX_H__*/
