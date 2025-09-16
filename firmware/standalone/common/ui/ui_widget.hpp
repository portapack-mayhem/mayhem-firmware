/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __UI_WIDGET_H__
#define __UI_WIDGET_H__

#include "ui.hpp"
#include "ui_text.hpp"
#include "ui_painter.hpp"
#include "ui_focus.hpp"
#include "theme.hpp"
// #include "rtc_time.hpp"
//  #include "radio.hpp"

// #include "portapack.hpp"
#include "utility.hpp"

// #include "ui/ui_font_fixed_5x8.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rf {

using Frequency = int64_t;
}

namespace ui {

void dirty_set();
void dirty_clear();
bool is_dirty();

class Context {
   public:
    FocusManager& focus_manager() {
        return focus_manager_;
    }

   private:
    FocusManager focus_manager_{};
};

class Widget {
   public:
    Widget()
        : _parent_rect{} {
    }

    Widget(
        Rect parent_rect)
        : _parent_rect{parent_rect} {
    }

    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;

    virtual ~Widget() = default;

    Point screen_pos();
    Size size() const;
    Rect screen_rect() const;
    Rect parent_rect() const;
    virtual void set_parent_rect(const Rect new_parent_rect);

    Widget* parent() const;
    void set_parent(Widget* const widget);

    bool hidden() const { return flags.hidden; }
    void hidden(bool hide);

    virtual void focus();
    virtual void on_focus();
    virtual void blur();
    virtual void on_blur();
    bool focusable() const;
    void set_focusable(const bool value);
    bool has_focus();

    virtual void paint(Painter& painter) = 0;

    virtual void on_show() {};
    virtual void on_hide() {};

    virtual bool on_key(const KeyEvent event);
    virtual bool on_encoder(const EncoderEvent event);
    virtual bool on_touch(const TouchEvent event);
    virtual bool on_keyboard(const KeyboardEvent event);
    virtual const std::vector<Widget*>& children() const;

    virtual Context& context() const;

    virtual void getAccessibilityText(std::string& result);
    virtual void getWidgetName(std::string& result);

    void set_style(const Style* new_style);

    const Style& style() const;
    int16_t id = 0;

    // State management methods.
    void set_dirty();
    bool dirty() const;
    void set_clean();

    void visible(bool v);
    bool visible() { return flags.visible; };

    bool highlighted() const;
    void set_highlighted(const bool value);

   protected:
    void dirty_overlapping_children_in_rect(const Rect& child_rect);

   private:
    /* Widget rectangle relative to parent pos(). */
    Rect _parent_rect;
    const Style* style_{nullptr};
    Widget* parent_{nullptr};

    struct flags_t {
        bool dirty : 1;        // Widget content has changed.
        bool hidden : 1;       // Hide widget and children.
        bool focusable : 1;    // Widget can receive focus.
        bool highlighted : 1;  // Show in a highlighted style.
        bool visible : 1;      // Object was visible during last paint.
    };

    flags_t flags{
        .dirty = true,
        .hidden = false,
        .focusable = false,
        .highlighted = false,
        .visible = false,
    };

    static const std::vector<Widget*> no_children;
};

class View : public Widget {
    // unlike Paint class, our Y ignored the top bar;
    // so when you draw some of us as Y = 0, it would be exact below the top bar, instead of overlapped with top bar
   public:
    View() {
    }

    View(Rect parent_rect) {
        set_parent_rect(parent_rect);
    }

    // TODO: ~View() should on_hide() all children?

    void paint(Painter& painter) override;

    void add_child(Widget* const widget);
    void add_children(const std::initializer_list<Widget*> children);
    void remove_child(Widget* const widget);
    void remove_children(const std::vector<Widget*>& children);
    const std::vector<Widget*>& children() const override;

    virtual std::string title() const;

   protected:
    std::vector<Widget*> children_{};

    void invalidate_child(Widget* const widget);
};

class Rectangle : public Widget {
   public:
    Rectangle(Color c);
    Rectangle(Rect parent_rect, Color c);
    Rectangle()
        : Rectangle{{}, {}} {
    }

