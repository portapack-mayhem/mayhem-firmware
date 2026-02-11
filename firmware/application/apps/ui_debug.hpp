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

#ifndef __UI_DEBUG_H__
#define __UI_DEBUG_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include "rffc507x.hpp"
#include "portapack.hpp"
#include "memory_map.hpp"
#include "irq_controls.hpp"

#include <functional>
#include <utility>

namespace ui {

class DebugMemoryView : public View {
   public:
    DebugMemoryView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Memory"; };

   private:
    Text text_title{
        {72, 96, 96, 16},
        "Memory Usage",
    };

    Text text_label_m0_core_free{
        {0, 128, 144, 16},
        "M0 Core Free Bytes",
    };

    Text text_label_m0_core_free_value{
        {200, 128, 40, 16},
    };

    Text text_label_m0_heap_fragmented_free{
        {0, 144, 184, 16},
        "M0 Heap Fragmented Free",
    };

    Text text_label_m0_heap_fragmented_free_value{
        {200, 144, 40, 16},
    };

    Text text_label_m0_heap_fragments{
        {0, 160, 136, 16},
        "M0 Heap Fragments",
    };

    Text text_label_m0_heap_fragments_value{
        {200, 160, 40, 16},
    };

    Button button_done{
        {72, 192, 96, 24},
        "Done"};
};

typedef enum {
    CT_PMEM,
    CT_RFFC5072,
    CT_MAX283X,
    CT_SI5351,
    CT_AUDIO,
    CT_MAX17055,
#ifdef PRALINE
    CT_FPGA,
#endif
    CT_SGPIO,
} chip_type_t;

struct RegistersWidgetConfig {
    chip_type_t chip_type;
    uint32_t registers_count;
    uint32_t registers_per_page;
    uint32_t register_bits;

    constexpr size_t legend_length() const {
        return (registers_count >= 0x10) ? 2 : 1;
    }

    constexpr size_t legend_width() const {
        return legend_length() * 8;
    }

    constexpr size_t value_length() const {
        return (register_bits + 3) / 4;
    }

    constexpr size_t value_width() const {
        return value_length() * 8;
    }

    constexpr size_t registers_per_row() const {
        return (value_length() >= 3) ? 4 : 8;
    }

    constexpr size_t registers_row_length() const {
        return (registers_per_row() * (value_length() + 1)) - 1;
    }

    constexpr size_t registers_row_width() const {
        return registers_row_length() * 8;
    }

    constexpr size_t row_width() const {
        return legend_width() + 8 + registers_row_width();
    }

    constexpr size_t rows() const {
        return registers_count / registers_per_row();
    }
};

class RegistersWidget : public Widget {
   public:
    RegistersWidget(RegistersWidgetConfig&& config);

    void update();

    void paint(Painter& painter) override;

    uint32_t reg_read(const uint32_t register_number);
    void reg_write(const uint32_t register_number, const uint32_t value);

    void set_page(int32_t value) { page_number = value; }
    uint32_t page(void) { return page_number; }
    uint32_t page_count(void) { return (config.registers_count + config.registers_per_page - 1) / config.registers_per_page; }

   private:
    const RegistersWidgetConfig config;
    uint32_t page_number;

    static constexpr size_t row_height = 16;

    void draw_legend(const Coord left, Painter& painter);
    void draw_values(const Coord left, Painter& painter);
};

class RegistersView : public View {
   public:
    RegistersView(NavigationView& nav, const std::string& title, RegistersWidgetConfig&& config);
    void focus() override;
    bool on_encoder(const EncoderEvent delta) override;

   private:
    Text text_title{};

    RegistersWidget registers_widget;

    Button button_update{
        {16, 280, 96, 24},
        "Update"};

    Button button_done{
        {128, 280, 96, 24},
        "Done"};

    Button button_write{
        {144, 248, 80, 20},
        "Write"};

    Labels labels{
        {{1 * 8, 248}, "Reg:", Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 248}, "Data:", Theme::getInstance()->fg_light->foreground}};

    SymField field_write_reg_num{
        {5 * 8, 248},
        2,
        SymField::Type::Hex};

    SymField field_write_data_val{
        {13 * 8, 248},
        4,
        SymField::Type::Hex};
};

