/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2019 Elia Yehuda (z4ziggy)
 * Copyright (C) 2024 u-foka
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

#ifndef __UI_BTNGRID_H__
#define __UI_BTNGRID_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "bitmap.hpp"
#include "signal.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// file used for listing apps to hide from menu
#define BLACKLIST u"/SETTINGS/blacklist"

namespace ui {

struct GridItem {
    std::string text;
    ui::Color color;
    const Bitmap* bitmap;
    std::function<void(void)> on_select;

    // TODO: Prevent default-constructed GridItems.
};

void load_blacklist();

class BtnGridView : public View {
   public:
    BtnGridView(Rect new_parent_rect = {0, 0, screen_width, screen_height - 16}, bool keep_highlight = false);

    ~BtnGridView();

    void add_items(std::initializer_list<GridItem> new_items, bool inhibit_update = false);
    void add_item(const GridItem& new_item, bool inhibit_update = false);
    void insert_item(const GridItem& new_item, size_t position, bool inhibit_update = false);
    void set_max_rows(int rows);
    int rows();
    void clear();

    NewButton* item_view(size_t index) const;

    bool show_arrows{true};  // flag used to hide arrows in main menu
    void show_arrows_enabled(bool enabled);

    bool set_highlighted(int32_t new_value);
    uint32_t highlighted_index();

    void set_parent_rect(const Rect new_parent_rect) override;
    bool arrow_up_enabled{false};
    bool arrow_down_enabled{false};
    void set_arrow_up_enabled(bool enabled);
    void set_arrow_down_enabled(bool enabled);
    void show_hide_arrows();
    void on_focus() override;
    void on_blur() override;
    void on_show() override;
    void on_hide() override;
    bool on_key(const KeyEvent event) override;
    bool on_encoder(const EncoderEvent event) override;
    bool blacklisted_app(GridItem new_item);

    void update_items();
    void set_btn_min_max_height(uint8_t min, uint8_t max) {
        btn_h_min = min;
        btn_h_max = max;
    }

   protected:
    virtual void on_populate() = 0;

   private:
    int rows_{3};
    uint8_t btn_h_min{40};
    uint8_t btn_h_max{60};
    bool keep_highlight{false};

    std::vector<GridItem> menu_items{};
    std::vector<std::unique_ptr<NewButton>> menu_item_views{};

    Button button_pgup{
        {0, 1324, 120, 16},
        "       "};

    Button button_pgdown{
        {121, 1324, 119, 16},
        "         "};

    int button_w = screen_width / rows_;
    int button_h = 48;
    size_t displayed_max{0};
    size_t highlighted_item{0};
    size_t offset{0};
};

} /* namespace ui */

#endif /*__UI_BTNGRID_H__*/
