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

    static constexpr size_t desc_edit_max = 0x80;

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
        {{0, 2}, "Category:", Color::light_grey()}};

    OptionsField options_category{
        {9 * 8, 2},
        14 /* length */,
        {}};

    FreqManUIList freqlist_view{
        {0, 3 * 8, screen_width, 12 * 16 + 2 /* 2 Keeps text out of border. */}};

    Button button_exit{
        {15 * 8, 17 * 16, 15 * 8, 2 * 16},
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

   private:
    std::string temp_buffer_{};
    freqman_entry entry_{};

    void refresh_ui();

    BigFrequency big_display{
        {0, 2 * 16, 28 * 8, 4 * 16},
        0};

    Labels labels{
        {{0 * 8, 6 * 16}, "Description:", Color::white()}};

    Text text_description{{0 * 8, 7 * 16, 30 * 8, 1 * 16}};

    Button button_clear{
        {4 * 8, 10 * 16, 10 * 8, 2 * 16},
        "Clear"};

    Button button_edit{
        {16 * 8, 10 * 16, 10 * 8, 2 * 16},
        "Edit"};

    Button button_save{
        {0 * 8, 17 * 16, 15 * 8, 2 * 16},
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
        {23 * 8, 0 * 16, 7 * 4, 20},
        {},
        &bitmap_icon_new_file,
        Color::white(),
        true};

    NewButton button_del_category{
        {26 * 8 + 4, 0 * 16, 7 * 4, 20},
        {},
        &bitmap_icon_trash,
        Color::red(),
        true};

    Button button_edit_entry{
        {0 * 8, 14 * 16 - 4, 15 * 8, 1 * 16 + 4},
        "Edit"};

    Rectangle rect_padding{
        {15 * 8, 14 * 16 - 4, 15 * 8, 1 * 16 + 4},
        Color::grey()};

    Button button_edit_freq{
        {0 * 8, 15 * 16, 15 * 8, 2 * 16},
        "Frequency"};

    Button button_edit_desc{
        {0 * 8, 17 * 16, 15 * 8, 2 * 16},
        "Description"};

    NewButton button_add_entry{
        {15 * 8, 15 * 16, 7 * 8 + 4, 2 * 16},
        {},
        &bitmap_icon_add,
        Color::white(),
        true};

    NewButton button_del_entry{
        {22 * 8 + 4, 15 * 16, 7 * 8 + 4, 2 * 16},
        {},
        &bitmap_icon_delete,
        Color::red(),
        true};
};

class FrequencyEditView : public View {
   public:
    std::function<void(const freqman_entry&)> on_save{};

    FrequencyEditView(
        NavigationView& nav,
        freqman_entry entry);
    std::string title() const override { return "Freqman"; }

    void focus() override;

   private:
    NavigationView& nav_;
    freqman_entry entry_;
    std::string temp_buffer_{};

    Labels labels{
        {{0 * 8, 2 * 16}, "Entry Type :", Color::light_grey()},
        {{0 * 8, 3 * 16}, "Frequency A:", Color::light_grey()},
        {{0 * 8, 4 * 16}, "Frequency B:", Color::light_grey()},
        {{0 * 8, 5 * 16}, "Modulation :", Color::light_grey()},
        {{0 * 8, 6 * 16}, "Bandwidth  :", Color::light_grey()},
        {{0 * 8, 7 * 16}, "Step       :", Color::light_grey()},
        {{0 * 8, 8 * 16}, "Tone Freq  :", Color::light_grey()},
        {{0 * 8, 9 * 16}, "Description:", Color::light_grey()},
    };

    TextField field_description{
        {0 * 8, 10 * 16, 30 * 8, 1 * 16},
        ""};

    Button button_save{
        {0 * 8, 17 * 16, 15 * 8, 2 * 16},
        "Save"};

    Button button_cancel{
        {15 * 8, 17 * 16, 15 * 8, 2 * 16},
        "Cancel"};
};

} /* namespace ui */
