/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2024 Mark Thompson
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

#include "ui_debug.hpp"
#include "debug.hpp"

#include "ch.h"

#include "radio.hpp"
#include "string_format.hpp"
#include "crc.hpp"

#include "audio.hpp"

#include "ui_sd_card_debug.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_painter.hpp"
#include "ui_external_items_menu_loader.hpp"
#include "ui_debug_max17055.hpp"
#include "ui_external_module_view.hpp"

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

/* RegistersWidget *******************************************************/

RegistersWidget::RegistersWidget(
    RegistersWidgetConfig&& config)
    : Widget{}, config(std::move(config)), page_number(0) {
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
    const std::string spaces(config.legend_length(), ' ');

    for (uint32_t i = 0; i < config.registers_per_page; i += config.registers_per_row()) {
        uint32_t r = page_number * config.registers_per_page + i;

        const Point offset{
            left, static_cast<int>((i / config.registers_per_row()) * row_height)};

        const auto text = (r >= config.registers_count) ? spaces : to_string_hex(r, config.legend_length());
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
    const std::string spaces(config.value_length(), ' ');

    for (uint32_t i = 0; i < config.registers_per_page; i++) {
        uint32_t r = page_number * config.registers_per_page + i;

        const Point offset = {
            static_cast<int>(left + config.legend_width() + 8 + (i % config.registers_per_row()) * (config.value_width() + 8)),
            static_cast<int>((i / config.registers_per_row()) * row_height)};

        const auto text = (r >= config.registers_count) ? spaces : to_string_hex(reg_read(r), config.value_length());
        painter.draw_string(
            pos + offset,
            style(),
            text);
    }
}

uint32_t RegistersWidget::reg_read(const uint32_t register_number) {
    if (register_number < config.registers_count) {
        switch (config.chip_type) {
            case CT_PMEM:
                return portapack::persistent_memory::pmem_data_word(register_number / 4) >> (register_number % 4 * 8);
            case CT_RFFC5072:
                return radio::debug::first_if::register_read(register_number);
            case CT_MAX283X:
                return radio::debug::second_if::register_read(register_number);
            case CT_SI5351:
                return portapack::clock_generator.read_register(register_number);
            case CT_MAX17055: {
                i2cdev::I2cDev_MAX17055* dev = (i2cdev::I2cDev_MAX17055*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055);
                return dev->read_register(register_number);
            }
            case CT_AUDIO:
                return audio::debug::reg_read(register_number);
        }
    }
    return 0xFFFF;
}

void RegistersWidget::reg_write(const uint32_t register_number, const uint32_t value) {
    if (register_number < config.registers_count) {
        switch (config.chip_type) {
            case CT_PMEM:
                break;
            case CT_RFFC5072:
                radio::debug::first_if::register_write(register_number, value);
                break;
            case CT_MAX283X:
                radio::debug::second_if::register_write(register_number, value);
                break;
            case CT_SI5351:
                portapack::clock_generator.write_register(register_number, value);
                break;
            case CT_MAX17055: {
                i2cdev::I2cDev_MAX17055* dev = (i2cdev::I2cDev_MAX17055*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055);
                dev->write_register(register_number, value);
                break;
            }
            case CT_AUDIO:
                audio::debug::reg_write(register_number, value);
                break;
        }
    }
}

/* RegistersView *********************************************************/

RegistersView::RegistersView(
    NavigationView& nav,
    const std::string& title,
    RegistersWidgetConfig&& config)
    : registers_widget{std::move(config)} {
    add_children({
        &text_title,
        &registers_widget,
        &button_update,
        &button_done,
        &labels,
        &field_write_reg_num,
        &field_write_data_val,
        &button_write,
    });

    button_update.on_select = [this](Button&) {
        this->registers_widget.update();
    };
    button_done.on_select = [&nav](Button&) { nav.pop(); };

    registers_widget.set_parent_rect({0, 48, screen_width, 192});
    registers_widget.set_page(0);

    text_title.set_parent_rect({(screen_width - static_cast<int>(title.size()) * 8) / 2, 16,
                                static_cast<int>(title.size()) * 8, 16});
    text_title.set(title);

    field_write_reg_num.on_change = [this](SymField&) {
        field_write_data_val.set_value(this->registers_widget.reg_read(field_write_reg_num.to_integer()));
        field_write_data_val.set_dirty();
    };

    const auto value = registers_widget.reg_read(0);
    field_write_data_val.set_value(value);

    button_write.set_style(Theme::getInstance()->fg_red);
    button_write.on_select = [this](Button&) {
        this->registers_widget.reg_write(field_write_reg_num.to_integer(), field_write_data_val.to_integer());
        this->registers_widget.update();
    };
}

void RegistersView::focus() {
    button_done.focus();
}

bool RegistersView::on_encoder(const EncoderEvent delta) {
    registers_widget.set_page(std::max(0ul, std::min(registers_widget.page_count() - 1, registers_widget.page() + delta)));
    registers_widget.update();

    return true;
}

/* ControlsSwitchesWidget ************************************************/

void ControlsSwitchesWidget::on_show() {
    display.fill_rectangle(
        screen_rect(),
        Theme::getInstance()->bg_darkest->background);
}

bool ControlsSwitchesWidget::on_key(const KeyEvent key) {
    key_event_mask = 1 << toUType(key);
    long_press_key_event_mask = key_is_long_pressed(key) ? key_event_mask : 0;
    return true;
}

bool ControlsSwitchesWidget::on_encoder(const EncoderEvent delta) {
    last_delta = delta;
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
        painter.fill_rectangle(r + pos, Theme::getInstance()->fg_blue->foreground);
    }

    if (get_touch_frame().touch)
        painter.fill_rectangle(button_rects[8] + pos, Theme::getInstance()->fg_yellow->foreground);

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
            painter.fill_rectangle(r + pos, Theme::getInstance()->fg_yellow->foreground);

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
            painter.fill_rectangle(r + pos, Theme::getInstance()->fg_green->foreground);

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
            painter.fill_rectangle(r + pos, Theme::getInstance()->fg_red->foreground);

        switches_event >>= 1;
    }

    switches_event = long_press_key_event_mask;
    for (const auto r : events_rects) {
        if (switches_event & 1)
            painter.fill_rectangle(r + pos, Theme::getInstance()->fg_cyan->foreground);

        switches_event >>= 1;
    }

    painter.draw_string({5 * 8, 12 * 16}, *Theme::getInstance()->fg_light, to_string_dec_int(last_delta, 3));
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

