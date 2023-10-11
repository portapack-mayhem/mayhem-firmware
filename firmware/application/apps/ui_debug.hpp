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
        {96, 96, 48, 16},
        "Memory",
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

class TemperatureWidget : public Widget {
   public:
    explicit TemperatureWidget(
        Rect parent_rect)
        : Widget{parent_rect} {
    }

    void paint(Painter& painter) override;

   private:
    using sample_t = uint32_t;
    using temperature_t = int32_t;

    temperature_t temperature(const sample_t sensor_value) const;
    Coord screen_y(const temperature_t temperature, const Rect& screen_rect) const;

    std::string temperature_str(const temperature_t temperature) const;

    static constexpr temperature_t display_temp_min = -10;  // Accomodate negative values, present in cold startup cases
    static constexpr temperature_t display_temp_scale = 3;
    static constexpr int bar_width = 1;
    static constexpr int temp_len = 5;  // Now scale shows up to 5 chars ("-10ºC")
};

class TemperatureView : public View {
   public:
    explicit TemperatureView(NavigationView& nav);

    void focus() override;

    std::string title() const override { return "Temperature"; };

   private:
    Text text_title{
        {76, 16, 240, 16},
        "Temperature",
    };

    TemperatureWidget temperature_widget{
        {0, 40, 240, 180},
    };

    Button button_done{
        {72, 264, 96, 24},
        "Done"};
};

struct RegistersWidgetConfig {
    size_t registers_count;
    size_t register_bits;

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
    RegistersWidget(
        RegistersWidgetConfig&& config,
        std::function<uint32_t(const size_t register_number)>&& reader);

    void update();

    void paint(Painter& painter) override;

   private:
    const RegistersWidgetConfig config;
    const std::function<uint32_t(const size_t register_number)> reader;

    static constexpr size_t row_height = 16;

    void draw_legend(const Coord left, Painter& painter);
    void draw_values(const Coord left, Painter& painter);
};

class RegistersView : public View {
   public:
    RegistersView(
        NavigationView& nav,
        const std::string& title,
        RegistersWidgetConfig&& config,
        std::function<uint32_t(const size_t register_number)>&& reader);

    void focus();

   private:
    Text text_title{};

    RegistersWidget registers_widget;

    Button button_update{
        {16, 256, 96, 24},
        "Update"};

    Button button_done{
        {128, 256, 96, 24},
        "Done"};
};

class ControlsSwitchesWidget : public Widget {
   public:
    ControlsSwitchesWidget(
        Rect parent_rect)
        : Widget{parent_rect},
          key_event_mask(0),
          long_press_key_event_mask{0} {
        set_focusable(true);
    }

    void on_show() override;
    bool on_key(const KeyEvent key) override;

    void paint(Painter& painter) override;

   private:
    uint8_t key_event_mask;
    uint8_t long_press_key_event_mask;

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
        {{8 * 8, 1 * 16}, "Controls State", Color::white()},
        {{0 * 8, 14 * 16}, "Long-Press Mode:", Color::grey()}};

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
        {72, 264, 96, 24},
        "Done"};
};

class DebugPmemView : public View {
   public:
    DebugPmemView(NavigationView& nav);
    void focus() override;
    bool on_encoder(const EncoderEvent delta) override;
    std::string title() const override { return "P.Mem Debug"; }

   private:
    struct pmem_data {
        uint32_t regfile[63];
        uint32_t check_value;
    };

    static constexpr uint8_t page_size{96};  // Must be multiply of 4 otherwise bit shifting for register view wont work properly
    static constexpr uint8_t page_max{(portapack::memory::map::backup_ram.size() + page_size - 1) / page_size - 1};

    int32_t page{0};

    volatile const pmem_data& data;

    Text text_page{{16, 16, 208, 16}};

    RegistersWidget registers_widget;

    Text text_checksum{{16, 232, 208, 16}};
    Text text_checksum2{{16, 248, 208, 16}};

    Button button_ok{
        {240 / 3, 270, 240 / 3, 24},
        "OK",
    };

    void update();
    uint32_t registers_widget_feed(const size_t register_number);
};

class DebugScreenTest : public View {
   public:
    DebugScreenTest(NavigationView& nav);
    bool on_key(KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_touch(TouchEvent event) override;
    uint16_t semirand();
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
                { 8, 16, 224, 240 }
        };

        Button button_exit {
                { 72, 264, 96, 32 },
                "Exit"
        };
};*/

class DebugPeripheralsMenuView : public BtnGridView {
   public:
    DebugPeripheralsMenuView(NavigationView& nav);
    std::string title() const override { return "Peripherals"; };
};

class DebugMenuView : public BtnGridView {
   public:
    DebugMenuView(NavigationView& nav);
    std::string title() const override { return "Debug"; };
};

} /* namespace ui */

#endif /*__UI_DEBUG_H__*/
