/*
 * Copyright (C) 2026 Rabbit-Labs LLC
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

#ifndef __UI_LCD_GAMMA_H__
#define __UI_LCD_GAMMA_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "portapack_persistent_memory.hpp"

namespace ui::external_app::lcd_gamma {

using portapack::persistent_memory::lcd_tuning_t;

/* Gray, R, G, B ramps plus primary/secondary bars. */
class TestPattern : public Widget {
   public:
    TestPattern(Rect parent_rect)
        : Widget{parent_rect} {}
    void paint(Painter& painter) override;
};

/* Full screen solid fills and ramps; encoder/left/right cycles, any other key exits. */
class FullscreenView : public View {
   public:
    FullscreenView(NavigationView& nav);
    ~FullscreenView();

    void focus() override;
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;

   private:
    NavigationView& nav_;
    int pattern_{0};
    void step(int d);
};

class LCDGammaView : public View {
   public:
    LCDGammaView(NavigationView& nav);
    ~LCDGammaView();

    std::string title() const override { return "LCD Gamma"; }
    void focus() override;

   private:
    NavigationView& nav_;

    lcd_tuning_t base_{};
    lcd_tuning_t work_{};
    bool advanced_{false};

    void load_preset(size_t index);
    void recompute();
    void apply();
    void set_advanced(bool advanced);
    void update_advanced_fields();
    void update_hex();
    uint8_t param_mask(int32_t index) const;
    uint8_t* param_ptr(int32_t index);
    void save();
    void export_file();
    void import_file();
    void show_status(const std::string& s);

    TestPattern pattern{{0, 0, 240, 96}};

    Labels labels_easy{
        {{UI_POS_X(0), UI_POS_Y(7.5)}, "VCOM offset:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(8.5)}, "Gamma low:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(9.5)}, "Gamma mid:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(10.5)}, "Gamma high:", Theme::getInstance()->fg_light->foreground},
    };

    OptionsField field_preset{
        {UI_POS_X(0), UI_POS_Y(6.25)},
        10,
        {{"Saved", 0}, {"Built-in", 1}, {"TN stock", 2}, {"IPS", 3}, {"Generic", 4}}};

    OptionsField field_mode{
        {UI_POS_X_RIGHT(9), UI_POS_Y(6.25)},
        9,
        {{"Easy", 0}, {"Advanced", 1}}};

    NumberField field_vcom{{UI_POS_X(13), UI_POS_Y(7.5)}, 3, {-64, 63}, 1, ' ', false};
    NumberField field_low{{UI_POS_X(13), UI_POS_Y(8.5)}, 3, {-32, 32}, 1, ' ', false};
    NumberField field_mid{{UI_POS_X(13), UI_POS_Y(9.5)}, 3, {-32, 32}, 1, ' ', false};
    NumberField field_high{{UI_POS_X(13), UI_POS_Y(10.5)}, 3, {-32, 32}, 1, ' ', false};

    Checkbox check_bgr{{UI_POS_X(0), UI_POS_Y(11.5)}, 12, "Swap red/blue", true};

    /* Advanced: params 0-14 E0h, 15-29 E1h, 30 C5h VMH, 31 C5h VML, 32 C7h. */
    NumberField field_param{{UI_POS_X(0), UI_POS_Y(7.5)}, 2, {0, 32}, 1, '0', true};
    Text text_param{{UI_POS_X(3), UI_POS_Y(7.5), UI_POS_WIDTH(14), UI_POS_HEIGHT(1)}, ""};
    NumberField field_value{{UI_POS_X_RIGHT(4), UI_POS_Y(7.5)}, 3, {0, 255}, 1, ' ', false};
    Text text_e0{{UI_POS_X(0), UI_POS_Y(8.5), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    Text text_e1{{UI_POS_X(0), UI_POS_Y(9.5), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    Text text_vcom{{UI_POS_X(0), UI_POS_Y(10.5), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};

    Text text_status{{UI_POS_X(0), UI_POS_Y(12.75), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};

    Button button_full{{UI_POS_X(0), UI_POS_Y(14), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Patterns"};
    Button button_save{{UI_POS_X(10), UI_POS_Y(14), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Save"};
    Button button_revert{{UI_POS_X(20), UI_POS_Y(14), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Revert"};
    Button button_export{{UI_POS_X(0), UI_POS_Y(16.25), UI_POS_WIDTH(15), UI_POS_HEIGHT(2)}, "Export SD"};
    Button button_import{{UI_POS_X(15), UI_POS_Y(16.25), UI_POS_WIDTH(15), UI_POS_HEIGHT(2)}, "Import SD"};
};

}  // namespace ui::external_app::lcd_gamma

#endif /*__UI_LCD_GAMMA_H__*/
