/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_debug.hpp"
#include "debug.hpp"

#include "ch.h"

#include "radio.hpp"
#include "string_format.hpp"
#include "crc.hpp"

#include "audio.hpp"

#include "ui_sd_card_debug.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_styles.hpp"
#include "ui_painter.hpp"
#include "ui_external_items_menu_loader.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
using namespace portapack;

#include "irq_controls.hpp"

namespace ui {

/* DebugMemoryView *******************************************************/

DebugMemoryView::DebugMemoryView(NavigationView& nav) {
    add_children({&text_title,
                  &text_label_m0_core_free,
                  &text_label_m0_core_free_value,
                  &text_label_m0_heap_fragmented_free,
                  &text_label_m0_heap_fragmented_free_value,
                  &text_label_m0_heap_fragments,
                  &text_label_m0_heap_fragments_value,
                  &button_done});

    const auto m0_core_free = chCoreStatus();
    text_label_m0_core_free_value.set(to_string_dec_uint(m0_core_free, 5));

    size_t m0_fragmented_free_space = 0;
    const auto m0_fragments = chHeapStatus(NULL, &m0_fragmented_free_space);
    text_label_m0_heap_fragmented_free_value.set(to_string_dec_uint(m0_fragmented_free_space, 5));
    text_label_m0_heap_fragments_value.set(to_string_dec_uint(m0_fragments, 5));

    button_done.on_select = [&nav](Button&) { nav.pop(); };
}

void DebugMemoryView::focus() {
    button_done.focus();
}

/* TemperatureWidget *****************************************************/

void TemperatureWidget::paint(Painter& painter) {
    const auto logger = portapack::temperature_logger;

    const auto rect = screen_rect();
    const Color color_background{0, 0, 64};
    const Color color_foreground = Color::green();
    const Color color_reticle{128, 128, 128};

    const auto graph_width = static_cast<int>(logger.capacity()) * bar_width;
    const Rect graph_rect{
        rect.left() + (rect.width() - graph_width) / 2, rect.top() + 8,
        graph_width, rect.height()};
    const Rect frame_rect{
        graph_rect.left() - 1, graph_rect.top() - 1,
        graph_rect.width() + 2, graph_rect.height() + 2};
    painter.draw_rectangle(frame_rect, color_reticle);
    painter.fill_rectangle(graph_rect, color_background);

    const auto history = logger.history();
    for (size_t i = 0; i < history.size(); i++) {
        const Coord x = graph_rect.right() - (history.size() - i) * bar_width;
        const auto sample = history[i];
        const auto temp = temperature(sample);
        const auto y = screen_y(temp, graph_rect);
        const Dim bar_height = graph_rect.bottom() - y;
        painter.fill_rectangle({x, y, bar_width, bar_height}, color_foreground);
    }

    if (!history.empty()) {
        const auto sample = history.back();
        const auto temp = temperature(sample);
        const auto last_y = screen_y(temp, graph_rect);
        const Coord x = graph_rect.right() + 8;
        const Coord y = last_y - 8;

        painter.draw_string({x, y}, style(), temperature_str(temp));
    }

    const auto display_temp_max = display_temp_min + (graph_rect.height() / display_temp_scale);
    for (auto temp = display_temp_min; temp <= display_temp_max; temp += 10) {
        const int32_t tick_length = 6;
        const auto tick_x = graph_rect.left() - tick_length;
        const auto tick_y = screen_y(temp, graph_rect);
        painter.fill_rectangle({tick_x, tick_y, tick_length, 1}, color_reticle);
        const auto text_x = graph_rect.left() - temp_len * 8 - 8;
        const auto text_y = tick_y - 8;
        painter.draw_string({text_x, text_y}, style(), temperature_str(temp));
    }
}

TemperatureWidget::temperature_t TemperatureWidget::temperature(const sample_t sensor_value) const {
    /*It seems to be a temperature difference of 25C*/
    return -40 + (sensor_value * 4.31) + 25;  // max2837 datasheet temp 25ºC has sensor value: 15
}

std::string TemperatureWidget::temperature_str(const temperature_t temperature) const {
    return to_string_dec_int(temperature, temp_len - 2) + STR_DEGREES_C;
}

Coord TemperatureWidget::screen_y(
    const temperature_t temperature,
    const Rect& rect) const {
    int y_raw = rect.bottom() - ((temperature - display_temp_min) * display_temp_scale);
    const auto y_limit = std::min(rect.bottom(), std::max(rect.top(), y_raw));
    return y_limit;
}

/* TemperatureView *******************************************************/

TemperatureView::TemperatureView(NavigationView& nav) {
    add_children({
        &text_title,
        &temperature_widget,
        &button_done,
    });

    button_done.on_select = [&nav](Button&) { nav.pop(); };
}

void TemperatureView::focus() {
    button_done.focus();
}

/* RegistersWidget *******************************************************/

RegistersWidget::RegistersWidget(
    RegistersWidgetConfig&& config,
    std::function<uint32_t(const size_t register_number)>&& reader)
    : Widget{},
      config(std::move(config)),
      reader(std::move(reader)) {
}

void RegistersWidget::update() {
    set_dirty();
}

void RegistersWidget::paint(Painter& painter) {
    const Coord left = (size().width() - config.row_width()) / 2;

    draw_legend(left, painter);
    draw_values(left, painter);
}

void RegistersWidget::draw_legend(const Coord left, Painter& painter) {
    const auto pos = screen_pos();

    for (size_t i = 0; i < config.registers_count; i += config.registers_per_row()) {
        const Point offset{
            left, static_cast<int>((i / config.registers_per_row()) * row_height)};

        const auto text = to_string_hex(i, config.legend_length());
        painter.draw_string(
            pos + offset,
            style().invert(),
            text);
    }
}

void RegistersWidget::draw_values(
    const Coord left,
    Painter& painter) {
    const auto pos = screen_pos();

    for (size_t i = 0; i < config.registers_count; i++) {
        const Point offset = {
            static_cast<int>(left + config.legend_width() + 8 + (i % config.registers_per_row()) * (config.value_width() + 8)),
            static_cast<int>((i / config.registers_per_row()) * row_height)};

        const auto value = reader(i);

        const auto text = to_string_hex(value, config.value_length());
        painter.draw_string(
            pos + offset,
            style(),
            text);
    }
}

/* RegistersView *********************************************************/

RegistersView::RegistersView(
    NavigationView& nav,
    const std::string& title,
    RegistersWidgetConfig&& config,
    std::function<uint32_t(const size_t register_number)>&& reader)
    : registers_widget{std::move(config), std::move(reader)} {
    add_children({
        &text_title,
        &registers_widget,
        &button_update,
        &button_done,
    });

    button_update.on_select = [this](Button&) {
        this->registers_widget.update();
    };
    button_done.on_select = [&nav](Button&) { nav.pop(); };

    registers_widget.set_parent_rect({0, 48, 240, 192});

    text_title.set_parent_rect({(240 - static_cast<int>(title.size()) * 8) / 2, 16,
                                static_cast<int>(title.size()) * 8, 16});
    text_title.set(title);
}

void RegistersView::focus() {
    button_done.focus();
}

/* ControlsSwitchesWidget ************************************************/

void ControlsSwitchesWidget::on_show() {
    display.fill_rectangle(
        screen_rect(),
        Color::black());
}

bool ControlsSwitchesWidget::on_key(const KeyEvent key) {
    key_event_mask = 1 << toUType(key);
    long_press_key_event_mask = key_is_long_pressed(key) ? key_event_mask : 0;
    return true;
}

void ControlsSwitchesWidget::paint(Painter& painter) {
    const auto pos = screen_pos();

    const std::array<Rect, 9> button_rects{{
        {64, 32, 16, 16},  // Right
        {0, 32, 16, 16},   // Left
        {32, 64, 16, 16},  // Down
        {32, 0, 16, 16},   // Up
        {32, 32, 16, 16},  // Select
        {96, 0, 16, 16},   // Dfu
        {16, 96, 16, 16},  // Encoder phase 0
        {48, 96, 16, 16},  // Encoder phase 1
        {96, 64, 16, 16},  // Touch
    }};

    for (const auto r : button_rects) {
        painter.fill_rectangle(r + pos, Color::blue());
    }

    if (get_touch_frame().touch)
        painter.fill_rectangle(button_rects[8] + pos, Color::yellow());

    const std::array<Rect, 8> raw_rects{{
        {64 + 1, 32 + 1, 16 - 2, 16 - 2},  // Right
        {0 + 1, 32 + 1, 16 - 2, 16 - 2},   // Left
        {32 + 1, 64 + 1, 16 - 2, 16 - 2},  // Down
        {32 + 1, 0 + 1, 16 - 2, 16 - 2},   // Up
        {32 + 1, 32 + 1, 16 - 2, 16 - 2},  // Select
        {96 + 1, 0 + 1, 16 - 2, 16 - 2},   // Dfu
        {16 + 1, 96 + 1, 16 - 2, 16 - 2},  // Encoder phase 0
        {48 + 1, 96 + 1, 16 - 2, 16 - 2},  // Encoder phase 1
    }};

    auto switches_raw = control::debug::switches();
    for (const auto r : raw_rects) {
        if (switches_raw & 1)
            painter.fill_rectangle(r + pos, Color::yellow());

        switches_raw >>= 1;
    }

    const std::array<Rect, 6> debounced_rects{{
        {64 + 2, 32 + 2, 16 - 4, 16 - 4},  // Right
        {0 + 2, 32 + 2, 16 - 4, 16 - 4},   // Left
        {32 + 2, 64 + 2, 16 - 4, 16 - 4},  // Down
        {32 + 2, 0 + 2, 16 - 4, 16 - 4},   // Up
        {32 + 2, 32 + 2, 16 - 4, 16 - 4},  // Select
        {96 + 2, 0 + 2, 16 - 4, 16 - 4},   // Dfu
    }};

    auto switches_debounced = get_switches_state().to_ulong();
    for (const auto r : debounced_rects) {
        if (switches_debounced & 1)
            painter.fill_rectangle(r + pos, Color::green());

        switches_debounced >>= 1;
    }

    const std::array<Rect, 6> events_rects{{
        {64 + 3, 32 + 3, 16 - 6, 16 - 6},  // Right
        {0 + 3, 32 + 3, 16 - 6, 16 - 6},   // Left
        {32 + 3, 64 + 3, 16 - 6, 16 - 6},  // Down
        {32 + 3, 0 + 3, 16 - 6, 16 - 6},   // Up
        {32 + 3, 32 + 3, 16 - 6, 16 - 6},  // Select
        {96 + 3, 0 + 3, 16 - 6, 16 - 6},   // Dfu
    }};

    auto switches_event = key_event_mask;
    for (const auto r : events_rects) {
        if (switches_event & 1)
            painter.fill_rectangle(r + pos, Color::red());

        switches_event >>= 1;
    }

    switches_event = long_press_key_event_mask;
    for (const auto r : events_rects) {
        if (switches_event & 1)
            painter.fill_rectangle(r + pos, Color::cyan());

        switches_event >>= 1;
    }
}

void ControlsSwitchesWidget::on_frame_sync() {
    set_dirty();
}

/* DebugControlsView *****************************************************/

DebugControlsView::DebugControlsView(NavigationView& nav) {
    add_children({
        &labels,
        &switches_widget,
        &options_switches_mode,
        &button_done,
    });

    button_done.on_select = [&nav](Button&) {
        set_switches_long_press_config(0);
        nav.pop();
    };

    options_switches_mode.on_change = [this](size_t, OptionsField::value_t v) {
        (void)v;
        set_switches_long_press_config(options_switches_mode.selected_index_value());
    };
}

void DebugControlsView::focus() {
    switches_widget.focus();
}

/* DebugPeripheralsMenuView **********************************************/

DebugPeripheralsMenuView::DebugPeripheralsMenuView(NavigationView& nav) {
    const char* max283x = hackrf_r9 ? "MAX2839" : "MAX2837";
    const char* si5351x = hackrf_r9 ? "Si5351A" : "Si5351C";
    add_items({
        {"RFFC5072", ui::Color::dark_cyan(), &bitmap_icon_peripherals_details, [&nav]() { nav.push<RegistersView>(
                                                                                              "RFFC5072", RegistersWidgetConfig{31, 16},
                                                                                              [](const size_t register_number) { return radio::debug::first_if::register_read(register_number); }); }},
        {max283x, ui::Color::dark_cyan(), &bitmap_icon_peripherals_details, [&nav, max283x]() { nav.push<RegistersView>(
                                                                                                    max283x, RegistersWidgetConfig{32, 10},
                                                                                                    [](const size_t register_number) { return radio::debug::second_if::register_read(register_number); }); }},
        {si5351x, ui::Color::dark_cyan(), &bitmap_icon_peripherals_details, [&nav, si5351x]() { nav.push<RegistersView>(
                                                                                                    si5351x, RegistersWidgetConfig{96, 8},
                                                                                                    [](const size_t register_number) { return portapack::clock_generator.read_register(register_number); }); }},
        {audio::debug::codec_name(), ui::Color::dark_cyan(), &bitmap_icon_peripherals_details, [&nav]() { nav.push<RegistersView>(
                                                                                                              audio::debug::codec_name(), RegistersWidgetConfig{audio::debug::reg_count(), audio::debug::reg_bits()},
                                                                                                              [](const size_t register_number) { return audio::debug::reg_read(register_number); }); }},
    });
    set_max_rows(2);  // allow wider buttons
}

/* DebugMenuView *********************************************************/

DebugMenuView::DebugMenuView(NavigationView& nav) {
    if (portapack::persistent_memory::show_gui_return_icon()) {
        add_items({{"..", ui::Color::light_grey(), &bitmap_icon_previous, [&nav]() { nav.pop(); }}});
    }
    add_items({
        {"Buttons Test", ui::Color::dark_cyan(), &bitmap_icon_controls, [&nav]() { nav.push<DebugControlsView>(); }},
        {"Debug Dump", ui::Color::dark_cyan(), &bitmap_icon_memory, [&nav]() { portapack::persistent_memory::debug_dump(); }},
        {"M0 Stack Dump", ui::Color::dark_cyan(), &bitmap_icon_memory, [&nav]() { stack_dump(); }},
        {"Memory", ui::Color::dark_cyan(), &bitmap_icon_memory, [&nav]() { nav.push<DebugMemoryView>(); }},
        {"P.Memory", ui::Color::dark_cyan(), &bitmap_icon_memory, [&nav]() { nav.push<DebugPmemView>(); }},
        {"Peripherals", ui::Color::dark_cyan(), &bitmap_icon_peripherals, [&nav]() { nav.push<DebugPeripheralsMenuView>(); }},
        //{ "Radio State",	ui::Color::white(),	nullptr,	[&nav](){ nav.push<NotImplementedView>(); } },
        {"SD Card", ui::Color::dark_cyan(), &bitmap_icon_sdcard, [&nav]() { nav.push<SDCardDebugView>(); }},
        {"Temperature", ui::Color::dark_cyan(), &bitmap_icon_temperature, [&nav]() { nav.push<TemperatureView>(); }},
        {"Touch Test", ui::Color::dark_cyan(), &bitmap_icon_notepad, [&nav]() { nav.push<DebugScreenTest>(); }},
    });

    for (auto const& gridItem : ExternalItemsMenuLoader::load_external_items(app_location_t::DEBUG, nav)) {
        add_item(gridItem);
    };

    set_max_rows(2);  // allow wider buttons
}

/* DebugPmemView *********************************************************/

uint32_t pmem_checksum(volatile const uint32_t data[63]) {
    CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};
    for (size_t i = 0; i < 63; i++) {
        const auto word = data[i];
        crc.process_byte((word >> 0) & 0xff);
        crc.process_byte((word >> 8) & 0xff);
        crc.process_byte((word >> 16) & 0xff);
        crc.process_byte((word >> 24) & 0xff);
    }
    return crc.checksum();
}