class ControlsSwitchesWidget : public Widget {
   public:
    ControlsSwitchesWidget(
        Rect parent_rect)
        : Widget{parent_rect},
          key_event_mask(0),
          long_press_key_event_mask{0},
          last_delta{0} {
        set_focusable(true);
    }

    void on_show() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;

    void paint(Painter& painter) override;

   private:
    uint8_t key_event_mask;
    uint8_t long_press_key_event_mask;
    EncoderEvent last_delta;

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_frame_sync();
        }};

    void on_frame_sync();
};

class DebugControlsView : public View {
   public:
    explicit DebugControlsView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Buttons Test"; };

   private:
    Labels labels{
        {{UI_POS_X_CENTER(14), 1 * 16}, "Controls State", Theme::getInstance()->bg_darkest->foreground},
        {{UI_POS_X(0), 11 * 16}, "Dial:", Theme::getInstance()->fg_medium->foreground},
        {{UI_POS_X(0), 14 * 16}, "Long-Press Mode:", Theme::getInstance()->fg_medium->foreground}};

    ControlsSwitchesWidget switches_widget{
        {80, 80, 80, 112},
    };

    OptionsField options_switches_mode{
        {17 * 8, 14 * 16},
        8,
        {
            {"Disabled", 0},
            {"Enabled", 0xFF},  // all KeyEvent bits to long-press mode
        }};

    Button button_done{
        {UI_POS_X_CENTER(12), 264, 96, 24},
        "Done"};
};

class DebugMemoryDumpView : public View {
   public:
    DebugMemoryDumpView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Memory Dump"; };

   private:
    Button button_dump{
        {UI_POS_X_CENTER(12), 4 * 16, 96, 24},
        "Dump"};

    Button button_read{
        {UI_POS_X_CENTER(12) - UI_POS_WIDTH(10), 11 * 16, 96, 24},
        "Read"};

    Button button_write{
        {UI_POS_X_CENTER(12) + UI_POS_WIDTH(10), 11 * 16, 96, 24},
        "Write"};

    Button button_done{
        {UI_POS_X_RIGHT(12), UI_POS_Y_BOTTOM(3), 96, 24},
        "Done"};

    Labels labels{
        {{5 * 8, 1 * 16}, "Dump Range to File", Theme::getInstance()->fg_yellow->foreground},
        {{UI_POS_X(0), 2 * 16}, "Starting Address: 0x", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 3 * 16}, "Byte Count:       0x", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, 8 * 16}, "Read/Write Single Word", Theme::getInstance()->fg_yellow->foreground},
        {{UI_POS_X(0), 9 * 16}, "Memory Address:   0x", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 10 * 16}, "Data Value:       0x", Theme::getInstance()->fg_light->foreground}};

    SymField field_starting_address{
        {20 * 8, 2 * 16},
        8,
        SymField::Type::Hex};

    SymField field_byte_count{
        {20 * 8, 3 * 16},
        8,
        SymField::Type::Hex};

    SymField field_rw_address{
        {20 * 8, 9 * 16},
        8,
        SymField::Type::Hex};

    SymField field_data_value{
        {20 * 8, 10 * 16},
        8,
        SymField::Type::Hex};
};

class DebugPmemView : public View {
   public:
    DebugPmemView(NavigationView& nav);
    void focus() override;
    bool on_encoder(const EncoderEvent delta) override;
    std::string title() const override { return "P.Mem Debug"; }

   private:
    static constexpr uint8_t page_size{96};  // Must be multiply of 4 otherwise bit shifting for register view wont work properly
    static constexpr uint8_t page_count{(portapack::memory::map::backup_ram.size() + page_size - 1) / page_size};

    RegistersWidget registers_widget;

    Text text_checksum{{16, 232, 208, 16}};
    Text text_checksum2{{16, 248, 208, 16}};

    Button button_ok{
        {screen_width / 3, 270, screen_width / 3, 24},
        "OK",
    };

    void update();
};

