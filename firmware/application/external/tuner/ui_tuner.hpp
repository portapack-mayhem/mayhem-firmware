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

#ifndef __UI_TUNER_H__
#define __UI_TUNER_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "audio.hpp"

namespace ui::external_app::tuner {

struct Instrument {
    std::string name;
    struct NoteInfo {
        uint16_t frequency;
        audio::Rate sample_rate;  // tune samplerate to allow for freqs
        int8_t octave_shift;
        // PP hardware can't handle some extremely low/high frequencies, this indicates how much the freq that were played is shifted up/down,
        // for example, if shift is -2 and note is A3, it means the real string should play A5 but PP's speaker plays A3
    };
    std::map<std::string, NoteInfo> notes;  // this is for fast looking : O(log(n)) ,  usage `auto note = guitar.notes["A4"];`
};

class TunerView : public View {
   public:
    TunerView(NavigationView& nav);
    ~TunerView();
    TunerView(const TunerView& other) = delete;
    TunerView& operator=(const TunerView& other) = delete;

    void focus() override;
    void update_audio_beep();

    std::string title() const override { return "Tuner"; };

   private:
    NavigationView& nav_;
    bool beep{false};

    void update_notes_for_instrument(const Instrument& instrument);
    void play_change_note();
    void stop_play();
    void update_current_note();
    uint32_t protected_sample_rate(audio::Rate r);
    void paint(Painter& painter) override;

    uint16_t current_note_frequency{440};
    audio::Rate current_note_sample_rate{audio::Rate::Hz_12000};
    int8_t current_note_octave_shift{0};
    bool current_note_playing{false};

    Labels labels{
        {{0 * 8, 1 * 16}, "Instrument:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "Note:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "Note Frequency:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Note Octave Shift:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Volume:", Theme::getInstance()->fg_light->foreground}};

    Text text_note_frequency{
        {(sizeof("Note Frequency:") + 1) * 8, 3 * 16, screen_width - (sizeof("Note Frequency:") + 1) * 8, 16},
        "",
    };

    Text text_note_octave_shift{
        {(sizeof("Note Octave Shift:") + 1) * 8, 4 * 16, screen_width - (sizeof("Note Octave Shift:") + 1) * 8, 16},
        "",
    };

    AudioVolumeField field_volume{
        {(sizeof("Volume:") + 1) * 8, 5 * 16}};

    // TODO: load list runtime
    OptionsField options_instrument{
        {(sizeof("Instrument:") + 1) * 8, 1 * 16},
        screen_width - (sizeof("Instrument:") + 1) * 8,
        {{"Guitar", 0},
         {"Violin", 1},
         {"Pitch Fork", 2}}};

    OptionsField options_note{
        {(sizeof("Note:") + 1) * 8, 2 * 16},
        screen_width - (sizeof("Note:") + 1) * 8,
        {}};

    NewButton button_play_stop{
        {0 * 8, 16 * 16, screen_width, screen_height - 16 * 16},
        {},
        &bitmap_icon_replay,
        Theme::getInstance()->fg_red->foreground};

    // TODO: load from file DB
    const Instrument guitar = {
        .name = "Folk Guitar",
        .notes = {
            {"E2", {165, audio::Rate::Hz_12000, -1}},  // actual: E2, PP speaker: E3
            {"A2", {110, audio::Rate::Hz_12000, 0}},
            {"D3", {147, audio::Rate::Hz_12000, 0}},
            {"G3", {196, audio::Rate::Hz_24000, 0}},
            {"B3", {247, audio::Rate::Hz_24000, 0}},
            {"E4", {330, audio::Rate::Hz_24000, 0}}}};

    const Instrument violin = {
        .name = "Violin 440 Standard, 12ET",
        .notes = {
            {"G3", {196, audio::Rate::Hz_12000, 0}},
            {"D4", {294, audio::Rate::Hz_24000, 0}},
            {"A4", {440, audio::Rate::Hz_48000, 0}},
            {"E5", {659, audio::Rate::Hz_48000, 0}}}};

    const Instrument pitch_fork = {// freq copied from flipper app
                                   .name = "Pitch Fork",
                                   .notes = {
                                       {"12ET A4", {440, audio::Rate::Hz_48000, 0}},
                                       {"Sarti's A4", {436, audio::Rate::Hz_48000, 0}},
                                       {"1858 A4", {435, audio::Rate::Hz_48000, 0}},
                                       {"Verdi's A4", {432, audio::Rate::Hz_48000, 0}}}};

    std::map<std::string, Instrument> instruments{
        {"Guitar", guitar},
        {"Violin", violin},
        {"Pitch Fork", pitch_fork}};
};

}  // namespace ui::external_app::tuner

#endif /*__UI_TUNER_H__*/