DebugPmemView::DebugPmemView(NavigationView& nav)
    : data{*reinterpret_cast<pmem_data*>(memory::map::backup_ram.base())}, registers_widget(RegistersWidgetConfig{page_size, 8}, std::bind(&DebugPmemView::registers_widget_feed, this, std::placeholders::_1)) {
    static_assert(sizeof(pmem_data) == memory::map::backup_ram.size());

    add_children({&text_page, &registers_widget, &text_checksum, &text_checksum2, &button_ok});

    registers_widget.set_parent_rect({0, 32, 240, 192});

    text_checksum.set("Size: " + to_string_dec_uint(portapack::persistent_memory::data_size(), 3) + "  CRC: " + to_string_hex(data.check_value, 8));
    text_checksum2.set("Calculated CRC: " + to_string_hex(pmem_checksum(data.regfile), 8));

    button_ok.on_select = [&nav](Button&) {
        nav.pop();
    };

    update();
}

bool DebugPmemView::on_encoder(const EncoderEvent delta) {
    page = std::max(0l, std::min((int32_t)page_max, page + delta));

    update();

    return true;
}

void DebugPmemView::focus() {
    button_ok.focus();
}

void DebugPmemView::update() {
    text_page.set(to_string_hex(page_size * page, 2) + "+");
    registers_widget.update();
}

