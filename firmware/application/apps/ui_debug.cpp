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
#include "hal.h"

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

#ifdef PRALINE
#include "max2831.hpp"
using namespace max2831;
#endif

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
#ifdef PRALINE
            case CT_FPGA:
                return radio::debug::fpga::register_read(register_number);
#endif
            case CT_SGPIO:
                return radio::debug::sgpio::register_read(register_number);
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
#ifdef PRALINE
            case CT_FPGA:
                radio::debug::fpga::register_write(register_number, value);
                break;
#endif
            case CT_SGPIO:
                // SGPIO registers are read-only for debug purposes
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

#ifdef PRALINE
/* RadioDiagnosticsView **************************************************/

RadioDiagnosticsView::RadioDiagnosticsView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_rffc,
        &text_rffc_status,
        &text_lbl_max,
        &text_max_status,
        &text_lbl_adc,
        &text_adc_status,
        &text_lbl_fpga,
        &text_fpga_status,
        &text_lbl_sgpio,
        &text_sgpio_status,
        &text_lbl_clock,
        &text_clock_status,
        &text_regs_title,
        &text_lbl_rffc_reg,
        &text_rffc_reg,
        &text_lbl_max_reg,
        &text_max_reg,
        &text_lbl_fpga_reg,
        &text_fpga_reg,
        &text_lbl_sgpio_reg,
        &text_sgpio_reg,
        &text_test_result,
        &button_refresh,
        &button_done,
    });

    // Set title colors
    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_regs_title.set_style(Theme::getInstance()->fg_yellow);

#ifdef PRALINE
    text_lbl_fpga.set("FPGA (iCE40):");
#else
    text_lbl_fpga.set("CPLD:");
#endif

    button_refresh.on_select = [this](Button&) {
        update_status();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Initial status update
    update_status();
}

void RadioDiagnosticsView::focus() {
    button_refresh.focus();
}

void RadioDiagnosticsView::update_status() {
    // Read RFFC5072 register 0 to check if it responds
    uint32_t rffc_reg0 = radio::debug::first_if::register_read(0);
    bool rffc_ok = (rffc_reg0 != 0x0000) && (rffc_reg0 != 0xFFFF);
    text_rffc_status.set(rffc_ok ? "OK" : "FAIL");
    text_rffc_status.set_style(rffc_ok ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_rffc_reg.set(to_string_hex(rffc_reg0, 4));

    // Read MAX283x register 0 to check if it responds
    uint32_t max_reg0 = radio::debug::second_if::register_read(0);
    bool max_ok = (max_reg0 != 0x0000) && (max_reg0 != 0x3FFF);
    text_max_status.set(max_ok ? "OK" : "FAIL");
    text_max_status.set_style(max_ok ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_max_reg.set(to_string_hex(max_reg0, 4));

    // MAX5864 has no readback - assume OK if other SPI works
    text_adc_status.set("(no readback)");
    text_adc_status.set_style(Theme::getInstance()->fg_medium);

#ifdef PRALINE
    // Read FPGA control register
    uint32_t fpga_ctrl = radio::debug::fpga::register_read(1);
    bool fpga_ok = (fpga_ctrl != 0xFF);  // 0xFF = not responding
    bool dc_block = (fpga_ctrl & 0x01) != 0;
    text_fpga_status.set(fpga_ok ? (dc_block ? "OK DC_BLK" : "OK NO_DC") : "FAIL");
    text_fpga_status.set_style(fpga_ok ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // Show all FPGA registers
    uint32_t fpga_r2 = radio::debug::fpga::register_read(2);
    uint32_t fpga_r3 = radio::debug::fpga::register_read(3);
    text_fpga_reg.set("C:" + to_string_hex(fpga_ctrl, 2) +
                      " D:" + to_string_hex(fpga_r2, 2) +
                      " T:" + to_string_hex(fpga_r3, 2));
#else
    text_fpga_status.set("(CPLD)");
    text_fpga_status.set_style(Theme::getInstance()->fg_medium);
    text_fpga_reg.set("N/A");
#endif

    // Check SGPIO status
    uint32_t sgpio_enable = radio::debug::sgpio::register_read(0);  // CTRL_ENABLE
    uint32_t sgpio_status = radio::debug::sgpio::register_read(4);  // STATUS_1
    bool sgpio_ok = (sgpio_enable != 0);
    text_sgpio_status.set(sgpio_ok ? "ENABLED" : "DISABLED");
    text_sgpio_status.set_style(sgpio_ok ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_orange);
    text_sgpio_reg.set("EN:" + to_string_hex(sgpio_enable, 4) +
                       " ST:" + to_string_hex(sgpio_status, 4));

    // Clock status - check if Si5351 is configured
    // We can't easily read back clock status, so just show assumed state
    text_clock_status.set("(assumed OK)");
    text_clock_status.set_style(Theme::getInstance()->fg_medium);

    // Summary
    bool all_ok = rffc_ok && max_ok;
#ifdef PRALINE
    all_ok = all_ok && fpga_ok;
#endif
    if (all_ok) {
        text_test_result.set("Peripherals responding. Try RX app.");
        text_test_result.set_style(Theme::getInstance()->fg_green);
    } else {
        text_test_result.set("Check failed peripherals above.");
        text_test_result.set_style(Theme::getInstance()->fg_red);
    }
}

#ifdef PRALINE
PralineRadioDebugView::PralineRadioDebugView(NavigationView& nav) {
    add_children({&text_title, &text_lbl_lock, &text_lock_status,
                  &text_lbl_clk5, &text_clk5_status,
                  &text_lbl_spi, &text_spi_status, &text_lbl_fpga_ctrl, &text_fpga_ctrl,
                  &text_lbl_vaa, &text_vaa_status, &text_status_msg,
                  &button_refresh, &button_toggle_clk5, &button_done});

    button_refresh.on_select = [this](Button&) { this->refresh(); };
    button_toggle_clk5.on_select = [this](Button&) { this->toggle_clk5(); };
    button_done.on_select = [&nav](Button&) { nav.pop(); };

    refresh();
}

void PralineRadioDebugView::focus() {
    button_refresh.focus();
}

void PralineRadioDebugView::toggle_clk5() {
    // Si5351 Register 3 is the Output Enable mask. Bit 5 = CLK5.
    // 0 = Enabled, 1 = Disabled.
    uint8_t reg3 = portapack::clock_manager.si5351_read_register(3);
    reg3 ^= 0x20;  // Toggle Bit 5 (CLK5)
    portapack::clock_manager.si5351_write_register(3, reg3);
    refresh();
}

void PralineRadioDebugView::refresh() {
    // 1. Check Mixer Lock Detect (GPIO6[25] / PD_11) [cite: 16, 17]
    uint32_t gpio6_state = LPC_GPIO->PIN[6];
    bool locked = (gpio6_state >> 25) & 1;
    text_lock_status.set(locked ? "LOCKED (OK)" : "UNLOCKED!");
    text_lock_status.set_style(locked ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // 2. Check Si5351 CLK5 Status (Reg 3, Bit 5) - 0 = ON, 1 = OFF
    uint8_t si_reg3 = portapack::clock_manager.si5351_read_register(3);
    bool clk5_on = !(si_reg3 & 0x20);
    text_clk5_status.set(clk5_on ? "ON (40MHz)" : "OFF");
    text_clk5_status.set_style(clk5_on ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // 3. Check SPI Bit Mode (SSP1 CR0)
    // DSS (Data Size Select) is Bit 3:0. 0x8 = 9-bit (MAX2831), 0xF = 16-bit (Classic)
    uint32_t cr0 = LPC_SSP1->CR0;
    uint8_t dss = cr0 & 0x0F;
    if (dss == 0x08)
        text_spi_status.set("9-Bit (Pro)");
    else
        text_spi_status.set("Other (Err)");
    text_spi_status.set_style((dss == 0x08) ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // 4. Check FPGA Register 1 (DC Block status) [cite: 12, 13]
    uint8_t fpga_r1 = radio::debug::fpga::register_read(1);
    text_fpga_ctrl.set(to_string_hex(fpga_r1, 2));

    // 5. Check VAA RF Power (GPIO4[1] / P8_1) - Active LOW [cite: 16, 17]
    uint32_t gpio4_state = LPC_GPIO->PIN[4];
    bool vaa_on = !((gpio4_state >> 1) & 1);
    text_vaa_status.set(vaa_on ? "ON" : "OFF");
    text_vaa_status.set_style(vaa_on ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // Diagnostic Summary
    if (!locked && clk5_on && vaa_on) {
        text_status_msg.set("Clock/Pwr OK. PLL not locked.   Check tuning registers.");
        text_status_msg.set_style(Theme::getInstance()->fg_red);
    } else if (!clk5_on) {
        text_status_msg.set("Mixer Clock is OFF.            PLL cannot lock.");
        text_status_msg.set_style(Theme::getInstance()->fg_red);
    } else {
        text_status_msg.set("Hardware Link Active.");
        text_status_msg.set_style(Theme::getInstance()->fg_green);
    }
}
#endif

#ifdef PRALINE
/* WFMAudioDebugView *************************************************/

WFMAudioDebugView::WFMAudioDebugView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_clk0,
        &text_clk0,
        &text_lbl_fpga_dec,
        &text_fpga_dec,
        &text_lbl_post_fpga,
        &text_post_fpga,
        &text_section1,
        &text_lbl_reg8,
        &text_reg8,
        &text_lbl_lpf_bw,
        &text_lpf_bw,
        &text_section2,
        &text_lbl_fpga_r1,
        &text_fpga_r1,
        &text_lbl_dc_q,
        &text_dc_q,
        &text_section3,
        &text_lbl_expected,
        &text_expected,
        &text_lbl_deemph,
        &text_deemph,
        &text_status,
        &text_status2,
        &button_refresh,
        &button_toggle_q,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_section1.set_style(Theme::getInstance()->fg_yellow);
    text_section2.set_style(Theme::getInstance()->fg_yellow);
    text_section3.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        refresh();
    };

    button_toggle_q.on_select = [this](Button&) {
        uint32_t current = radio::debug::fpga::register_read(1);
        uint8_t new_val = current ^ 0x02;  // Toggle Q_INVERT bit
        radio::debug::fpga::register_write(1, new_val);
        radio::invalidate_spi_config();
        refresh();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh();
}

void WFMAudioDebugView::focus() {
    button_refresh.focus();
}

void WFMAudioDebugView::refresh() {
    // === Si5351 CLK0 Sample Rate ===
    // Read MS0 parameters to calculate frequency
    uint8_t reg44 = portapack::clock_manager.si5351_read_register(44);
    uint8_t reg45 = portapack::clock_manager.si5351_read_register(45);
    uint8_t reg46 = portapack::clock_manager.si5351_read_register(46);

    uint8_t r_div_encoded = (reg44 >> 4) & 0x07;
    uint32_t r_div = 1 << r_div_encoded;
    uint32_t p1 = ((uint32_t)(reg44 & 0x03) << 16) | ((uint32_t)reg45 << 8) | reg46;
    uint32_t ms_div = (p1 + 512) / 128;

    // PLL A is 800 MHz
    uint32_t clk0_khz = 800000 / ms_div / r_div;
    uint32_t clk0_mhz_int = clk0_khz / 1000;
    uint32_t clk0_khz_frac = clk0_khz % 1000;

    text_clk0.set(to_string_dec_uint(clk0_mhz_int) + "." +
                  to_string_dec_uint(clk0_khz_frac / 100) +
                  to_string_dec_uint((clk0_khz_frac / 10) % 10) +
                  to_string_dec_uint(clk0_khz_frac % 10) + " MHz");

    if (clk0_khz >= 3000 && clk0_khz <= 3200) {
        text_clk0.set_style(Theme::getInstance()->fg_green);
    } else {
        text_clk0.set_style(Theme::getInstance()->fg_red);
    }

    // === FPGA Decimation ===
    uint8_t fpga_decim = radio::debug::fpga::register_read(2);
    uint32_t fpga_div = 1 << fpga_decim;
    text_fpga_dec.set("/" + to_string_dec_uint(fpga_div) + " (n=" + to_string_dec_uint(fpga_decim) + ")");

    // === Post-FPGA Rate ===
    uint32_t post_fpga_khz = clk0_khz / fpga_div;
    text_post_fpga.set(to_string_dec_uint(post_fpga_khz) + " kHz");

    // For WFM, post-FPGA should be >= 384 kHz for proper audio decimation
    if (post_fpga_khz >= 384) {
        text_post_fpga.set_style(Theme::getInstance()->fg_green);
    } else {
        text_post_fpga.set_style(Theme::getInstance()->fg_orange);
    }

    // === MAX2831 LPF ===
    uint32_t reg8 = radio::debug::second_if::register_read(8);
    text_reg8.set("0x" + to_string_hex(reg8, 4));

    uint8_t lpf_coarse = reg8 & 0x03;
    const char* lpf_names[] = {"7.5 MHz", "8.5 MHz", "15 MHz", "18 MHz"};
    text_lpf_bw.set(lpf_names[lpf_coarse]);

    // 7.5 MHz is minimum, OK for mono WFM but tight for stereo
    if (lpf_coarse >= 1) {
        text_lpf_bw.set_style(Theme::getInstance()->fg_green);
    } else {
        text_lpf_bw.set_style(Theme::getInstance()->fg_orange);
    }

    // === FPGA Control Register ===
    uint32_t fpga_ctrl = radio::debug::fpga::register_read(1);
    text_fpga_r1.set("0x" + to_string_hex(fpga_ctrl, 2));

    bool dc_block = fpga_ctrl & 0x01;
    bool q_invert = fpga_ctrl & 0x02;
    uint8_t quarter_shift = (fpga_ctrl >> 2) & 0x03;

    text_dc_q.set(std::string(dc_block ? "DC:ON" : "DC:OFF") +
                  " Q:" + std::string(q_invert ? "INV" : "NOR") +
                  " QS:" + to_string_dec_uint(quarter_shift));

    if (dc_block) {
        text_dc_q.set_style(Theme::getInstance()->fg_green);
    } else {
        text_dc_q.set_style(Theme::getInstance()->fg_orange);
    }

    // === Expected Audio Rate ===
    // WFM typically: 3072 kHz / 64 = 48 kHz audio
    // Or: 3072 kHz → /8 (channel) → 384 kHz → /8 (audio) → 48 kHz
    uint32_t expected_audio = post_fpga_khz / 64;  // Simplified assumption
    text_expected.set(to_string_dec_uint(expected_audio) + " kHz (est)");

    if (expected_audio >= 44 && expected_audio <= 50) {
        text_expected.set_style(Theme::getInstance()->fg_green);
    } else {
        text_expected.set_style(Theme::getInstance()->fg_red);
    }

    // === De-emphasis Status ===
    // We can't directly read the M4 de-emphasis config, but we can indicate what SHOULD be set
    // 75µs for USA, 50µs for Europe
    text_deemph.set("Chk M4 cfg.");
    text_deemph.set_style(Theme::getInstance()->fg_orange);

    // === Status Summary ===
    bool sample_rate_ok = (clk0_khz >= 3000 && clk0_khz <= 3200);
    bool lpf_ok = (lpf_coarse <= 0x0F);  // check against the 4-bit max
    bool dc_ok = dc_block;

    if (sample_rate_ok && lpf_ok && dc_ok) {
        text_status.set("Hardware config looks OK.");
        text_status.set_style(Theme::getInstance()->fg_green);
        text_status2.set("If ringy: Chk d-emph in M4!");
        text_status2.set_style(Theme::getInstance()->fg_orange);
    } else {
        std::string issues = "Issues: ";
        if (!sample_rate_ok) issues += "SampleRate ";
        if (!lpf_ok) issues += "LPF ";
        if (!dc_ok) issues += "DC_Block ";
        text_status.set(issues);
        text_status.set_style(Theme::getInstance()->fg_red);
        text_status2.set("Fix before checking audio.");
        text_status2.set_style(Theme::getInstance()->fg_red);
    }
}
#endif

/* BasebandStatusView ******************************************************/

BasebandStatusView::BasebandStatusView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_marker,
        &text_marker,
        &text_lbl_loops,
        &text_loops,
        &text_lbl_wait,
        &text_wait,
        &text_lbl_xfr,
        &text_xfr,
        &text_lbl_missed,
        &text_missed,
        &text_status_line1,
        &text_status_line2,
        &text_status_line3,
        &button_refresh,
        &button_done,
    });

    // Set title color
    text_title.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        update();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Initial update
    update();
}

void BasebandStatusView::focus() {
    button_refresh.focus();
}

void BasebandStatusView::update() {
    // Read counters from shared memory
    uint8_t marker = shared_memory.m4_streaming_marker;
    uint32_t loops = shared_memory.m4_baseband_loops;
    uint32_t wait = shared_memory.m4_dma_wait_count;
    uint32_t xfr = shared_memory.m4_dma_xfr_count;
    uint16_t missed = shared_memory.m4_buffer_missed;

    // Display counter values
    text_marker.set(to_string_hex(marker, 2));
    text_marker.set_style((marker == 0xAA) ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    text_loops.set(to_string_dec_uint(loops));
    text_wait.set(to_string_dec_uint(wait));
    text_xfr.set(to_string_dec_uint(xfr));
    text_missed.set(to_string_dec_uint(missed));

    // Status interpretation
    if (marker == 0x00) {
        text_status_line1.set("Thread NOT started!");
        text_status_line2.set("M4 baseband crash.");
        text_status_line3.set("Check thread race condition.");
        text_status_line1.set_style(Theme::getInstance()->fg_red);
        text_status_line2.set_style(Theme::getInstance()->fg_red);
        text_status_line3.set_style(Theme::getInstance()->fg_red);
    } else if (marker == 0xAA && loops == 0) {
        text_status_line1.set("Thread started but");
        text_status_line2.set("not looping yet.");
        text_status_line3.set("Wait a moment...");
        text_status_line1.set_style(Theme::getInstance()->fg_orange);
        text_status_line2.set_style(Theme::getInstance()->fg_orange);
        text_status_line3.set_style(Theme::getInstance()->fg_orange);
    } else if (marker == 0xAA && xfr == 0) {
        text_status_line1.set("Thread looping " + to_string_dec_uint(loops) + "x");
        text_status_line2.set("But DMA NOT firing!");
        text_status_line3.set("Check SGPIO14 enable.");
        text_status_line1.set_style(Theme::getInstance()->fg_orange);
        text_status_line2.set_style(Theme::getInstance()->fg_orange);
        text_status_line3.set_style(Theme::getInstance()->fg_orange);
    } else if (xfr > 0) {
        text_status_line1.set("DMA WORKING!");
        text_status_line2.set("Xfr: " + to_string_dec_uint(xfr));
        text_status_line3.set("Data flowing to baseband.");
        text_status_line1.set_style(Theme::getInstance()->fg_green);
        text_status_line2.set_style(Theme::getInstance()->fg_green);
        text_status_line3.set_style(Theme::getInstance()->fg_green);
    }
}

/* SGPIOLiveMonitorView ****************************************************/

SGPIOLiveMonitorView::SGPIOLiveMonitorView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_ctrl,
        &text_ctrl,
        &text_lbl_in,
        &text_in,
        &text_lbl_ss,
        &text_ss,
        &text_lbl_status,
        &text_status,
        &text_lbl_out,
        &text_out,
        &text_lbl_oen,
        &text_oen,
        &text_diag_line1,
        &text_diag_line2,
        &text_diag_line3,
        &text_diag_line4,
        &button_refresh,
        &button_done,
    });

    // Set title color
    text_title.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        update();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Initial update
    update();
}

void SGPIOLiveMonitorView::focus() {
    button_refresh.focus();
}

void SGPIOLiveMonitorView::update() {
    // Read SGPIO registers via radio debug namespace
    uint32_t ctrl = radio::debug::sgpio::register_read(0);    // CTRL_ENABLE
    uint32_t in_reg = radio::debug::sgpio::register_read(1);  // GPIO_INREG
    uint32_t status = radio::debug::sgpio::register_read(4);  // STATUS_1

    // Read registers directly from LPC_SGPIO peripheral
    uint32_t reg_ss = LPC_SGPIO->REG_SS[0];
    uint32_t out_reg = LPC_SGPIO->GPIO_OUTREG;
    uint32_t oen_reg = LPC_SGPIO->GPIO_OENREG;

    // Display register values
    text_ctrl.set(to_string_hex(ctrl, 4));
    text_in.set(to_string_hex(in_reg, 8));
    text_ss.set(to_string_hex(reg_ss, 8));
    text_status.set(to_string_hex(status, 4));
    text_out.set(to_string_hex(out_reg, 4));
    text_oen.set(to_string_hex(oen_reg, 4));

    // Diagnostics based on register values
    bool gpio_changing = (in_reg & 0xFF) != 0;  // Check data pins
    bool regss_active = (reg_ss != 0);
    bool disable_high = (out_reg & (1U << 10)) != 0;  // Bit 10 = DISABLE signal
    bool sgpio8_high = (in_reg & (1U << 8)) != 0;     // Bit 8 = SGPIO8 clock
    bool sgpio8_output = (oen_reg & (1U << 8)) != 0;  // Bit 8 = SGPIO8 direction (should be INPUT=0)

    // Line 1: SGPIO8 direction check (CRITICAL - must be INPUT)
    if (sgpio8_output) {
        text_diag_line1.set("SGPIO8 OUTPUT! (bus conflict)");
        text_diag_line1.set_style(Theme::getInstance()->fg_red);
    } else if (disable_high) {
        text_diag_line1.set("DISABLE=HIGH! FPGA stopped!");
        text_diag_line1.set_style(Theme::getInstance()->fg_red);
    } else {
        text_diag_line1.set("SGPIO8=IN, DISABLE=LOW");
        text_diag_line1.set_style(Theme::getInstance()->fg_green);
    }

    // Line 2: Clock signal status (snapshot - can't detect toggling)
    if (disable_high) {
        text_diag_line2.set("Clock N/A (FPGA disabled)");
        text_diag_line2.set_style(Theme::getInstance()->fg_medium);
    } else if (sgpio8_high) {
        text_diag_line2.set("SGPIO8=HIGH (snapshot)");
        text_diag_line2.set_style(Theme::getInstance()->fg_green);
    } else {
        text_diag_line2.set("SGPIO8=LOW (snapshot)");
        text_diag_line2.set_style(Theme::getInstance()->fg_green);
    }

    // Line 3: REG_SS[0] capture status
    if (!regss_active && !disable_high) {
        text_diag_line3.set("REG_SS[0]=0 (NOT CAPTURING!)");
        text_diag_line3.set_style(Theme::getInstance()->fg_red);
    } else if (regss_active) {
        text_diag_line3.set("REG_SS[0] has data");
        text_diag_line3.set_style(Theme::getInstance()->fg_green);
    } else {
        text_diag_line3.set("Capture N/A (FPGA disabled)");
        text_diag_line3.set_style(Theme::getInstance()->fg_medium);
    }

    // Line 4: Summary based on key indicators
    if (sgpio8_output) {
        text_diag_line4.set("FIX: Set SGPIO8 to INPUT!");
        text_diag_line4.set_style(Theme::getInstance()->fg_red);
    } else if (disable_high) {
        text_diag_line4.set("FIX: Clear DISABLE bit!");
        text_diag_line4.set_style(Theme::getInstance()->fg_red);
    } else if (regss_active && gpio_changing) {
        text_diag_line4.set("SGPIO capturing data");
        text_diag_line4.set_style(Theme::getInstance()->fg_green);
    } else if (!regss_active && gpio_changing) {
        text_diag_line4.set("Data present, check slices");
        text_diag_line4.set_style(Theme::getInstance()->fg_orange);
    } else if (!regss_active) {
        text_diag_line4.set("No data activity");
        text_diag_line4.set_style(Theme::getInstance()->fg_orange);
    } else {
        text_diag_line4.set("Check DMA config");
        text_diag_line4.set_style(Theme::getInstance()->fg_green);
    }
}

/* RadioRxTestView ********************************************************/

RadioRxTestView::RadioRxTestView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &labels,
        &console,
        &button_init,
        &button_rx,
        &button_freq,
        &button_sgpio,
        &button_full,
        &button_step,
        &button_done,
    });

    button_init.on_select = [this](Button&) {
        run_init_test();
    };

    button_rx.on_select = [this](Button&) {
        run_rx_mode_test();
    };

    button_freq.on_select = [this](Button&) {
        run_freq_test();
    };

    button_sgpio.on_select = [this](Button&) {
        run_sgpio_test();
    };

    button_full.on_select = [this](Button&) {
        run_full_test();
    };

    button_step.on_select = [this](Button&) {
        run_step_test();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    log("Ready. Press buttons to test.");
    log("Init->RX->Freq->SGPIO");
}