    void paint(Painter& painter) override;

    void set_color(const Color c);
    void set_outline(const bool outline);

   private:
    Color color;
    bool _outline = false;
};

class Text : public Widget {
   public:
    Text()
        : text{""} {
    }

    Text(Rect parent_rect, std::string text);
    Text(Rect parent_rect);

    void set(std::string_view value);

    void paint(Painter& painter) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   protected:
    // NB: Don't truncate this string. The UI will only render
    // as many characters as will fit in its rectange.
    // Apps expect to be able to retrieve this string to avoid
    // needing to hold additional copies in memory.
    std::string text;
};

class Labels : public Widget {
   public:
    struct Label {
        Point pos;
        std::string text;
        ui::Color color;
    };

    Labels(const Labels&) = delete;
    Labels(Labels&&) = delete;
    Labels& operator=(const Labels&) = delete;
    Labels& operator=(Labels&&) = delete;

    Labels(std::initializer_list<Label> labels);

    void set_labels(std::initializer_list<Label> labels);

    void paint(Painter& painter) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    std::vector<Label> labels_;
};

class LiveDateTime : public Widget {
   public:
    LiveDateTime(Rect parent_rect);
    ~LiveDateTime();

    void paint(Painter& painter) override;

    void set_hide_clock(bool new_value);
    void set_seconds_enabled(bool new_value);
    void set_date_enabled(bool new_value);

    std::string& string() {
        return text;
    }

   private:
    void on_tick_second();

    uint16_t init_delay = 4;
    bool hide_clock = false;
    bool date_enabled = true;
    bool seconds_enabled = false;

    // TODO: wire standalone api: rtc::RTC datetime{};
    // TODO: wire standalone api: SignalToken signal_token_tick_second{};
    std::string text{};
};

class BigFrequency : public Widget {
   public:
    BigFrequency(Rect parent_rect, rf::Frequency frequency);

    void set(const rf::Frequency frequency);

    void paint(Painter& painter) override;

   private:
    rf::Frequency _frequency;

    static constexpr Dim digit_width = 32;

    const uint8_t segment_font[11] = {
        0b00111111,  // 0: ABCDEF
        0b00000110,  // 1: AB
        0b01011011,  // 2: ABDEG
        0b01001111,  // 3: ABCDG
        0b01100110,  // 4: BCFG
        0b01101101,  // 5: ACDFG
        0b01111101,  // 6: ACDEFG
        0b00000111,  // 7: ABC
        0b01111111,  // 8: ABCDEFG
        0b01101111,  // 9: ABCDFG
        0b01000000   // -: G
    };

    const Rect segments[7] = {
        {{4, 0}, {20, 4}},
        {{24, 4}, {4, 20}},
        {{24, 28}, {4, 20}},
        {{4, 48}, {20, 4}},
        {{0, 28}, {4, 20}},
        {{0, 4}, {4, 20}},
        {{4, 24}, {20, 4}}};
};

class ProgressBar : public Widget {
   public:
    ProgressBar(Rect parent_rect);

    void set_max(const uint32_t max);
    void set_value(const uint32_t value);

    void paint(Painter& painter) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    uint32_t _value = 0;
    uint32_t _max = 100;
};

/* A simple status indicator that can be used to indicate activity. */
class ActivityDot : public Widget {
   public:
    ActivityDot(Rect parent_rect, Color color);

    void paint(Painter& painter) override;
    void toggle();
    void reset();

   private:
    bool _on{false};
    Color _color;
};

class Console : public Widget {
   public:
    Console(Rect parent_rect);

    void clear(bool clear_buffer);
    void write(std::string message);
    void writeln(std::string message);

    void paint(Painter&) override;

    void enable_scrolling(bool enable);
    void on_show() override;
    void on_hide() override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    Point pos{0, 0};
    Dim scroll_height = 0;
    std::string buffer{};
    static bool scrolling_enabled;

    void crlf();
};

class Checkbox : public Widget {
   public:
    std::function<void(Checkbox&, bool)> on_select{};