uint32_t DebugPmemView::registers_widget_feed(const size_t register_number) {
    if (page_size * page + register_number >= memory::map::backup_ram.size()) {
        return 0xff;
    }
    return data.regfile[(page_size * page + register_number) / 4] >> (register_number % 4 * 8);
}

/* DebugScreenTest ****************************************************/

DebugScreenTest::DebugScreenTest(NavigationView& nav)
    : nav_{nav} {
    set_focusable(true);
}

bool DebugScreenTest::on_key(const KeyEvent key) {
    Painter painter;
    switch (key) {
        case KeyEvent::Select:
            nav_.pop();
            break;
        case KeyEvent::Down:
            painter.fill_rectangle({0, 0, screen_width, screen_height}, semirand());
            break;
        case KeyEvent::Left:
            pen_color = semirand();
            break;
        default:
            break;
    }
    return true;
}

bool DebugScreenTest::on_encoder(EncoderEvent delta) {
    pen_size = clip<int32_t>(pen_size + delta, 1, screen_width);
    return true;
}

bool DebugScreenTest::on_touch(const TouchEvent event) {
    Painter painter;
    pen_pos = event.point;
    painter.fill_rectangle({pen_pos.x() - pen_size / 2, pen_pos.y() - pen_size / 2, pen_size, pen_size}, pen_color);
    return true;
}

