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
                  &text_clk0_freq_label,
                  &text_clk0_freq_value,
                  &text_clk0_div_label,
                  &text_clk0_div_value,
                  &text_clk1_label,
                  &text_clk1_status,
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
    bool los_clkin = (status & 0x10);      // Bit 4: LOS (Loss of Signal)

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

    // Calculate frequency: f_out = 800 MHz / ms_div / r_div
    uint32_t freq_khz = 800000 / ms_div / r_div;  // Result in kHz

    // Show P1 value and R45 for debugging
    text_clk0_freq_value.set(to_string_dec_uint(freq_khz) + " kHz (P1:" + to_string_hex(p1, 4) + ")");
    text_clk0_div_value.set("MS=" + to_string_dec_uint(ms_div) +
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
	&text_lbl_filter, &text_filter,
        &text_lbl_mixer, &text_mixer,
        &text_lbl_rf_amp,
        &text_rf_amp,
        &text_lbl_lna,
        &text_lna,
        &text_lbl_vga,
        &text_vga,
        &text_lbl_fpga_decim,
        &text_fpga_decim,
        &text_status,
        &button_refresh,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
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
/* RFFC5072StatusView *************************************************/

RFFC5072StatusView::RFFC5072StatusView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title,
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
        &text_lbl_decode,
        &text_lbl_n,
        &text_n,
        &text_lbl_lodiv,
        &text_lodiv,
        &text_lbl_calc,
        &text_calc,
        &text_status,
        &button_refresh,
        &button_done,
    });

    text_title.set_style(Theme::getInstance()->fg_yellow);
    text_lbl_decode.set_style(Theme::getInstance()->fg_yellow);

    button_refresh.on_select = [this](Button&) {
        refresh_status();
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
    // Read CORRECT registers for Path 2 (active path!)
    uint32_t r0 = radio::debug::first_if::register_read(0);    // Control
    uint32_t r15 = radio::debug::first_if::register_read(15);  // P2_FREQ1
    uint32_t r16 = radio::debug::first_if::register_read(16);  // P2_FREQ2
    uint32_t r17 = radio::debug::first_if::register_read(17);  // P2_FREQ3

    // Display
    text_r0.set(to_string_hex(r0, 4));
    text_r1.set(to_string_hex(r15, 4) + " (R15)");
    text_r2.set(to_string_hex(r16, 4) + " (R16)");

    // Check enabled (R0 bit 4)
    bool enabled = (r0 & 0x0010) != 0;
    text_enabled.set(enabled ? "ENABLED" : "DISABLED");
    text_enabled.set_style(enabled ? Theme::getInstance()->fg_green
                                   : Theme::getInstance()->fg_red);

    // Decode from P2_FREQ1 (R15) using struct layout:
    // bits [1:0]   = p2vcosel
    // bits [3:2]   = p2presc (prescaler)
    // bits [6:4]   = p2lodiv (LO divider)
    // bits [15:7]  = p2n (N divider integer)

    uint16_t n_int = (r15 >> 7) & 0x1FF;    // 9 bits
    uint8_t lodiv_sel = (r15 >> 4) & 0x07;  // 3 bits
    uint8_t presc_sel = (r15 >> 2) & 0x03;  // 2 bits
    uint8_t vcosel = r15 & 0x03;            // 2 bits

    text_n.set(to_string_dec_uint(n_int));

    // LO divider: 0=÷2, 1=÷4, 2=÷8, 3=÷16, 4=÷32, 5=÷64
    uint16_t lodiv_val = 1u << lodiv_sel;

    // Prescaler: 0=÷2, 1=÷4 (but code only uses 1 or 2 per rffc507x.cpp)
    uint16_t presc_val = 1u << presc_sel;

    text_lodiv.set("/" + to_string_dec_uint(lodiv_val) +
                   " (P: /" + to_string_dec_uint(presc_val) + ")");

    // Calculate frequencies
    // F_VCO = (F_ref × N) / Prescaler
    // F_LO = F_VCO / LODIV
    const uint32_t f_ref_mhz = 40;

    // N divider is 24-bit fractional
    // For quick calc, use integer part only
    uint32_t f_vco_mhz = (f_ref_mhz * n_int) / presc_val;
    uint32_t f_lo_mhz = f_vco_mhz / lodiv_val;

    text_calc.set(to_string_dec_uint(f_lo_mhz) + " MHz");
    text_freq.set(to_string_dec_uint(f_vco_mhz) + " MHz VCO");

    // Check ranges
    bool vco_ok = (f_vco_mhz >= 2700) && (f_vco_mhz <= 5400);
    bool lo_ok = (f_lo_mhz >= 2300) && (f_lo_mhz <= 2700);

    text_calc.set_style(lo_ok ? Theme::getInstance()->fg_green
                              : Theme::getInstance()->fg_red);
    text_freq.set_style(vco_ok ? Theme::getInstance()->fg_green
                               : Theme::getInstance()->fg_red);

    // Check mixer mode (R0 bit 5 = MODE, 0=path1, 1=path2)
    bool path2_active = (r0 & 0x0020) != 0;
    text_path.set(path2_active ? "PATH2" : "PATH1");
    text_mixer.set(path2_active ? "ACTIVE" : "INACTIVE");
    text_mixer.set_style(path2_active ? Theme::getInstance()->fg_green
                                      : Theme::getInstance()->fg_orange);

    // Summary
    if (!enabled) {
        text_status.set("DISABLED!");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (!path2_active) {
        text_status.set("Path 2 not selected!");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (!vco_ok) {
        text_status.set("VCO " + to_string_dec_uint(f_vco_mhz) + "MHz OOR");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else if (!lo_ok) {
        text_status.set("LO " + to_string_dec_uint(f_lo_mhz) + "MHz OOR");
        text_status.set_style(Theme::getInstance()->fg_red);
    } else {
        text_status.set(to_string_dec_uint(f_ref_mhz) + "x" +
                        to_string_dec_uint(n_int) + "/" +
                        to_string_dec_uint(presc_val) + "/" +
                        to_string_dec_uint(lodiv_val) + "=" +
                        to_string_dec_uint(f_lo_mhz) + "MHz.");
        text_status.set_style(Theme::getInstance()->fg_green);
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
        &text_lbl_called, &text_called,
        &text_lbl_valid, &text_valid,
        &text_lbl_req, &text_req,
        &text_lbl_calc_n, &text_calc_n,
        &text_lbl_calc_frac, &text_calc_frac,
        &text_spacer,
        &text_lbl_r3, &text_r3,
        &text_lbl_r4, &text_r4,
        &text_lbl_act_n, &text_act_n,
        &text_lbl_act_frac, &text_act_frac,
        &text_lbl_calc_freq, &text_calc_freq,
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
    auto info = radio::debug::second_if::get_max2831_info();
    
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

void DebugMenuView::on_populate() {
    if (portapack::persistent_memory::show_gui_return_icon()) {
        add_items({{"..", ui::Theme::getInstance()->fg_light->foreground, &bitmap_icon_previous, [this]() { nav_.pop(); }}});
    }
    add_items({
#ifdef PRALINE
        {"Radio Diag", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RadioDiagnosticsView>(); }},
        {"Baseband Status", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<BasebandStatusView>(); }},
        {"SGPIO Live", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SGPIOLiveMonitorView>(); }},
        {"SGPIO8 Clock", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SGPIO8ClockDetectorView>(); }},
        {"Si5351 Clocks", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<Si5351DebugView>(); }},
        {"Signal Path", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<SignalPathStatusView>(); }},
        {"RFFC Status", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RFFC5072StatusView>(); }},
        {"RFFC Tuning", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RFFCTuningDebugView>(); }},
	{"MAX2831 Debug", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<MAX2831DebugView>(); }},
        {"RX Test", ui::Theme::getInstance()->fg_yellow->foreground, &bitmap_icon_peripherals, [this]() { nav_.push<RadioRxTestView>(); }},
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