    Checkbox(Point parent_pos, size_t length, std::string text, bool small);
    Checkbox(
        Point parent_pos,
        size_t length,
        std::string text)
        : Checkbox{parent_pos, length, text, false} {
    }

    Checkbox()
        : Checkbox{{}, {}, {}} {
    }

    Checkbox(const Checkbox&) = delete;
    Checkbox(Checkbox&&) = delete;
    Checkbox& operator=(const Checkbox&) = delete;
    Checkbox& operator=(Checkbox&&) = delete;

    void set_text(const std::string value);
    bool set_value(const bool value);
    bool value() const;

    void paint(Painter& painter) override;

    bool on_key(const KeyEvent key) override;
    bool on_keyboard(const KeyboardEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    std::string text_;
    bool small_{false};
    bool value_{false};
    const Style* style_{nullptr};
};

class Button : public Widget {
   public:
    std::function<void(Button&)> on_select{};
    std::function<void(Button&)> on_touch_release{};  // Executed when releasing touch, after on_select.
    std::function<void(Button&)> on_touch_press{};    // Executed when touching, before on_select.
    std::function<bool(Button&, KeyEvent)> on_dir{};
    std::function<void(Button&)> on_highlight{};

    Button(Rect parent_rect, std::string text, bool instant_exec);  // instant_exec: Execute on_select when you touching instead of releasing
    Button(
        Rect parent_rect,
        std::string text)
        : Button{parent_rect, text, false} {
    }

    Button()
        : Button{{}, {}} {
    }

    void set_text(const std::string value);
    std::string text() const;

    void paint(Painter& painter) override;

    void on_focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    std::string text_;
    bool instant_exec_{false};
};

class ButtonWithEncoder : public Widget {
   public:
    std::function<void(ButtonWithEncoder&)> on_select{};
    std::function<void(ButtonWithEncoder&)> on_touch_release{};  // Executed when releasing touch, after on_select.
    std::function<void(ButtonWithEncoder&)> on_touch_press{};    // Executed when touching, before on_select.
    std::function<bool(ButtonWithEncoder&, KeyEvent)> on_dir{};
    std::function<void(ButtonWithEncoder&)> on_highlight{};

    ButtonWithEncoder(Rect parent_rect, std::string text, bool instant_exec);  // instant_exec: Execute on_select when you touching instead of releasing
    ButtonWithEncoder(
        Rect parent_rect,
        std::string text)
        : ButtonWithEncoder{parent_rect, text, false} {
    }

    ButtonWithEncoder()
        : ButtonWithEncoder{{}, {}} {
    }

    std::function<void()> on_change{};

    void set_text(const std::string value);
    int32_t get_encoder_delta();
    void set_encoder_delta(const int32_t delta);
    std::string text() const;

    void paint(Painter& painter) override;

    void on_focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    std::string text_;
    int32_t encoder_delta = 0;
    bool delta_change = 0;
    bool instant_exec_{false};
};

class NewButton : public Widget {
   public:
    std::function<void(void)> on_select{};
    // std::function<void(NewButton&)> on_select{};
    std::function<bool(NewButton&, KeyEvent)> on_dir{};
    std::function<void(NewButton&)> on_highlight{};

    NewButton(const NewButton&) = delete;
    NewButton& operator=(const NewButton&) = delete;
    NewButton(Rect parent_rect, std::string text, const Bitmap* bitmap);
    NewButton(Rect parent_rect, std::string text, const Bitmap* bitmap, Color color, bool vertical_center = false);
    NewButton()
        : NewButton{{}, {}, {}} {
    }

    void set_bitmap(const Bitmap* bitmap);
    void set_text(const std::string value);
    void set_color(Color value);
    void set_bg_color(Color value);
    void set_vertical_center(bool value);
    std::string text() const;
    const Bitmap* bitmap();
    ui::Color color();

    void on_focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

    void paint(Painter& painter) override;

   protected:
    virtual Style paint_style();
    Color color_;
    Color bg_color_{Theme::getInstance()->bg_medium->background};