uint16_t DebugScreenTest::semirand() {
    static uint64_t seed{0x0102030405060708};
    seed = seed * 137;
    seed = (seed >> 1) | ((seed & 0x01) << 63);
    return (uint16_t)seed;
}

void DebugScreenTest::paint(Painter& painter) {
    painter.fill_rectangle({0, 16, screen_width, screen_height - 16}, Color::white());
    painter.draw_string({10 * 8, screen_height / 2}, Styles::white, "Use Stylus");
    pen_color = semirand();
}

/* DebugLCRView *******************************************************/

/*DebugLCRView::DebugLCRView(NavigationView& nav, std::string lcr_string) {

        std::string debug_text;

        add_children({
                &console,
                &button_exit
        });

        for(const auto c : lcr_string) {
                if ((c < 32) || (c > 126))
                        debug_text += "[" + to_string_dec_uint(c) + "]";
                else
                        debug_text += c;
        }

        debug_text += "\n\n";
        debug_text += "Length: " + to_string_dec_uint(lcr_string.length()) + '\n';
        debug_text += "Checksum: " + to_string_dec_uint(lcr_string.back()) + '\n';

        console.write(debug_text);

        button_exit.on_select = [this, &nav](Button&){
                nav.pop();
        };
}

void DebugLCRView::focus() {
        button_exit.focus();
}*/

} /* namespace ui */
