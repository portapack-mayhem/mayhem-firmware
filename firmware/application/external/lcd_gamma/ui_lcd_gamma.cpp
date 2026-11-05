/*
 * Copyright (C) 2026 TehRabbitt, Rabbit-Labs LLC
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

#include "ui_lcd_gamma.hpp"

#include "portapack.hpp"
#include "lcd_ili9341.hpp"
#include "lcd_tuning_file.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"

#include <cstring>

extern ui::SystemView* system_view_ptr;

using namespace portapack;
namespace pmem = portapack::persistent_memory;

namespace ui::external_app::lcd_gamma {

static constexpr size_t GLEN = lcd::GAMMA_LEN;

static const char* const field_names[GLEN] = {"V63", "V62", "V61", "V59", "V57", "V50", "V43", "V27|V36",
                                              "V20", "V13", "V6", "V4", "V2", "V1", "V0"};

// Commonly used ILI9341 init gamma (Adafruit / TFT_eSPI style).
static constexpr uint8_t generic_pos[GLEN] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                                              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
static constexpr uint8_t generic_neg[GLEN] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                                              0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};

static Color gray(uint8_t v) {
    return Color(v, v, v);
}

static void draw_ramp(Painter& painter, Rect r, int steps, uint8_t rm, uint8_t gm, uint8_t bm) {
    for (int i = 0; i < steps; i++) {
        const Coord x0 = r.left() + (r.width() * i) / steps;
        const Coord x1 = r.left() + (r.width() * (i + 1)) / steps;
        const uint8_t v = (255 * i) / (steps - 1);
        painter.fill_rectangle({x0, r.top(), x1 - x0, r.height()}, Color(v & rm, v & gm, v & bm));
    }
}

static void draw_bars(Painter& painter, Rect r) {
    static const Color bars[8] = {Color::black(), Color::white(), Color::red(), Color::green(),
                                  Color::blue(), Color::cyan(), Color::magenta(), Color::yellow()};
    for (int i = 0; i < 8; i++) {
        const Coord x0 = r.left() + (r.width() * i) / 8;
        const Coord x1 = r.left() + (r.width() * (i + 1)) / 8;
        painter.fill_rectangle({x0, r.top(), x1 - x0, r.height()}, bars[i]);
    }
}

/* TestPattern */

void TestPattern::paint(Painter& painter) {
    const auto r = screen_rect();
    const Dim h = r.height() / 6;
    draw_ramp(painter, {r.left(), r.top(), r.width(), h * 2}, 32, 0xFF, 0xFF, 0xFF);
    draw_ramp(painter, {r.left(), r.top() + h * 2, r.width(), h}, 32, 0xFF, 0, 0);
    draw_ramp(painter, {r.left(), r.top() + h * 3, r.width(), h}, 32, 0, 0xFF, 0);
    draw_ramp(painter, {r.left(), r.top() + h * 4, r.width(), h}, 32, 0, 0, 0xFF);
    draw_bars(painter, {r.left(), r.top() + h * 5, r.width(), r.height() - h * 5});
}

/* FullscreenView */

static constexpr int fullscreen_patterns = 9;

FullscreenView::FullscreenView(NavigationView& nav)
    : nav_{nav} {
    set_focusable(true);
    system_view_ptr->set_app_fullscreen(true);
}

FullscreenView::~FullscreenView() {
    system_view_ptr->set_app_fullscreen(false);
    system_view_ptr->set_dirty();
}

void FullscreenView::focus() {
    View::focus();
}

