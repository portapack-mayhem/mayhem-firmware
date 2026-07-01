/*
 * Copyright (C) 2026 PortaPack Mayhem
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
 * along with this program; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __UI_VOR_RX_H__
#define __UI_VOR_RX_H__

#include "audio.hpp"
#include "baseband_api.hpp"
#include "file_path.hpp"
#include "log_file.hpp"
#include "rtc_time.hpp"
#include "portapack.hpp"
#include "ui.hpp"
#include "ui_freq_field.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"

namespace ui::external_app::vor_rx {

class VorLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void write_header();
    void log_status(const VorRxStatusDataMessage& message, uint16_t course_deg);

   private:
    LogFile log_file{};
};

class VorCdiIndicator : public Widget {
   public:
    VorCdiIndicator(Point position);

    void set_course(uint16_t course_deg);
    void set_radial(uint16_t radial_deg);
    void set_valid(bool valid);

    void paint(Painter& painter) override;

   private:
    static float normalize_signed_degrees(float degrees);

    uint16_t course_deg_{0};
    uint16_t radial_deg_{0};
    bool valid_{false};
};

class VorRxView : public View {
   public:
    VorRxView(NavigationView& nav);
    ~VorRxView();

    void focus() override;

    std::string title() const override { return "VOR RX"; }

   private:
    void start_receiver();
    void stop_receiver();
    void update_status();
    void on_vor_status(const VorRxStatusDataMessage& message);
    void update_cdi();
    void refresh_radial();
    uint16_t calibrated_radial(uint16_t radial_deg) const;
    void update_logging();

    NavigationView& nav_;
    bool running_{false};
    bool logging_{false};
    bool have_status_{false};
    uint16_t last_radial_deg_{0};
    bool last_valid_{false};
    bool last_to_from_{false};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};
    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    AudioVolumeField field_volume{
        {screen_width - 2 * 8, UI_POS_Y(0)}};

    NumberField field_course{
        {UI_POS_X(14), UI_POS_Y(4)},
        3,
        {0, 359},
        1,
        '0'};

    NumberField field_calibration{
        {UI_POS_X(14), UI_POS_Y(7)},
        4,
        {-180, 180},
        1,
        ' '};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Status:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "Band:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(3)}, "Next:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(4)}, "Course (OBS):", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(5)}, "Radial:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Flag:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(7)}, "Calib.:", Theme::getInstance()->fg_light->foreground}};

    Text text_status{
        {UI_POS_X(14), UI_POS_Y(1), UI_POS_WIDTH_REMAINING(14), UI_POS_HEIGHT(1)},
        "Idle"};
    Text text_band{
        {UI_POS_X(14), UI_POS_Y(2), UI_POS_WIDTH_REMAINING(14), UI_POS_HEIGHT(1)},
        "108.00-117.95MHz"};
    Text text_next{
        {UI_POS_X(14), UI_POS_Y(3), UI_POS_WIDTH_REMAINING(14), UI_POS_HEIGHT(1)},
        "Pending"};
    Text text_course_unit{
        {UI_POS_X(18), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(18), UI_POS_HEIGHT(1)},
        "deg"};
    Text text_radial{
        {UI_POS_X(14), UI_POS_Y(5), UI_POS_WIDTH_REMAINING(14), UI_POS_HEIGHT(1)},
        "--"};
    Text text_flag{
        {UI_POS_X(14), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(14), UI_POS_HEIGHT(1)},
        "--"};
    Text text_calib_unit{
        {UI_POS_X(19), UI_POS_Y(7), UI_POS_WIDTH_REMAINING(19), UI_POS_HEIGHT(1)},
        "deg"};

    Text text_cdi_title{
        {UI_POS_X(0), UI_POS_Y(8), UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)},
        "Course Deviation Indicator"};

    VorCdiIndicator cdi_indicator{
        {UI_POS_X(0), UI_POS_Y(9)}};

    std::unique_ptr<VorLogger> logger{};

    Button button_start_stop{
        {UI_POS_X(9), UI_POS_Y(13), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)},
        "Start"};

    Checkbox check_log{
        {UI_POS_X(13), UI_POS_Y(16)},
        3,
        "LOG",
        true};

    MessageHandlerRegistration message_handler_vor_status{
        Message::ID::VorRxStatusData,
        [this](const Message* p) {
            const auto message = *reinterpret_cast<const VorRxStatusDataMessage*>(p);
            on_vor_status(message);
        }};
};

}  // namespace ui::external_app::vor_rx

#endif  // __UI_VOR_RX_H__