class DebugScreenTest : public View {
   public:
    DebugScreenTest(NavigationView& nav);
    bool on_key(KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_touch(TouchEvent event) override;
    void paint(Painter& painter) override;

   private:
    NavigationView& nav_;
    Point pen_pos{};
    Color pen_color{0};
    int16_t pen_size{10};
};

/*class DebugLCRView : public View {
public:
        DebugLCRView(NavigationView& nav, std::string lcrstring);

        void focus() override;

        std::string title() const override { return "LCR debug"; };

private:
        Console console {
                { 8, 16, 224, screen_height-80 }
        };

        Button button_exit {
                { 72, 264, 96, 32 },
                "Exit"
        };
};*/

/* Radio Signal Path Diagnostics View
 * Shows status of each component in the RX/TX signal chain:
 * Antenna -> RF Path -> RFFC5072 -> MAX283x -> MAX5864 -> FPGA -> SGPIO -> MCU
 */
class RadioDiagnosticsView : public View {
   public:
    RadioDiagnosticsView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "Radio Diag"; };

   private:
    NavigationView& nav_;

    void update_status();

    Text text_title{{0, 0, 240, 16}, "=== Signal Path Status ==="};

    Text text_lbl_rffc{{0, 20, 140, 16}, "RFFC5072 (1st IF):"};
    Text text_rffc_status{{144, 20, 96, 16}, "---"};

    Text text_lbl_max{{0, 36, 140, 16}, "MAX283x (2nd IF):"};
    Text text_max_status{{144, 36, 96, 16}, "---"};

    Text text_lbl_adc{{0, 52, 140, 16}, "MAX5864 (ADC):"};
    Text text_adc_status{{144, 52, 96, 16}, "---"};

    Text text_lbl_fpga{{0, 68, 140, 16}, "FPGA/CPLD:"};
    Text text_fpga_status{{144, 68, 96, 16}, "---"};

    Text text_lbl_sgpio{{0, 84, 140, 16}, "SGPIO:"};
    Text text_sgpio_status{{144, 84, 96, 16}, "---"};

    Text text_lbl_clock{{0, 100, 140, 16}, "Si5351 Clocks:"};
    Text text_clock_status{{144, 100, 96, 16}, "---"};

    Text text_regs_title{{0, 124, 240, 16}, "=== Key Registers ==="};

    Text text_lbl_rffc_reg{{0, 144, 80, 16}, "RFFC R0:"};
    Text text_rffc_reg{{80, 144, 160, 16}, "---"};

    Text text_lbl_max_reg{{0, 160, 80, 16}, "MAX R0:"};
    Text text_max_reg{{80, 160, 160, 16}, "---"};

    Text text_lbl_fpga_reg{{0, 176, 80, 16}, "FPGA:"};
    Text text_fpga_reg{{80, 176, 160, 16}, "---"};

    Text text_lbl_sgpio_reg{{0, 192, 80, 16}, "SGPIO:"};
    Text text_sgpio_reg{{80, 192, 160, 16}, "---"};

    Text text_test_result{{0, 220, 240, 32}, ""};

    Button button_refresh{
        {8, 280, 72, 24},
        "Refresh"};

    Button button_done{
        {168, 280, 64, 24},
        "Done"};
};

/* BasebandStatusView ***************************************************/

class BasebandStatusView : public View {
   public:
    BasebandStatusView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "Baseband Status"; };

   private:
    NavigationView& nav_;

    void update();

    Text text_title{{0, 0, 240, 16}, "=== Baseband Counters ==="};

    Text text_lbl_marker{{0, 20, 140, 16}, "Streaming Marker:"};
    Text text_marker{{144, 20, 96, 16}, "---"};

    Text text_lbl_loops{{0, 36, 140, 16}, "Baseband Loops:"};
    Text text_loops{{144, 36, 96, 16}, "---"};

    Text text_lbl_wait{{0, 52, 140, 16}, "DMA Wait Count:"};
    Text text_wait{{144, 52, 96, 16}, "---"};

    Text text_lbl_xfr{{0, 68, 140, 16}, "DMA Xfr Count:"};
    Text text_xfr{{144, 68, 96, 16}, "---"};

    Text text_lbl_missed{{0, 84, 140, 16}, "Buffer Missed:"};
    Text text_missed{{144, 84, 96, 16}, "---"};

    Text text_status_line1{{0, 110, 240, 16}, ""};
    Text text_status_line2{{0, 126, 240, 16}, ""};
    Text text_status_line3{{0, 142, 240, 16}, ""};

    Button button_refresh{
        {8, 280, 72, 24},
        "Refresh"};

    Button button_done{
        {168, 280, 64, 24},
        "Done"};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) { this->update(); }};
};

