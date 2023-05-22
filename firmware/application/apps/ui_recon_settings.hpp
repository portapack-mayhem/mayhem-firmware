/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef _UI_RECON_SETTINGS
#define _UI_RECON_SETTINGS

#include "serializer.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_tabview.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"

// maximum usable freq
#define MAX_UFREQ 7200000000

// 1Mhz helper
#ifdef OneMHz
#undef OneMHz
#endif
#define OneMHz 1000000

// modes
#define RECON_MATCH_CONTINUOUS 0
#define RECON_MATCH_SPARSE 1

// statistics update interval in ms (change here if the statistics API is changing it's pace)
#define STATS_UPDATE_INTERVAL 100

// maximum lock duration
#define RECON_MAX_LOCK_DURATION 9900

// default number of match to have a lock
#define RECON_DEF_NB_MATCH 3
#define RECON_MIN_LOCK_DURATION 100   // have to be >= and a multiple of STATS_UPDATE_INTERVAL
#define RECON_DEF_WAIT_DURATION 1000  // will be incremented/decremented by STATS_UPDATE_INTERVAL steps

// screen size helper
#define SCREEN_W 240
//#define SCREEN_H 320

// recon settings nb params
#define RECON_SETTINGS_NB_PARAMS 8

namespace ui {

class ReconSetupViewMain : public View {
   public:
    ReconSetupViewMain(NavigationView& nav, Rect parent_rect, std::string input_file, std::string output_file);
    void save(std::string& input_file, std::string& output_file);
    void focus() override;

   private:
    std::string _input_file = {"RECON"};
    std::string _output_file = {"RECON_RESULTS"};

    Button button_load_freqs{
        {1 * 8, 12, 18 * 8, 22},
        "select input file"};
    Text text_input_file{
        {1 * 8, 4 + 2 * 16, 18 * 8, 22},
        "RECON"};

    Button button_save_freqs{
        {1 * 8, 4 * 16 - 8, 18 * 8, 22},
        "select output file"};
    Button button_output_file{
        {1 * 8, 5 * 16 - 2, 18 * 8, 22},
        "RECON_RESULTS"};

    Checkbox checkbox_autosave_freqs{
        {1 * 8, 7 * 16 - 4},
        3,
        "autosave freqs"};

    Checkbox checkbox_autostart_recon{
        {1 * 8, 9 * 16 - 4},
        3,
        "autostart recon"};

    Checkbox checkbox_continuous{
        {1 * 8, 11 * 16 - 4},
        3,
        "continuous"};
    Checkbox checkbox_clear_output{
        {1 * 8, 13 * 16 - 4},
        3,
        "clear output at start"};
};

class ReconSetupViewMore : public View {
   public:
    ReconSetupViewMore(NavigationView& nav, Rect parent_rect, uint32_t _recon_lock_duration, uint32_t _recon_lock_nb_match, uint32_t _recon_match_mode);

    void save(uint32_t& recon_lock_duration, uint32_t& recon_lock_nb_match, uint32_t& recon_match_mode);

    void focus() override;

   private:
    uint32_t _recon_lock_duration = STATS_UPDATE_INTERVAL;
    uint32_t _recon_lock_nb_match = RECON_DEF_NB_MATCH;
    uint32_t _recon_match_mode = RECON_MATCH_CONTINUOUS;

    Checkbox checkbox_load_freqs{
        {1 * 8, 12},
        3,
        "input: load freqs"};

    Checkbox checkbox_load_ranges{
        {1 * 8, 42},
        3,
        "input: load ranges"};

    Checkbox checkbox_load_hamradios{
        {1 * 8, 72},
        3,
        "input: load hamradios"};

    Checkbox checkbox_update_ranges_when_recon{
        {1 * 8, 102},
        3,
        "auto update m-ranges"};

    NumberField field_recon_lock_duration{
        {1 * 8, 132},                                      // position X , Y
        4,                                                 // number of displayed digits (even empty)
        {STATS_UPDATE_INTERVAL, RECON_MAX_LOCK_DURATION},  // range of number
        STATS_UPDATE_INTERVAL,                             // rotary encoder increment
        ' ',                                               // filling character
        false                                              // can loop
    };

    Text text_recon_lock_duration{
        {1 * 8, 132, 22 * 8, 22},
        "    ms (lock duration)"};

    NumberField field_recon_lock_nb_match{
        {1 * 8, 162},
        4,
        {1, 99},
        1,
        ' ',
        false};

    Text text_recon_lock_nb{
        {1 * 8, 162, 25 * 8, 22},
        "    x (nb lock to match freq)"};

    OptionsField field_recon_match_mode{
        {1 * 8, 192},
        20,  // CONTINUOUS MATCH MODE / SPARSE TIMED MATCH MODE
        {
            {"SQL MATCH: CONTINOUS", 0},
            {"SQL MATCH: SPARSE", 1}}};
};

class ReconSetupView : public View {
   public:
    ReconSetupView(NavigationView& nav, std::string _input_file, std::string _output_file, uint32_t _recon_lock_duration, uint32_t _recon_lock_nb_match, uint32_t _recon_match_mode);

    std::function<void(std::vector<std::string> messages)> on_changed{};

    void focus() override;

    std::string title() const override { return "Recon setup"; };

   private:
    NavigationView& nav_;

    std::string input_file = {"RECON"};
    std::string output_file = {"RECON_RESULTS"};
    uint32_t recon_lock_duration = STATS_UPDATE_INTERVAL;
    uint32_t recon_lock_nb_match = RECON_DEF_NB_MATCH;
    uint32_t recon_match_mode = RECON_MATCH_CONTINUOUS;

    Rect view_rect = {0, 3 * 8, SCREEN_W, 230};

    ReconSetupViewMain viewMain{nav_, view_rect, input_file, output_file};
    ReconSetupViewMore viewMore{nav_, view_rect, recon_lock_duration, recon_lock_nb_match, recon_match_mode};

    TabView tab_view{
        {"Main", Color::cyan(), &viewMain},
        {"More", Color::green(), &viewMore}};

    Button button_save{
        {9 * 8, 255, 14 * 8, 40},
        "SAVE"};
};

} /* namespace ui */

#endif
