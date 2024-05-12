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

namespace ui {

void create_thread(int32_t (*fn)(void*), void* arg, size_t stack_size, int priority) {
    // TODO: collect memory on terminate, once this is used
    chThdCreateFromHeap(NULL, stack_size, priority, fn, arg);
}

void fill_rectangle(int x, int y, int width, int height, uint16_t color) {
    ui::Painter painter;
    painter.fill_rectangle({x, y, width, height}, ui::Color(color));
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

standalone_application_api_t api = {
    /* .malloc = */ &alloc,
    /* .calloc = */ &calloc,
    /* .realloc = */ &realloc,
    /* .free = */ &chHeapFree,
    /* .create_thread = */ &create_thread,
    /* .fill_rectangle = */ &fill_rectangle,
    /* .swizzled_switches = */ &swizzled_switches,
    /* .get_switches_state = */ &get_switches_state_ulong,
};

StandaloneView::StandaloneView(NavigationView& nav, std::unique_ptr<uint8_t[]> app_image)
    : nav_(nav), _app_image(std::move(app_image)) {
    get_application_information()->initialize(api);
    add_children({&dummy});
}

void StandaloneView::focus() {
    dummy.focus();
}

void StandaloneView::paint(Painter& painter) {
    (void)painter;
}

void StandaloneView::frame_sync() {
    // skip first refresh
    if (!initialized) {
        initialized = true;
    } else {
        get_application_information()->on_event(1);
    }
}

}  // namespace ui