/* SGPIOLiveMonitorView ***************************************************/

class SGPIOLiveMonitorView : public View {
   public:
    SGPIOLiveMonitorView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "SGPIO Live"; };

   private:
    NavigationView& nav_;

    void update();

    Text text_title{{0, 0, 240, 16}, "=== SGPIO Registers ==="};

    Text text_lbl_ctrl{{0, 20, 140, 16}, "CTRL_ENABLE:"};
    Text text_ctrl{{144, 20, 96, 16}, "---"};

    Text text_lbl_in{{0, 36, 140, 16}, "GPIO_INREG:"};
    Text text_in{{144, 36, 96, 16}, "---"};

    Text text_lbl_ss{{0, 52, 140, 16}, "REG_SS[0]:"};
    Text text_ss{{144, 52, 96, 16}, "---"};

    Text text_lbl_status{{0, 68, 140, 16}, "STATUS_1:"};
    Text text_status{{144, 68, 96, 16}, "---"};

    Text text_lbl_out{{0, 84, 140, 16}, "GPIO_OUTREG:"};
    Text text_out{{144, 84, 96, 16}, "---"};

    Text text_lbl_oen{{0, 100, 140, 16}, "GPIO_OENREG:"};
    Text text_oen{{144, 100, 96, 16}, "---"};

    Text text_diag_line1{{0, 126, 240, 16}, ""};
    Text text_diag_line2{{0, 142, 240, 16}, ""};
    Text text_diag_line3{{0, 158, 240, 16}, ""};
    Text text_diag_line4{{0, 174, 240, 16}, ""};

    Button button_refresh{
        {8, 280, 72, 24},
        "Refresh"};

    Button button_done{
        {168, 280, 64, 24},
        "Done"};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) { this->update(); }};
};

/* Radio RX Step-by-Step Test View
 * Tests radio hardware directly without M0 baseband involvement.
 * Logs each step to help isolate where the signal chain breaks.
 */
class RadioRxTestView : public View {
   public:
    RadioRxTestView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "RX Test"; };

   private:
    NavigationView& nav_;
    bool radio_initialized_{false};
    uint32_t test_frequency_{433000000};  // 433 MHz default

    void log(const std::string& msg);
    void log_registers(const std::string& label);
    void run_init_test();
    void run_rx_mode_test();
    void run_freq_test();
    void run_sgpio_test();
    void run_full_test();
    void run_step_test();
    bool check_gpio_changing();

    Labels labels{
        {{0, 0}, "=== Radio RX Test ===", Theme::getInstance()->fg_yellow->foreground}};

    Console console{
        {0, 20, 240, 200}};

    Button button_init{
        {0, 224, 56, 24},
        "Init"};

    Button button_rx{
        {60, 224, 56, 24},
        "RX"};

    Button button_freq{
        {120, 224, 56, 24},
        "Freq"};

    Button button_sgpio{
        {180, 224, 56, 24},
        "SGPIO"};

    Button button_full{
        {0, 252, 56, 24},
        "Full"};

    Button button_step{
        {60, 252, 56, 24},
        "Step"};

    Button button_done{
        {120, 252, 112, 24},
        "Done"};
};

/* SGPIO8 Clock Detector View
 * Samples SGPIO8 pin to verify external clock is present.
 * Shows toggle count and estimated frequency.
 */
class SGPIO8ClockDetectorView : public View {
   public:
    SGPIO8ClockDetectorView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "SGPIO8 Clock"; };

   private:
    NavigationView& nav_;

    Text text_title{{8, 16, 224, 16}, "SGPIO8 Clock Detector"};
    Text text_lbl_samples{{8, 48, 160, 16}, "Samples (first 20):"};
    Text text_samples{{8, 64, 224, 16}, "                    "};
    Text text_lbl_toggles{{8, 96, 160, 16}, "Toggle count:"};
    Text text_toggles{{8, 112, 224, 16}, "                         "};
    Text text_status{{8, 144, 224, 32}, "                              "};

    Button button_sample{{8, 200, 96, 24}, "Sample"};
    Button button_done{{128, 200, 96, 24}, "Done"};

    void sample_sgpio8();
};