DebugPeripheralsMenuView::DebugPeripheralsMenuView(NavigationView& nav)
    : nav_(nav) {
    set_max_rows(2);  // allow wider buttons
}

void DebugPeripheralsMenuView::on_populate() {
    const char* max283x = hackrf_r9 ? "MAX2839" : "MAX2837";
    const char* si5351x = hackrf_r9 ? "Si5351A" : "Si5351C";
    add_items({
        {"RFFC5072", Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<RegistersView>("RFFC5072", RegistersWidgetConfig{CT_RFFC5072, 31, 31, 16}); }},
        {max283x, Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this, max283x]() { nav_.push<RegistersView>(max283x, RegistersWidgetConfig{CT_MAX283X, 32, 32, 10}); }},
        {si5351x, Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this, si5351x]() { nav_.push<RegistersView>(si5351x, RegistersWidgetConfig{CT_SI5351, 188, 96, 8}); }},
        {audio::debug::codec_name(), Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<RegistersView>(audio::debug::codec_name(), RegistersWidgetConfig{CT_AUDIO, audio::debug::reg_count(), audio::debug::reg_count(), audio::debug::reg_bits()}); }},
    });
    if (i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055)) {
        add_item(
            {"MAX17055", Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<RegistersView>("MAX17055", RegistersWidgetConfig{CT_MAX17055, 256, 16, 16}); }});
    }
    set_max_rows(2);  // allow wider buttons
}

/* DebugReboot **********************************************/

DebugReboot::DebugReboot(NavigationView& nav) {
    (void)nav;

    LPC_RGU->RESET_CTRL[0] = (1 << 0);

    while (1)
        __WFE();
}

void DebugReboot::on_populate() {
}

/* DebugMenuView *********************************************************/

DebugMenuView::DebugMenuView(NavigationView& nav)
    : nav_(nav) {
    set_max_rows(2);  // allow wider buttons
}