void RadioRxTestView::focus() {
    button_full.focus();
}

void RadioRxTestView::log(const std::string& msg) {
    console.writeln(msg);
}

void RadioRxTestView::log_registers(const std::string& label) {
    // RFFC5072 register 0
    uint32_t rffc_r0 = radio::debug::first_if::register_read(0);

    // MAX283x registers 0, 3, 4 (key freq regs)
    uint32_t max_r0 = radio::debug::second_if::register_read(0);
    uint32_t max_r3 = radio::debug::second_if::register_read(3);
    uint32_t max_r4 = radio::debug::second_if::register_read(4);

    // SGPIO
    uint32_t sgpio_en = radio::debug::sgpio::register_read(0);
    uint32_t sgpio_data = radio::debug::sgpio::register_read(5);

    log(label);
    log(" RFFC:" + to_string_hex(rffc_r0, 4));
    log(" MAX r0:" + to_string_hex(max_r0, 4) +
        " r3:" + to_string_hex(max_r3, 4) +
        " r4:" + to_string_hex(max_r4, 4));
    log(" SGPIO en:" + to_string_hex(sgpio_en, 4) +
        " dat:" + to_string_hex(sgpio_data, 8));

#ifdef PRALINE
    uint32_t fpga_ctrl = radio::debug::fpga::register_read(1);
    log(" FPGA ctrl:" + to_string_hex(fpga_ctrl, 2));
#endif
}

void RadioRxTestView::run_init_test() {
    console.clear(true);
    log("=== INIT TEST ===");

    log("radio::init()...");
    radio::init();
    radio_initialized_ = true;

    log("set_baseband_rate(8M)...");
    radio::set_baseband_rate(8000000);

    // Read Si5351 status to check PLL lock
    uint8_t si_status = portapack::clock_manager.si5351_read_status();
    log("Si5351 status: " + to_string_hex(si_status, 2));
    if (si_status & 0x20) {
        log("  WARNING: PLL A unlocked!");
    } else {
        log("  PLL A locked OK");
    }

    // Read crystal cap register
    uint8_t xtal_cap = portapack::clock_manager.si5351_read_register(183);
    log("Crystal cap: " + to_string_hex(xtal_cap, 2));

    log_registers("[After init]");
    log("Init+clocks done.");
}

void RadioRxTestView::run_rx_mode_test() {
    console.clear(true);
    log("=== RX MODE TEST ===");

    if (!radio_initialized_) {
        log("ERROR: Run Init first!");
        return;
    }

    log_registers("[Before RX mode]");

    log("Calling set_direction(Receive)...");
    radio::set_direction(rf::Direction::Receive);

    log_registers("[After RX mode]");

    // Check MAX283x mode register
    uint32_t max_r0 = radio::debug::second_if::register_read(0);
    log("MAX r0 after RX: " + to_string_hex(max_r0, 4));
    log("RX mode set.");
}

void RadioRxTestView::run_freq_test() {
    console.clear(true);
    log("=== FREQ TEST ===");

    if (!radio_initialized_) {
        log("ERROR: Run Init first!");
        return;
    }

    log_registers("[Before freq set]");

    log("Setting " + to_string_dec_uint(test_frequency_ / 1000000) + " MHz...");
    bool result = radio::set_tuning_frequency(test_frequency_);

    log_registers("[After freq set]");

    log(result ? "Freq set OK" : "Freq set FAILED");

    // Show expected vs actual for MAX2831 freq regs
    // For 433 MHz with MAX2831: F_LO = 40M * (N + F/2^20) / 2
    // N = 43, F = ~629146 for ~433 MHz
    log("(Expected: N~43 in r3, F_hi in r4)");
}

void RadioRxTestView::run_sgpio_test() {
    console.clear(true);
    log("=== SGPIO TEST (FIXED) ===");

    // CRITICAL FIX: Set DISABLE=HIGH first (reference HackRF pattern)
    LPC_SGPIO->GPIO_OENREG = (1U << 10) | (1U << 11);  // SGPIO10,11 outputs
    LPC_SGPIO->GPIO_OUTREG = (1U << 10);               // DISABLE=HIGH during config
    log("Set DISABLE=HIGH");

    // Small delay for signals to settle
    for (volatile int i = 0; i < 10000; i++) {
    }

    // NOW enable streaming (DISABLE=LOW)
    LPC_SGPIO->GPIO_OUTREG = 0;  // DISABLE=LOW, DIRECTION=LOW (RX)
    log("Set DISABLE=LOW (streaming)");
    for (volatile int i = 0; i < 10000; i++) {
    }

    // Read raw GPIO_INREG multiple times
    uint32_t g[4];
    for (int i = 0; i < 4; i++) {
        g[i] = LPC_SGPIO->GPIO_INREG;
        for (volatile int j = 0; j < 10000; j++) {
        }
    }

    log("GPIO_IN:");
    log(" " + to_string_hex(g[0], 8) + " " + to_string_hex(g[1], 8));
    log(" " + to_string_hex(g[2], 8) + " " + to_string_hex(g[3], 8));

    bool changing = (g[0] != g[1]) || (g[1] != g[2]) || (g[2] != g[3]);

    if (changing) {
        log("PASS: Data changing!");
    } else if (g[0] == 0) {
        log("FAIL: All zeros");
    } else if (g[0] == 0x00000FFF) {
        log("FAIL: 0xFFF = pull-ups");
        log("FPGA not driving data");
    } else {
        log("FAIL: Static " + to_string_hex(g[0], 8));
    }

    uint32_t out = LPC_SGPIO->GPIO_OUTREG;
    log("HOST_DIS=" + to_string_dec_uint((out >> 10) & 1));
}