void FullscreenView::paint(Painter& painter) {
    const Rect r{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT};
    switch (pattern_) {
        case 0:
            draw_ramp(painter, r, 32, 0xFF, 0xFF, 0xFF);
            break;
        case 1:
            draw_ramp(painter, r, 8, 0xFF, 0xFF, 0xFF);
            break;
        case 2:
            painter.fill_rectangle(r, Color::white());
            break;
        case 3:
            painter.fill_rectangle(r, Color::black());
            break;
        case 4:
            painter.fill_rectangle(r, gray(128));
            break;
        case 5:
            painter.fill_rectangle(r, Color::red());
            break;
        case 6:
            painter.fill_rectangle(r, Color::green());
            break;
        case 7:
            painter.fill_rectangle(r, Color::blue());
            break;
        default:
            draw_bars(painter, r);
            break;
    }
}

void FullscreenView::step(int d) {
    pattern_ = (pattern_ + d + fullscreen_patterns) % fullscreen_patterns;
    set_dirty();
}

bool FullscreenView::on_key(const KeyEvent key) {
    if (key == KeyEvent::Right || key == KeyEvent::Down)
        step(1);
    else if (key == KeyEvent::Left || key == KeyEvent::Up)
        step(-1);
    else
        nav_.pop();
    return true;
}

bool FullscreenView::on_encoder(const EncoderEvent delta) {
    step(delta > 0 ? 1 : -1);
    return true;
}

bool FullscreenView::on_touch(const TouchEvent event) {
    if (event.type == TouchEvent::Type::Start) step(1);
    return true;
}

/* LCDGammaView */

LCDGammaView::LCDGammaView(NavigationView& nav)
    : nav_{nav} {
    add_children({&pattern, &field_preset, &field_mode,
                  &labels_easy, &field_vcom, &field_low, &field_mid, &field_high, &check_bgr,
                  &field_param, &text_param, &field_value, &text_e0, &text_e1, &text_vcom,
                  &text_status,
                  &button_full, &button_save, &button_revert, &button_reset});

    lcd::ILI9341::active_tuning(base_);
    work_ = base_;
    field_preset.set_selected_index(0, false);
    field_mode.set_selected_index(0, false);

    field_preset.on_change = [this](size_t index, OptionsField::value_t) {
        load_preset(index);
    };
    field_mode.on_change = [this](size_t, OptionsField::value_t v) {
        set_advanced(v == 1);
    };

    const auto easy_change = [this](int32_t) { recompute(); };
    field_vcom.on_change = easy_change;
    field_low.on_change = easy_change;
    field_mid.on_change = easy_change;
    field_high.on_change = easy_change;
    check_bgr.on_select = [this](Checkbox&, bool) { recompute(); };

    field_param.on_change = [this](int32_t) { update_advanced_fields(); };
    field_value.on_change = [this](int32_t v) {
        uint8_t* p = param_ptr(field_param.value());
        const int32_t idx = field_param.value();
        uint8_t nv = static_cast<uint8_t>(v) & param_mask(idx);
        if (idx == 32) nv |= 0x80;  // nVM=1: use VMF from register
        if (*p == nv) return;
        *p = nv;
        update_hex();
        apply();
    };

    button_full.on_select = [this](Button&) { nav_.push<FullscreenView>(); };
    button_save.on_select = [this](Button&) { save(); };
    button_revert.on_select = [this](Button&) {
        field_preset.set_selected_index(0, false);
        load_preset(0);
        show_status("Reverted to saved");
    };
    button_reset.on_select = [this](Button&) { reset_to_default(); };

    text_status.set_style(Theme::getInstance()->fg_yellow);

    load_preset(0);
    set_advanced(false);

    panel_tuning_t file_tuning;
    bool file_ips = false;
    if (portapack::device_type != portapack::DeviceType::DEV_PORTAPACK)
        show_status("Only for 240x320 panels");
    else if (lcd_tuning_file::read(file_tuning, file_ips) == lcd_tuning_file::Status::Invalid)
        show_status("Bad lcd_gamma.txt, Reset deletes it");
    else
        show_status(pmem::config_lcd_ips() ? "Mode: IPS" : "Mode: TN");
}

LCDGammaView::~LCDGammaView() {
    // Drop any unsaved changes.
    portapack::display.apply_panel_config();
}

