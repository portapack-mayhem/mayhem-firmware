/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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
#include "ui_transmitter.hpp"
#include "ui_textentry.hpp"
#include "ui_tabview.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "rds.hpp"

using namespace rds;

namespace ui {

class RDSPSNView : public OptionTabView {
   public:
    RDSPSNView(NavigationView& nav, Rect parent_rect);

    std::string PSN{"TEST1234"};
    bool mono_stereo{false};
    bool TA{false};
    bool MS{false};

   private:
    Labels labels{
        {{1 * 8, 3 * 8}, "Program Service Name", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 7 * 8}, "PSN:", Theme::getInstance()->fg_light->foreground}};

    Button button_set{
        {18 * 8, 3 * 16, 80, 32},
        "Set"};
    Text text_psn{
        {6 * 8, 3 * 16 + 8, 8 * 8, 16},
        ""};

    Checkbox check_mono_stereo{
        {2 * 8, 12 * 8},
        6,
        "Stereo"};
    Checkbox check_MS{
        {14 * 8, 12 * 8},
        5,
        "Music"};
    Checkbox check_TA{
        {2 * 8, 16 * 8},
        20,
        "Traffic announcement"};
};

class RDSRadioTextView : public OptionTabView {
   public:
    RDSRadioTextView(NavigationView& nav, Rect parent_rect);

    std::string radiotext{"Radiotext test ABCD1234"};

   private:
    Labels labels{
        {{2 * 8, 3 * 8}, "Radiotext", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 6 * 8}, "Text:", Theme::getInstance()->fg_light->foreground}};

    Text text_radiotext{
        {1 * 8, 4 * 16, 28 * 8, 16},
        "-"};
    Button button_set{
        {88, 6 * 16, 64, 32},
        "Set"};
};

class RDSDateTimeView : public OptionTabView {
   public:
    RDSDateTimeView(Rect parent_rect);

   private:
    Labels labels{
        {{44, 5 * 16}, "Not yet implemented", Theme::getInstance()->error_dark->foreground}};
};

class RDSAudioView : public OptionTabView {
   public:
    RDSAudioView(Rect parent_rect);

   private:
    Labels labels{
        {{44, 5 * 16}, "Not yet implemented", Theme::getInstance()->error_dark->foreground}};
};

class RDSThread {
   public:
    RDSThread(std::vector<RDSGroup>** frames);
    ~RDSThread();

    RDSThread(const RDSThread&) = delete;
    RDSThread(RDSThread&&) = delete;
    RDSThread& operator=(const RDSThread&) = delete;
    RDSThread& operator=(RDSThread&&) = delete;

   private:
    std::vector<RDSGroup>** frames_{};
    Thread* thread{nullptr};

    static msg_t static_fn(void* arg);

    void run();
};

class RDSView : public View {
   public:
    RDSView(NavigationView& nav);
    ~RDSView();

    RDSView(const RDSView&) = delete;
    RDSView(RDSView&&) = delete;
    RDSView& operator=(const RDSView&) = delete;
    RDSView& operator=(RDSView&&) = delete;

    void focus() override;

    std::string title() const override { return "RDS TX"; };

   private:
    NavigationView& nav_;
    RDS_flags rds_flags{};

    TxRadioState radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        2280000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_rds", app_settings::Mode::TX};

    std::vector<RDSGroup> frame_psn{};
    std::vector<RDSGroup> frame_radiotext{};
    std::vector<RDSGroup> frame_datetime{};
    std::vector<RDSGroup>* frames[3]{&frame_psn, &frame_radiotext, &frame_datetime};

    bool txing = false;

    uint16_t message_length{0};

    void start_tx();

    Rect view_rect = {0, 8 * 8, 240, 192};

    RDSPSNView view_PSN{nav_, view_rect};
    RDSRadioTextView view_radiotext{nav_, view_rect};
    RDSDateTimeView view_datetime{view_rect};
    RDSAudioView view_audio{view_rect};

    TabView tab_view{
        {"Name", Theme::getInstance()->fg_cyan->foreground, &view_PSN},
        {"Text", Theme::getInstance()->fg_green->foreground, &view_radiotext},
        {"Time", Theme::getInstance()->fg_yellow->foreground, &view_datetime},
        {"Audio", Theme::getInstance()->fg_orange->foreground, &view_audio}};

    Labels labels{
        {{0 * 8, 28}, "Program type:", Theme::getInstance()->fg_light->foreground},
        //{ { 14 * 8, 16 + 8 }, "CC:", Theme::getInstance()->fg_light->foreground },
        {{2 * 8, 28 + 16}, "Program ID:", Theme::getInstance()->fg_light->foreground},
        //{ { 13 * 8, 32 + 8 }, "Cov:",Theme::getInstance()->fg_light->foreground },
    };