/* Slice Status View
 * Shows SGPIO slice status: which slices are enabled vs active,
 * counter values, and data capture status.
 */

/* Si5351 Debug View
 * Dedicated diagnostic tool for Si5351 clock generator.
 * Shows PLL lock status, clock configurations, and allows testing.
 */
class Si5351DebugView : public View {
   public:
    Si5351DebugView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Si5351 Clocks"; };

   private:
    NavigationView& nav_;

    Text text_title{{8, 16, 200, 16}, "Si5351 Clock Generator"};

    Text text_status_label{{8, 40, 80, 16}, "Status Reg:"};
    Text text_status_value{{96, 40, 144, 16}, ""};

    Text text_pll_a_label{{8, 60, 80, 16}, "PLL A:"};
    Text text_pll_a_status{{96, 60, 144, 16}, ""};

    Text text_pll_b_label{{8, 80, 80, 16}, "PLL B:"};
    Text text_pll_b_status{{96, 80, 144, 16}, ""};

    Text text_sys_init_label{{8, 100, 80, 16}, "SYS_INIT:"};
    Text text_sys_init_status{{96, 100, 144, 16}, ""};

    Text text_xtal_cap_label{{8, 120, 80, 16}, "XTAL Cap:"};
    Text text_xtal_cap_value{{96, 120, 144, 16}, ""};

    Text text_clk0_label{{8, 150, 72, 16}, "CLK0:"};
    Text text_clk0_status{{88, 150, 152, 16}, ""};

    Text text_clk0_freq_label{{8, 170, 72, 16}, "  Freq:"};
    Text text_clk0_freq_value{{88, 170, 152, 16}, ""};

    Text text_clk0_div_label{{8, 190, 72, 16}, "  Div:"};
    Text text_clk0_div_value{{88, 190, 152, 16}, ""};

    Text text_clk1_label{{8, 210, 96, 16}, "CLK1 (SCT):"};
    Text text_clk1_status{{112, 210, 128, 16}, ""};

    Button button_refresh{{8, 240, 72, 24}, "Refresh"};
    Button button_reset_pll{{88, 240, 72, 24}, "Reset PLL"};
    Button button_done{{168, 240, 64, 24}, "Done"};

    void refresh_status();
    void reset_pll();
};

#ifdef PRALINE
/* SignalPathStatusView *************************************************/
class SignalPathStatusView : public View {
   public:
    SignalPathStatusView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Signal Path"; };

   private:
    NavigationView& nav_;
    void refresh_status();

    Text text_title{{0, 0, 240, 16}, "=== Signal Path Status ==="};

    Text text_lbl_max_enable{{0, 20, 1114, 16}, "MAX2831:"};
    Text text_max_enable{{116, 20, 124, 16}, "---"};

    Text text_lbl_max_mode{{0, 36, 114, 16}, "RX Mode:"};
    Text text_max_mode{{116, 36, 124, 16}, "---"};

    Text text_lbl_rf_path{{0, 52, 114, 16}, "RF Path:"};
    Text text_rf_path{{116, 52, 124, 16}, "---"};

    Text text_lbl_filter{{0, 68, 114, 16}, "Filter Band:"};
    Text text_filter{{116, 68, 124, 16}, "---"};

    Text text_lbl_mixer{{0, 84, 114, 16}, "Mixer Enable:"};
    Text text_mixer{{116, 84, 124, 16}, "---"};

    Text text_lbl_rf_amp{{0, 100, 114, 16}, "RF Amp:"};
    Text text_rf_amp{{116, 100, 124, 16}, "---"};

    Text text_lbl_lna{{0, 116, 114, 16}, "LNA Gain:"};
    Text text_lna{{116, 116, 124, 16}, "---"};

    Text text_lbl_vga{{0, 132, 114, 16}, "VGA Gain:"};
    Text text_vga{{116, 132, 124, 16}, "---"};

