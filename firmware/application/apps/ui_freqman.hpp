/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

#include "freqman.hpp"
#include "freqman_db.hpp"
#include "ui.hpp"
#include "ui_freqlist.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_receiver.hpp"
#include "ui_textentry.hpp"
#include "ui_widget.hpp"

namespace ui {

class FreqManBaseView : public View {
   public:
    FreqManBaseView(NavigationView& nav);

    void focus() override;

   protected:
    using options_t = OptionsField::options_t;

    NavigationView& nav_;
    freqman_error error_{NO_ERROR};
    std::function<void(void)> on_select_frequency{};

    void change_category(size_t new_index);
    /* Access the categories directly from the OptionsField.
     * This avoids holding multiple copies of the file list. */
    const options_t& categories() const { return options_category.options(); }
    const auto& current_category() const { return options_category.selected_index_name(); }
    auto current_index() const { return freqlist_view.get_index(); }
    freqman_entry current_entry() const { return db_[current_index()]; }
    void refresh_categories();
    void refresh_list(int delta_selected = 0);

    FreqmanDB db_{};

    /* The top section (category) is 20px tall. */
    Labels label_category{
        {{0, 2}, "F:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_category{
        {3 * 8, 2},
        19 /* length */,
        {}};

    FreqManUIList freqlist_view{
        {0, 3 * 8, screen_width, 12 * 16 + 2 /* 2 Keeps text out of border. */}};

    Button button_exit{
        {UI_POS_X_RIGHT(14), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(14), UI_POS_HEIGHT(2)},
        "Exit"};

   protected:
    /* Static so selected category is persisted across UI instances. */
    static size_t current_category_index;
};

// TODO: support for new category.
class FrequencySaveView : public FreqManBaseView {
   public:
    FrequencySaveView(NavigationView& nav, const rf::Frequency value);
    std::string title() const override { return "Save freq"; }
    void focus() override;

   private:
    freqman_entry entry_{};

    void refresh_ui();

    BigFrequency big_display{
        {0, 2 * 16, 28 * 8, 4 * 16},
        0};

    Labels labels{
        {{UI_POS_X(0), 6 * 16}, "Description:", Theme::getInstance()->bg_darkest->foreground}};

    TextField field_description{
        {UI_POS_X(0), 7 * 16, screen_width, 1 * 16},
        ""};

    Button button_save{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(14), UI_POS_HEIGHT(2)},
        "Save"};
};

class FrequencyLoadView : public FreqManBaseView {
   public:
    std::function<void(rf::Frequency)> on_frequency_loaded{};
    std::function<void(rf::Frequency, rf::Frequency)> on_range_loaded{};

    FrequencyLoadView(NavigationView& nav);
    std::string title() const override { return "Load freq"; }
};

class FrequencyManagerView : public FreqManBaseView {
   public:
    FrequencyManagerView(NavigationView& nav);
    std::string title() const override { return "Freqman"; }

   private:
    std::string temp_buffer_{};

    void on_edit_entry();
    void on_edit_freq();
    void on_edit_desc();
    void on_add_category();
    void on_del_category();
    void on_add_entry();
    void on_del_entry();

    NewButton button_add_category{
        {UI_POS_X_RIGHT(8), UI_POS_Y(0), 7 * 4, 20},
        {},
        &bitmap_icon_new_file,
        Theme::getInstance()->bg_darkest->foreground,
        true};

    NewButton button_del_category{
        {UI_POS_X_RIGHT(4), UI_POS_Y(0), 7 * 4, 20},
        {},
        &bitmap_icon_trash,
        Theme::getInstance()->fg_red->foreground,
        true};

    Button button_edit_entry{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(6.5) + 4, 15 * 8, 1 * 16 + 4},
        "Edit"};

    Rectangle rect_padding{
        {15 * 8, UI_POS_Y_BOTTOM(6.5), 15 * 8, 1 * 16 + 4},
        Theme::getInstance()->fg_medium->background};

    Button button_edit_freq{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(5), 15 * 8, 2 * 16},
        "Frequency"};

    Button button_edit_desc{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(3), 15 * 8, 2 * 16},
        "Description"};

    NewButton button_add_entry{
        {15 * 8, UI_POS_Y_BOTTOM(5), 7 * 8 + 4, 2 * 16},
        {},
        &bitmap_icon_add,
        Theme::getInstance()->bg_darkest->foreground,
        true};

    NewButton button_del_entry{
        {22 * 8 + 4, UI_POS_Y_BOTTOM(5), 7 * 8 + 4, 2 * 16},
        {},
        &bitmap_icon_delete,
        Theme::getInstance()->fg_red->foreground,
        true};
};

class FrequencyEditView : public View {
   public:
    std::function<void(const freqman_entry&)> on_save{};

    FrequencyEditView(
        NavigationView& nav,
        freqman_entry entry);
    std::string title() const override { return "Freqman Edit"; }

    void focus() override;

   private:
    NavigationView& nav_;
    freqman_entry entry_;
    std::string temp_buffer_{};

    void refresh_ui();
    void populate_bandwidth_options();
    void populate_step_options();
    void populate_tone_options();

    Labels labels{
        {{5 * 8, 1 * 16}, "Edit Frequency Entry", Theme::getInstance()->bg_darkest->foreground},
        {{UI_POS_X(0), 3 * 16}, "Entry Type :", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 4 * 16}, "Frequency A:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 5 * 16}, "Frequency B:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 6 * 16}, "Modulation :", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 7 * 16}, "Bandwidth  :", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 8 * 16}, "Step       :", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 9 * 16}, "Tone Freq  :", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 10 * 16}, "Description:", Theme::getInstance()->fg_light->foreground},
    };

    OptionsField field_type{
        {13 * 8, 3 * 16},
        8,
        {
            {"Single", 0},
            {"Range", 1},
            {"HamRadio", 2},
            {"Repeater", 3},
            {"Raw", 4},
        }};

    FrequencyField field_freq_a{{13 * 8, 4 * 16}};

    FrequencyField field_freq_b{{13 * 8, 5 * 16}};

    OptionsField field_modulation{{13 * 8, 6 * 16}, 5, {}};

    OptionsField field_bandwidth{{13 * 8, 7 * 16}, 7, {}};

    OptionsField field_step{{13 * 8, 8 * 16}, 12, {}};

    OptionsField field_tone{{13 * 8, 9 * 16}, 13, {}};

    TextField field_description{
        {13 * 8, 10 * 16, 17 * 8, 1 * 16},
        {}};

    Text text_validation{
        {12 * 8, 12 * 16, 5 * 8, 1 * 16},
        {}};

    Button button_save{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(3), 15 * 8, 2 * 16},
        "Save"};

    Button button_cancel{
        {UI_POS_X_RIGHT(15), UI_POS_Y_BOTTOM(3), 15 * 8, 2 * 16},
        "Cancel"};
};

} /* namespace ui */

void freqman_set_bandwidth_option(freqman_index_t modulation, ui::OptionsField& option);
void freqman_set_modulation_option(ui::OptionsField& option);
void freqman_set_step_option(ui::OptionsField& option);
void freqman_set_step_option_short(ui::OptionsField& option);
void freqman_set_tone_option(ui::OptionsField& option);
