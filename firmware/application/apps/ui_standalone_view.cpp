/*
 * Copyright (C) 2024 Bernd Herzog
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

#include "ui_standalone_view.hpp"
#include "irq_controls.hpp"

#include "i2cdevmanager.hpp"
#include "i2cdev_ppmod.hpp"

#include "ui_font_fixed_5x8.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

void create_thread(int32_t (*fn)(void*), void* arg, size_t stack_size, int priority) {
    // TODO: collect memory on terminate, once this is used
    chThdCreateFromHeap(NULL, stack_size, priority, fn, arg);
}

void fill_rectangle(int x, int y, int width, int height, uint16_t color) {
    ui::Painter painter;
    painter.fill_rectangle({x, y, width, height}, ui::Color(color));
}

void fill_rectangle_unrolled8(int x, int y, int width, int height, uint16_t color) {
    ui::Painter painter;
    painter.fill_rectangle_unrolled8({x, y, width, height}, ui::Color(color));
}

void draw_bitmap(int x, int y, int width, int height, const uint8_t* pixels, uint16_t foreground, uint16_t background) {
    ui::Painter painter;
    painter.draw_bitmap({x, y}, {{width, height}, pixels}, ui::Color(foreground), ui::Color(background));
}

void* alloc(size_t size) {
    void* p = chHeapAlloc(0x0, size);
    if (p == nullptr)
        chDbgPanic("Out of Memory");
    return p;
}

uint64_t get_switches_state_ulong() {
    return get_switches_state().to_ulong();
}

ui::Coord scroll_area_y(const ui::Coord y) {
    return portapack::display.scroll_area_y(y);
}

void scroll_set_area(const ui::Coord top_y, const ui::Coord bottom_y) {
    portapack::display.scroll_set_area(top_y, bottom_y);
}

void scroll_disable() {
    portapack::display.scroll_disable();
}

ui::Coord scroll_set_position(const ui::Coord position) {
    return portapack::display.scroll_set_position(position);
}

ui::Coord scroll(const int32_t delta) {
    return portapack::display.scroll(delta);
}

bool i2c_read(uint8_t* cmd, size_t cmd_len, uint8_t* data, size_t data_len) {
    auto dev = (i2cdev::I2cDev_PPmod*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);
    if (!dev) {
        return false;
    }

    if (data_len == 0 || data == nullptr)
        return dev->i2c_write(nullptr, 0, cmd, cmd_len);

    return dev->i2c_read(cmd, cmd_len, data, data_len);
}

StandaloneView* standaloneView = nullptr;

void set_dirty() {
    if (standaloneView != nullptr)
        standaloneView->set_dirty();
}

standalone_application_api_t api = {
    /* .malloc = */ &alloc,
    /* .calloc = */ &calloc,
    /* .realloc = */ &realloc,
    /* .free = */ &chHeapFree,
    /* .create_thread = */ &create_thread,
    /* .fill_rectangle = */ &fill_rectangle,
    /* .swizzled_switches = */ &swizzled_switches,
    /* .get_switches_state = */ &get_switches_state_ulong,
    /* .fixed_5x8_glyph_data = */ ui::font::fixed_5x8.get_data(),
    /* .fixed_8x16_glyph_data = */ ui::font::fixed_8x16.get_data(),
    /* .fill_rectangle_unrolled8 = */ &fill_rectangle_unrolled8,
    /* .draw_bitmap = */ &draw_bitmap,
    /* .scroll_area_y = */ &scroll_area_y,
    /* .scroll_set_area = */ &scroll_set_area,
    /* .scroll_disable = */ &scroll_disable,
    /* .scroll_set_position = */ &scroll_set_position,
    /* .scroll = */ &scroll,
    /* .i2c_read = */ &i2c_read,
    /* .panic = */ &chDbgPanic,
    /* .set_dirty = */ &set_dirty,
};

StandaloneView::StandaloneView(NavigationView& nav, uint8_t* app_image)
    : nav_(nav),
      _app_image(*app_image) {
    if (app_image == nullptr) {
        chDbgPanic("Invalid application image");
    }

    set_focusable(true);

    standaloneView = this;
}

void StandaloneView::focus() {
    View::focus();
}

void StandaloneView::paint(Painter& painter) {
    (void)painter;

    if (initialized &&
        get_application_information()->header_version > 1) {
        get_application_information()->PaintViewMirror();
    }
}

void StandaloneView::on_focus() {
    if (get_application_information()->header_version > 1) {
        get_application_information()->OnFocus();
    }
}

bool StandaloneView::on_key(const KeyEvent key) {
    if (get_application_information()->header_version > 1) {
        return get_application_information()->OnKeyEvent((uint8_t)key);
    }

    return false;
}

bool StandaloneView::on_encoder(const EncoderEvent event) {
    if (get_application_information()->header_version > 1) {
        return get_application_information()->OnEncoder((int32_t)event);
    }
    return false;
}

bool StandaloneView::on_touch(const TouchEvent event) {
    if (get_application_information()->header_version > 1) {
        get_application_information()->OnTouchEvent(event.point.x(), event.point.y(), (uint32_t)event.type);
    }
    return true;
}

bool StandaloneView::on_keyboard(const KeyboardEvent event) {
    if (get_application_information()->header_version > 1) {
        return get_application_information()->OnKeyboard((uint8_t)event);
    }
    return false;
}

void StandaloneView::frame_sync() {
    // skip first refresh
    if (!initialized) {
        initialized = true;
    } else {
        get_application_information()->on_event(1);
    }
}

void StandaloneView::on_after_attach() {
    context().focus_manager().setMirror(this);
    get_application_information()->initialize(api);
}

void StandaloneView::on_before_detach() {
    get_application_information()->shutdown();
    context().focus_manager().clearMirror();
}

}  // namespace ui
