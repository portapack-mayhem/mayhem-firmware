/*
 * copyleft 2024 sommermorgentraum
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

#ifndef __UI_METRONOME_H__
#define __UI_METRONOME_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "audio.hpp"
#include "ch.h"

namespace ui::external_app::metronome {

class MetronomeView : public View {
   public:
    MetronomeView(NavigationView& nav);
    ~MetronomeView();
    MetronomeView(const MetronomeView& other) = delete;
    MetronomeView& operator=(const MetronomeView& other) = delete;

    void focus() override;

    std::string title() const override { return "Metronome"; };

   private:
    NavigationView& nav_;

    void beep_accent_beat();    // e.g. 3 of 3/4 beat
    void beep_unaccent_beat();  // e.g. 4 of 3/4 beat
    void stop_play();
    void play();
    // void paint(Painter& painter) override;

    Thread* thread{nullptr};
    bool should_exit{false};
    static msg_t static_fn(void* arg);
    void run();

    bool playing_{false};
    uint32_t current_beat_{0};

    Labels labels{
        {{0 * 8, 1 * 16}, "BPM:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "Accent Beep Tune:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "Unaccent Beep Tune:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Rhythm:", Theme::getInstance()->fg_light->foreground},
        {{(sizeof("Rhythm:") + 1) * 8 + 4 * 8, 4 * 16}, "/", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Beep Flash Duration:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 6 * 16}, "Volume:", Theme::getInstance()->fg_light->foreground}};

    NumberField field_bpm{
        {(sizeof("BPM:") + 1) * 8, 1 * 16},
        4,
        {1, 1000},
        1,
        ' '};

    NumberField field_rythm_unaccent_time{// e.g. 3 in 3/4 beat
                                          {(sizeof("Rhythm:") + 1) * 8, 4 * 16},
                                          2,
                                          {1, 99},
                                          1,
                                          ' '};

    NumberField field_rythm_accent_time{// e.g. 4 in 3/4 beat
                                        {(sizeof("Rhythm:") + 1) * 8 + 5 * 8, 4 * 16},
                                        2,
                                        {1, 99},
                                        1,
                                        ' '};

    NumberField field_beep_flash_duration{
        {(sizeof("Beep Flash Duration:") + 1) * 8, 5 * 16},
        3,
        {10, 999},
        1,
        ' '};

    NumberField field_accent_beep_tune{
        {(sizeof("Accent Beep Tune:") + 1) * 8, 2 * 16},
        5,
        {380, 24000},
        20,
        ' '};

    NumberField field_unaccent_beep_tune{
        {(sizeof("Unaccent Beep Tune:") + 1) * 8, 3 * 16},
        5,
        {380, 24000},
        20,
        ' '};

    AudioVolumeField field_volume{
        {(sizeof("Volume:") + 1) * 8, 6 * 16}};

    NewButton button_play_stop{
        {0 * 16, 16 * 16, screen_width, screen_height - 16 * 16},
        {},
        &bitmap_icon_replay,
        Theme::getInstance()->fg_red->foreground};

    ProgressBar progressbar{
        {0 * 16, 8 * 16, screen_width, screen_height - 14 * 16}};
};

}  // namespace ui::external_app::metronome

#endif /*__UI_METRONOME_H__*/