void RadioRxTestView::run_full_test() {
    console.clear(true);
    log("=== FULL RX TEST ===");

    // Step 1: Init
    log("[1/6] Init radio...");
    radio::init();
    radio_initialized_ = true;

    // Step 2: Set sample rate (configures Si5351 clocks!)
    log("[2/6] Set 8M sample rate...");
    radio::set_baseband_rate(8000000);

    // Step 3: RX mode
    log("[3/6] Set RX mode...");
    radio::set_direction(rf::Direction::Receive);

    // Step 4: Frequency - use 2437 MHz (WiFi ch6) which is in MAX2831 range
    uint32_t wifi_freq = 2437000000;
    log("[4/6] Set 2437 MHz (WiFi)...");
    bool freq_ok = radio::set_tuning_frequency(wifi_freq);
    log(freq_ok ? "  Freq OK" : "  Freq FAIL");

    // Step 5: Configure SGPIO outputs with correct DISABLE sequence
    log("[5/6] Configure SGPIO...");
    // CRITICAL FIX: Set DISABLE=HIGH first
    LPC_SGPIO->GPIO_OENREG = (1U << 10) | (1U << 11);  // SGPIO10,11 as outputs
    LPC_SGPIO->GPIO_OUTREG = (1U << 10);               // DISABLE=HIGH during config
    log("  DISABLE=HIGH");

    // Delay for settle
    for (volatile int i = 0; i < 10000; i++) {
    }

    // NOW enable streaming (DISABLE=LOW)
    LPC_SGPIO->GPIO_OUTREG = 0;  // DISABLE=LOW, DIRECTION=LOW (RX)
    log("  DISABLE=LOW (streaming)");

    // Step 6: Check raw GPIO pins
    log("[6/6] Check GPIO pins...");

    // Delay for stabilization
    for (volatile int i = 0; i < 200000; i++) {
    }

    // Read raw GPIO_INREG multiple times
    uint32_t g1 = LPC_SGPIO->GPIO_INREG;
    for (volatile int i = 0; i < 10000; i++) {
    }
    uint32_t g2 = LPC_SGPIO->GPIO_INREG;
    for (volatile int i = 0; i < 10000; i++) {
    }
    uint32_t g3 = LPC_SGPIO->GPIO_INREG;
    for (volatile int i = 0; i < 10000; i++) {
    }
    uint32_t g4 = LPC_SGPIO->GPIO_INREG;

    log("GPIO_IN readings:");
    log(" " + to_string_hex(g1, 8) + " " + to_string_hex(g2, 8));
    log(" " + to_string_hex(g3, 8) + " " + to_string_hex(g4, 8));

    bool gpio_changing = (g1 != g2) || (g2 != g3) || (g3 != g4);
    bool gpio_not_zero = (g1 != 0);
    bool gpio_not_fff = (g1 != 0x00000FFF);

    // Check Si5351 output enable register (reg 3)
    // Bits 0-7: CLK0-7 output enable (0=enabled, 1=disabled)
    // We want CLK0 and CLK1 enabled (bits 0,1 = 0)
    log("---");

    if (gpio_changing) {
        log("=== PASS: Data flowing! ===");
    } else if (gpio_not_fff && gpio_not_zero) {
        log("=== PARTIAL: Static data ===");
        log("FPGA outputs but no clock?");
    } else if (!gpio_not_zero) {
        log("=== FAIL: All zeros ===");
        log("FPGA not driving outputs");
    } else {
        log("=== FAIL: All FFF (pull-ups) ===");
        log("FPGA outputs high-Z");
        log("Check: Si5351 CLK0/CLK1");
    }

    log_registers("[Final]");
}

bool RadioRxTestView::check_gpio_changing() {
    uint32_t g[4];
    for (int i = 0; i < 4; i++) {
        g[i] = LPC_SGPIO->GPIO_INREG;
        for (volatile int j = 0; j < 10000; j++) {
        }
    }
    return (g[0] != g[1]) || (g[1] != g[2]) || (g[2] != g[3]);
}

void RadioRxTestView::run_step_test() {
    console.clear(true);
    log("=== STEP TEST (FIXED) ===");
    log("Correct DISABLE sequence");

    // Ensure radio is initialized
    if (!radio_initialized_) {
        log("Init radio...");
        radio::init();
        radio_initialized_ = true;
        radio::set_baseband_rate(8000000);
        radio::set_direction(rf::Direction::Receive);
        radio::set_tuning_frequency(2437000000);
    }

    // Step 0: Baseline with DISABLE=HIGH first
    log("[0] Baseline (DISABLE=HIGH)");
    LPC_SGPIO->CTRL_ENABLE = 0;           // Disable all slices
    LPC_SGPIO->GPIO_OENREG = 0x0C00;      // Bits 10, 11 outputs
    LPC_SGPIO->GPIO_OUTREG = (1U << 10);  // DISABLE=HIGH first!
    for (volatile int i = 0; i < 100000; i++) {
    }

    // Now enable streaming to check baseline
    LPC_SGPIO->GPIO_OUTREG = 0x0000;  // DISABLE=LOW
    for (volatile int i = 0; i < 100000; i++) {
    }
    bool step0 = check_gpio_changing();
    log(step0 ? "  PASS: Data changing" : "  FAIL: Data static");
    if (!step0) {
        log("ABORT: Baseline broken");
        return;
    }

    // NOW disable streaming for configuration
    log("[Config] Set DISABLE=HIGH");
    LPC_SGPIO->GPIO_OUTREG = (1U << 10);  // DISABLE=HIGH
    for (volatile int i = 0; i < 100000; i++) {
    }

    // Step 1: OUT_MUX_CFG data pins - test individually
    log("[1] OUT_MUX_CFG[0-7] data");
    uint32_t data_out_mux = (9U << 0) | (0U << 4);  // DOUT_DOUTM8A, GPIO_OE

    for (size_t i = 0; i < 8; i++) {
        uint32_t before = LPC_SGPIO->GPIO_INREG;
        LPC_SGPIO->OUT_MUX_CFG[i] = data_out_mux;
        for (volatile int j = 0; j < 50000; j++) {
        }
        uint32_t after = LPC_SGPIO->GPIO_INREG;
        bool ok = check_gpio_changing();

        log("  [" + to_string_dec_uint(i) + "] " +
            to_string_hex(before & 0xFF, 2) + "->" +
            to_string_hex(after & 0xFF, 2) +
            (ok ? " OK" : " FAIL"));

        if (!ok) {
            log("CULPRIT: OUT_MUX_CFG[" + to_string_dec_uint(i) + "]");
            return;
        }
    }
    log("  All data pins PASS");

    // Step 2: OUT_MUX_CFG control pins - SKIP PIN 10 (HOST_DISABLE)
    log("[2] OUT_MUX_CFG ctrl pins");
    log("  (skipping pin 10 - breaks)");

    struct {
        int pin;
        uint32_t val;
    } ctrl_pins[] = {
        {8, (0U << 0) | (0U << 4)},
        {9, (0U << 0) | (0U << 4)},
        // {10, (4U << 0) | (0U << 4)},  // SKIP - causes failure
        {11, (4U << 0) | (0U << 4)},
        {14, (0U << 0) | (0U << 4)}};

    for (auto& p : ctrl_pins) {
        uint32_t before = LPC_SGPIO->GPIO_INREG;
        LPC_SGPIO->OUT_MUX_CFG[p.pin] = p.val;
        for (volatile int i = 0; i < 50000; i++) {
        }
        uint32_t after = LPC_SGPIO->GPIO_INREG;
        bool ok = check_gpio_changing();

        log("  [" + to_string_dec_uint(p.pin) + "] " +
            to_string_hex(before & 0xFF, 2) + "->" +
            to_string_hex(after & 0xFF, 2) +
            (ok ? " OK" : " FAIL"));

        if (!ok) {
            log("CULPRIT: OUT_MUX_CFG[" + to_string_dec_uint(p.pin) + "]");
            return;
        }
    }
    log("  All ctrl pins PASS");

    // Step 3: Set GPIO_OENREG for RX
    log("[3] GPIO_OENREG full RX");
    LPC_SGPIO->GPIO_OENREG = 0x0C00;  // Keep same as baseline
    for (volatile int i = 0; i < 100000; i++) {
    }
    bool step3 = check_gpio_changing();
    log(step3 ? "  PASS" : "  FAIL: Data stopped!");
    if (!step3) {
        log("CULPRIT: GPIO_OENREG");
        return;
    }

    // Step 3.5: Configure slice D as clock source (CRITICAL!)
    log("[3.5] Slice D clock source");
    const uint32_t slice_d = 3;
    // SGPIO_MUX_CFG: External clock from SGPIO8, qualifier from SGPIO9
    LPC_SGPIO->SGPIO_MUX_CFG[slice_d] = (1U << 0) | (0U << 1) | (0U << 3) | (3U << 5) | (1U << 7) | (0U << 9) | (0U << 11) | (0U << 12);
    // SLICE_MUX_CFG: 1 bit per clock, CLKGEN_MODE=1 (external clock!) <- FIX
    LPC_SGPIO->SLICE_MUX_CFG[slice_d] = (0U << 0) | (0U << 1) | (0U << 2) | (0U << 3) | (1U << 4) | (0U << 6) | (0U << 8);
    LPC_SGPIO->PRESET[slice_d] = 0;
    LPC_SGPIO->COUNT[slice_d] = 0;
    LPC_SGPIO->POS[slice_d] = (0x1F << 0) | (0x1F << 8);
    LPC_SGPIO->REG[slice_d] = 0x11111111;
    LPC_SGPIO->REG_SS[slice_d] = 0x11111111;
    // Enable slice D counter
    LPC_SGPIO->CTRL_ENABLE = (1U << slice_d);
    for (volatile int i = 0; i < 100000; i++) {
    }
    bool step3_5 = check_gpio_changing();
    log(step3_5 ? "  PASS" : "  FAIL: Data stopped!");
    if (!step3_5) {
        log("CULPRIT: Slice D config");
        return;
    }

    // Step 4: SGPIO_MUX_CFG slice A
    log("[4] SGPIO_MUX_CFG[A]");
    LPC_SGPIO->SGPIO_MUX_CFG[0] = (1U << 0) | (0U << 1) | (3U << 3) | (3U << 5) | (1U << 7) | (0U << 9) | (0U << 11) | (0U << 12);  // Clock from slice D (bit3-4=3), external pin SGPIO8, qualifier SGPIO9
    for (volatile int i = 0; i < 100000; i++) {
    }
    bool step4 = check_gpio_changing();
    log(step4 ? "  PASS" : "  FAIL: Data stopped!");
    if (!step4) {
        log("CULPRIT: SGPIO_MUX_CFG[A]");
        return;
    }

    // Step 5: SLICE_MUX_CFG slice A
    log("[5] SLICE_MUX_CFG[A]");
    LPC_SGPIO->SLICE_MUX_CFG[0] = (0U << 0) | (0U << 1) | (1U << 2) | (0U << 3) | (1U << 4) | (3U << 6) | (0U << 8);  // CLKGEN_MODE=1 (external clock!), PARALLEL_MODE 1 byte
    for (volatile int i = 0; i < 100000; i++) {
    }
    bool step5 = check_gpio_changing();
    log(step5 ? "  PASS" : "  FAIL: Data stopped!");
    if (!step5) {
        log("CULPRIT: SLICE_MUX_CFG[A]");
        return;
    }

    // Step 6: Slice A registers
    log("[6] Slice A registers");
    LPC_SGPIO->PRESET[0] = 0;
    LPC_SGPIO->COUNT[0] = 0;
    LPC_SGPIO->POS[0] = (0x1F << 0) | (0x1F << 8);  // pos, pos_reset
    LPC_SGPIO->REG[0] = 0;
    LPC_SGPIO->REG_SS[0] = 0;
    for (volatile int i = 0; i < 100000; i++) {
    }
    bool step6 = check_gpio_changing();
    log(step6 ? "  PASS" : "  FAIL: Data stopped!");
    if (!step6) {
        log("CULPRIT: Slice A registers");
        return;
    }

    // Step 7: Enable slice A counter (keep slice D enabled) - still with DISABLE=HIGH
    log("[7] Enable slices D+A");
    LPC_SGPIO->CTRL_ENABLE = (1U << 3) | (1U << 0);  // Slice D + Slice A
    for (volatile int i = 0; i < 100000; i++) {
    }

    // Check STATUS_1 BEFORE enabling streaming
    uint32_t status_pre = LPC_SGPIO->STATUS_1;
    log("  STATUS_1 (pre): " + to_string_hex(status_pre, 4));

    // Step 8: Enable streaming (DISABLE=LOW) - THIS IS THE CRITICAL TEST
    log("[8] Enable streaming (DISABLE=LOW)");
    LPC_SGPIO->GPIO_OUTREG = 0;  // DISABLE=LOW
    for (volatile int i = 0; i < 100000; i++) {
    }

    // Check if slices become active
    uint32_t status_post = LPC_SGPIO->STATUS_1;
    uint32_t regss = LPC_SGPIO->REG_SS[0];
    uint32_t count_a = LPC_SGPIO->COUNT[0];

    log("  STATUS_1 (post): " + to_string_hex(status_post, 4));
    log("  REG_SS[0]: " + to_string_hex(regss, 8));
    log("  COUNT[0]: " + to_string_hex(count_a, 8));

    bool step8 = check_gpio_changing();
    log(step8 ? "  GPIO still changing" : "  GPIO stopped!");

    if ((status_post & 1) && regss != 0) {
        log("=== SUCCESS! Slice A capturing! ===");
    } else if (status_post & 1) {
        log("=== PARTIAL: Slice A active but REG_SS=0 ===");
    } else {
        log("=== FAIL: Slice A not active ===");
        log("Expected: STATUS_1 bit 0 = 1");
        log("Actual: STATUS_1 bit 0 = " + to_string_dec_uint(status_post & 1));
    }
}

/* SGPIO8ClockDetectorView ***********************************************/

SGPIO8ClockDetectorView::SGPIO8ClockDetectorView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_samples,
        &text_samples,
        &text_lbl_toggles,
        &text_toggles,
        &text_status,
        &button_sample,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);

    button_sample.on_select = [this](Button&) {
        sample_sgpio8();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Auto-sample on load
    sample_sgpio8();
}

void SGPIO8ClockDetectorView::focus() {
    button_sample.focus();
}

void SGPIO8ClockDetectorView::sample_sgpio8() {
    // Sample SGPIO8 (bit 8 of GPIO_INREG) as fast as possible
    // NOTE: Software sampling cannot accurately measure clock frequency
    // This only detects presence/absence of clock activity
    const int num_samples = 2000;
    uint8_t samples[num_samples];

    // Sample as fast as possible
    for (int i = 0; i < num_samples; i++) {
        samples[i] = (LPC_SGPIO->GPIO_INREG >> 8) & 1;
    }

    // Count toggles (transitions 0→1 or 1→0)
    int toggles = 0;
    for (int i = 1; i < num_samples; i++) {
        if (samples[i] != samples[i - 1]) {
            toggles++;
        }
    }

    // Display first 20 samples
    std::string sample_str;
    for (int i = 0; i < 20 && i < num_samples; i++) {
        sample_str += (samples[i] ? "1" : "0");
    }
    text_samples.set(sample_str);

    // Display toggle count
    text_toggles.set(to_string_dec_uint(toggles) + " / " +
                     to_string_dec_uint(num_samples - 1) + " transitions");

    // Status interpretation - just presence detection
    if (toggles > 100) {
        text_status.set("CLOCK ACTIVE");
        text_status.set_style(Theme::getInstance()->fg_green);
    } else if (toggles > 0) {
        text_status.set("SOME ACTIVITY (" + to_string_dec_uint(toggles) + ")");
        text_status.set_style(Theme::getInstance()->fg_orange);
    } else {
        text_status.set("NO CLOCK - Stuck " +
                        std::string(samples[0] ? "HIGH" : "LOW"));
        text_status.set_style(Theme::getInstance()->fg_red);
    }
}

/* Si5351DebugView *******************************************************/

Si5351DebugView::Si5351DebugView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_title,
                  &text_status_label,
                  &text_status_value,
                  &text_pll_a_label,
                  &text_pll_a_status,
                  &text_pll_b_label,
                  &text_pll_b_status,
                  &text_sys_init_label,
                  &text_sys_init_status,
                  &text_xtal_cap_label,
                  &text_xtal_cap_value,
                  &text_clk0_label,
                  &text_clk0_status,
                  &text_clk0_freq_value,
                  &text_clk0_div_value,
                  &text_clk1_label,
                  &text_clk1_status,
                  &text_clk4_label,
                  &text_clk4_status,
                  &text_clk5_label,
                  &text_clk5_status,
                  &button_refresh,
                  &button_reset_pll,
                  &button_done});

    text_title.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        refresh_status();
    };

    button_reset_pll.on_select = [this](Button&) {
        reset_pll();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Auto-refresh on load
    refresh_status();
}