   private:
    std::string text_;
    const Bitmap* bitmap_;
    bool vertical_center_{false};
};

class Image : public Widget {
   public:
    Image();
    Image(
        const Rect parent_rect,
        const Bitmap* bitmap,
        const Color foreground,
        const Color background);

    Image(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    void set_bitmap(const Bitmap* bitmap);
    void set_foreground(const Color color);
    void set_background(const Color color);
    void invert_colors();

    void paint(Painter& painter) override;

    const Bitmap& bitmap() & { return *bitmap_; }

   private:
    const Bitmap* bitmap_;
    Color foreground_;
    Color background_;
};

class ImageButton : public Image {
   public:
    std::function<void(ImageButton&)> on_select{};

    ImageButton(
        const Rect parent_rect,
        const Bitmap* bitmap,
        const Color foreground,
        const Color background);

    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;
};

/* A button that toggles between two images when set. */
class ImageToggle : public ImageButton {
   public:
    std::function<void(bool value)> on_change{};

    ImageToggle(
        Rect parent_rect,
        const Bitmap* bitmap_);

    ImageToggle(
        Rect parent_rect,
        const Bitmap* bitmap_,
        Color foreground_true,
        Color foreground_false,
        Color background_);

    ImageToggle(
        Rect parent_rect,
        const Bitmap* bitmap_true,
        const Bitmap* bitmap_false,
        Color foreground_true,
        Color background_true,
        Color foreground_false,
        Color background_false);

    ImageToggle(const ImageToggle&) = delete;
    ImageToggle& operator=(const ImageToggle&) = delete;

    bool value() const;
    void set_value(bool b);

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    // Hide this field.
    using ImageButton::on_select;
    const Bitmap* bitmap_true_;
    const Bitmap* bitmap_false_;
    const Color foreground_true_;
    const Color background_true_;
    const Color foreground_false_;
    const Color background_false_;
    bool value_;
};

class ImageOptionsField : public Widget {
   public:
    using value_t = int32_t;
    using option_t = std::pair<const Bitmap*, value_t>;
    using options_t = std::vector<option_t>;

    std::function<void(size_t, value_t)> on_change{};
    std::function<void(void)> on_show_options{};

    ImageOptionsField(
        Rect parent_rect,
        Color foreground,
        Color background,
        options_t options);

    ImageOptionsField()
        : ImageOptionsField{{}, Theme::getInstance()->bg_darkest->foreground, Theme::getInstance()->bg_darkest->background, {}} {
    }

    void set_options(options_t new_options);

    size_t selected_index() const;
    size_t selected_index_value() const;
    void set_selected_index(const size_t new_index);

    void set_by_value(value_t v);

    void paint(Painter& painter) override;

    void on_focus() override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;
    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    options_t options;
    size_t selected_index_{0};
    Color foreground_;
    Color background_;
};

class OptionsField : public Widget {
   public:
    using name_t = std::string;
    using value_t = int32_t;
    using option_t = std::pair<name_t, value_t>;
    using options_t = std::vector<option_t>;

    std::function<void(size_t, value_t)> on_change{};
    std::function<void(void)> on_show_options{};

    OptionsField(Point parent_pos, size_t length, options_t options, bool centered = false);

    options_t& options() { return options_; }
    const options_t& options() const { return options_; }
    void set_options(options_t new_options);

    size_t selected_index() const;
    const name_t& selected_index_name() const;
    const value_t& selected_index_value() const;
    void set_selected_index(const size_t new_index, bool trigger_change = true);

    void set_by_value(value_t v);
    void set_by_nearest_value(value_t v);

    void paint(Painter& painter) override;

    void on_focus() override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    const size_t length_;
    options_t options_;
    size_t selected_index_{0};
    bool centered_{false};  // e.g.: length as screen_width/8, x position as 0, it will be centered in x axis
};

// A TextEdit is bound to a string reference and allows the string
// to be manipulated. The field itself does not provide the UI for
// setting the value. It provides the UI of rendering the text,
// a cursor, and an API to edit the string content.
class TextEdit : public Widget {
   public:
    TextEdit(std::string& str, Point position, uint32_t length = 30)
        : TextEdit{str, 64, position, length} {}