    OptionsField options_pty{
        {13 * 8, 28},
        8,
        {{"None", 0},
         {"News", 1},
         {"Affairs", 2},
         {"Info", 3},
         {"Sport", 4},
         {"Educate", 5},
         {"Drama", 6},
         {"Culture", 7},
         {"Science", 8},
         {"Varied", 9},
         {"Pop", 10},
         {"Rock", 11},
         {"Easy", 12},
         {"Light", 13},
         {"Classics", 14},
         {"Other", 15},
         {"Weather", 16},
         {"Finance", 17},
         {"Children", 18},
         {"Social", 19},
         {"Religion", 20},
         {"PhoneIn", 21},
         {"Travel", 22},
         {"Leisure", 23},
         {"Jazz", 24},
         {"Country", 25},
         {"National", 26},
         {"Oldies", 27},
         {"Folk", 28},
         {"Docs", 29},
         {"AlarmTst", 30},
         {"Alarm", 31}}};

    /*OptionsField options_countrycode {
                { 17 * 8, 16 + 8 },
                11,
                {
                        { "Albania", 	9 },
                        { "Algeria", 	2 },
                        { "Andorra", 	3 },
                        { "Austria", 	10 },
                        { "Azores", 	8 },
                        { "Belgium", 	6 },
                        { "Belarus", 	15 },
                        { "Bosnia", 	15 },
                        { "Bulgaria", 	8 },
                        { "Canaries", 	14 },
                        { "Croatia", 	12 },
                        { "Cyprus", 	2 },
                        { "Czech-Rep", 	2 },
                        { "Denmark", 	9 },
                        { "Egypt", 		15 },
                        { "Estonia", 	2 },
                        { "Faroe", 		9 },
                        { "Finland", 	6 },
                        { "France", 	15 },
                        { "Germany 1", 	13 },
                        { "Germany 2", 	1 },
                        { "Gibraltar", 	10 },
                        { "Greece", 	1 },
                        { "Hungary", 	11 },
                        { "Iceland", 	10 },
                        { "Iraq", 		11 },
                        { "Ireland", 	2 },
                        { "Israel", 	4 },
                        { "Italy", 		5 },
                        { "Jordan", 	5 },
                        { "Latvia", 	9 },
                        { "Lebanon", 	10 },
                        { "Libya", 		13 },
                        { "Liechtenst.", 9 },
                        { "Lithuania", 	12 },
                        { "Luxembourg", 7 },
                        { "Macedonia", 	4 },
                        { "Madeira", 	8 },
                        { "Malta", 		12 },
                        { "Moldova", 	1 },
                        { "Monaco", 	11 },
                        { "Morocco", 	1 },
                        { "Netherlands", 8 },
                        { "Norway", 	15 },
                        { "Palestine", 	8 },
                        { "Poland", 	3 },
                        { "Portugal", 	8 },
                        { "Romania", 	14 },
                        { "Russia", 	7 },
                        { "San Marino", 3 },
                        { "Slovakia", 	5 },
                        { "Slovenia", 	9 },
                        { "Spain", 		14 },
                        { "Sweden", 	14 },
                        { "Switzerland", 4 },
                        { "Syria", 		6 },
                        { "Tunisia", 	7 },
                        { "Turkey", 	3 },
                        { "Ukraine", 	6 },
                        { "U.K.", 		12 },
                        { "Vatican", 	4 },
                        { "Yugoslavia", 13 }
                }
        };*/

    SymField sym_pi_code{
        {13 * 8, 28 + 16},
        4,
        SymField::Type::Hex};

    /*OptionsField options_coverage {
                { 17 * 8, 32 + 8 },
                12,
                {
                        { "Local", 0 },
                        { "Internat.", 1 },
                        { "National", 2 },
                        { "Sup-regional", 3 },
                        { "R11", 4 },
                        { "R12", 5 },
                        { "R13", 6 },
                        { "R14", 7 },
                        { "R15", 8 },
                        { "R16", 9 },
                        { "R17", 10 },
                        { "R18", 11 },
                        { "R19", 12 },
                        { "R110", 13 },
                        { "R111", 14 },
                        { "R112", 15 }
                }
        };*/

    Checkbox check_TP{
        {23 * 8, 4 * 8},
        2,
        "TP"};

    TransmitterView tx_view{
        16 * 16,
        50000,
        9};

    std::unique_ptr<RDSThread> tx_thread{};
};

} /* namespace ui */