void LCDGammaView::focus() {
    field_preset.focus();
}

void LCDGammaView::show_status(const std::string& s) {
    text_status.set(s);
}

void LCDGammaView::load_preset(size_t index) {
    switch (index) {
        case 0:
            lcd::ILI9341::active_tuning(base_);
            break;
        case 1:
            lcd::ILI9341::default_tuning(pmem::config_lcd_ips(), base_);
            break;
        case 2:
            lcd::ILI9341::default_tuning(false, base_);
            break;
        case 3:
            lcd::ILI9341::default_tuning(true, base_);
            break;
        default:
            lcd::ILI9341::default_tuning(true, base_);
            memcpy(base_.gamma_pos, generic_pos, GLEN);
            memcpy(base_.gamma_neg, generic_neg, GLEN);
            base_.vcom1[0] = 0x3E;
            base_.vcom1[1] = 0x28;
            base_.vcom2 = 0x86;
            break;
    }
    if (index != 0) base_.flags = 0;

    field_vcom.set_value((base_.vcom2 & 0x7F) - 64, false);
    field_low.set_value(0, false);
    field_mid.set_value(0, false);
    field_high.set_value(0, false);
    check_bgr.set_value(base_.flags & lcd::TUNING_BGR_SWAP);
    if (advanced_) {
        work_ = base_;
        update_advanced_fields();
        update_hex();
        apply();
    } else {
        recompute();
    }
}

/* Easy mode: offsets on top of base_. Positive and negative tables are moved in
 * opposite directions so their symmetry is kept. Offsets scale with field width. */
static uint8_t offset_field(uint8_t v, uint8_t mask, int32_t d) {
    if (mask == 0xFF) {
        int32_t hi = (v >> 4) + d / 4;
        int32_t lo = (v & 0x0F) + d / 4;
        hi = hi < 0 ? 0 : (hi > 15 ? 15 : hi);
        lo = lo < 0 ? 0 : (lo > 15 ? 15 : lo);
        return (hi << 4) | lo;
    }
    int32_t n = v + (d * (mask + 1)) / 64;
    if (n < 0) n = 0;
    if (n > mask) n = mask;
    return n;
}

void LCDGammaView::recompute() {
    if (advanced_) return;
    work_ = base_;
    for (size_t i = 1; i < GLEN - 1; i++) {
        int32_t d = 0;
        if (i <= 4)
            d = field_high.value();
        else if (i <= 8)
            d = field_mid.value();
        else
            d = field_low.value();
        const uint8_t m = lcd::ILI9341::gamma_field_mask[i];
        work_.gamma_pos[i] = offset_field(base_.gamma_pos[i], m, d);
        work_.gamma_neg[i] = offset_field(base_.gamma_neg[i], m, -d);
    }
    work_.vcom2 = 0x80 | ((field_vcom.value() + 64) & 0x7F);
    if (work_.vcom2 == (base_.vcom2 | 0x80) && !(base_.vcom2 & 0x80)) work_.vcom2 = base_.vcom2;
    if (check_bgr.value())
        work_.flags |= lcd::TUNING_BGR_SWAP;
    else
        work_.flags &= ~lcd::TUNING_BGR_SWAP;
    update_hex();
    apply();
}

void LCDGammaView::apply() {
    portapack::display.apply_tuning(work_);
}

void LCDGammaView::set_advanced(bool advanced) {
    advanced_ = advanced;
    labels_easy.hidden(advanced);
    field_vcom.hidden(advanced);
    field_low.hidden(advanced);
    field_mid.hidden(advanced);
    field_high.hidden(advanced);
    check_bgr.hidden(advanced);
    field_param.hidden(!advanced);
    text_param.hidden(!advanced);
    field_value.hidden(!advanced);
    text_e0.hidden(!advanced);
    text_e1.hidden(!advanced);
    text_vcom.hidden(!advanced);
    if (!advanced) {
        // Continue easy tuning from whatever advanced mode produced.
        base_ = work_;
        field_vcom.set_value((base_.vcom2 & 0x7F) - 64, false);
        field_low.set_value(0, false);
        field_mid.set_value(0, false);
        field_high.set_value(0, false);
        check_bgr.set_value(base_.flags & lcd::TUNING_BGR_SWAP);
    } else {
        update_advanced_fields();
    }
    update_hex();
    set_dirty();
}