    // Str: the string containing the content to edit.
    // Max_length: max length the string is allowed to use.
    // Position: the top-left corner of the control.
    // Length: the number of characters to display.
    //   - Characters are 8 pixels wide.
    //   - The screen can show 30 characters max.
    //   - The control is 16 pixels tall.
    TextEdit(std::string& str, size_t max_length, Point position, uint32_t length = 30);

    TextEdit(const TextEdit&) = delete;
    TextEdit(TextEdit&&) = delete;
    TextEdit& operator=(const TextEdit&) = delete;
    TextEdit& operator=(TextEdit&&) = delete;

    const std::string& value() const;

    void set_cursor(uint32_t pos);
    void set_insert_mode();
    void set_overwrite_mode();

    void char_add(char c);
    void char_delete();

    void paint(Painter& painter) override;

    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

    void on_focus() override;
    void on_blur() override;

   protected:
    std::string& text_;
    size_t max_length_;
    uint32_t char_count_;
    uint32_t cursor_pos_;
    bool insert_mode_;
};

class TextField : public Text {
   public:
    std::function<void(TextField&)> on_select{};
    std::function<void(TextField&)> on_change{};
    std::function<void(TextField&, EncoderEvent)> on_encoder_change{};

    TextField(Rect parent_rect, std::string text);

    const std::string& get_text() const;
    void set_text(std::string_view value);