    Text text_lbl_fpga_decim{{0, 148, 114, 16}, "FPGA Decim:"};
    Text text_fpga_decim{{116, 148, 124, 16}, "---"};

    Text text_lbl_fpga_ctrl_dc_q{{0, 164, 114, 16}, "FPGA Ctrl:"};
    Text text_fpga_ctrl_dc_q{{116, 164, 124, 16}, "---"};

    Text text_lbl_fpga_ctrl_qs{{0, 180, 114, 16}, ""};
    Text text_fpga_ctrl_qs{{116, 180, 124, 16}, "---"};

    Text text_status{{0, 200, 240, 32}, ""};

    Button button_refresh{{0, 280, 72, 24}, "Refresh"};
    Button button_toggle_q{{88, 280, 72, 24}, "Q Inv"};
    Button button_done{{176, 280, 64, 24}, "Done"};
};
#endif

#ifdef PRALINE
/* RFFC5072StatusView *************************************************/
class RFFC5072StatusView : public View {
   public:
    RFFC5072StatusView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "RFFC5072 Status"; };

   private:
    NavigationView& nav_;
    void refresh_status();

    Text text_title{{0, 0, 240, 16}, "=== RFFC5072 (1st IF) ==="};

    Text text_lbl_enabled{{0, 20, 114, 16}, "Status:"};
    Text text_enabled{{116, 20, 124, 16}, "---"};

    Text text_lbl_freq{{0, 36, 114, 16}, "LO Freq:"};
    Text text_freq{{116, 36, 124, 16}, "---"};

    Text text_lbl_path{{0, 52, 114, 16}, "Path:"};
    Text text_path{{116, 52, 124, 16}, "---"};

    Text text_lbl_mixer{{0, 68, 114, 16}, "Mixer:"};
    Text text_mixer{{116, 68, 124, 16}, "---"};

    Text text_lbl_r0{{0, 92, 114, 16}, "Reg 0:"};
    Text text_r0{{116, 92, 124, 16}, "---"};

    Text text_lbl_r1{{0, 108, 114, 16}, "Reg 1 (N):"};
    Text text_r1{{116, 108, 124, 16}, "---"};

    Text text_lbl_r2{{0, 124, 114, 16}, "Reg 2:"};
    Text text_r2{{116, 124, 124, 16}, "---"};

    Text text_lbl_decode{{0, 148, 240, 16}, "--- Decoded Values ---"};

    Text text_lbl_n{{0, 168, 114, 16}, "N divider:"};
    Text text_n{{116, 168, 124, 16}, "---"};

    Text text_lbl_lodiv{{0, 184, 114, 16}, "LO divider:"};
    Text text_lodiv{{116, 184, 124, 16}, "---"};

    Text text_lbl_calc{{0, 200, 114, 16}, "Calc freq:"};
    Text text_calc{{116, 200, 124, 16}, "---"};

    Text text_status{{0, 224, 240, 32}, ""};

    Button button_refresh{{8, 280, 72, 24}, "Refresh"};
    Button button_done{{168, 280, 64, 24}, "Done"};
};