void DebugMenuView::on_populate() {
    if (portapack::persistent_memory::show_gui_return_icon()) {
        add_items({{"..", ui::Theme::getInstance()->fg_light->foreground, &bitmap_icon_previous, [this]() { nav_.pop(); }}});
    }
    add_items({
        {"Buttons Test", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_controls, [this]() { nav_.push<DebugControlsView>(); }},
        {"M0 Stack Dump", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_memory, [this]() { stack_dump(); }},
        {"Memory Dump", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_memory, [this]() { nav_.push<DebugMemoryDumpView>(); }},
        {"Peripherals", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<DebugPeripheralsMenuView>(); }},
        {"Pers. Memory", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_memory, [this]() { nav_.push<DebugPmemView>(); }},
        {"SD Card", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_sdcard, [this]() { nav_.push<SDCardDebugView>(); }},
        {"Touch Test", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_notepad, [this]() { nav_.push<DebugScreenTest>(); }},
        {"Reboot", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_setup, [this]() { nav_.push<DebugReboot>(); }},
        {"Ext Module", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<ExternalModuleView>(); }},
    });

    if (i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDEVMDL_MAX17055)) {
        add_item(
            {"Battery", ui::Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_batt_icon, [this]() { nav_.push<BatteryCapacityView>(); }});
    }

    for (auto const& gridItem : ExternalItemsMenuLoader::load_external_items(app_location_t::DEBUG, nav_)) {
        add_item(gridItem);
    };
}

/* DebugMemoryDumpView *********************************************************/

DebugMemoryDumpView::DebugMemoryDumpView(NavigationView& nav) {
    add_children({
        &button_dump,
        &button_read,
        &button_write,
        &button_done,
        &labels,
        &field_starting_address,
        &field_byte_count,
        &field_rw_address,
        &field_data_value,
    });

    button_done.on_select = [&nav](Button&) { nav.pop(); };

    button_dump.on_select = [this](Button&) {
        if (field_byte_count.to_integer() != 0)
            memory_dump((uint32_t*)field_starting_address.to_integer(), ((uint32_t)field_byte_count.to_integer() + 3) / 4, false);
    };

    button_read.on_select = [this](Button&) {
        field_data_value.set_value(*(uint32_t*)field_rw_address.to_integer());
        field_data_value.set_dirty();
    };

    button_write.set_style(Theme::getInstance()->fg_red);
    button_write.on_select = [this](Button&) {
        *(uint32_t*)field_rw_address.to_integer() = (uint32_t)field_data_value.to_integer();
    };
}

void DebugMemoryDumpView::focus() {
    button_done.focus();
}

/* DebugPmemView *********************************************************/

DebugPmemView::DebugPmemView(NavigationView& nav)
    : registers_widget(RegistersWidgetConfig{CT_PMEM, PMEM_SIZE_BYTES, page_size, 8}) {
    add_children({&registers_widget, &text_checksum, &text_checksum2, &button_ok});

    registers_widget.set_parent_rect({0, 32, screen_width, 192});

    text_checksum.set("Size: " + to_string_dec_uint(portapack::persistent_memory::data_size(), 3) + "  CRC: " + to_string_hex(portapack::persistent_memory::pmem_stored_checksum(), 8));
    text_checksum2.set("Calculated CRC: " + to_string_hex(portapack::persistent_memory::pmem_calculated_checksum(), 8));

    button_ok.on_select = [&nav](Button&) {
        nav.pop();
    };

    update();
}

bool DebugPmemView::on_encoder(const EncoderEvent delta) {
    registers_widget.set_page(std::max(0ul, std::min((uint32_t)page_count - 1, registers_widget.page() + delta)));

    update();

    return true;
}

void DebugPmemView::focus() {
    button_ok.focus();
}

void DebugPmemView::update() {
    registers_widget.update();
}

/* DebugScreenTest ****************************************************/

DebugScreenTest::DebugScreenTest(NavigationView& nav)
    : nav_{nav} {
    set_focusable(true);
    std::srand(LPC_RTC->CTIME0);
}

bool DebugScreenTest::on_key(const KeyEvent key) {
    Painter painter;
    switch (key) {
        case KeyEvent::Select:
            nav_.pop();
            break;
        case KeyEvent::Down:
            painter.fill_rectangle({0, 0, screen_width, screen_height}, std::rand());
            break;
        case KeyEvent::Left:
            pen_color = std::rand();
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

void DebugScreenTest::paint(Painter& painter) {
    painter.fill_rectangle({0, 16, screen_width, screen_height - 16}, Theme::getInstance()->bg_darkest->foreground);
    painter.draw_string({10 * 8, screen_height / 2}, *Theme::getInstance()->bg_darkest, "Use Stylus");
    pen_color = std::rand();
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