uint8_t LCDGammaView::param_mask(int32_t index) const {
    if (index < 15) return lcd::ILI9341::gamma_field_mask[index];
    if (index < 30) return lcd::ILI9341::gamma_field_mask[index - 15];
    return 0x7F;
}

uint8_t* LCDGammaView::param_ptr(int32_t index) {
    if (index < 15) return &work_.gamma_pos[index];
    if (index < 30) return &work_.gamma_neg[index - 15];
    if (index == 30) return &work_.vcom1[0];
    if (index == 31) return &work_.vcom1[1];
    return &work_.vcom2;
}

void LCDGammaView::update_advanced_fields() {
    const int32_t idx = field_param.value();
    std::string name;
    if (idx < 30)
        name = std::string(idx < 15 ? "E0 " : "E1 ") + field_names[idx % 15];
    else if (idx == 30)
        name = "C5 VMH";
    else if (idx == 31)
        name = "C5 VML";
    else
        name = "C7 VMF";
    const uint8_t m = param_mask(idx);
    name += " max " + to_string_dec_uint(m);
    text_param.set(name);
    field_value.set_range(0, m);
    field_value.set_value(*param_ptr(idx) & m, false);
}

static std::string hex_row(const uint8_t* p, size_t n) {
    std::string s;
    for (size_t i = 0; i < n; i++) s += to_string_hex(p[i], 2);
    return s;
}

void LCDGammaView::update_hex() {
    text_e0.set(hex_row(work_.gamma_pos, GLEN));
    text_e1.set(hex_row(work_.gamma_neg, GLEN));
    text_vcom.set("C5 " + hex_row(work_.vcom1, 2) + "  C7 " + to_string_hex(work_.vcom2, 2) +
                  ((work_.flags & lcd::TUNING_BGR_SWAP) ? "  BGR swap" : ""));
}

void LCDGammaView::save() {
    const bool ips = pmem::config_lcd_ips();
    panel_tuning_t def;
    lcd::ILI9341::default_tuning(ips, def);
    const bool is_default = !(work_.flags & lcd::TUNING_BGR_SWAP) &&
                            !memcmp(work_.gamma_pos, def.gamma_pos, GLEN) &&
                            !memcmp(work_.gamma_neg, def.gamma_neg, GLEN) &&
                            !memcmp(work_.vcom1, def.vcom1, 2) && work_.vcom2 == def.vcom2;
    if (is_default) {
        lcd_tuning_file::remove();
        lcd::ILI9341::set_override(nullptr);
        show_status("Using built-in tuning");
    } else if (lcd_tuning_file::write(work_, ips)) {
        lcd::ILI9341::set_override(&work_);
        show_status("Saved SETTINGS/lcd_gamma.txt");
    } else {
        show_status("Save failed, check SD card");
        return;
    }
    base_ = work_;
}

/* Back to a clean state: no file, no override, built-in tuning, Easy mode,
 * all offsets 0, no red/blue swap, and the whole panel config re-sent. */
void LCDGammaView::reset_to_default() {
    lcd_tuning_file::remove();
    lcd::ILI9341::set_override(nullptr);
    lcd::ILI9341::default_tuning(pmem::config_lcd_ips(), base_);
    work_ = base_;

    field_preset.set_selected_index(1, false);
    field_mode.set_selected_index(0, false);
    field_param.set_value(0, false);
    set_advanced(false);
    recompute();

    portapack::display.apply_panel_config();
    show_status("Reset to built-in tuning");
}

}  // namespace ui::external_app::lcd_gamma
