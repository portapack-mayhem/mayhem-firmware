/*
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

#ifndef __UI_AUDIO_TEST_H__
#define __UI_AUDIO_TEST_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"

namespace ui {

class AudioTestView : public View {
   public:
    AudioTestView(NavigationView& nav);
    ~AudioTestView();
    AudioTestView(const AudioTestView& other) = delete;
    AudioTestView& operator=(const AudioTestView& other) = delete;

    void focus() override;
    void update_audio_beep();

    std::string title() const override { return "Audio Test"; };

   private:
    NavigationView& nav_;
    bool beep{false};

    Labels labels{
        {{7 * 8, 3 * 16}, "Audio Beep Test", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 6 * 16}, "Sample Rate (Hz):", Theme::getInstance()->fg_light->foreground},
        {{25 * 8, 7 * 16}, "Step:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 8 * 16}, "Frequency (Hz):", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 10 * 16}, "Duration (ms):", Theme::getInstance()->fg_light->foreground},
        {{25 * 8, 10 * 16}, "0=con", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 12 * 16}, "Volume:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_sample_rate{
        {18 * 8, 6 * 16},
        5,
        {{"24000", 24000},
         {"48000", 48000},
         {"12000", 12000}}};

    OptionsField options_step{
        {26 * 8, 8 * 16},
        4,
        {{"   1", 1},
         {"  10", 10},
         {" 100", 100},
         {"1000", 1000}}};

    NumberField field_frequency{
        {18 * 8, 8 * 16},
        5,
        {},
        100,
        ' ',
        true};

    NumberField field_duration{
        {18 * 8, 10 * 16},
        5,
        {0, 60000},
        50,
        ' ',
        true};

    AudioVolumeField field_volume{
        {21 * 8, 12 * 16}};

    ImageToggle toggle_speaker{
        {21 * 8, 14 * 16, 2 * 8, 1 * 16},
        &bitmap_icon_speaker_mute,
        &bitmap_icon_speaker,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background,
        Theme::getInstance()->fg_green->foreground,
        Theme::getInstance()->bg_dark->background};
};

} /* namespace ui */

#endif /*__UI_AUDIO_TEST_H__*/