    bool on_key(KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_touch(TouchEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    using Text::set;
};

class BatteryTextField : public Widget {
   public:
    std::function<void()> on_select{};

    BatteryTextField(Rect parent_rect, uint8_t percent);
    void paint(Painter& painter) override;

    void set_battery(uint8_t valid_mask, uint8_t percentage, bool charge);
    void set_text(std::string_view value);

    bool on_key(KeyEvent key) override;
    bool on_touch(TouchEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    uint8_t percent_{102};
    uint8_t valid_{0};
    bool charge_{false};

    Style style{
        .font = font::fixed_5x8(),
        .background = Theme::getInstance()->bg_dark->background,
        .foreground = Theme::getInstance()->bg_dark->foreground,
    };
};

class BatteryIcon : public Widget {
   public:
    std::function<void()> on_select{};

    BatteryIcon(Rect parent_rect, uint8_t percent);
    void paint(Painter& painter) override;
    void set_battery(uint8_t valid_mask, uint8_t percentage, bool charge);

    bool on_key(KeyEvent key) override;
    bool on_touch(TouchEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    uint8_t percent_{102};
    uint8_t valid_{0};
    bool charge_{false};
};

class NumberField : public Widget {
   public:
    std::function<void(NumberField&)> on_select{};
    std::function<void(int32_t)> on_change{};
    std::function<void(int32_t)> on_wrap{};

    using range_t = std::pair<int32_t, int32_t>;

    NumberField(Point parent_pos, int length, range_t range, int32_t step, char fill_char, bool can_loop);

    NumberField(Point parent_pos, int length, range_t range, int32_t step, char fill_char)
        : NumberField{parent_pos, length, range, step, fill_char, false} {
    }

    NumberField()
        : NumberField{{0, 0}, 1, {0, 1}, 1, ' ', false} {
    }

    NumberField(const NumberField&) = delete;
    NumberField(NumberField&&) = delete;

    int32_t value() const;
    void set_value(int32_t new_value, bool trigger_change = true);
    void set_range(const int32_t min, const int32_t max);
    void set_step(const int32_t new_step);

    void paint(Painter& painter) override;

    bool on_key(const KeyEvent key) override;
    bool on_encoder(const EncoderEvent delta) override;
    bool on_touch(const TouchEvent event) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    range_t range;
    int32_t step;
    const int length_;
    const char fill_char;
    int32_t value_{0};
    bool can_loop{};
};

/* A widget that allows for character-by-character editing of its value. */
class SymField : public Widget {
   public:
    std::function<void(SymField&)> on_change{};

    enum class Type {
        Custom,
        Oct,
        Dec,
        Hex,
        Alpha,
    };

    /* When "explicit_edits" is true, the field must be selected before it can
     * be modified. This means that "slots" are not individually highlighted.
     * This makes navigation on Views with SymFields easier because the
     * whole control can be skipped over instead of one "slot" at a time. */

    SymField(
        Point parent_pos,
        size_t length,
        Type type = Type::Dec,
        bool explicit_edits = false);

    SymField(
        Point parent_pos,
        size_t length,
        std::string symbol_list,
        bool explicit_edits = false);

    SymField(const SymField&) = delete;
    SymField(SymField&&) = delete;

    /* Gets the symbol (character) at the specified index. */
    char get_symbol(size_t index) const;

    /* Sets the symbol (character) at the specified index. */
    void set_symbol(size_t index, char symbol);

    /* Gets the symbol's offset in the symbol list at the specified index. */
    size_t get_offset(size_t index) const;

    /* Sets the symbol's offset in the symbol list at the specified index. */
    void set_offset(size_t index, size_t offset);

    /* Sets the list of allowable symbols for the field. */
    void set_symbol_list(std::string symbol_list);

    /* Sets the integer value of the field. Only works for OCT/DEC/HEX. */
    void set_value(uint64_t value);

    /* Sets the string value of the field. Characters that are not in
     * the symbol list will be set to the first symbol in the symbol list. */
    void set_value(std::string_view value);

    /* Gets the integer value of the field for OCT/DEC/HEX. */
    uint64_t to_integer() const;

    /* Gets the string value of the field. */
    const std::string& to_string() const;

    void paint(Painter& painter) override;
    bool on_key(KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_touch(TouchEvent event) override;

    void getAccessibilityText(std::string& result) override;
    void getWidgetName(std::string& result) override;

   private:
    /* Ensure the specified symbol is in the symbol list. */
    char ensure_valid(char symbol) const;

    /* Ensures all the symbols are valid. */
    void ensure_all_symbols();

    /* Sets without validation and fires on_change if necessary. */
    void set_symbol_internal(size_t index, char symbol);

    /* Gets the radix for the current Type. */
    uint8_t get_radix() const;

    std::string symbols_{};
    std::string value_{};
    Type type_ = Type::Custom;
    size_t selected_ = 0;
    bool editing_ = false;
    bool explicit_edits_ = true;
};

class Waveform : public Widget {
   public:
    Waveform(Rect parent_rect, int16_t* data, uint32_t length, uint32_t offset, bool digital, Color color);

    Waveform(const Waveform&) = delete;
    Waveform(Waveform&&) = delete;
    Waveform& operator=(const Waveform&) = delete;
    Waveform& operator=(Waveform&&) = delete;

    void set_offset(const uint32_t new_offset);
    void set_length(const uint32_t new_length);
    void set_cursor(const uint32_t i, const int16_t position);

    void paint(Painter& painter) override;

   private:
    const Color cursor_colors[2] = {Theme::getInstance()->fg_cyan->foreground, Theme::getInstance()->fg_magenta->foreground};

    int16_t* data_;
    uint32_t length_;
    uint32_t offset_;
    bool digital_{false};
    Color color_;
    int16_t cursors[2]{};
    bool show_cursors{false};
};

class VuMeter : public Widget {
   public:
    VuMeter(Rect parent_rect, uint32_t LEDs, bool show_max);

    void set_value(const uint32_t new_value);
    void set_mark(const uint32_t new_mark);

    void paint(Painter& painter) override;

   private:
    uint32_t LEDs_, LED_height{0};
    uint32_t value_{0}, prev_value{255};  // Forces painting on first display
    uint32_t split{0};
    uint16_t max{0}, prev_max{0}, hold_timer{0}, mark{0}, prev_mark{0};
    bool show_max_;
};

class OptionTabView : public View {
   public:
    OptionTabView(Rect parent_rect);

    void focus() override;

    bool is_enabled();
    void set_type(std::string type);

   protected:
    bool enabled{false};

    void set_enabled(bool value);

   private:
    Checkbox check_enable{
        {2 * 8, 0 * 16},
        20,
        "",
        false};
};

} /* namespace ui */

#endif /*__UI_WIDGET_H__*/