#ifdef PRALINE
/* RFFCTuningDebugView *************************************************/
class RFFCTuningDebugView : public View {
   public:
    RFFCTuningDebugView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "RFFC Tuning Debug"; };

   private:
    void refresh();

    Text text_title{{0, 0, 240, 16}, "RFFC5072 Tuning Debug"};

    Text text_lbl_called{{0, 18, 120, 16}, "Freq Set:"};
    Text text_called{{122, 18, 118, 16}, "NO"};

    Text text_lbl_req{{0, 38, 120, 16}, "Requested:"};
    Text text_req{{122, 38, 118, 16}, "---"};

    Text text_lbl_exp_n{{0, 56, 120, 16}, "Expected N:"};
    Text text_exp_n{{122, 56, 118, 16}, "---"};

    Text text_lbl_act_n{{0, 74, 120, 16}, "Actual N:"};
    Text text_act_n{{122, 74, 118, 16}, "---"};

    Text text_lbl_exp_div{{0, 92, 120, 16}, "Exp LO/Pres:"};
    Text text_exp_div{{122, 92, 118, 16}, "---"};

    Text text_lbl_act_div{{0, 110, 120, 16}, "Act LO/Pres:"};
    Text text_act_div{{122, 110, 118, 16}, "---"};

    Text text_lbl_calc{{0, 128, 120, 16}, "Calc LO freq:"};
    Text text_calc{{122, 128, 118, 16}, "---"};

    Text text_lbl_calc_lo{{0, 146, 120, 16}, "Calc input:"};
    Text text_calc_lo{{122, 146, 118, 16}, "---"};

    Text text_lbl_calc_vco{{0, 164, 120, 16}, "In Calc VCO:"};
    Text text_calc_vco{{122, 164, 118, 16}, "---"};

    Text text_lbl_vco{{0, 182, 120, 16}, "Calc VCO:"};
    Text text_vco{{122, 182, 118, 16}, "---"};

    Text text_lbl_n_q24{{0, 200, 120, 16}, "N (Q24):"};
    Text text_n_q24{{122, 200, 118, 16}, "---"};

    Text text_status{{0, 224, 240, 48}, ""};

    Button button_refresh{{8, 280, 72, 24}, "Refresh"};
    Button button_done{{168, 280, 64, 24}, "Done"};
};

#endif

#ifdef PRALINE
/* MAX2831DebugView *************************************************/

class MAX2831DebugView : public View {
   public:
    MAX2831DebugView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "MAX2831 Debug"; };

   private:
    void refresh();

    Text text_title{{0, 0, 240, 16}, "MAX2831 (2nd IF) Debug"};

    Text text_lbl_called{{0, 24, 120, 16}, "Freq Set:"};
    Text text_called{{122, 24, 118, 16}, "NO"};

    Text text_lbl_valid{{0, 42, 120, 16}, "In Range:"};
    Text text_valid{{122, 42, 118, 16}, "---"};

    Text text_lbl_req{{0, 60, 120, 16}, "Requested:"};
    Text text_req{{122, 60, 118, 16}, "---"};

    Text text_lbl_calc_n{{0, 78, 120, 16}, "Calc N:"};
    Text text_calc_n{{122, 78, 118, 16}, "---"};

    Text text_lbl_calc_frac{{0, 96, 120, 16}, "Calc Frac:"};
    Text text_calc_frac{{122, 96, 118, 16}, "---"};

    Text text_spacer{{0, 114, 240, 16}, "--- Hardware Regs ---"};

    Text text_lbl_r3{{0, 132, 120, 16}, "Reg 3:"};
    Text text_r3{{122, 132, 118, 16}, "---"};

    Text text_lbl_r4{{0, 150, 120, 16}, "Reg 4:"};
    Text text_r4{{122, 150, 118, 16}, "---"};

    Text text_lbl_act_n{{0, 168, 120, 16}, "Actual N:"};
    Text text_act_n{{122, 168, 118, 16}, "---"};

    Text text_lbl_act_frac{{0, 186, 120, 16}, "Actual Frac:"};
    Text text_act_frac{{122, 186, 118, 16}, "---"};

    Text text_lbl_calc_freq{{0, 204, 120, 16}, "Calc Freq:"};
    Text text_calc_freq{{122, 204, 118, 16}, "---"};

    Text text_status{{0, 228, 240, 44}, ""};

    Button button_refresh{{8, 280, 72, 24}, "Refresh"};
    Button button_done{{168, 280, 64, 24}, "Done"};
};
#endif

#endif

class DebugPeripheralsMenuView : public BtnGridView {
   public:
    DebugPeripheralsMenuView(NavigationView& nav);
    std::string title() const override { return "Peripherals"; };

   private:
    NavigationView& nav_;
    void on_populate() override;
};

class DebugReboot : public BtnGridView {
   public:
    DebugReboot(NavigationView& nav);

   private:
    void on_populate() override;
};

class DebugMenuView : public BtnGridView {
   public:
    DebugMenuView(NavigationView& nav);
    std::string title() const override { return "Debug"; };

   private:
    NavigationView& nav_;
    void on_populate() override;
};

} /* namespace ui */

#endif /*__UI_DEBUG_H__*/