void Si5351DebugView::focus() {
    button_refresh.focus();
}

void Si5351DebugView::refresh_status() {
    // Read device status register (reg 0)
    uint8_t status = portapack::clock_manager.si5351_read_status();
    text_status_value.set("0x" + to_string_hex(status, 2));

    // Decode status bits
    bool pll_a_locked = !(status & 0x20);  // Bit 5: LOL_A (Loss of Lock A)
    bool pll_b_locked = !(status & 0x40);  // Bit 6: LOL_B (Loss of Lock B)
    bool sys_init = (status & 0x80);       // Bit 7: SYS_INIT
    // bool los_clkin = (status & 0x10);      // Bit 4: LOS (Loss of Signal) (quoted as it's computed but unused)

    // PLL A status
    if (pll_a_locked) {
        text_pll_a_status.set("LOCKED");
        text_pll_a_status.set_style(Theme::getInstance()->fg_green);
    } else {
        text_pll_a_status.set("UNLOCKED");
        text_pll_a_status.set_style(Theme::getInstance()->fg_red);
    }

    // PLL B status
    if (pll_b_locked) {
        text_pll_b_status.set("LOCKED");
        text_pll_b_status.set_style(Theme::getInstance()->fg_green);
    } else {
        text_pll_b_status.set("UNLOCKED (unused)");
        text_pll_b_status.set_style(Theme::getInstance()->fg_orange);
    }

    // SYS_INIT status
    if (sys_init) {
        text_sys_init_status.set("IN PROGRESS");
        text_sys_init_status.set_style(Theme::getInstance()->fg_orange);
    } else {
        text_sys_init_status.set("COMPLETE");
        text_sys_init_status.set_style(Theme::getInstance()->fg_green);
    }

    // Read crystal load capacitance (reg 183)
    uint8_t xtal_cap = portapack::clock_manager.si5351_read_register(183);
    text_xtal_cap_value.set("0x" + to_string_hex(xtal_cap, 2) +
                            " (" + to_string_dec_uint((xtal_cap >> 6) & 0x03) + ")");

    // Read clock output enables (reg 16-23 control, reg 3 for output enable mask)
    uint8_t output_enable_mask = portapack::clock_manager.si5351_read_register(3);

    // CLK0 (bit 0 of reg 3, reg 16 for control)
    uint8_t clk0_ctrl = portapack::clock_manager.si5351_read_register(16);
    bool clk0_enabled = !(output_enable_mask & 0x01) && !(clk0_ctrl & 0x80);
    text_clk0_status.set(clk0_enabled ? "ON" : "OFF");
    text_clk0_status.set_style(clk0_enabled ? Theme::getInstance()->fg_green
                                            : Theme::getInstance()->fg_red);

    // Read MS0 multisynth parameters (registers 42-49) to calculate actual frequency
    // Si5351 MS0 Register Layout:
    // Reg 42: P3[15:8]
    // Reg 43: P3[7:0]
    // Reg 44: bits 6:4 = R_DIV[2:0], bits 1:0 = P1[17:16]
    // Reg 45: P1[15:8]
    // Reg 46: P1[7:0]
    // Reg 47: bits 7:4 = P3[19:16], bits 3:0 = P2[19:16]
    // Reg 48: P2[15:8]
    // Reg 49: P2[7:0]
    uint8_t reg44 = portapack::clock_manager.si5351_read_register(44);
    uint8_t reg45 = portapack::clock_manager.si5351_read_register(45);
    uint8_t reg46 = portapack::clock_manager.si5351_read_register(46);

    // Decode R divider from bits 6:4 of register 44
    uint8_t r_div_encoded = (reg44 >> 4) & 0x07;
    uint32_t r_div = 1 << r_div_encoded;  // R = 2^r_div_encoded

    // Decode P1 (18-bit value): bits 1:0 of reg44 = P1[17:16], reg45 = P1[15:8], reg46 = P1[7:0]
    uint32_t p1 = ((uint32_t)(reg44 & 0x03) << 16) | ((uint32_t)reg45 << 8) | reg46;

    // Calculate divider: a = (P1 + 512) / 128 for integer dividers (b=0)
    // Expected for 8 MHz: P1=5888 (0x1700), a=50
    uint32_t ms_div = (p1 + 512) / 128;  // Integer divider value

    // Calculate frequency: f_out = 800 or 800 MHz / ms_div / r_div
    uint32_t vco_khz = (clk0_ctrl & 0x20) ? 800000 : 800000;  // PLLB vs PLLA
    uint32_t freq_khz = vco_khz / ms_div / r_div;             // Result in kHz

    // Show P1 value and R45 for debugging
    text_clk0_freq_value.set("F:" + to_string_dec_uint(freq_khz / 1000) + "MHz (P1:" + to_string_hex(p1, 4) + ")");
    text_clk0_div_value.set("DIV: MS=" + to_string_dec_uint(ms_div) +
                            " R=" + to_string_dec_uint(r_div));

    // Color code based on expected 8 MHz
    if (freq_khz >= 7900 && freq_khz <= 8100) {
        text_clk0_freq_value.set_style(Theme::getInstance()->fg_green);
        text_clk0_div_value.set_style(Theme::getInstance()->fg_green);
    } else {
        text_clk0_freq_value.set_style(Theme::getInstance()->fg_red);
        text_clk0_div_value.set_style(Theme::getInstance()->fg_red);
    }

    // CLK1 (bit 1 of reg 3, reg 17 for control)
    uint8_t clk1_ctrl = portapack::clock_manager.si5351_read_register(17);
    bool clk1_enabled = !(output_enable_mask & 0x02) && !(clk1_ctrl & 0x80);
    text_clk1_status.set(clk1_enabled ? "ON" : "OFF");
    text_clk1_status.set_style(clk1_enabled ? Theme::getInstance()->fg_green
                                            : Theme::getInstance()->fg_red);

    // CLK4 (MAX2831 reference - bit 4 of reg 3, reg 20 for control)
    uint8_t clk4_ctrl = portapack::clock_manager.si5351_read_register(20);
    bool clk4_enabled = !(output_enable_mask & 0x10) && !(clk4_ctrl & 0x80);
    text_clk4_status.set(clk4_enabled ? "ON (40MHz)" : "OFF");
    text_clk4_status.set_style(clk4_enabled ? Theme::getInstance()->fg_green
                                            : Theme::getInstance()->fg_red);

    // CLK5 (RFFC5072 reference - bit 5 of reg 3, reg 21 for control)
    uint8_t clk5_ctrl = portapack::clock_manager.si5351_read_register(21);
    bool clk5_enabled = !(output_enable_mask & 0x20) && !(clk5_ctrl & 0x80);
    text_clk5_status.set(clk5_enabled ? "ON (40MHz)" : "OFF");
    text_clk5_status.set_style(clk5_enabled ? Theme::getInstance()->fg_green
                                            : Theme::getInstance()->fg_red);
}

void Si5351DebugView::reset_pll() {
    // Reset both PLLs (write to reg 177)
    portapack::clock_manager.si5351_read_register(177);         // Read first
    portapack::clock_manager.si5351_write_register(177, 0xAC);  // Reset both PLLs

    // Small delay for PLL to settle
    chThdSleepMilliseconds(10);

    // Refresh status to show new lock state
    refresh_status();
}

#ifdef PRALINE
/* SignalPathStatusView *************************************************/

SignalPathStatusView::SignalPathStatusView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_max_enable,
        &text_max_enable,
        &text_lbl_max_mode,
        &text_max_mode,
        &text_lbl_rf_path,
        &text_rf_path,
        &text_lbl_filter,
        &text_filter,
        &text_lbl_mixer,
        &text_mixer,
        &text_lbl_rf_amp,
        &text_rf_amp,
        &text_lbl_lna,
        &text_lna,
        &text_lbl_vga,
        &text_vga,
        &text_lbl_fpga_decim,
        &text_fpga_decim,
        &text_lbl_fpga_ctrl_dc_q,
        &text_fpga_ctrl_dc_q,
        &text_lbl_fpga_ctrl_qs,
        &text_fpga_ctrl_qs,
        &text_status,
        &button_refresh,
        &button_toggle_q,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        refresh_status();
    };

    button_toggle_q.on_select = [this](Button&) {
        static bool q_override = false;
        q_override = !q_override;

        uint8_t ctrl_reg = 0x01;  // DC_BLOCK always on
        if (q_override) {
            ctrl_reg |= 0x02;  // Force Q_INVERT ON
        }

        radio::debug::fpga::register_write(1, ctrl_reg);
        radio::invalidate_spi_config();

        refresh_status();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Initial update
    refresh_status();
}

void SignalPathStatusView::focus() {
    button_refresh.focus();
}

void SignalPathStatusView::refresh_status() {
    // Get cached state from radio driver
    rf::Direction direction = radio::debug::get_cached_direction();
    bool rf_amp = radio::debug::get_cached_rf_amp();
    int_fast8_t cached_lna = radio::debug::get_cached_lna_gain();
    int_fast8_t cached_vga = radio::debug::get_cached_vga_gain();

    // Get current band.
    auto current_band = radio::debug::rf_path_info::get_current_band();
    switch (current_band) {
        case rf::path::Band::Low:
            text_filter.set("LOW PASS");
            text_filter.set_style(Theme::getInstance()->fg_green);
            text_mixer.set("ENABLED");
            text_mixer.set_style(Theme::getInstance()->fg_green);
            break;

        case rf::path::Band::Mid:
            text_filter.set("BYPASS");
            text_filter.set_style(Theme::getInstance()->fg_green);
            text_mixer.set("DISABLED");
            text_mixer.set_style(Theme::getInstance()->fg_orange);
            break;

        case rf::path::Band::High:
            text_filter.set("HIGH PASS");
            text_filter.set_style(Theme::getInstance()->fg_green);
            text_mixer.set("ENABLED");
            text_mixer.set_style(Theme::getInstance()->fg_green);
            break;

        default:
            text_filter.set("UNKNOWN");
            text_filter.set_style(Theme::getInstance()->fg_red);
            text_mixer.set("UNKNOWN");
            text_mixer.set_style(Theme::getInstance()->fg_red);
    }

    // Read actual register values to verify
    uint32_t max_r11 = radio::debug::second_if::register_read(11);

    // Decode actual LNA gain from register (bits 6:5)
    uint8_t lna_bits = (max_r11 >> 5) & 0x03;
    int actual_lna_db;
    switch (lna_bits) {
        case 0:
            actual_lna_db = 0;
            break;  // -33 dB from max
        case 2:
            actual_lna_db = 17;
            break;  // -16 dB from max
        case 3:
            actual_lna_db = 33;
            break;  // Maximum
        default:
            actual_lna_db = -1;
            break;  // Invalid
    }

    // Decode actual VGA gain from register (bits 4:0)
    uint8_t vga_bits = (max_r11 >> 0) & 0x1F;
    int actual_vga_db = vga_bits * 2;  // 0-31 → 0-62 dB

    // MAX2831 Enable/Mode (GPIO-controlled, show cached state)
    bool rx_mode = (direction == rf::Direction::Receive);

    text_max_enable.set("ENABLED cached");
    text_max_enable.set_style(Theme::getInstance()->fg_green);

    text_max_mode.set(rx_mode ? "RX cached" : "TX cached");
    text_max_mode.set_style(rx_mode ? Theme::getInstance()->fg_green
                                    : Theme::getInstance()->fg_orange);

    // RF path direction
    text_rf_path.set(rx_mode ? "RECEIVE" : "TRANSMIT");
    text_rf_path.set_style(rx_mode ? Theme::getInstance()->fg_green
                                   : Theme::getInstance()->fg_orange);

    // RF amp (GPIO-controlled, show cached state)
    text_rf_amp.set(rf_amp ? "ON cached" : "OFF cached");
    text_rf_amp.set_style(rf_amp ? Theme::getInstance()->fg_green
                                 : Theme::getInstance()->fg_orange);

    // LNA gain - show both cached and actual
    if (actual_lna_db == cached_lna) {
        text_lna.set(to_string_dec_uint(actual_lna_db) + " dB");
        text_lna.set_style(Theme::getInstance()->fg_green);
    } else if (actual_lna_db >= 0) {
        // MAX2831 has discrete steps: 0, 17, 33 dB
        // Allow ±8 dB tolerance for rounding
        int diff = (actual_lna_db > cached_lna) ? (actual_lna_db - cached_lna) : (cached_lna - actual_lna_db);

        if (diff <= 8) {
            // Within rounding tolerance - show as OK with note
            text_lna.set(to_string_dec_uint(actual_lna_db) + " dB (req:" +
                         to_string_dec_uint(cached_lna) + ")");
            text_lna.set_style(Theme::getInstance()->fg_green);
        } else {
            // Genuine mismatch
            text_lna.set(to_string_dec_uint(actual_lna_db) + " dB (!" +
                         to_string_dec_uint(cached_lna) + ")");
            text_lna.set_style(Theme::getInstance()->fg_red);
        }
    } else {
        text_lna.set("INVALID");
        text_lna.set_style(Theme::getInstance()->fg_red);
    }

    // VGA gain - show both cached and actual
    if (actual_vga_db == cached_vga) {
        text_vga.set(to_string_dec_uint(actual_vga_db) + " dB");
        text_vga.set_style(Theme::getInstance()->fg_green);
    } else {
        text_vga.set(to_string_dec_uint(actual_vga_db) + " dB (!" +
                     to_string_dec_uint(cached_vga) + ")");
        text_vga.set_style(Theme::getInstance()->fg_red);
    }

    // FPGA decimation register
    uint8_t fpga_decim = radio::debug::fpga::register_read(2);
    text_fpga_decim.set("n=" + to_string_dec_uint(fpga_decim) +
                        " (/" + to_string_dec_uint(1 << fpga_decim) + ")");

    // FPGA Register Ctrl Info
    uint32_t fpga_ctrl = radio::debug::fpga::register_read(1);
    // text_fpga_ctrl_dc_q.set(to_string_hex(fpga_ctrl, 2));
    // text_fpga_ctrl_qs.set(to_string_hex(fpga_ctrl, 2));

    // Decode bits
    bool dc_block = fpga_ctrl & 0x01;
    bool q_invert = fpga_ctrl & 0x02;
    uint8_t quarter_shift = (fpga_ctrl >> 2) & 0x03;

    // Display human-readable
    std::string fpga_status_dc_q = "DC:" + std::string(dc_block ? "ON" : "OFF") +
                                   " Q:" + std::string(q_invert ? "INV" : "NOR");

    std::string fpga_status_qs = "QS:" + to_string_dec_uint(quarter_shift);

    text_fpga_ctrl_dc_q.set(fpga_status_dc_q);
    text_fpga_ctrl_qs.set(fpga_status_qs);

    // Summary status
    // Summary status - update to account for rounding tolerance
    int lna_diff = (actual_lna_db > cached_lna) ? (actual_lna_db - cached_lna) : (cached_lna - actual_lna_db);
    bool lna_ok = (actual_lna_db == cached_lna) || (lna_diff <= 8);
    bool gains_match = lna_ok && (actual_vga_db == cached_vga);
    if (rx_mode && gains_match) {
        text_status.set("RX mode, gains verified!");
        text_status.set_style(Theme::getInstance()->fg_green);
    } else if (rx_mode && !gains_match) {
        text_status.set("RX mode, gain MISMATCH!");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (gains_match) {
        text_status.set("TX mode, gains verified ✓");
        text_status.set_style(Theme::getInstance()->fg_orange);
    } else {
        text_status.set("TX mode, gain MISMATCH!");
        text_status.set_style(Theme::getInstance()->fg_red);
    }
}
#endif

#ifdef PRALINE
/* SystemDiagnosticsView *************************************************/
SystemDiagnosticsView::SystemDiagnosticsView(NavigationView& nav) {
    add_children({
        &text_title,
        &text_lbl_sample,
        &text_sample_rate,
        &text_lbl_band,
        &text_band,
        &button_sample_2m,
        &button_sample_4m,
        &button_sample_8m,
        &button_sample_20m,
        &text_lbl_bb_filter,
        &text_bb_filter,
        &text_lbl_reg8,
        &text_reg8,
        &text_lbl_gpio,
        &text_lbl_lpf,
        &text_gpio_lpf,
        &text_lbl_mix,
        &text_gpio_mix,
        &text_lbl_amp,
        &text_gpio_amp,
        &text_lbl_fpga,
        &text_lbl_fpga_ctrl,
        &text_fpga_ctrl,
        &text_lbl_fpga_decode,
        &button_toggle_q,
        &button_toggle_dc,
        &button_refresh,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_gpio.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_fpga.set_style(Theme::getInstance()->fg_yellow);

    // Sample rate buttons
    button_sample_2m.on_select = [this](Button&) {
        set_sample_rate(2000000);
    };

    button_sample_4m.on_select = [this](Button&) {
        set_sample_rate(4000000);
    };

    button_sample_8m.on_select = [this](Button&) {
        set_sample_rate(8000000);
    };

    button_sample_20m.on_select = [this](Button&) {
        set_sample_rate(20000000);
    };

    // Q inversion toggle
    button_toggle_q.on_select = [this](Button&) {
        uint32_t current = radio::debug::fpga::register_read(1);
        uint8_t new_val = current ^ 0x02;  // Toggle Q_INVERT bit
        radio::debug::fpga::register_write(1, new_val);
        radio::invalidate_spi_config();
        refresh();
    };

    // DC block toggle
    button_toggle_dc.on_select = [this](Button&) {
        uint32_t current = radio::debug::fpga::register_read(1);
        uint8_t new_val = current ^ 0x01;  // Toggle DC_BLOCK bit
        radio::debug::fpga::register_write(1, new_val);
        radio::invalidate_spi_config();
        refresh();
    };

    button_refresh.on_select = [this](Button&) {
        refresh();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh();
}

void SystemDiagnosticsView::focus() {
    button_refresh.focus();
}

void SystemDiagnosticsView::set_sample_rate(uint32_t rate) {
    portapack::clock_manager.set_sampling_frequency(rate);
    refresh();
}

void SystemDiagnosticsView::read_gpio_states() {
    // Read actual GPIO port states
    uint32_t gpio3_state = LPC_GPIO->PIN[3];  // GPIO3 for mixer
    uint32_t gpio4_state = LPC_GPIO->PIN[4];  // GPIO4 for LPF and amp

    bool mix_n_actual = (gpio3_state >> 2) & 1;  // GPIO3[2]
    bool lpf_actual = (gpio4_state >> 8) & 1;    // GPIO4[8]
    bool amp_actual = (gpio4_state >> 9) & 1;    // GPIO4[9]

    // Mixer is active LOW, so invert for display
    bool mixer_enabled = !mix_n_actual;

    // Display with GPIO pin numbers
    text_gpio_lpf.set(
        std::string(lpf_actual ? "ON" : "OFF") +
        " (GPIO4[8]=" + to_string_dec_uint(lpf_actual ? 1 : 0) + ")");

    text_gpio_mix.set(
        std::string(mixer_enabled ? "ENABLED" : "BYPASSED") +
        " (GPIO3[2]=" + to_string_dec_uint(mix_n_actual ? 1 : 0) + ")");

    text_gpio_amp.set(
        std::string(amp_actual ? "ON" : "OFF") +
        " (GPIO4[9]=" + to_string_dec_uint(amp_actual ? 1 : 0) + ")");

    // Color code
    text_gpio_lpf.set_style(lpf_actual ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_gpio_mix.set_style(mixer_enabled ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_gpio_amp.set_style(amp_actual ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_orange);
}

void SystemDiagnosticsView::refresh() {
    // Sample rate
    uint32_t sample_rate = portapack::clock_manager.get_sampling_frequency();
    if (sample_rate >= 1000000) {
        text_sample_rate.set(to_string_dec_uint(sample_rate / 1000000) + " MSS");
    } else {
        text_sample_rate.set(to_string_dec_uint(sample_rate / 1000) + " kSS");
    }

    // Baseband filter bandwidth
    uint32_t reg8 = radio::debug::second_if::register_read(8);
    text_reg8.set("0x" + to_string_hex(reg8, 4));

    uint8_t lpf_coarse = reg8 & 0x03;  // Bits 1:0
    const char* bw_names[] = {"7.5 MHz", "8.5 MHz", "15 MHz", "18 MHz"};
    text_bb_filter.set(bw_names[lpf_coarse]);

    // Color code - green if >= 8 MHz, red otherwise
    if (lpf_coarse >= 1) {
        text_bb_filter.set_style(Theme::getInstance()->fg_green);
    } else {
        text_bb_filter.set_style(Theme::getInstance()->fg_red);
    }

    // Read actual GPIO pin states
    read_gpio_states();

    uint32_t gpio6_pin = LPC_GPIO->PIN[6];
    bool rffc_locked = (gpio6_pin >> 25) & 1;

    // Display it by changing one of the existing fields temporarily
    // For example, modify the band display to show lock status:

    auto current_band = radio::debug::rf_path_info::get_current_band();
    switch (current_band) {
        case rf::path::Band::Low:
            text_band.set("LOW(0-2320MHz)|" + std::string(rffc_locked ? "LCK)" : "ULCK)"));
            text_band.set_style(rffc_locked ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
            break;
        case rf::path::Band::Mid:
            text_band.set("MID(2320-2740MHz)");
            text_band.set_style(Theme::getInstance()->fg_green);
            break;
        case rf::path::Band::High:
            text_band.set("HIGH(2740-7250MHz)");
            text_band.set_style(Theme::getInstance()->fg_green);
            break;
    }
    // FPGA control register
    uint32_t fpga_ctrl = radio::debug::fpga::register_read(1);
    text_fpga_ctrl.set("0x" + to_string_hex(fpga_ctrl, 2));

    // Decode FPGA register bits
    bool dc_block = fpga_ctrl & 0x01;
    bool q_invert = fpga_ctrl & 0x02;
    uint8_t quarter_shift = (fpga_ctrl >> 2) & 0x03;

    text_lbl_fpga_decode.set(
        "|DC:" + std::string(dc_block ? "ON" : "OFF") +
        " Q:" + std::string(q_invert ? "INV" : "NOR") +
        " QS:" + to_string_dec_uint(quarter_shift));
}
#endif

#ifdef PRALINE
/* Si5351PLLADebugView *******************************************************/

Si5351PLLADebugView::Si5351PLLADebugView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_title, &text_lbl_raw, &text_r26_27, &text_r28_30, &text_r31_33,
                  &text_lbl_decoded, &text_lbl_p1, &text_p1, &text_lbl_p2, &text_p2,
                  &text_lbl_p3, &text_p3, &text_lbl_calc, &text_lbl_mult, &text_mult,
                  &text_lbl_vco, &text_vco, &text_status, &button_refresh, &button_done});

    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_raw.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_decoded.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_calc.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) { refresh(); };
    button_done.on_select = [&nav](Button&) { nav.pop(); };

    refresh();
}

void Si5351PLLADebugView::focus() {
    button_refresh.focus();
}

void Si5351PLLADebugView::refresh() {
    // Read PLL A parameters
    uint8_t r26 = portapack::clock_manager.si5351_read_register(26);
    uint8_t r27 = portapack::clock_manager.si5351_read_register(27);
    uint8_t r28 = portapack::clock_manager.si5351_read_register(28);
    uint8_t r29 = portapack::clock_manager.si5351_read_register(29);
    uint8_t r30 = portapack::clock_manager.si5351_read_register(30);
    uint8_t r31 = portapack::clock_manager.si5351_read_register(31);
    uint8_t r32 = portapack::clock_manager.si5351_read_register(32);
    uint8_t r33 = portapack::clock_manager.si5351_read_register(33);

    // Display Raw registers
    text_r26_27.set("R26-27 (P3 LO): " + to_string_hex(r26, 2) + " " + to_string_hex(r27, 2));
    text_r28_30.set("R28-30 (P1):    " + to_string_hex(r28, 2) + " " + to_string_hex(r29, 2) + " " + to_string_hex(r30, 2));
    text_r31_33.set("R31-33 (P3H|P2):" + to_string_hex(r31, 2) + " " + to_string_hex(r32, 2) + " " + to_string_hex(r33, 2));

    // Decode parameters
    uint32_t pll_p1 = ((uint32_t)(r28 & 0x03) << 16) | ((uint32_t)r29 << 8) | r30;
    uint32_t pll_p2 = ((uint32_t)(r31 & 0x0F) << 16) | ((uint32_t)r32 << 8) | r33;
    uint32_t pll_p3 = ((uint32_t)(r31 >> 4) << 16) | ((uint32_t)r26 << 8) | r27;

    text_p1.set(to_string_dec_uint(pll_p1) + " (0x" + to_string_hex(pll_p1, 5) + ")");
    text_p2.set(to_string_dec_uint(pll_p2) + " (0x" + to_string_hex(pll_p2, 5) + ")");
    text_p3.set(to_string_dec_uint(pll_p3) + " (0x" + to_string_hex(pll_p3, 5) + ")");

    // Calculate Multiplier: M = (128 * P3 + P1 * 128 + 512 + P2) / (128 * P3)
    // Simplified as: Multiplier = ((P1 + 512) / 128) + (P2 / P3)
    uint32_t a = (pll_p1 + 512) / 128;

    if (pll_p3 > 0) {
        text_mult.set(to_string_dec_uint(a) + " + " + to_string_dec_uint(pll_p2) + "/" + to_string_dec_uint(pll_p3));
    } else {
        text_mult.set("ERR: P3=0");
    }

    // Calculate VCO Frequency (f_vco = f_xtal * Multiplier)
    // The HackRF crystal (f_xtal) is 25 MHz.
    if (pll_p3 > 0) {
        uint64_t vco_num = (uint64_t)pll_p2 + (uint64_t)pll_p3 * (pll_p1 + 512);
        uint64_t vco_den = 128ULL * pll_p3;
        uint32_t vco_khz = (uint32_t)((25000ULL * vco_num) / vco_den);

        text_vco.set(to_string_dec_uint(vco_khz / 1000) + "." + to_string_dec_uint(vco_khz % 1000, 3) + " MHz");

        // Si5351 VCO range is 600-900 MHz
        if (vco_khz >= 600000 && vco_khz <= 900000) {
            text_vco.set_style(Theme::getInstance()->fg_green);
            text_status.set("VCO within valid range.");
        } else {
            text_vco.set_style(Theme::getInstance()->fg_red);
            text_status.set("VCO OUT OF RANGE (600-900)!");
        }
    }
}
#endif

#ifdef PRALINE
Si5351PLLBDebugView::Si5351PLLBDebugView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_title,
                  &text_lbl_raw, &text_r34_35, &text_r36_38, &text_r39_41,
                  &text_lbl_decoded, &text_lbl_p1, &text_p1, &text_lbl_p2, &text_p2, &text_lbl_p3, &text_p3,
                  &text_lbl_calc, &text_lbl_vco, &text_vco,
                  &text_status, &button_refresh, &button_done});

    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_raw.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_decoded.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_calc.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) { refresh(); };
    button_done.on_select = [&nav](Button&) { nav.pop(); };
    refresh();
}

void Si5351PLLBDebugView::focus() {
    button_refresh.focus();
}

void Si5351PLLBDebugView::refresh() {
    uint8_t r34 = portapack::clock_manager.si5351_read_register(34);  // P3[15:8]
    uint8_t r35 = portapack::clock_manager.si5351_read_register(35);  // P3[7:0]
    uint8_t r36 = portapack::clock_manager.si5351_read_register(36);  // P1[17:16]
    uint8_t r37 = portapack::clock_manager.si5351_read_register(37);  // P1[15:8]
    uint8_t r38 = portapack::clock_manager.si5351_read_register(38);  // P1[7:0]
    uint8_t r39 = portapack::clock_manager.si5351_read_register(39);  // P3[19:16] | P2[19:16]
    uint8_t r40 = portapack::clock_manager.si5351_read_register(40);  // P2[15:8]
    uint8_t r41 = portapack::clock_manager.si5351_read_register(41);  // P2[7:0]

    text_r34_35.set("R34-35: " + to_string_hex(r34, 2) + " " + to_string_hex(r35, 2));
    text_r36_38.set("R36-38: " + to_string_hex(r36, 2) + " " + to_string_hex(r37, 2) + " " + to_string_hex(r38, 2));
    text_r39_41.set("R39-41: " + to_string_hex(r39, 2) + " " + to_string_hex(r40, 2) + " " + to_string_hex(r41, 2));

    uint32_t p1 = ((uint32_t)(r36 & 0x03) << 16) | ((uint32_t)r37 << 8) | r38;
    uint32_t p2 = ((uint32_t)(r39 & 0x0F) << 16) | ((uint32_t)r40 << 8) | r41;
    uint32_t p3 = ((uint32_t)(r39 >> 4) << 16) | ((uint32_t)r34 << 8) | r35;

    text_p1.set(to_string_dec_uint(p1) + " (0x" + to_string_hex(p1, 5) + ")");
    text_p2.set(to_string_dec_uint(p2) + " (0x" + to_string_hex(p2, 5) + ")");
    text_p3.set(to_string_dec_uint(p3) + " (0x" + to_string_hex(p3, 5) + ")");

    if (p3 > 0) {
        uint64_t vco_num = (uint64_t)p2 + (uint64_t)p3 * (p1 + 512);
        uint64_t vco_den = 128ULL * p3;
        uint32_t vco_khz = (uint32_t)((25000ULL * vco_num) / vco_den);
        text_vco.set(to_string_dec_uint(vco_khz / 1000) + "." + to_string_dec_uint(vco_khz % 1000, 3) + " MHz");

        bool ok = (vco_khz >= 600000 && vco_khz <= 900000);
        text_vco.set_style(ok ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
        text_status.set(ok ? "PLL B VCO OK" : "VCO OUT OF RANGE!");
    }
}
#endif

#ifdef PRALINE
/* Si5351MultiSynthDebugView *************************************************/

Si5351MultiSynthDebugView::Si5351MultiSynthDebugView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
        &text_lbl_clk_ctrl,
        &text_clk_ctrl,
        &text_lbl_ms_int,
        &text_ms_int,
        &text_lbl_raw,
        &text_lbl_r42_43,
        &text_r42_43,
        &text_lbl_r44_46,
        &text_r44_46,
        &text_lbl_r47_49,
        &text_r47_49,
        &text_lbl_decoded,
        &text_lbl_p1,
        &text_p1,
        &text_lbl_p2,
        &text_p2,
        &text_lbl_p3,
        &text_p3,
        &text_lbl_rdiv,
        &text_rdiv,
        &text_lbl_calc,
        &text_lbl_div,
        &text_div,
        &text_lbl_freq,
        &text_freq,
        &text_status,
        &button_refresh,
        &button_reset,
        &button_frac,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_raw.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_decoded.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_calc.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        refresh();
    };

    button_reset.on_select = [this](Button&) {
        force_pll_reset();
    };

    button_frac.on_select = [this](Button&) {
        force_fractional_mode();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh();
}

void Si5351MultiSynthDebugView::focus() {
    button_refresh.focus();
}

void Si5351MultiSynthDebugView::force_pll_reset() {
    // Reset PLL A (bit 5)
    portapack::clock_manager.si5351_write_register(177, 0x20);

    // Wait for PLL to settle
    chThdSleepMilliseconds(10);

    refresh();
}

void Si5351MultiSynthDebugView::force_fractional_mode() {
    // Force CLK0 to fractional mode
    // Read current control register
    uint8_t clk0_ctrl = portapack::clock_manager.si5351_read_register(16);

    // Clear MS_INT bit (bit 6) to enable fractional mode
    clk0_ctrl &= ~0x40;

    // Write back
    portapack::clock_manager.si5351_write_register(16, clk0_ctrl);

    // Reset PLL to apply
    portapack::clock_manager.si5351_write_register(177, 0x20);

    chThdSleepMilliseconds(10);

    refresh();
}

void Si5351MultiSynthDebugView::refresh() {
    // === Clock Control Register 16 (CLK0) ===
    uint8_t clk0_ctrl = portapack::clock_manager.si5351_read_register(16);
    text_clk_ctrl.set("0x" + to_string_hex(clk0_ctrl, 2) +
                      " (" + to_string_bin(clk0_ctrl, 8) + ")");

    // Decode MS_INT bit (bit 6)
    bool ms_int = (clk0_ctrl >> 6) & 1;
    if (ms_int) {
        text_ms_int.set("1:INT MODE!");
        text_ms_int.set_style(Theme::getInstance()->fg_red);
    } else {
        text_ms_int.set("0:Fract Mode");
        text_ms_int.set_style(Theme::getInstance()->fg_green);
    }

    // === Read Raw MS0 Registers (42-49) ===
    uint8_t r42 = portapack::clock_manager.si5351_read_register(42);
    uint8_t r43 = portapack::clock_manager.si5351_read_register(43);
    uint8_t r44 = portapack::clock_manager.si5351_read_register(44);
    uint8_t r45 = portapack::clock_manager.si5351_read_register(45);
    uint8_t r46 = portapack::clock_manager.si5351_read_register(46);
    uint8_t r47 = portapack::clock_manager.si5351_read_register(47);
    uint8_t r48 = portapack::clock_manager.si5351_read_register(48);
    uint8_t r49 = portapack::clock_manager.si5351_read_register(49);

    // Display raw registers
    text_r42_43.set(to_string_hex(r42, 2) + " " + to_string_hex(r43, 2) +
                    " (P3[15:0])");
    text_r44_46.set(to_string_hex(r44, 2) + " " + to_string_hex(r45, 2) +
                    " " + to_string_hex(r46, 2) + " (R|P1)");
    text_r47_49.set(to_string_hex(r47, 2) + " " + to_string_hex(r48, 2) +
                    " " + to_string_hex(r49, 2) + " (P3|P2)");

    // === Decode P1, P2, P3 ===
    // Si5351 MS Register Layout:
    // Reg 42: P3[15:8]
    // Reg 43: P3[7:0]
    // Reg 44: bits 6:4 = R_DIV[2:0], bits 1:0 = P1[17:16]
    // Reg 45: P1[15:8]
    // Reg 46: P1[7:0]
    // Reg 47: bits 7:4 = P3[19:16], bits 3:0 = P2[19:16]
    // Reg 48: P2[15:8]
    // Reg 49: P2[7:0]

    // Decode R_DIV
    uint8_t r_div_encoded = (r44 >> 4) & 0x07;
    uint32_t r_div = 1 << r_div_encoded;
    text_rdiv.set("/" + to_string_dec_uint(r_div) + " (enc=" + to_string_dec_uint(r_div_encoded) + ")");

    // Decode P1 (18-bit)
    uint32_t p1 = ((uint32_t)(r44 & 0x03) << 16) | ((uint32_t)r45 << 8) | r46;
    text_p1.set(to_string_dec_uint(p1) + " (0x" + to_string_hex(p1, 5) + ")");

    // Decode P2 (20-bit)
    uint32_t p2 = ((uint32_t)(r47 & 0x0F) << 16) | ((uint32_t)r48 << 8) | r49;
    text_p2.set(to_string_dec_uint(p2) + " (0x" + to_string_hex(p2, 5) + ")");

    // Decode P3 (20-bit)
    uint32_t p3 = ((uint32_t)(r47 >> 4) << 16) | ((uint32_t)r42 << 8) | r43;
    text_p3.set(to_string_dec_uint(p3) + " (0x" + to_string_hex(p3, 5) + ")");

    // Color code P2/P3 based on whether fractional is being used
    if (p2 == 0 && p3 == 1) {
        text_p2.set_style(Theme::getInstance()->fg_orange);
        text_p3.set_style(Theme::getInstance()->fg_orange);
    } else if (p3 > 1) {
        text_p2.set_style(Theme::getInstance()->fg_green);
        text_p3.set_style(Theme::getInstance()->fg_green);
    } else {
        // Default/neutral style when P2/P3 don't match known patterns
        text_p2.set_style(Theme::getInstance()->fg_light);
        text_p3.set_style(Theme::getInstance()->fg_light);
    }

    // === Calculate Output Frequency ===

    // === Calculate Multisynth Divider ===
    // Correct Si5351 formula:
    // MS_DIV = (P2+P3 × (P1 + 512)) / (128 × P3)
    // MS_DIV = P2/(128*P3) + P1+512/(128*P3)
    // x = (P1 + 512) / 128
    // y = P2/128
    // z = P3
    // f_out = f_vco / MS_DIV / R_DIV
    // a = floor((P1 + 512) / 128)
    // k = (P1 + 512) - 128*a
    // b = b = (P2 + c*k) / 128
    // c = P3
    // In the case of integer division: b=0, c=1, so MS_DIV = a

    uint64_t ms_div_numerator = (uint64_t)p2 + (uint64_t)p3 * (p1 + 512);
    uint64_t ms_div_denominator = 128ULL * p3;

    uint32_t x = (p1 + 512) / 128;
    uint32_t y = p2 / 128;
    uint32_t z = p3;

    // For display, show the full fractional value
    // MS_DIV = (a+b)/c
    if (p3 > 1 && p2 > 0) {
        std::string div_str = "(" + to_string_dec_uint(x) + "+" + to_string_dec_uint(y) + ")/" + to_string_dec_uint(z);
        text_div.set(div_str);
    } else {
        std::string div_str = to_string_dec_uint(x);
        text_div.set(div_str);
    }

    // === Calculate Output Frequency ===

    // f_vco = 800,000,000 (PLL A)
    // f_out in kHz = 800,000,000 / MS_DIV / r_div / 1000
    //              = 800,000 × ms_div_denominator / ms_div_numerator / r_div

    uint32_t freq_khz = 0;
    if (ms_div_numerator > 0) {
        freq_khz = (uint32_t)((800000ULL * ms_div_denominator) / ms_div_numerator / r_div);
    }

    uint32_t freq_mhz = freq_khz / 1000;
    uint32_t freq_frac = freq_khz % 1000;

    text_freq.set(to_string_dec_uint(freq_mhz) + "." +
                  to_string_dec_uint(freq_frac / 100) +
                  to_string_dec_uint((freq_frac / 10) % 10) +
                  to_string_dec_uint(freq_frac % 10) + " MHz");

    // Color code based on expected ~24.576 MHz for WFM stereo
    if (freq_khz >= 24500 && freq_khz <= 24700) {
        text_freq.set_style(Theme::getInstance()->fg_green);
    } else if (freq_khz >= 24000 && freq_khz <= 26000) {
        text_freq.set_style(Theme::getInstance()->fg_orange);
    } else {
        text_freq.set_style(Theme::getInstance()->fg_red);
    }

    // === Status Summary ===
    // Expected values for 24.576 MHz (3.072 MHz * 8 decimation):
    // VCO = 800 MHz
    // Target freq = 49.152 MHz (before R_DIV=/2)
    // MS_DIV = 800M / 49.152M = 16.276...
    // a = 16, b = 53, c = 192
    // P1 = 128*16 + floor(128*53/192) - 512 = 2048 + 35 - 512 = 1571
    // P2 = 128*53 - 192*35 = 6784 - 6720 = 64
    // P3 = 192

    if (ms_int) {
        text_status.set("ERROR: Integer mode! P2/P3 ignored!");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (p2 == 0 && p3 == 1) {
        text_status.set("WARN P2:0,P3:1 INT Equiv");
        text_status.set_style(Theme::getInstance()->fg_orange);
    } else if (p3 == 192 && p2 == 64) {
        text_status.set("GOOD Exp 3.072M values!");
        text_status.set_style(Theme::getInstance()->fg_green);
    } else if (freq_khz >= 24500 && freq_khz <= 24700) {
        text_status.set("OK: Freq in range");
        text_status.set_style(Theme::getInstance()->fg_green);
    } else {
        text_status.set("CHECK: P2=" + to_string_dec_uint(p2) +
                        " P3=" + to_string_dec_uint(p3));
        text_status.set_style(Theme::getInstance()->fg_orange);
    }
}
#endif

#ifdef PRALINE
PralineClockDebugView::PralineClockDebugView(NavigationView& nav)
    : View(),
      rows{
          {&t0_id, &t0_ma, &t0_mode, &t0_src, &t0_ph, &t0_st},
          {&t1_id, &t1_ma, &t1_mode, &t1_src, &t1_ph, &t1_st},
          {&t2_id, &t2_ma, &t2_mode, &t2_src, &t2_ph, &t2_st},
          {&t3_id, &t3_ma, &t3_mode, &t3_src, &t3_ph, &t3_st},
          {&t4_id, &t4_ma, &t4_mode, &t4_src, &t4_ph, &t4_st},
          {&t5_id, &t5_ma, &t5_mode, &t5_src, &t5_ph, &t5_st},
          {&t6_id, &t6_ma, &t6_mode, &t6_src, &t6_ph, &t6_st},
          {&t7_id, &t7_ma, &t7_mode, &t7_src, &t7_ph, &t7_st}} {
    add_children({&text_title, &text_lbl_pll, &text_pll_status,
                  &text_lbl_afe, &text_afe_rate, &text_lbl_n, &text_n_val,
                  &text_header,
                  &t0_id, &t0_ma, &t0_mode, &t0_src, &t0_ph, &t0_st,
                  &t1_id, &t1_ma, &t1_mode, &t1_src, &t1_ph, &t1_st,
                  &t2_id, &t2_ma, &t2_mode, &t2_src, &t2_ph, &t2_st,
                  &t3_id, &t3_ma, &t3_mode, &t3_src, &t3_ph, &t3_st,
                  &t4_id, &t4_ma, &t4_mode, &t4_src, &t4_ph, &t4_st,
                  &t5_id, &t5_ma, &t5_mode, &t5_src, &t5_ph, &t5_st,
                  &t6_id, &t6_ma, &t6_mode, &t6_src, &t6_ph, &t6_st,
                  &t7_id, &t7_ma, &t7_mode, &t7_src, &t7_ph, &t7_st,
                  &button_refresh, &button_done});

    button_refresh.on_select = [this](Button&) { this->refresh(); };
    button_done.on_select = [&nav](Button&) { nav.pop(); };

    refresh();
}

void PralineClockDebugView::focus() {
    button_refresh.focus();
}

void PralineClockDebugView::refresh() {
    // 1. System Status
    uint8_t status = portapack::clock_manager.si5351_read_status();
    bool pll_a = !(status & 0x20);
    bool pll_b = !(status & 0x40);
    text_pll_status.set(std::string(pll_a ? "A:OK " : "A:ERR ") + (pll_b ? "B:OK" : "B:ERR"));
    text_pll_status.set_style((pll_a && pll_b) ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // 2. AFE & Decimation Info
    uint32_t base_rate = portapack::clock_manager.get_sampling_frequency();
    uint8_t n = portapack::clock_manager.get_resampling_n();
    text_afe_rate.set(to_string_dec_uint(base_rate << n) + " Hz");
    text_n_val.set(to_string_dec_uint(n));

    // 3. Clock Table Decoding
    uint8_t output_en = portapack::clock_manager.si5351_read_register(3);
    const char* ma_lookup[] = {"2m", "4m", "6m", "8m"};

    for (size_t i = 0; i < 8; i++) {
        uint8_t ctrl = portapack::clock_manager.si5351_read_register(16 + i);

        // mA (Bits 1:0)
        rows[i].ma->set(ma_lookup[ctrl & 0x03]);

        // Mode (Bit 6: 1=Integer, 0=Fractional)
        rows[i].mode->set((ctrl & 0x40) ? "INT" : "FRAC");
        rows[i].mode->set_style((ctrl & 0x40) ? Theme::getInstance()->fg_blue : Theme::getInstance()->fg_yellow);

        // PLL Source (Bit 5: 0=PLLA, 1=PLLB)
        rows[i].src->set((ctrl & 0x20) ? "PLLB" : "PLLA");
        rows[i].src->set_style((ctrl & 0x20) ? Theme::getInstance()->fg_blue : Theme::getInstance()->fg_green);

        // Phase (Bit 4: 1=Inverted, 0=Normal)
        // Use 0x10 (Bit 4)
        rows[i].phase->set((ctrl & 0x10) ? "INVRT" : "NORM ");
        rows[i].phase->set_style((ctrl & 0x10) ? Theme::getInstance()->fg_orange : Theme::getInstance()->fg_light);

        // Status (Powered On and Output Enabled)
        bool is_on = !(ctrl & 0x80) && !(output_en & (1 << i));
        rows[i].stat->set(is_on ? "ON" : "OFF");
        rows[i].stat->set_style(is_on ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    }
}
#endif

#ifdef PRALINE
/* GPIODebugView *************************************************/
GPIODebugView::GPIODebugView(NavigationView& nav) {
    add_children({
        &text_lbl_gpio4,
        &text_lbl_mixr1,
        &text_mixr1,
        &text_lbl_pin4,
        &text_pin4,
        &text_lbl_set4,
        &text_set4,
        &text_lbl_lpf_bit,
        &text_lpf_dir,
        &text_lpf_pin,
        &text_lpf_set,
        &button_lpf_toggle,
        &button_lpf_on,
        &button_lpf_off,
        &text_lbl_amp_bit,
        &text_amp_dir,
        &text_amp_pin,
        &text_amp_set,
        &button_amp_toggle,
        &button_amp_on,
        &button_amp_off,
        &text_lbl_gpio3,
        &text_lbl_mix_bit,
        &text_mix_dir,
        &text_mix_pin,
        &text_mix_set,
        &button_refresh,
        &button_done,
    });

    text_lbl_gpio4.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_gpio3.set_style(Theme::getInstance()->fg_yellow);

    // LPF control buttons
    button_lpf_toggle.on_select = [this](Button&) {
        // Read current state
        uint32_t current = LPC_GPIO->PIN[4];
        bool current_state = (current >> 8) & 1;

        // Toggle
        if (current_state) {
            LPC_GPIO->CLR[4] = (1 << 8);  // Clear bit 8
        } else {
            LPC_GPIO->SET[4] = (1 << 8);  // Set bit 8
        }

        refresh();
    };

    button_lpf_on.on_select = [this](Button&) {
        LPC_GPIO->SET[4] = (1 << 8);  // Force ON
        refresh();
    };

    button_lpf_off.on_select = [this](Button&) {
        LPC_GPIO->CLR[4] = (1 << 8);  // Force OFF
        refresh();
    };

    // RF Amp control buttons
    button_amp_toggle.on_select = [this](Button&) {
        uint32_t current = LPC_GPIO->PIN[4];
        bool current_state = (current >> 9) & 1;

        if (current_state) {
            LPC_GPIO->CLR[4] = (1 << 9);
        } else {
            LPC_GPIO->SET[4] = (1 << 9);
        }

        refresh();
    };

    button_amp_on.on_select = [this](Button&) {
        LPC_GPIO->SET[4] = (1 << 9);  // Force ON
        refresh();
    };

    button_amp_off.on_select = [this](Button&) {
        LPC_GPIO->CLR[4] = (1 << 9);  // Force OFF
        refresh();
    };

    button_refresh.on_select = [this](Button&) {
        refresh();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh();
}

void GPIODebugView::focus() {
    button_refresh.focus();
}

void GPIODebugView::refresh() {
    // Read GPIO4 registers
    uint32_t gpio4_dir = LPC_GPIO->DIR[4];  // Direction: 1=output, 0=input
    uint32_t gpio4_pin = LPC_GPIO->PIN[4];  // Actual pin state
    uint32_t gpio4_set = LPC_GPIO->SET[4];  // What we're trying to output

    // Display full registers
    // text_dir4.set("0x" + to_string_hex(gpio4_dir, 8));
    text_pin4.set("0x" + to_string_hex(gpio4_pin, 8));
    text_set4.set("0x" + to_string_hex(gpio4_set, 8));

    // Extract bit 8 (LPF)
    bool lpf_dir = (gpio4_dir >> 8) & 1;
    bool lpf_pin = (gpio4_pin >> 8) & 1;
    bool lpf_set = (gpio4_set >> 8) & 1;

    text_lpf_dir.set("DIR: " + std::string(lpf_dir ? "OUT" : "IN"));
    text_lpf_pin.set("PIN: " + std::string(lpf_pin ? "1" : "0"));
    text_lpf_set.set("SET: " + std::string(lpf_set ? "1" : "0"));

    // Color code
    text_lpf_dir.set_style(lpf_dir ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_lpf_pin.set_style(lpf_pin ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // Extract bit 9 (RF Amp)
    bool amp_dir = (gpio4_dir >> 9) & 1;
    bool amp_pin = (gpio4_pin >> 9) & 1;
    bool amp_set = (gpio4_set >> 9) & 1;

    text_amp_dir.set("DIR: " + std::string(amp_dir ? "OUT" : "IN"));
    text_amp_pin.set("PIN: " + std::string(amp_pin ? "1" : "0"));
    text_amp_set.set("SET: " + std::string(amp_set ? "1" : "0"));

    text_amp_dir.set_style(amp_dir ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_amp_pin.set_style(amp_pin ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // Read GPIO3 (Mixer) - bit 2
    uint32_t gpio3_dir = LPC_GPIO->DIR[3];
    uint32_t gpio3_pin = LPC_GPIO->PIN[3];
    uint32_t gpio3_set = LPC_GPIO->SET[3];

    bool mix_dir = (gpio3_dir >> 2) & 1;
    bool mix_pin = (gpio3_pin >> 2) & 1;
    bool mix_set = (gpio3_set >> 2) & 1;

    text_mix_dir.set("DIR: " + std::string(mix_dir ? "OUT" : "IN"));
    text_mix_pin.set("PIN: " + std::string(mix_pin ? "1" : "0"));
    text_mix_set.set("SET: " + std::string(mix_set ? "1" : "0"));

    text_mix_dir.set_style(mix_dir ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
    text_mix_pin.set_style(mix_pin ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);

    // Read GPIO5 (Mixer R1) - bit 2
    uint32_t gpio3_state = LPC_GPIO->PIN[3];     // GPIO3 for mixer
    bool mix_n_actual = (gpio3_state >> 2) & 1;  // GPIO3[2]
    uint32_t gpio5_state = LPC_GPIO->PIN[5];
    bool mix_r10_pin = (gpio5_state >> 6) & 1;  // GPIO5[6] = P2_6
    // Mixer is active LOW, so invert for display
    bool mixer_enabled = !mix_n_actual;

    // Append to existing mixer display:
    text_mixr1.set(
        std::string(mixer_enabled ? "ENABLED" : "BYPASSED") +
        " P6_3=" + to_string_dec_uint(mix_n_actual ? 1 : 0) +
        " P2_6=" + to_string_dec_uint(mix_r10_pin ? 1 : 0));

    text_mixr1.set_style(mixer_enabled ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red);
}
#endif

#ifdef PRALINE
/* RFFC5072StatusView *************************************************/

RFFC5072StatusView::RFFC5072StatusView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_gpio4,
        &text_ctrl,
        &text_lbl_enabled,
        &text_enabled,
        &text_lbl_freq,
        &text_freq,
        &text_lbl_path,
        &text_path,
        &text_lbl_mixer,
        &text_mixer,
        &text_lbl_r0,
        &text_r0,
        &text_lbl_r1,
        &text_r1,
        &text_lbl_r2,
        &text_r2,
        &text_lbl_n,
        &text_n,
        &text_lbl_lodiv,
        &text_lodiv,
        &text_lbl_calc,
        &text_calc,
        &text_status,
        &text_status2,
        &text_status3,
        &text_regs_status,
        &button_refresh,
        &button_force,
        &button_done,
    });

    button_refresh.on_select = [this](Button&) {
        refresh_status();
    };

    button_force.on_select = [this](Button&) {
        // Force ENX to OUTPUT and drive LOW
        // LPC_GPIO->DIR[2] |= (1 << 13);  // Set as OUTPUT
        // LPC_GPIO->CLR[2] = (1 << 13);   // Drive LOW (enabled)

        // refresh_status();

        // Disable RFFC5072
        // uint32_t r0 = radio::debug::first_if::register_read(0);
        // radio::debug::first_if::register_write(0, r0 & ~0x0010);  // Clear ENBL

        // Wait 1ms
        // chThdSleepMilliseconds(1);

        // Re-enable - this triggers new calibration
        // radio::debug::first_if::register_write(0, r0 | 0x0010);  // Set ENBL

        // Wait for calibration
        // chThdSleepMilliseconds(10);

        // refresh_status();

        // Force lodiv=4 (log2=2) instead of lodiv=2 (log2=1)
        // This gives VCO = LO × 4 = 2595 × 4 = 10380 MHz - TOO HIGH!

        // Actually, we need lodiv=1 which gives VCO = 2595 MHz - TOO LOW (below 2700)

        // Let's try a different approach: manually write registers for VCO ~ 3500 MHz
        // LO = 3500/2 = 1750 MHz (not useful for FM, but tests if VCO can lock)

        // VCO = 3500 MHz, lodiv=2, presc=2, f_ref=40
        // N = (VCO × presc) / f_ref = (3500 × 2) / 40 = 175

        // Write P2_FREQ1: N=175, lodiv=1 (log2), presc=1 (log2)
        // uint16_t p2_freq1 = (175 << 7) | (1 << 4) | (1 << 2);
        // radio::debug::first_if::register_write(15, p2_freq1);

        // Clear fractional part
        // radio::debug::first_if::register_write(16, 0);
        // radio::debug::first_if::register_write(17, 0);

        // Trigger recalibration by toggling ENBL
        // uint32_t r0 = radio::debug::first_if::register_read(0);
        // radio::debug::first_if::register_write(0, r0 & ~0x0010);
        // chThdSleepMilliseconds(1);
        // radio::debug::first_if::register_write(0, r0 | 0x0010);
        // chThdSleepMilliseconds(20);

        // refresh_status();

        // Test SPI SDATA direction switching
        // PRALINE: SDATA = P9_2 = GPIO4[14]

        // Check current direction
        uint32_t dir_before = LPC_GPIO->DIR[4];
        bool sdata_output_before = (dir_before >> 14) & 1;

        // Try a register read
        uint32_t dummy = radio::debug::first_if::register_read(0);
        (void)dummy;

        // Check direction after read
        uint32_t dir_after = LPC_GPIO->DIR[4];
        bool sdata_output_after = (dir_after >> 14) & 1;

        // Read the actual SDATA pin state
        uint32_t pin_state = LPC_GPIO->PIN[4];
        bool sdata_pin = (pin_state >> 14) & 1;

        text_status.set("SDATA: dir_b=" + to_string_dec_uint(sdata_output_before) +
                        " dir_a=" + to_string_dec_uint(sdata_output_after) +
                        " pin=" + to_string_dec_uint(sdata_pin) + "        ");

        // If both are 1 (OUTPUT), the read direction switch isn't happening
        // if (sdata_output_before && sdata_output_after) {
        //    text_status2.set("ERROR: SDATA stuck as OUTPUT!            ");
        //    text_status2.set_style(Theme::getInstance()->fg_red);
        //} else {
        //    text_status2.set("SDATA direction OK            ");
        //    text_status2.set_style(Theme::getInstance()->fg_green);
        //}

        // Test: Write a known pattern to register 0, then read back
        // Register 0 (DEV_CTRL) default = 0xBEFA

        // Step 1: Read current value
        uint32_t before = radio::debug::first_if::register_read(0);

        // Step 2: Write a different value (change ENBL bit to toggle)
        uint32_t test_val = before ^ 0x0010;  // Toggle ENBL bit
        radio::debug::first_if::register_write(0, test_val);

        // Step 3: Read back
        uint32_t after = radio::debug::first_if::register_read(0);

        // Step 4: Restore original
        radio::debug::first_if::register_write(0, before);

        // Display results
        text_status.set("WR TEST: " + to_string_hex(before, 4) +
                        "->" + to_string_hex(test_val, 4) +
                        " rb:" + to_string_hex(after, 4));

        // If after == before (not test_val), reads are broken
        // If after == test_val, reads work
        if (after == test_val) {
            text_status2.set("READ-AFTER-WRITE: PASS!          ");
            text_status2.set_style(Theme::getInstance()->fg_green);
        } else if (after == before) {
            text_status2.set("READ-AFTER-WRITE: FAIL (no change)          ");
            text_status2.set_style(Theme::getInstance()->fg_red);
        } else {
            text_status2.set("READ-AFTER-WRITE: CORRUPT " + to_string_hex(after, 4) + "         ");
            text_status2.set_style(Theme::getInstance()->fg_red);
        }
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    // Initial update
    refresh_status();
}

void RFFC5072StatusView::focus() {
    button_refresh.focus();
}

void RFFC5072StatusView::refresh_status() {
    // === DIAGNOSTIC: Capture initial GPIO state ===
    uint32_t gpio2_initial = LPC_GPIO->PIN[2];
    uint32_t dir2_initial = LPC_GPIO->DIR[2];
    bool enx_initial = (gpio2_initial >> 13) & 1;

    // === READ RAW GPIO STATES FOR DISPLAY ===
    uint32_t gpio2_dir = LPC_GPIO->DIR[2];
    uint32_t gpio2_pin = LPC_GPIO->PIN[2];
    bool enx_is_output = (gpio2_dir >> 13) & 1;
    bool resetx_is_output = (gpio2_dir >> 14) & 1;

    // === LOCK DETECT ===
    uint32_t gpio6_pin = LPC_GPIO->PIN[6];
    bool rffc_locked = (gpio6_pin >> 25) & 1;

    // === FPGA REGISTERS (non-SPI) ===
    uint8_t fpga_reg1 = radio::debug::fpga::register_read(1);
    uint8_t fpga_reg2 = radio::debug::fpga::register_read(2);
    uint8_t fpga_reg3 = radio::debug::fpga::register_read(3);
    text_regs_status.set("FPGA R1:" + to_string_hex(fpga_reg1, 2) +
                         " R2:" + to_string_hex(fpga_reg2, 2) +
                         " R3:" + to_string_hex(fpga_reg3, 2));

    // === CONTROL PINS ===
    bool enx = (gpio2_pin >> 13) & 1;
    bool resetx = (gpio2_pin >> 14) & 1;
    text_ctrl.set("ENX: " + std::string(enx ? "DIS" : "EN") +
                  " O:" + std::string(enx_is_output ? "Y" : "N") +
                  " | RSTX: " + std::string(resetx ? "H" : "L") +
                  " O:" + std::string(resetx_is_output ? "Y" : "N"));
    text_ctrl.set_style((enx == 0 && resetx == 1) ? Theme::getInstance()->fg_green
                                                  : Theme::getInstance()->fg_red);

    // === DIAGNOSTIC: Check BEFORE first RFFC5072 SPI read ===
    uint32_t gpio2_before_spi = LPC_GPIO->PIN[2];
    bool enx_before_spi = (gpio2_before_spi >> 13) & 1;

    // === RFFC5072 REGISTERS (SPI reads - this is where corruption happens) ===
    uint32_t r0 = radio::debug::first_if::register_read(0);

    // === DIAGNOSTIC: Check AFTER first read ===
    uint32_t gpio2_after_r0 = LPC_GPIO->PIN[2];
    bool enx_after_r0 = (gpio2_after_r0 >> 13) & 1;

    uint32_t r15 = radio::debug::first_if::register_read(15);

    // === DIAGNOSTIC: Check AFTER second read ===
    uint32_t gpio2_after_r15 = LPC_GPIO->PIN[2];
    bool enx_after_r15 = (gpio2_after_r15 >> 13) & 1;

    uint32_t r16 = radio::debug::first_if::register_read(16);

    // === DIAGNOSTIC: Check AFTER third read ===
    uint32_t gpio2_final = LPC_GPIO->PIN[2];
    uint32_t dir2_final = LPC_GPIO->DIR[2];
    bool enx_final = (gpio2_final >> 13) & 1;

    // === Display register values ===
    text_r0.set(to_string_hex(r0, 4));
    text_r1.set(to_string_hex(r15, 4) + " (R15)");
    text_r2.set(to_string_hex(r16, 4) + " (R16)");

    bool enabled = (r0 & 0x0010) != 0;
    text_enabled.set(enabled ? "ENABLED" : "DISABLED");
    text_enabled.set_style(enabled ? Theme::getInstance()->fg_green
                                   : Theme::getInstance()->fg_red);

    // === Decode frequency info (keeping existing code) ===
    uint16_t n_int = (r15 >> 7) & 0x1FF;
    uint8_t lodiv_sel = (r15 >> 4) & 0x07;
    uint8_t presc_sel = (r15 >> 2) & 0x03;
    text_n.set(to_string_dec_uint(n_int));

    uint16_t lodiv_val = 1u << lodiv_sel;
    uint16_t presc_val = 1u << presc_sel;
    text_lodiv.set("/" + to_string_dec_uint(lodiv_val) +
                   " (P:/" + to_string_dec_uint(presc_val) + ")");

    const uint32_t f_ref_mhz = 40;
    uint32_t f_vco_mhz = (f_ref_mhz * n_int) / presc_val;
    uint32_t f_lo_mhz = f_vco_mhz / lodiv_val;

    bool vco_ok = (f_vco_mhz >= 2700) && (f_vco_mhz <= 5400);
    bool lo_ok = (f_lo_mhz >= 85) && (f_lo_mhz <= 4200);
    bool in_bypass_range = (f_lo_mhz >= 2320) && (f_lo_mhz <= 2740);

    if (in_bypass_range) {
        text_calc.set(to_string_dec_uint(f_lo_mhz) + " MHz (MID)");
        text_calc.set_style(Theme::getInstance()->fg_orange);
    } else {
        text_calc.set(to_string_dec_uint(f_lo_mhz) + " MHz");
        text_calc.set_style(lo_ok ? Theme::getInstance()->fg_green
                                  : Theme::getInstance()->fg_red);
    }

    text_freq.set(to_string_dec_uint(f_vco_mhz) + " MHz VCO");
    text_freq.set_style(vco_ok ? Theme::getInstance()->fg_green
                               : Theme::getInstance()->fg_red);

    bool path2_active = (r0 & 0x0020) != 0;
    text_path.set(path2_active ? "PATH2" : "PATH1");
    text_mixer.set(path2_active ? "ACTIVE" : "INACTIVE");
    text_mixer.set_style(path2_active ? Theme::getInstance()->fg_green
                                      : Theme::getInstance()->fg_orange);

    // === DIAGNOSTIC STATUS (replaces normal status) ===
    // Read register 31 with readsel=0 (device ID)
    radio::debug::first_if::register_write(0, (r0 & 0xFFF0) | 0x0000);  // readsel=0
    uint32_t device_id = radio::debug::first_if::register_read(31);

    // Read calibration status (readback register 1)
    // First, set DEV_CTRL.readsel = 1, then read READBACK register
    uint32_t dev_ctrl_orig = radio::debug::first_if::register_read(0);  // Save original

    // Write DEV_CTRL with readsel=1 (bits 3:0)
    radio::debug::first_if::register_write(0, (dev_ctrl_orig & 0xFFF0) | 0x0001);

    // Now read the READBACK register (register address for readback)
    uint32_t cal_status = radio::debug::first_if::register_read(31);  // READBACK is at reg 31

    // Decode calibration status:
    // Bit 15: lock (should be 1)
    // Bits 14:8: ct_cal (coarse tune calibration value, 0-127)
    // Bits 7:1: cp_cal (charge pump calibration value)
    // Bit 0: ctfail (1 = calibration FAILED)

    bool lock_bit = (cal_status >> 15) & 1;
    uint8_t ct_cal = (cal_status >> 8) & 0x7F;
    uint8_t cp_cal = (cal_status >> 1) & 0x7F;
    bool ct_fail = cal_status & 1;

    // Add to refresh_status():
    uint32_t r6 = radio::debug::first_if::register_read(6);
    uint32_t r5 = radio::debug::first_if::register_read(5);
    uint32_t r3 = radio::debug::first_if::register_read(3);  // VCO_CTRL

    // Check SDATA (GPIO4[14]) direction
    uint32_t gpio4_dir = LPC_GPIO->DIR[4];
    bool sdata_is_output = (gpio4_dir >> 14) & 1;
    text_gpio4.set("GPIO4 DIR: " + to_string_hex(gpio4_dir, 8) +
                   " SDATA=" + std::string(sdata_is_output ? "OUT" : "IN"));

    // Display these values
    text_status2.set("CAL ct=" + to_string_dec_uint(ct_cal) +
                     " cp=" + to_string_dec_uint(cp_cal) +
                     (ct_fail ? " FAIL!" : " OK") +
                     " lck_b=" + to_string_dec_uint(lock_bit));
    text_status3.set("R3:" + to_string_hex(r3, 4) +
                     " R5:" + to_string_hex(r5, 4) +
                     " R6:" + to_string_hex(r6, 4));

    if (enx_initial != enx_final || dir2_initial != dir2_final) {
        // ENX or DIR changed - report which operation caused it
        std::string diag = "CHG: ";
        if (enx_initial != enx_before_spi) diag += "pre ";
        if (enx_before_spi != enx_after_r0) diag += "R0 ";
        if (enx_after_r0 != enx_after_r15) diag += "R15 ";
        if (enx_after_r15 != enx_final) diag += "R16 ";
        diag += std::to_string(enx_initial) + "->" + std::to_string(enx_final);

        if (dir2_initial != dir2_final) {
            diag += " DIR!";
        }

        text_status.set(diag);
        text_status.set_style(Theme::getInstance()->fg_red);

        // Blink LED
        /*for (int i = 0; i < 3; i++) {
            hackrf::one::led_rx.on();
            chThdSleepMilliseconds(100);
            hackrf::one::led_rx.off();
            chThdSleepMilliseconds(100);
        }*/
    } else {
        // No change - normal status
        if (!rffc_locked) {
            text_status.set("ID 0x" + to_string_hex(device_id, 4) + " PLL UNLOCKED!");
            text_status.set_style(Theme::getInstance()->fg_red);
        } else if (enx == 1) {
            text_status.set("ID 0x" + to_string_hex(device_id, 4) + " DSBLD,ENX=1!");
            text_status.set_style(Theme::getInstance()->fg_red);
        } else {
            text_status.set("ID 0x" + to_string_hex(device_id, 4) + " Passed!");
            text_status.set_style(Theme::getInstance()->fg_green);
        }
    }
}

/* RFFCTuningDebugView *************************************************/
RFFCTuningDebugView::RFFCTuningDebugView(NavigationView& nav) {
    add_children({
        &text_title,
        &text_lbl_called,
        &text_called,
        &text_lbl_req,
        &text_req,
        &text_lbl_exp_n,
        &text_exp_n,
        &text_lbl_act_n,
        &text_act_n,
        &text_lbl_exp_div,
        &text_exp_div,
        &text_lbl_act_div,
        &text_act_div,
        &text_lbl_calc,
        &text_calc,
        &text_lbl_calc_lo,
        &text_calc_lo,
        &text_lbl_calc_vco,
        &text_calc_vco,
        &text_lbl_vco,
        &text_vco,
        &text_lbl_n_q24,
        &text_n_q24,
        &text_status,
        &button_refresh,
        &button_done,
    });

    button_refresh.on_select = [this](Button&) {
        refresh();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh();
}

void RFFCTuningDebugView::focus() {
    button_refresh.focus();
}

void RFFCTuningDebugView::refresh() {
    // Get expected values from last tuning attempt
    auto tuning = radio::debug::first_if::get_tuning_info();

    // Show if set_frequency was ever called
    if (tuning.was_called) {
        text_called.set("YES");
        text_called.set_style(Theme::getInstance()->fg_green);

        text_req.set(to_string_dec_uint(tuning.requested_freq_mhz) + " MHz");
        text_exp_n.set(to_string_dec_uint(tuning.expected_n));

        uint16_t exp_lo = 1 << tuning.expected_lodiv;
        uint16_t exp_pr = 1 << tuning.expected_presc;
        text_exp_div.set(to_string_dec_uint(exp_lo) + " / " + to_string_dec_uint(exp_pr));
    } else {
        text_called.set("NO");
        text_called.set_style(Theme::getInstance()->fg_red);
        text_req.set("---");
        text_exp_n.set("---");
        text_exp_div.set("---");
    }

    // Read actual hardware values
    uint32_t r15 = radio::debug::first_if::register_read(15);

    uint16_t act_n = (r15 >> 7) & 0x1FF;
    uint8_t act_lo_sel = (r15 >> 4) & 0x07;
    uint8_t act_pr_sel = (r15 >> 2) & 0x03;

    uint16_t act_lo = 1 << act_lo_sel;
    uint16_t act_pr = 1 << act_pr_sel;

    text_act_n.set(to_string_dec_uint(act_n));
    text_act_div.set(to_string_dec_uint(act_lo) + " / " + to_string_dec_uint(act_pr));

    // Calculate what this produces
    uint32_t calc_vco = (40 * act_n) / act_pr;
    uint32_t calc_lo = calc_vco / act_lo;
    text_calc.set(to_string_dec_uint(calc_lo) + " MHz");
    text_vco.set(to_string_dec_uint(tuning.calculated_vco_mhz) + " MHz");

    text_calc_lo.set(to_string_dec_uint(tuning.calc_lo_freq_mhz) + " MHz");
    text_calc_vco.set(to_string_dec_uint(tuning.calc_vco_inside_mhz) + " MHz");
    text_n_q24.set(to_string_dec_uint(tuning.calc_n_q24 >> 24));  // Show integer part

    // Status comparison
    if (!tuning.was_called) {
        text_status.set("RFFC Freq set NEVER called!");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (act_n == tuning.expected_n) {
        text_status.set("MATCH! Hardware as expected!");
        text_status.set_style(Theme::getInstance()->fg_green);
    } else {
        text_status.set("MISMATCH! Exp:" + to_string_dec_uint(tuning.expected_n) +
                        " Act:" + to_string_dec_uint(act_n));
        text_status.set_style(Theme::getInstance()->fg_red);
    }
}

/* MAX2831DebugView *************************************************/
MAX2831DebugView::MAX2831DebugView(NavigationView& nav) {
    add_children({
        &text_title,
        &text_lbl_called,
        &text_called,
        &text_lbl_valid,
        &text_valid,
        &text_lbl_req,
        &text_req,
        &text_lbl_calc_n,
        &text_calc_n,
        &text_lbl_calc_frac,
        &text_calc_frac,
        &text_spacer,
        &text_lbl_r3,
        &text_r3,
        &text_lbl_r4,
        &text_r4,
        &text_lbl_act_n,
        &text_act_n,
        &text_lbl_act_frac,
        &text_act_frac,
        &text_lbl_calc_freq,
        &text_calc_freq,
        &text_status,
        &button_refresh,
        &button_done,
    });

    text_spacer.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        refresh();
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh();
}

void MAX2831DebugView::focus() {
    button_refresh.focus();
}

void MAX2831DebugView::refresh() {
    auto info = get_max2831_info();

    // Show if set_frequency was called
    if (info.set_frequency_called) {
        text_called.set("YES");
        text_called.set_style(Theme::getInstance()->fg_green);

        if (info.frequency_valid) {
            text_valid.set("YES (2.3-2.6G)");
            text_valid.set_style(Theme::getInstance()->fg_green);
        } else {
            text_valid.set("NO - OUT OF RANGE!");
            text_valid.set_style(Theme::getInstance()->fg_red);
        }

        text_req.set(to_string_dec_uint(info.requested_freq_mhz) + " MHz");
        text_calc_n.set(to_string_dec_uint(info.calculated_n));
        text_calc_frac.set(to_string_hex(info.calculated_frac, 5));
    } else {
        text_called.set("NO");
        text_called.set_style(Theme::getInstance()->fg_red);
        text_valid.set("---");
        text_req.set("---");
        text_calc_n.set("---");
        text_calc_frac.set("---");
    }

    // Read actual hardware registers
    uint32_t r3 = radio::debug::second_if::register_read(3);
    uint32_t r4 = radio::debug::second_if::register_read(4);

    text_r3.set(to_string_hex(r3, 4));
    text_r4.set(to_string_hex(r4, 4));

    // Decode actual values from registers
    uint16_t act_n = r3 & 0xFF;
    uint32_t act_frac_lo = (r3 >> 8) & 0x3F;
    uint32_t act_frac_hi = r4 & 0x3FFF;
    uint32_t act_frac = (act_frac_hi << 6) | act_frac_lo;

    text_act_n.set(to_string_dec_uint(act_n));
    text_act_frac.set(to_string_hex(act_frac, 5));

    // Calculate actual frequency from registers
    // F_LO = 20 MHz × (N + Frac/2^20)
    // For display, show integer part only
    uint32_t calc_freq_mhz = 20 * act_n;
    // Add fractional contribution (approximate)
    uint32_t frac_contribution = (act_frac * 20) >> 20;
    calc_freq_mhz += frac_contribution;

    text_calc_freq.set(to_string_dec_uint(calc_freq_mhz) + " MHz");

    // Status
    if (!info.set_frequency_called) {
        text_status.set("MAX2831 set_frequency\nNEVER called!");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (!info.frequency_valid) {
        text_status.set("Freq " + to_string_dec_uint(info.requested_freq_mhz) +
                        " MHz OUT OF RANGE!\n(need 2300-2600)");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (act_n == info.calculated_n && act_frac == info.calculated_frac) {
        text_status.set("MATCH!\nHardware = Expected");
        text_status.set_style(Theme::getInstance()->fg_green);
    } else {
        text_status.set("MISMATCH!\nN: exp=" + to_string_dec_uint(info.calculated_n) +
                        " act=" + to_string_dec_uint(act_n));
        text_status.set_style(Theme::getInstance()->fg_red);
    }
}

#endif

#endif
/* DebugPeripheralsMenuView **********************************************/

DebugPeripheralsMenuView::DebugPeripheralsMenuView(NavigationView& nav)
    : nav_(nav) {
    set_max_rows(2);  // allow wider buttons
}

void DebugPeripheralsMenuView::on_populate() {
#ifdef PRALINE
    const char* max283x = "MAX2831";
#else
    const char* max283x = hackrf_r9 ? "MAX2839" : "MAX2837";
#endif
    const char* si5351x = hackrf_r9 ? "Si5351A" : "Si5351C";
    add_items({
        {"RFFC5072", Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<RegistersView>("RFFC5072", RegistersWidgetConfig{CT_RFFC5072, 31, 31, 16}); }},
#ifdef PRALINE
        {max283x, Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this, max283x]() { nav_.push<RegistersView>(max283x, RegistersWidgetConfig{CT_MAX283X, 16, 16, 14}); }},
        {"FPGA", Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<RegistersView>("FPGA (iCE40)", RegistersWidgetConfig{CT_FPGA, 6, 6, 8}); }},
#else
        {max283x, Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this, max283x]() { nav_.push<RegistersView>(max283x, RegistersWidgetConfig{CT_MAX283X, 32, 32, 10}); }},
#endif
        {"SGPIO", Theme::getInstance()->fg_darkcyan->foreground, &bitmap_icon_peripherals_details, [this]() { nav_.push<RegistersView>("SGPIO", RegistersWidgetConfig{CT_SGPIO, 6, 6, 16}); }},
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

#ifdef PRALINE
/* PralineDebugMenuView *************************************************/

PralineDebugMenuView::PralineDebugMenuView(NavigationView& nav)
    : nav_(nav) {
    set_max_rows(2);  // Allows wider buttons for descriptive titles
}

void PralineDebugMenuView::on_populate() {
    if (portapack::persistent_memory::show_gui_return_icon()) {
        add_items({{"..", ui::Theme::getInstance()->fg_light->foreground, &bitmap_icon_previous, [this]() { nav_.pop(); }}});
    }
    add_items({
        {"WFM Audio", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<WFMAudioDebugView>(); }},
        {"Clocks", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<ui::PralineClockDebugView>(); }},
        {"MSynth", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<Si5351MultiSynthDebugView>(); }},
        {"Radio Diag", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RadioDiagnosticsView>(); }},
        {"Radio Debug", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<PralineRadioDebugView>(); }},
        {"Signal Path", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SignalPathStatusView>(); }},
        {"System Diag", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SystemDiagnosticsView>(); }},
        {"PLL A", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<Si5351PLLADebugView>(); }},
        {"PLL B", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<Si5351PLLBDebugView>(); }},
        {"GPIO", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<GPIODebugView>(); }},
        {"RFFC Status", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RFFC5072StatusView>(); }},
        {"RFFC Tuning", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RFFCTuningDebugView>(); }},
        {"MAX2831", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<MAX2831DebugView>(); }},
        {"Si5351", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<Si5351DebugView>(); }},
        {"SGPIO Clk", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SGPIO8ClockDetectorView>(); }},
        {"Baseband", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<BasebandStatusView>(); }},
        {"SGPIO Live", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SGPIOLiveMonitorView>(); }},
        {"RX Test", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RadioRxTestView>(); }},
    });
}
#endif

void DebugMenuView::on_populate() {
    if (portapack::persistent_memory::show_gui_return_icon()) {
        add_items({{"..", ui::Theme::getInstance()->fg_light->foreground, &bitmap_icon_previous, [this]() { nav_.pop(); }}});
    }
    add_items({
#ifdef PRALINE
        {"Pro Debug", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_setup, [this]() { nav_.push<PralineDebugMenuView>(); }},
#endif
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
    srand(LPC_RTC->CTIME0);
}

bool DebugScreenTest::on_key(const KeyEvent key) {
    Painter painter;
    switch (key) {
        case KeyEvent::Select:
            nav_.pop();
            break;
        case KeyEvent::Down:
            painter.fill_rectangle({0, 0, screen_width, screen_height}, rand());
            break;
        case KeyEvent::Left:
            pen_color = rand();
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
    pen_color = rand();
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
