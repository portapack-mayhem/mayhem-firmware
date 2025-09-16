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

#include "ui_widget.hpp"
#include "ui_painter.hpp"
// #include "portapack.hpp"
#include "standalone_application.hpp"

#include <cstdint>
#include <cstddef>
#include <algorithm>

// #include "chprintf.h"
// #include "irq_controls.hpp"
#include "string_format.hpp"
// #include "usb_serial_device_to_host.h"
// #include "rtc_time.hpp"
// #include "battery.hpp"

// using namespace portapack;
// using namespace rtc_time;

namespace ui
{

    static bool ui_dirty = true;

    void dirty_set()
    {
        _api->set_dirty();
        ui_dirty = true;
    }

    void dirty_clear()
    {
        ui_dirty = false;
    }

    bool is_dirty()
    {
        return ui_dirty;
    }

    /* Widget ****************************************************************/

    const std::vector<Widget *> Widget::no_children{};

    Point Widget::screen_pos()
    {
        return screen_rect().location();
    }

    Size Widget::size() const
    {
        return _parent_rect.size();
    }

    Rect Widget::screen_rect() const
    {
        return parent() ? (parent_rect() + parent()->screen_pos()) : parent_rect();
    }

    Rect Widget::parent_rect() const
    {
        return _parent_rect;
    }

    void Widget::set_parent_rect(const Rect new_parent_rect)
    {
        _parent_rect = new_parent_rect;
        set_dirty();
    }

    Widget *Widget::parent() const
    {
        return parent_;
    }

    void Widget::set_parent(Widget *const widget)
    {
        if (widget == parent_)
        {
            return;
        }

        if (parent_ && !widget)
        {
            // We have a parent, but are losing it. Update visible status.
            dirty_overlapping_children_in_rect(screen_rect());
            visible(false);
        }

        parent_ = widget;

        set_dirty();
    }

    void Widget::set_dirty()
    {
        flags.dirty = true;
        dirty_set();
    }

    bool Widget::dirty() const
    {
        return flags.dirty;
    }

    void Widget::set_clean()
    {
        flags.dirty = false;
    }

    void Widget::hidden(bool hide)
    {
        if (hide != flags.hidden)
        {
            flags.hidden = hide;

            // If parent is hidden, either of these is a no-op.
            if (hide)
            {
                // TODO: Instead of dirtying parent entirely, dirty only children
                // that overlap with this widget.

                // parent()->dirty_overlapping_children_in_rect(parent_rect());

                /* TODO: Notify self and all non-hidden children that they're
                 * now effectively hidden?
                 */
            }
            else
            {
                set_dirty();
                /* TODO: Notify self and all non-hidden children that they're
                 * now effectively shown?
                 */
            }
        }
    }

    void Widget::focus()
    {
        context().focus_manager().set_focus_widget(this);
    }

    void Widget::on_focus()
    {
    }

    void Widget::blur()
    {
        context().focus_manager().set_focus_widget(nullptr);
    }

    void Widget::on_blur()
    {
    }

    bool Widget::focusable() const
    {
        return flags.focusable;
    }

    void Widget::set_focusable(const bool value)
    {
        flags.focusable = value;
    }

    bool Widget::has_focus()
    {
        return (context().focus_manager().focus_widget() == this);
    }

    bool Widget::on_key(const KeyEvent event)
    {
        (void)event;
        return false;
    }

    bool Widget::on_encoder(const EncoderEvent event)
    {
        (void)event;
        return false;
    }

    bool Widget::on_touch(const TouchEvent event)
    {
        (void)event;
        return false;
    }
    bool Widget::on_keyboard(const KeyboardEvent event)
    {
        (void)event;
        return false;
    }

    const std::vector<Widget *> &Widget::children() const
    {
        return no_children;
    }

    Context &Widget::context() const
    {
        if (parent_ == nullptr)
            _api->panic("parent__ is null");

        return parent()->context();
    }

    void Widget::set_style(const Style *new_style)
    {
        if (new_style != style_)
        {
            style_ = new_style;
            set_dirty();
        }
    }

    const Style &Widget::style() const
    {
        if (style_ != nullptr)
            return *style_;
        else
        {
            auto p = parent();
            if (p == nullptr)
                // TODO: debug
                while (true)
                    ;
            return p->style();
        }
    }

    void Widget::visible(bool v)
    {
        if (v != flags.visible)
        {
            flags.visible = v;

            /* TODO: This on_show/on_hide implementation seems inelegant.
             * But I need *some* way to take/configure resources when
             * a widget becomes visible, and reverse the process when the
             * widget becomes invisible, whether the widget (or parent) is
             * hidden, or the widget (or parent) is removed from the tree.
             */
            if (v)
            {
                on_show();
            }
            else
            {
                on_hide();

                // Set all children invisible too.
                for (const auto child : children())
                {
                    child->visible(false);
                }
            }
        }
    }

    bool Widget::highlighted() const
    {
        return flags.highlighted;
    }

    void Widget::set_highlighted(const bool value)
    {
        flags.highlighted = value;
    }

    void Widget::dirty_overlapping_children_in_rect(const Rect &child_rect)
    {
        for (auto child : children())
        {
            if (!child_rect.intersect(child->parent_rect()).is_empty())
            {
                child->set_dirty();
            }
        }
    }

    void Widget::getAccessibilityText(std::string &result)
    {
        result = "";
    }
    void Widget::getWidgetName(std::string &result)
    {
        result = "";
    }
    /* View ******************************************************************/

    void View::paint(Painter &painter)
    {
        painter.fill_rectangle(
            screen_rect(),
            style().background);
    }

    void View::add_child(Widget *const widget)
    {
        if (widget)
        {
            if (widget->parent() == nullptr)
            {
                widget->set_parent(this);
                children_.push_back(widget);
            }
        }
    }

    void View::add_children(const std::initializer_list<Widget *> children)
    {
        children_.insert(std::end(children_), children);
        for (auto child : children)
        {
            child->set_parent(this);
        }
    }

    void View::remove_child(Widget *const widget)
    {
        if (widget)
        {
            children_.erase(std::remove(children_.begin(), children_.end(), widget), children_.end());
            widget->set_parent(nullptr);
        }
    }

    void View::remove_children(const std::vector<Widget *> &children)
    {
        for (auto child : children)
        {
            remove_child(child);
        }
    }

    const std::vector<Widget *> &View::children() const
    {
        return children_;
    }

    std::string View::title() const
    {
        return "";
    };

    /* OptionTabView *********************************************************/

    OptionTabView::OptionTabView(Rect parent_rect)
    {
        set_parent_rect(parent_rect);

        add_child(&check_enable);
        hidden(true);

        check_enable.on_select = [this](Checkbox &, bool value)
        {
            enabled = value;
        };
    }

    void OptionTabView::set_enabled(bool value)
    {
        check_enable.set_value(value);
    }

    bool OptionTabView::is_enabled()
    {
        return check_enable.value();
    }

    void OptionTabView::set_type(std::string type)
    {
        check_enable.set_text("Transmit " + type);
    }

    void OptionTabView::focus()
    {
        check_enable.focus();
    }

    /* Rectangle *************************************************************/

    Rectangle::Rectangle(
        Color c)
        : Widget{},
          color{c}
    {
    }

    Rectangle::Rectangle(
        Rect parent_rect,
        Color c)
        : Widget{parent_rect},
          color{c}
    {
    }

    void Rectangle::set_color(const Color c)
    {
        color = c;
        set_dirty();
    }

    void Rectangle::set_outline(const bool outline)
    {
        _outline = outline;
        set_dirty();
    }

    void Rectangle::paint(Painter &painter)
    {
        if (!_outline)
        {
            painter.fill_rectangle(
                screen_rect(),
                color);
        }
        else
        {
            painter.draw_rectangle(
                screen_rect(),
                color);
        }
    }

    /* Text ******************************************************************/

    Text::Text(
        Rect parent_rect,
        std::string text)
        : Widget{parent_rect},
          text{std::move(text)}
    {
    }

    Text::Text(
        Rect parent_rect)
        : Text{parent_rect, {}}
    {
    }

    void Text::set(std::string_view value)
    {
        text = std::string{value};
        set_dirty();
    }

    void Text::getAccessibilityText(std::string &result)
    {
        result = text;
    }
    void Text::getWidgetName(std::string &result)
    {
        result = "Text";
    }
    void Text::paint(Painter &painter)
    {
        const auto rect = screen_rect();
        auto s = has_focus() ? style().invert() : style();
        auto max_len = (unsigned)rect.width() / s.font.char_width();
        auto text_view = std::string_view{text};

        painter.fill_rectangle(rect, s.background);

        if (text_view.length() > max_len)
            text_view = text_view.substr(0, max_len);

        painter.draw_string(
            rect.location(),
            s,
            text_view);
    }

    /* Labels ****************************************************************/

    Labels::Labels(
        std::initializer_list<Label> labels)
        : labels_{labels}
    {
    }

    void Labels::set_labels(std::initializer_list<Label> labels)
    {
        labels_ = labels;
        set_dirty();
    }

    void Labels::paint(Painter &painter)
    {
        for (auto &label : labels_)
        {
            painter.draw_string(
                label.pos + screen_pos(),
                style().font,
                label.color,
                style().background,
                label.text);
        }
    }

    void Labels::getAccessibilityText(std::string &result)
    {
        result = "";
        for (auto &label : labels_)
        {
            result += label.text;
            result += ", ";
        }
    }
    void Labels::getWidgetName(std::string &result)
    {
        result = "Labels";
    }

    /* LiveDateTime **********************************************************/

    void LiveDateTime::on_tick_second()
    {
        // TODO: wire standalone api: rtc_time::now(datetime);
        /*
        text = "";
        if (!hide_clock)
        {
            if (date_enabled)
            {
                text = to_string_dec_uint(datetime.year(), 4, '0') + "-" +
                       to_string_dec_uint(datetime.month(), 2, '0') + "-" +
                       to_string_dec_uint(datetime.day(), 2, '0') + " ";
            }
            else
            {
                text = "           ";
            }

            text = text + to_string_dec_uint(datetime.hour(), 2, '0') + ":" + to_string_dec_uint(datetime.minute(), 2, '0');

            if (seconds_enabled)
            {
                text += ":";

                if (init_delay == 0)
                    text += to_string_dec_uint(datetime.second(), 2, '0');
                else
                {
                    // Placeholder while the seconds are not updated
                    text += "XX";
                    init_delay--;
                }
            }
        }*/
        set_dirty();
    }

    LiveDateTime::LiveDateTime(
        Rect parent_rect)
        : Widget{parent_rect}
    {
        // TODO: wire standalone api:
        /*
        signal_token_tick_second = rtc_time::signal_tick_second += [this]()
        {
            this->on_tick_second();
        };*/
    }

    LiveDateTime::~LiveDateTime()
    {
        // TODO: wire standalone api: rtc_time::signal_tick_second -= signal_token_tick_second;
    }

    void LiveDateTime::paint(Painter &painter)
    {
        const auto rect = screen_rect();
        const auto s = style();

        on_tick_second();

        painter.fill_rectangle(rect, s.background);

        painter.draw_string(
            rect.location(),
            s,
            text);
    }
    void LiveDateTime::set_hide_clock(bool new_value)
    {
        this->hide_clock = new_value;
    }

    void LiveDateTime::set_date_enabled(bool new_value)
    {
        this->date_enabled = new_value;
    }

    void LiveDateTime::set_seconds_enabled(bool new_value)
    {
        this->seconds_enabled = new_value;
    }

    /* BigFrequency **********************************************************/

    BigFrequency::BigFrequency(
        Rect parent_rect,
        rf::Frequency frequency)
        : Widget{parent_rect},
          _frequency{frequency}
    {
    }

    void BigFrequency::set(const rf::Frequency frequency)
    {
        _frequency = frequency;
        set_dirty();
    }

    void BigFrequency::paint(Painter &painter)
    {
        uint32_t i, digit_def;
        std::array<char, 7> digits;
        char digit;
        Point digit_pos;
        ui::Color segment_color;

        const auto rect = screen_rect();

        // Erase
        painter.fill_rectangle(
            {{0, rect.location().y()}, {screen_width, 52}},
            Theme::getInstance()->bg_darkest->background);

        // Prepare digits
        if (!_frequency)
        {
            digits.fill(10); // ----.---
            digit_pos = {0, rect.location().y()};
        }
        else
        {
            _frequency /= 1000; // GMMM.KKK(uuu)

            for (i = 0; i < 7; i++)
            {
                digits[6 - i] = _frequency % 10;
                _frequency /= 10;
            }

            // Remove leading zeros
            for (i = 0; i < 3; i++)
            {
                if (!digits[i])
                    digits[i] = 16; // "Don't draw" code
                else
                    break;
            }

            digit_pos = {(Coord)(240 - ((7 * digit_width) + 8) - (i * digit_width)) / 2, rect.location().y()};
        }

        segment_color = style().foreground;

        // Draw
        for (i = 0; i < 7; i++)
        {
            digit = digits[i];

            if (digit < 16)
            {
                digit_def = segment_font[(uint8_t)digit];

                for (size_t s = 0; s < 7; s++)
                {
                    if (digit_def & 1)
                        painter.fill_rectangle({digit_pos + segments[s].location(), segments[s].size()}, segment_color);
                    digit_def >>= 1;
                }
            }

            if (i == 3)
            {
                // Dot
                painter.fill_rectangle({digit_pos + Point(34, 48), {4, 4}}, segment_color);
                digit_pos += {(digit_width + 8), 0};
            }
            else
            {
                digit_pos += {digit_width, 0};
            }
        }
    }

    /* ProgressBar ***********************************************************/

    ProgressBar::ProgressBar(
        Rect parent_rect)
        : Widget{parent_rect}
    {
    }

    void ProgressBar::set_max(const uint32_t max)
    {
        if (max == _max)
            return;

        if (_value > _max)
            _value = _max;

        _max = max;
        set_dirty();
    }

    void ProgressBar::set_value(const uint32_t value)
    {
        if (value == _value)
            return;

        if (value > _max)
            _value = _max;
        else
            _value = value;
        set_dirty();
    }

    void ProgressBar::getAccessibilityText(std::string &result)
    {
        result = to_string_dec_uint(_value) + " / " + to_string_dec_uint(_max);
    }
    void ProgressBar::getWidgetName(std::string &result)
    {
        result = "ProgressBar";
    }

    void ProgressBar::paint(Painter &painter)
    {
        int v_scaled;

        const auto sr = screen_rect();
        const auto s = style();

        v_scaled = (sr.size().width() * (uint64_t)_value) / _max;

        painter.fill_rectangle({sr.location(), {v_scaled, sr.size().height()}}, style().foreground);
        painter.fill_rectangle({{sr.location().x() + v_scaled, sr.location().y()}, {sr.size().width() - v_scaled, sr.size().height()}}, s.background);

        painter.draw_rectangle(sr, s.foreground);
    }

    /* ActivityDot ***********************************************************/

    ActivityDot::ActivityDot(
        Rect parent_rect,
        Color color)
        : Widget{parent_rect},
          _color{color} {}

    void ActivityDot::paint(Painter &painter)
    {
        painter.fill_rectangle(screen_rect(), _on ? _color : Theme::getInstance()->bg_medium->background);
    }

    void ActivityDot::toggle()
    {
        _on = !_on;
        set_dirty();
    }

    void ActivityDot::reset()
    {
        _on = false;
        set_dirty();
    }

    /* Console ***************************************************************/

    Console::Console(
        Rect parent_rect)
        : Widget{parent_rect}
    {
    }

    void Console::clear(bool clear_buffer = false)
    {
        if (clear_buffer)
            buffer.clear();

        if (!hidden() && visible())
        {
            _api->fill_rectangle(screen_rect().left(), screen_rect().top(), screen_rect().width(), screen_rect().height(), Theme::getInstance()->bg_darkest->background.v);
        }

        pos = {0, 0};
    }

    void Console::write(std::string message)
    {
        bool escape = false;

        if (!hidden() && visible())
        {
            const Style &s = style();
            const Font &font = s.font;
            auto rect = screen_rect();
            ui::Color pen_color = s.foreground;

            for (auto c : message)
            {
                if (escape)
                {
                    if (c < std::size(term_colors))
                        pen_color = term_colors[(uint8_t)c];
                    else
                        pen_color = s.foreground;
                    escape = false;
                }
                else
                {
                    if (c == '\n')
                    {
                        crlf();
                    }
                    else if (c == '\r')
                    {
                        pos = {0, pos.y()};
                    }
                    else if (c == '\x1B')
                    {
                        escape = true;
                    }
                    else
                    {
                        auto glyph = font.glyph(c);
                        auto advance = glyph.advance();
                        // Would drawing next character be off the end? Newline.
                        if ((pos.x() + advance.x()) > rect.width())
                            crlf();

                        Point pos_glyph{rect.left() + pos.x(), _api->scroll_area_y(pos.y())};
                        _api->draw_bitmap(pos_glyph.x(), pos_glyph.y(), glyph.size().width(), glyph.size().height(), glyph.pixels(), pen_color.v, s.background.v);

                        pos += {advance.x(), 0};
                    }
                }
            }
            buffer = message;
        }
        else
        {
            if (buffer.size() < 256)
                buffer += message;
        }
    }

    void Console::getAccessibilityText(std::string &result)
    {
        result = "{" + buffer + "}";
    }
    void Console::getWidgetName(std::string &result)
    {
        result = "Console";
    }

    void Console::writeln(std::string message)
    {
        write(message + "\n");
    }

    void Console::paint(Painter &)
    {
        write(buffer);
    }

    void Console::on_show()
    {
        enable_scrolling(true);
        clear();
    }

    bool Console::scrolling_enabled = false;

    void Console::enable_scrolling(bool enable)
    {
        if (enable)
        {
            auto sr = screen_rect();

            auto line_height = style().font.line_height();

            // Count full lines that can fit in console's rectangle.
            auto max_lines = sr.height() / line_height; // NB: int division to floor.

            // The scroll area must be a multiple of the line_height
            // or some lines will end up vertically truncated.
            scroll_height = max_lines * line_height;

            _api->scroll_set_area(sr.top(), sr.top() + scroll_height);
            _api->scroll_set_position(0);
            scrolling_enabled = true;
        }
        else
        {
            _api->scroll_disable();
            scrolling_enabled = false;
        }
    }

    void Console::on_hide()
    {
        /* TODO: Clear region to eliminate brief flash of content at un-shifted
         * position? */
        enable_scrolling(false);
    }

    void Console::crlf()
    {
        if (hidden() || !visible())
            return;

        const auto &s = style();
        auto sr = screen_rect();
        auto line_height = s.font.line_height();

        // Advance to the next line (\n) position and "carriage return" x to 0.
        pos = {0, pos.y() + line_height};

        if (pos.y() >= scroll_height)
        {
            // Line is past off the "bottom", need to scroll.
            if (!scrolling_enabled)
                enable_scrolling(true);

            // See the notes in lcd_ili9341.hpp about how scrolling works.
            // The gist is that VSA will be moved to scroll the "top" off the
            // screen. The drawing code uses 'scroll_area_y' to get the actual
            // screen coordinate based on VSA. The "bottom" line is *always*
            // at 'VSA + ((max_lines - 1) * line_height)' and so is constant.
            pos = {0, scroll_height - line_height};

            // Scroll off the "top" line.
            _api->scroll(-line_height);

            // Clear the new line at the "bottom".
            Rect dirty{sr.left(), _api->scroll_area_y(pos.y()), sr.width(), line_height};
            _api->fill_rectangle(dirty.left(), dirty.top(), dirty.width(), dirty.height(), s.background.v);
        }
    }

    /* Checkbox **************************************************************/

    Checkbox::Checkbox(
        Point parent_pos,
        size_t length,
        std::string text,
        bool small)
        : Widget{},
          text_{text},
          small_{small}
    {
        if (!small_)
            set_parent_rect({parent_pos, {static_cast<ui::Dim>((8 * length) + 24), 24}});
        else
            set_parent_rect({parent_pos, {static_cast<ui::Dim>((8 * length) + 16), 16}});

        set_focusable(true);
    }

    void Checkbox::set_text(const std::string value)
    {
        text_ = value;
        set_dirty();
    }

    bool Checkbox::set_value(const bool value)
    {
        value_ = value;
        set_dirty();

        if (on_select)
        {
            on_select(*this, value_);
            return true;
        }

        return false;
    }

    void Checkbox::getAccessibilityText(std::string &result)
    {
        result = text_ + ((value_) ? " checked" : " unchecked");
    }
    void Checkbox::getWidgetName(std::string &result)
    {
        result = "Checkbox";
    }

    bool Checkbox::value() const
    {
        return value_;
    }

    void Checkbox::paint(Painter &painter)
    {
        const auto r = screen_rect();

        const auto paint_style = (has_focus() || highlighted()) ? style().invert() : style();

        const auto x = r.location().x();
        const auto y = r.location().y();

        const auto label_r = paint_style.font.size_of(text_);

        if (!small_)
        {
            painter.draw_rectangle({{r.location()}, {24, 24}}, style().foreground);

            painter.fill_rectangle({x + 1, y + 1, 24 - 2, 24 - 2}, style().background);

            // Highlight
            painter.draw_rectangle({x + 2, y + 2, 24 - 4, 24 - 4}, paint_style.background);

            if (value_ == true)
            {
                // Check
                // TODO: wire standalone api: portapack::display.draw_line({x + 2, y + 14}, {x + 6, y + 18}, Theme::getInstance()->fg_green->foreground);
                // TODO: wire standalone api: portapack::display.draw_line({x + 6, y + 18}, {x + 20, y + 4}, Theme::getInstance()->fg_green->foreground);
            }
            else
            {
                // Cross
                // TODO: wire standalone api: portapack::display.draw_line({x + 1, y + 1}, {x + 24 - 2, y + 24 - 2}, Theme::getInstance()->fg_red->foreground);
                // TODO: wire standalone api: portapack::display.draw_line({x + 24 - 2, y + 1}, {x + 1, y + 24 - 2}, Theme::getInstance()->fg_red->foreground);
            }

            painter.draw_string(
                {static_cast<Coord>(x + 24 + 4),
                 static_cast<Coord>(y + (24 - label_r.height()) / 2)},
                paint_style,
                text_);
        }
        else
        {
            painter.draw_rectangle({{r.location()}, {16, 16}}, style().foreground);

            painter.fill_rectangle({x + 1, y + 1, 16 - 2, 16 - 2}, style().background);

            // Highlight
            painter.draw_rectangle({x + 1, y + 1, 16 - 2, 16 - 2}, paint_style.background);

            if (value_ == true)
            {
                // Check
                // TODO: wire standalone api: portapack::display.draw_line({x + 2, y + 8}, {x + 6, y + 12}, Theme::getInstance()->fg_green->foreground);
                // TODO: wire standalone api: portapack::display.draw_line({x + 6, y + 12}, {x + 13, y + 5}, Theme::getInstance()->fg_green->foreground);
            }
            else
            {
                // Cross
                // TODO: wire standalone api: portapack::display.draw_line({x + 1, y + 1}, {x + 16 - 2, y + 16 - 2}, Theme::getInstance()->fg_red->foreground);
                // TODO: wire standalone api: portapack::display.draw_line({x + 16 - 2, y + 1}, {x + 1, y + 16 - 2}, Theme::getInstance()->fg_red->foreground);
            }

            painter.draw_string(
                {static_cast<Coord>(x + 16 + 2),
                 static_cast<Coord>(y + (16 - label_r.height()) / 2)},
                paint_style,
                text_);
        }
    }

    bool Checkbox::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Select)
            return set_value(not value_);

        return false;
    }

    bool Checkbox::on_keyboard(const KeyboardEvent event)
    {
        if (event == 10 || event == 32)
            return set_value(not value_);
        return false;
    }

    bool Checkbox::on_touch(const TouchEvent event)
    {
        switch (event.type)
        {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            return true;

        case TouchEvent::Type::End:
            set_highlighted(false);
            value_ = not value_;
            set_dirty();
            if (on_select)
            {
                on_select(*this, value_);
            }
            return true;

        default:
            return false;
        }
    }

    /* Button ****************************************************************/

    Button::Button(
        Rect parent_rect,
        std::string text,
        bool instant_exec)
        : Widget{parent_rect},
          text_{text},
          instant_exec_{instant_exec}
    {
        set_focusable(true);
    }

    void Button::set_text(const std::string value)
    {
        text_ = value;
        set_dirty();
    }

    std::string Button::text() const
    {
        return text_;
    }

    void Button::getAccessibilityText(std::string &result)
    {
        result = text_;
    }
    void Button::getWidgetName(std::string &result)
    {
        result = "Button";
    }

    void Button::paint(Painter &painter)
    {
        Color bg, fg;
        const auto r = screen_rect();

        if (has_focus() || highlighted())
        {
            bg = style().foreground;
            fg = Theme::getInstance()->bg_darkest->background;
        }
        else
        {
            bg = Theme::getInstance()->bg_medium->background;
            fg = style().foreground;
        }

        const Style paint_style = {style().font, bg, fg};

        painter.draw_rectangle({r.location(), {r.size().width(), 1}}, Theme::getInstance()->bg_light->background);
        painter.draw_rectangle({r.location().x(), r.location().y() + r.size().height() - 1, r.size().width(), 1}, Theme::getInstance()->bg_dark->background);
        painter.draw_rectangle({r.location().x() + r.size().width() - 1, r.location().y(), 1, r.size().height()}, Theme::getInstance()->bg_dark->background);

        painter.fill_rectangle(
            {r.location().x(), r.location().y() + 1, r.size().width() - 1, r.size().height() - 2},
            paint_style.background);

        const auto label_r = paint_style.font.size_of(text_);
        painter.draw_string(
            {r.location().x() + (r.size().width() - label_r.width()) / 2, r.location().y() + (r.size().height() - label_r.height()) / 2},
            paint_style,
            text_);
    }

    void Button::on_focus()
    {
        if (on_highlight)
            on_highlight(*this);
    }

    bool Button::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Select)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }
        else
        {
            if (on_dir)
            {
                return on_dir(*this, key);
            }
        }

        return false;
    }

    bool Button::on_keyboard(const KeyboardEvent event)
    {
        if (event == 10 || event == 32)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }
        return false;
    }

    bool Button::on_touch(const TouchEvent event)
    {
        switch (event.type)
        {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            if (on_touch_press)
            {
                on_touch_press(*this);
            }
            if (on_select && instant_exec_)
            {
                on_select(*this);
            }
            return true;

        case TouchEvent::Type::End:
            set_highlighted(false);
            set_dirty();
            if (on_touch_release)
            {
                on_touch_release(*this);
            }
            if (on_select && !instant_exec_)
            {
                on_select(*this);
            }
            return true;

        default:
            return false;
        }
#if 0
	switch(event.type) {
	case TouchEvent::Type::Start:
		flags.highlighted = true;
		set_dirty();
		return true;

	case TouchEvent::Type::Move:
		{
			const bool new_highlighted = screen_rect().contains(event.point);
			if( flags.highlighted != new_highlighted ) {
				flags.highlighted = new_highlighted;
				set_dirty();
			}
		}
		return true;

	case TouchEvent::Type::End:
		if( flags.highlighted ) {
			flags.highlighted = false;
			set_dirty();
			if( on_select ) {
				on_select(*this);
			}
		}
		return true;

	default:
		return false;
	}
#endif
    }

    /* ButtonWithEncoder ****************************************************************/

    ButtonWithEncoder::ButtonWithEncoder(
        Rect parent_rect,
        std::string text,
        bool instant_exec)
        : Widget{parent_rect},
          text_{text},
          instant_exec_{instant_exec}
    {
        set_focusable(true);
    }

    void ButtonWithEncoder::set_text(const std::string value)
    {
        text_ = value;
        set_dirty();
    }
    int32_t ButtonWithEncoder::get_encoder_delta()
    {
        return encoder_delta;
    }
    void ButtonWithEncoder::set_encoder_delta(const int32_t delta)
    {
        encoder_delta = delta;
    }

    std::string ButtonWithEncoder::text() const
    {
        return text_;
    }

    void ButtonWithEncoder::getAccessibilityText(std::string &result)
    {
        result = text_;
    }
    void ButtonWithEncoder::getWidgetName(std::string &result)
    {
        result = "ButtonWithEncoder";
    }

    void ButtonWithEncoder::paint(Painter &painter)
    {
        Color bg, fg;
        const auto r = screen_rect();

        if (has_focus() || highlighted())
        {
            bg = style().foreground;
            fg = Theme::getInstance()->bg_darkest->background;
        }
        else
        {
            bg = Theme::getInstance()->bg_medium->background;
            fg = style().foreground;
        }

        const Style paint_style = {style().font, bg, fg};

        painter.draw_rectangle({r.location(), {r.size().width(), 1}}, Theme::getInstance()->fg_light->foreground);
        painter.draw_rectangle({r.location().x(), r.location().y() + r.size().height() - 1, r.size().width(), 1}, Theme::getInstance()->bg_dark->background);
        painter.draw_rectangle({r.location().x() + r.size().width() - 1, r.location().y(), 1, r.size().height()}, Theme::getInstance()->bg_dark->background);

        painter.fill_rectangle(
            {r.location().x(), r.location().y() + 1, r.size().width() - 1, r.size().height() - 2},
            paint_style.background);

        const auto label_r = paint_style.font.size_of(text_);
        painter.draw_string(
            {r.location().x() + (r.size().width() - label_r.width()) / 2, r.location().y() + (r.size().height() - label_r.height()) / 2},
            paint_style,
            text_);
    }

    void ButtonWithEncoder::on_focus()
    {
        if (on_highlight)
            on_highlight(*this);
    }

    bool ButtonWithEncoder::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Select)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }
        else
        {
            if (on_dir)
            {
                return on_dir(*this, key);
            }
        }

        return false;
    }

    bool ButtonWithEncoder::on_keyboard(const KeyboardEvent key)
    {
        if (key == 32 || key == 10)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }
        return false;
    }

    bool ButtonWithEncoder::on_touch(const TouchEvent event)
    {
        switch (event.type)
        {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            if (on_touch_press)
            {
                on_touch_press(*this);
            }
            if (on_select && instant_exec_)
            {
                on_select(*this);
            }
            return true;

        case TouchEvent::Type::End:
            set_highlighted(false);
            set_dirty();
            if (on_touch_release)
            {
                on_touch_release(*this);
            }
            if (on_select && !instant_exec_)
            {
                on_select(*this);
            }
            return true;

        default:
            return false;
        }
#if 0
	switch(event.type) {
	case TouchEvent::Type::Start:
		flags.highlighted = true;
		set_dirty();
		return true;

	case TouchEvent::Type::Move:
		{
			const bool new_highlighted = screen_rect().contains(event.point);
			if( flags.highlighted != new_highlighted ) {
				flags.highlighted = new_highlighted;
				set_dirty();
			}
		}
		return true;

	case TouchEvent::Type::End:
		if( flags.highlighted ) {
			flags.highlighted = false;
			set_dirty();
			if( on_select ) {
				on_select(*this);
			}
		}
		return true;

	default:
		return false;
	}
#endif
    }
    bool ButtonWithEncoder::on_encoder(const EncoderEvent delta)
    {
        if (delta != 0)
        {
            encoder_delta += delta;
            delta_change = true;
            on_change();
        }
        else
            delta_change = 0;
        return true;
    }

    /* NewButton ****************************************************************/

    NewButton::NewButton(
        Rect parent_rect,
        std::string text,
        const Bitmap *bitmap)
        : NewButton{parent_rect, text, bitmap, Theme::getInstance()->fg_darkcyan->foreground} {}

    NewButton::NewButton(
        Rect parent_rect,
        std::string text,
        const Bitmap *bitmap,
        Color color,
        bool vertical_center)
        : Widget{parent_rect},
          color_{color},
          text_{text},
          bitmap_{bitmap},
          vertical_center_{vertical_center}
    {
        set_focusable(true);
    }

    void NewButton::set_text(const std::string value)
    {
        text_ = value;
        set_dirty();
    }

    void NewButton::getAccessibilityText(std::string &result)
    {
        result = text_;
    }
    void NewButton::getWidgetName(std::string &result)
    {
        result = "NewButton";
    }

    std::string NewButton::text() const
    {
        return text_;
    }

    void NewButton::set_bitmap(const Bitmap *bitmap)
    {
        bitmap_ = bitmap;
        set_dirty();
    }

    const Bitmap *NewButton::bitmap()
    {
        return bitmap_;
    }

    void NewButton::set_color(Color color)
    {
        color_ = color;
        set_dirty();
    }

    void NewButton::set_bg_color(Color color)
    {
        bg_color_ = color;
        set_dirty();
    }

    void NewButton::set_vertical_center(bool value)
    {
        vertical_center_ = value;
        set_dirty();
    }

    ui::Color NewButton::color()
    {
        return color_;
    }

    void NewButton::paint(Painter &painter)
    {
        if (!bitmap_ && text_.empty())
            return;

        const auto r = screen_rect();
        const Style style = paint_style();

        painter.draw_rectangle({r.location(), {r.width(), 1}}, Theme::getInstance()->bg_light->background);
        painter.draw_rectangle({r.left(), r.top() + r.height() - 1, r.width(), 1}, Theme::getInstance()->bg_dark->background);
        painter.draw_rectangle({r.left() + r.width() - 1, r.top(), 1, r.height()}, Theme::getInstance()->bg_dark->background);

        painter.fill_rectangle(
            {r.left(), r.top() + 1, r.width() - 1, r.height() - 2},
            style.background);

        int y = r.top();
        if (bitmap_)
        {
            int offset_y = vertical_center_ ? (r.height() / 2) - (bitmap_->size.height() / 2) : 6;
            Point bmp_pos = {r.left() + (r.width() / 2) - (bitmap_->size.width() / 2), r.top() + offset_y};
            y += bitmap_->size.height() - offset_y;

            painter.draw_bitmap(
                bmp_pos,
                *bitmap_,
                color_,
                style.background);
        }

        if (!text_.empty())
        {
            const auto label_r = style.font.size_of(text_);
            painter.draw_string(
                {r.left() + (r.width() - label_r.width()) / 2, y + (r.height() - label_r.height()) / 2},
                style,
                text_);
        }
    }

    Style NewButton::paint_style()
    {
        MutableStyle s{style()};

        if (has_focus() || highlighted())
        {
            s.background = style().foreground;
            s.foreground = Theme::getInstance()->bg_darkest->background;
        }
        else
        {
            s.background = bg_color_;
            s.foreground = style().foreground;
        }

        return s;
    }

    void NewButton::on_focus()
    {
        if (on_highlight)
            on_highlight(*this);
    }

    bool NewButton::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Select)
        {
            if (on_select)
            {
                on_select();
                return true;
            }
        }
        else
        {
            if (on_dir)
            {
                return on_dir(*this, key);
            }
        }

        return false;
    }

    bool NewButton::on_keyboard(const KeyboardEvent key)
    {
        if (key == 32 || key == 10)
        {
            if (on_select)
            {
                on_select();
                return true;
            }
        }
        return false;
    }

    bool NewButton::on_touch(const TouchEvent event)
    {
        switch (event.type)
        {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            return true;

        case TouchEvent::Type::End:
            set_highlighted(false);
            set_dirty();
            if (on_select)
            {
                on_select();
            }
            return true;

        default:
            return false;
        }
    }

    /* Image *****************************************************************/

    Image::Image()
        : Image{{}, nullptr, Theme::getInstance()->bg_darkest->foreground, Theme::getInstance()->bg_darkest->background}
    {
    }

    Image::Image(
        const Rect parent_rect,
        const Bitmap *bitmap,
        const Color foreground,
        const Color background)
        : Widget{parent_rect},
          bitmap_{bitmap},
          foreground_{foreground},
          background_{background}
    {
    }

    void Image::set_bitmap(const Bitmap *bitmap)
    {
        bitmap_ = bitmap;
        set_dirty();
    }

    void Image::set_foreground(const Color color)
    {
        foreground_ = color;
        set_dirty();
    }

    void Image::set_background(const Color color)
    {
        background_ = color;
        set_dirty();
    }

    void Image::invert_colors()
    {
        Color temp;

        temp = background_;
        background_ = foreground_;
        foreground_ = temp;
        set_dirty();
    }

    void Image::paint(Painter &painter)
    {
        if (bitmap_)
        {
            // Code also handles ImageButton behavior.
            const bool selected = (has_focus() || highlighted());
            painter.draw_bitmap(
                screen_pos(),
                *bitmap_,
                selected ? background_ : foreground_,
                selected ? foreground_ : background_);
        }
    }

    /* ImageButton ***********************************************************/

    // TODO: Virtually all this code is duplicated from Button. Base class?

    ImageButton::ImageButton(
        const Rect parent_rect,
        const Bitmap *bitmap,
        const Color foreground,
        const Color background)
        : Image{parent_rect, bitmap, foreground, background}
    {
        set_focusable(true);
    }

    void ImageButton::getAccessibilityText(std::string &result)
    {
        result = "image";
    }
    void ImageButton::getWidgetName(std::string &result)
    {
        result = "ImageButton";
    }

    bool ImageButton::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Select)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }

        return false;
    }

    bool ImageButton::on_keyboard(const KeyboardEvent key)
    {
        if (key == 32 || key == 10)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }
        return false;
    }

    bool ImageButton::on_touch(const TouchEvent event)
    {
        switch (event.type)
        {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            return true;

        case TouchEvent::Type::End:
            set_highlighted(false);
            set_dirty();
            if (on_select)
            {
                on_select(*this);
            }
            return true;

        default:
            return false;
        }
    }

    /* ImageToggle ***********************************************************/
    ImageToggle::ImageToggle(
        Rect parent_rect,
        const Bitmap *bitmap_)
        : ImageToggle{parent_rect,
                      bitmap_,
                      Theme::getInstance()->fg_green->foreground,
                      Theme::getInstance()->fg_light->foreground,
                      Theme::getInstance()->bg_dark->background} {}

    ImageToggle::ImageToggle(
        Rect parent_rect,
        const Bitmap *bitmap_,
        Color foreground_true,
        Color foreground_false,
        Color background_)
        : ImageToggle{parent_rect,
                      bitmap_,
                      bitmap_,
                      foreground_true,
                      background_,
                      foreground_false,
                      background_} {}

    ImageToggle::ImageToggle(
        Rect parent_rect,
        const Bitmap *bitmap_true,
        const Bitmap *bitmap_false,
        Color foreground_true,
        Color background_true,
        Color foreground_false,
        Color background_false)
        : ImageButton{parent_rect, bitmap_false, foreground_false, background_false},
          bitmap_true_{bitmap_true},
          bitmap_false_{bitmap_false},
          foreground_true_{foreground_true},
          background_true_{background_true},
          foreground_false_{foreground_false},
          background_false_{background_false},
          value_{false}
    {
        ImageButton::on_select = [this](ImageButton &)
        {
            set_value(!value());
        };
    }

    bool ImageToggle::value() const
    {
        return value_;
    }

    void ImageToggle::getAccessibilityText(std::string &result)
    {
        result = value_ ? "checked" : "unchecked";
    }
    void ImageToggle::getWidgetName(std::string &result)
    {
        result = "ImageToggle";
    }

    void ImageToggle::set_value(bool b)
    {
        if (b == value_)
            return;

        value_ = b;
        set_bitmap(b ? bitmap_true_ : bitmap_false_);
        set_foreground(b ? foreground_true_ : foreground_false_);
        set_background(b ? background_true_ : background_false_);

        if (on_change)
            on_change(b);
    }

    /* ImageOptionsField *****************************************************/

    ImageOptionsField::ImageOptionsField(
        Rect parent_rect,
        Color foreground,
        Color background,
        options_t options)
        : Widget{parent_rect},
          options{std::move(options)},
          foreground_{foreground},
          background_{background}
    {
        set_focusable(true);
    }

    size_t ImageOptionsField::selected_index() const
    {
        return selected_index_;
    }

    size_t ImageOptionsField::selected_index_value() const
    {
        return options[selected_index_].second;
    }

    void ImageOptionsField::getAccessibilityText(std::string &result)
    {
        result = "selected index: " + to_string_dec_uint(selected_index_);
    }
    void ImageOptionsField::getWidgetName(std::string &result)
    {
        result = "ImageOptionsField";
    }

    void ImageOptionsField::set_selected_index(const size_t new_index)
    {
        if (new_index < options.size())
        {
            if (new_index != selected_index())
            {
                selected_index_ = new_index;
                if (on_change)
                {
                    on_change(selected_index(), options[selected_index()].second);
                }
                set_dirty();
            }
        }
    }

    void ImageOptionsField::set_by_value(value_t v)
    {
        size_t new_index = 0;
        for (const auto &option : options)
        {
            if (option.second == v)
            {
                set_selected_index(new_index);
                return;
            }

            new_index++;
        }

        // No exact match was found, default to 0.
        set_selected_index(0);
    }

    void ImageOptionsField::set_options(options_t new_options)
    {
        options = std::move(new_options);

        // Set an invalid index to force on_change.
        selected_index_ = (size_t)-1;
        set_selected_index(0);
        set_dirty();
    }

    void ImageOptionsField::paint(Painter &painter)
    {
        const bool selected = (has_focus() || highlighted());
        const auto paint_style = selected ? style().invert() : style();

        painter.draw_rectangle(
            {screen_rect().location(), {screen_rect().size().width() + 4, screen_rect().size().height() + 4}},
            paint_style.background);

        painter.draw_bitmap(
            {screen_pos().x() + 2, screen_pos().y() + 2},
            *options[selected_index_].first,
            foreground_,
            background_);
    }

    void ImageOptionsField::on_focus()
    {
        if (on_show_options)
        {
            on_show_options();
        }
    }

    bool ImageOptionsField::on_encoder(const EncoderEvent delta)
    {
        set_selected_index(selected_index() + delta);
        return true;
    }

    bool ImageOptionsField::on_keyboard(const KeyboardEvent key)
    {
        if (key == '+' || key == ' ' || key == 10)
            return on_encoder(1);
        if (key == '-' || key == 8)
            return on_encoder(-1);
        return false;
    }

    bool ImageOptionsField::on_touch(const TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
        {
            focus();
        }
        return true;
    }

    /* OptionsField **********************************************************/

    OptionsField::OptionsField(
        Point parent_pos,
        size_t length,
        options_t options,
        bool centered)
        : Widget{{parent_pos, {8 * (int)length, 16}}},
          length_{length},
          options_{std::move(options)},
          centered_{centered}
    {
        set_focusable(true);
    }

    size_t OptionsField::selected_index() const
    {
        return selected_index_;
    }

    const OptionsField::name_t &OptionsField::selected_index_name() const
    {
        return options_[selected_index_].first;
    }

    const OptionsField::value_t &OptionsField::selected_index_value() const
    {
        return options_[selected_index_].second;
    }

    void OptionsField::getAccessibilityText(std::string &result)
    {
        result = "options: ";
        bool first = true;
        for (const auto &option : options_)
        {
            if (!first)
                result += " ,";
            first = false;
            result += option.first;
        }
        result += "; selected: " + selected_index_name();
    }
    void OptionsField::getWidgetName(std::string &result)
    {
        result = "OptionsField";
    }

    void OptionsField::set_selected_index(const size_t new_index, bool trigger_change)
    {
        if (new_index < options_.size())
        {
            if (new_index != selected_index() || trigger_change)
            {
                selected_index_ = new_index;
                if (on_change)
                {
                    on_change(selected_index(), options_[selected_index()].second);
                }
                set_dirty();
            }
        }
    }

    void OptionsField::set_by_value(value_t v)
    {
        size_t new_index = 0;
        for (const auto &option : options_)
        {
            if (option.second == v)
            {
                set_selected_index(new_index);
                return;
            }
            new_index++;
        }

        // No exact match was found, default to 0.
        set_selected_index(0);
    }

    void OptionsField::set_by_nearest_value(value_t v)
    {
        size_t new_index = 0;
        size_t curr_index = 0;
        int32_t min_diff = INT32_MAX;

        for (const auto &option : options_)
        {
            auto diff = abs(v - option.second);
            if (diff < min_diff)
            {
                min_diff = diff;
                new_index = curr_index;
            }

            curr_index++;
        }

        set_selected_index(new_index);
    }

    void OptionsField::set_options(options_t new_options)
    {
        options_ = std::move(new_options);

        // Set an invalid index to force on_change.
        selected_index_ = (size_t)-1;
        set_selected_index(0);
        set_dirty();
    }

    void OptionsField::paint(Painter &painter)
    {
        const auto paint_style = has_focus() ? style().invert() : style();

        painter.fill_rectangle({screen_rect().location(), {(int)length_ * 8, 16}}, Theme::getInstance()->bg_darkest->background);

        if (selected_index() < options_.size())
        {
            std::string_view temp = selected_index_name();
            if (temp.length() > length_)
                temp = temp.substr(0, length_);

            Point draw_pos = screen_pos();
            if (centered_)
            {
                // 8 is because big font width is 8px
                // type is from: struct Point : constexpr int x() const
                int text_width = temp.length() * 8;
                int available_width = length_ * 8;
                int x_offset = (available_width - text_width) / 2;
                draw_pos = {draw_pos.x() + x_offset, draw_pos.y()};
            }

            painter.draw_string(
                draw_pos,
                paint_style,
                temp);
        }
    }

    void OptionsField::on_focus()
    {
        if (on_show_options)
        {
            on_show_options();
        }
    }

    bool OptionsField::on_encoder(const EncoderEvent delta)
    {
        int32_t new_value = selected_index() + delta;
        if (new_value < 0)
            new_value = options_.size() - 1;
        else if ((size_t)new_value >= options_.size())
            new_value = 0;

        set_selected_index(new_value);
        return true;
    }
    bool OptionsField::on_keyboard(const KeyboardEvent key)
    {
        if (key == '+' || key == ' ' || key == 10)
            return on_encoder(1);
        if (key == '-' || key == 8)
            return on_encoder(-1);
        return false;
    }

    bool OptionsField::on_touch(const TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
        {
            focus();
        }
        return true;
    }

    /* TextEdit ***********************************************************/

    TextEdit::TextEdit(
        std::string &str,
        size_t max_length,
        Point position,
        uint32_t length)
        : Widget{{position, {8 * static_cast<int>(length), 16}}},
          text_{str},
          max_length_{std::max<size_t>(max_length, str.length())},
          char_count_{std::max<uint32_t>(length, 1)},
          cursor_pos_{text_.length()},
          insert_mode_{true}
    {
        set_focusable(true);
    }

    const std::string &TextEdit::value() const
    {
        return text_;
    }

    void TextEdit::getAccessibilityText(std::string &result)
    {
        result = text_;
    }
    void TextEdit::getWidgetName(std::string &result)
    {
        result = "TextEdit";
    }

    void TextEdit::set_cursor(uint32_t pos)
    {
        cursor_pos_ = std::min<size_t>(pos, text_.length());
        set_dirty();
    }

    void TextEdit::set_insert_mode()
    {
        insert_mode_ = true;
    }

    void TextEdit::set_overwrite_mode()
    {
        insert_mode_ = false;
    }

    void TextEdit::char_add(char c)
    {
        // Don't add if inserting and at max_length and
        // don't overwrite if past the end of the text.
        if ((text_.length() >= max_length_ && insert_mode_) ||
            (cursor_pos_ >= text_.length() && !insert_mode_))
            return;

        if (insert_mode_)
            text_.insert(cursor_pos_, 1, c);
        else
            text_[cursor_pos_] = c;

        cursor_pos_++;
        set_dirty();
    }

    void TextEdit::char_delete()
    {
        if (cursor_pos_ == 0)
            return;

        cursor_pos_--;
        text_.erase(cursor_pos_, 1);
        set_dirty();
    }

    void TextEdit::paint(Painter &painter)
    {
        auto rect = screen_rect();
        auto text_style = has_focus() ? style().invert() : style();
        auto offset = 0;

        // Does the string need to be shifted?
        if (cursor_pos_ >= char_count_)
            offset = cursor_pos_ - char_count_ + 1;

        // Draw the text starting at the offset.
        for (uint32_t i = 0; i < char_count_; i++)
        {
            // Using draw_char to blank the rest of the line with spaces produces less flicker.
            auto c = (i + offset < text_.length()) ? text_[i + offset] : ' ';

            painter.draw_char(
                {rect.location().x() + (static_cast<int>(i) * char_width), rect.location().y()},
                text_style, c);
        }

        // Determine cursor position on screen (either the cursor position or the last char).
        int32_t cursor_x = char_width * (offset > 0 ? char_count_ - 1 : cursor_pos_);
        Point cursor_point{screen_pos().x() + cursor_x, screen_pos().y()};
        auto cursor_style = text_style.invert();

        // Invert the cursor character when in overwrite mode.
        if (!insert_mode_ && (cursor_pos_) < text_.length())
            painter.draw_char(cursor_point, cursor_style, text_[cursor_pos_]);

        // Draw the cursor.
        Rect cursor_box{cursor_point, {char_width, char_height}};
        painter.draw_rectangle(cursor_box, cursor_style.background);
    }

    bool TextEdit::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Left && cursor_pos_ > 0)
            cursor_pos_--;
        else if (key == KeyEvent::Right && cursor_pos_ < text_.length())
            cursor_pos_++;
        else if (key == KeyEvent::Select)
        {
            if (key_is_long_pressed(key))
            {
                // Delete text to the cursor.
                text_ = text_.substr(cursor_pos_);
                set_cursor(0);
            }
            else
            {
                insert_mode_ = !insert_mode_;
            }
        }
        else
            return false;

        set_dirty();
        return true;
    }

    bool TextEdit::on_keyboard(const KeyboardEvent key)
    {
        // if ascii printable
        if (key >= 0x20 && key <= 0x7e)
        {
            char_add(key);
            return true;
        }
        if (key == 8)
        {
            char_delete();
            return true;
        }
        return false;
    }

    bool TextEdit::on_encoder(const EncoderEvent delta)
    {
        int32_t new_pos = cursor_pos_ + delta;

        // Let the encoder wrap around the ends of the text.
        if (new_pos < 0)
            new_pos = text_.length();
        else if (static_cast<size_t>(new_pos) > text_.length())
            new_pos = 0;

        set_cursor(new_pos);
        return true;
    }

    bool TextEdit::on_touch(const TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
            focus();

        set_dirty();
        return true;
    }

    void TextEdit::on_focus()
    {
        // Enable long press on "Select".
        // TODO: wire standalone api: SwitchesState config;
        // TODO: wire standalone api: config[toUType(Switch::Sel)] = true;
        // TODO: wire standalone api: set_switches_long_press_config(config);
    }

    void TextEdit::on_blur()
    {
        // Reset long press.
        // TODO: wire standalone api: SwitchesState config{};
        // TODO: wire standalone api: set_switches_long_press_config(config);
    }

    /* TextField *************************************************************/

    TextField::TextField(Rect parent_rect, std::string text)
        : Text(parent_rect, std::move(text))
    {
        set_focusable(true);
    }

    const std::string &TextField::get_text() const
    {
        return text;
    }

    void TextField::getAccessibilityText(std::string &result)
    {
        result = text;
    }
    void TextField::getWidgetName(std::string &result)
    {
        result = "TextField";
    }

    void TextField::set_text(std::string_view value)
    {
        set(value);
        if (on_change)
            on_change(*this);
    }

    bool TextField::on_key(KeyEvent key)
    {
        if (key == KeyEvent::Select && on_select)
        {
            on_select(*this);
            return true;
        }

        return false;
    }

    bool TextField::on_encoder(EncoderEvent delta)
    {
        if (on_encoder_change)
        {
            on_encoder_change(*this, delta);
            return true;
        }

        return false;
    }

    bool TextField::on_touch(TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
        {
            focus();
            return true;
        }

        return false;
    }

    /* BatteryIcon *************************************************************/

    BatteryIcon::BatteryIcon(Rect parent_rect, uint8_t percent)
        : Widget(parent_rect)
    {
        this->set_battery(percent <= 100 ? 1 : 0, percent, false);
        set_focusable(true);
    }

    void BatteryIcon::getAccessibilityText(std::string &result)
    {
        result = to_string_dec_uint(percent_) + "%";
    }
    void BatteryIcon::getWidgetName(std::string &result)
    {
        result = "Battery percent";
    }

    void BatteryIcon::set_battery(uint8_t valid_mask, uint8_t percentage, bool charge)
    {
        if (charge == charge_ && percent_ == percentage && valid_ == valid_mask)
            return;
        percent_ = percentage;
        charge_ = charge;
        valid_ = valid_mask;
        // TODO: wire standalone api: if ((valid_mask & battery::BatteryManagement::BATT_VALID_VOLTAGE) != battery::BatteryManagement::BATT_VALID_VOLTAGE) percent_ = 102; // to indicate error
        set_dirty();
    }

    bool BatteryIcon::on_key(KeyEvent key)
    {
        if (key == KeyEvent::Select && on_select)
        {
            on_select();
            return true;
        }
        return false;
    }

    bool BatteryIcon::on_touch(TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
        {
            focus();
            return true;
        }
        if (event.type == TouchEvent::Type::End && on_select)
        {
            on_select();
            return true;
        }
        return false;
    }
    void BatteryIcon::paint(Painter &painter)
    {
        ui::Rect rect = screen_rect();                                                                                                                       // 10, 1 * 16
        painter.fill_rectangle(rect, has_focus() || highlighted() ? Theme::getInstance()->fg_light->foreground : Theme::getInstance()->bg_dark->background); // clear
        ui::Color battColor = (charge_) ? Theme::getInstance()->fg_blue->foreground : Theme::getInstance()->fg_green->foreground;
        // batt body:
        painter.draw_vline({rect.left() + 1, rect.top() + 2}, rect.height() - 4, battColor);
        painter.draw_vline({rect.right() - 2, rect.top() + 2}, rect.height() - 4, battColor);
        painter.draw_hline({rect.left() + 1, rect.top() + 2}, rect.width() - 2, battColor);
        painter.draw_hline({rect.left() + 1, rect.bottom() - 2}, rect.width() - 2, battColor);
        // batt cap:
        painter.draw_hline({rect.left() + 3, rect.top() + 1}, rect.width() - 6, battColor);
        painter.draw_hline({rect.left() + 3, 0}, rect.width() - 6, battColor);
        if (percent_ > 100)
        { // error / unk
            painter.draw_string({rect.left() + 2, rect.top() + 3}, font::fixed_5x8(), Theme::getInstance()->bg_dark->foreground, Theme::getInstance()->bg_dark->background, "?");
            return;
        }
        int8_t ppx = (rect.bottom() - 3) - (rect.top() + 2);                               // 11px max height to draw bars
        int8_t ptd = (int8_t)((static_cast<float>(percent_) / 100.0f) * (float)ppx + 0.5); // pixels to draw
        int8_t pp = ppx - ptd;                                                             // pixels to start from

        if (percent_ >= 70)
            battColor = Theme::getInstance()->fg_green->foreground;
        else if (percent_ >= 40)
            battColor = Theme::getInstance()->fg_orange->foreground;
        else
            battColor = Theme::getInstance()->fg_red->foreground;
        // fill the bars
        for (int y = pp; y < ppx; y++)
        {
            painter.draw_hline({rect.left() + 2, rect.top() + 3 + y}, rect.width() - 4, battColor);
        }
    }

    /* BatteryTextField *************************************************************/

    BatteryTextField::BatteryTextField(Rect parent_rect, uint8_t percent)
        : Widget(parent_rect)
    {
        this->set_battery(percent <= 100 ? 1 : 0, percent, false);
        set_focusable(true);
    }

    void BatteryTextField::paint(Painter &painter)
    {
        Color bg = has_focus() || highlighted() ? Theme::getInstance()->fg_light->foreground : Theme::getInstance()->bg_dark->background;
        ui::Rect rect = screen_rect();    // 2 * 8, 1 * 16
        painter.fill_rectangle(rect, bg); // clear
        std::string txt_batt = percent_ <= 100 ? to_string_dec_uint(percent_) : "UNK";
        int xdelta = 0;
        if (txt_batt.length() == 1)
            xdelta = 5;
        else if (txt_batt.length() == 2)
            xdelta = 2;
        painter.draw_string({rect.left() + xdelta, rect.top()}, font::fixed_5x8(), Theme::getInstance()->bg_dark->foreground, bg, txt_batt);
        painter.draw_string({rect.left(), rect.top() + 8}, font::fixed_5x8(), Theme::getInstance()->bg_dark->foreground, bg, (charge_) ? "+%" : " %");
    }

    void BatteryTextField::getAccessibilityText(std::string &result)
    {
        result = to_string_dec_uint(percent_) + "%";
    }
    void BatteryTextField::getWidgetName(std::string &result)
    {
        result = "Battery percent";
    }

    void BatteryTextField::set_battery(uint8_t valid_mask, uint8_t percentage, bool charge)
    {
        if (charge == charge_ && percent_ == percentage && valid_ == valid_mask)
            return;
        charge_ = charge;
        percent_ = percentage;
        valid_ = valid_mask;
        // TODO: wire standalone api: if ((valid_mask & battery::BatteryManagement::BATT_VALID_VOLTAGE) != battery::BatteryManagement::BATT_VALID_VOLTAGE) percent_ = 102; // to indicate error
        set_dirty();
    }

    bool BatteryTextField::on_key(KeyEvent key)
    {
        if (key == KeyEvent::Select && on_select)
        {
            on_select();
            return true;
        }
        return false;
    }

    bool BatteryTextField::on_touch(TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
        {
            focus();
            return true;
        }
        if (event.type == TouchEvent::Type::End && on_select)
        {
            on_select();
            return true;
        }
        return false;
    }

    /* NumberField ***********************************************************/

    NumberField::NumberField(
        Point parent_pos,
        int length,
        range_t range,
        int32_t step,
        char fill_char,
        bool can_loop)
        : Widget{{parent_pos, {8 * length, 16}}},
          range{range},
          step{step},
          length_{length},
          fill_char{fill_char},
          can_loop{can_loop}
    {
        set_focusable(true);
    }

    int32_t NumberField::value() const
    {
        return value_;
    }

    void NumberField::getAccessibilityText(std::string &result)
    {
        result = to_string_dec_int(value_);
    }
    void NumberField::getWidgetName(std::string &result)
    {
        result = "NumberField";
    }

    void NumberField::set_value(int32_t new_value, bool trigger_change)
    {
        if (can_loop)
        {
            if (new_value >= range.first)
                new_value = new_value % (range.second + 1);
            else
                new_value = range.second + new_value + 1;
        }

        new_value = clip(new_value, range.first, range.second);

        if (new_value != value())
        {
            value_ = new_value;
            if (on_change && trigger_change)
            {
                on_change(value_);
            }
            set_dirty();
        }
    }

    void NumberField::set_range(const int32_t min, const int32_t max)
    {
        range.first = min;
        range.second = max;
        set_value(value(), false);
    }

    void NumberField::set_step(const int32_t new_step)
    {
        step = new_step;
    }

    void NumberField::paint(Painter &painter)
    {
        const auto text = to_string_dec_int(value_, length_, fill_char);

        const auto paint_style = has_focus() ? style().invert() : style();

        painter.draw_string(
            screen_pos(),
            paint_style,
            text);
    }

    bool NumberField::on_key(const KeyEvent key)
    {
        if (key == KeyEvent::Select)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }

        return false;
    }

    bool NumberField::on_encoder(const EncoderEvent delta)
    {
        int32_t old_value = value();
        set_value(value() + (delta * step));

        if (on_wrap)
        {
            if ((delta > 0) && (value() < old_value))
                on_wrap(1);
            else if ((delta < 0) && (value() > old_value))
                on_wrap(-1);
        }
        return true;
    }

    bool NumberField::on_keyboard(const KeyboardEvent key)
    {
        if (key == 10)
        {
            if (on_select)
            {
                on_select(*this);
                return true;
            }
        }
        if (key == '+' || key == ' ')
        {
            return on_encoder(1);
        }
        if (key == '-' || key == 8)
        {
            return on_encoder(-1);
        }
        return false;
    }

    bool NumberField::on_touch(const TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
        {
            focus();
        }
        return true;
    }

    /* SymField **************************************************************/

    SymField::SymField(
        Point parent_pos,
        size_t length,
        Type type,
        bool explicit_edits)
        : Widget{{parent_pos, {char_width * (int)length, 16}}},
          type_{type},
          explicit_edits_{explicit_edits}
    {
        if (length == 0)
            length = 1;

        selected_ = length - 1;
        value_.resize(length);

        switch (type)
        {
        case Type::Oct:
            set_symbol_list("01234567");
            break;

        case Type::Dec:
            set_symbol_list("0123456789");
            break;

        case Type::Hex:
            set_symbol_list("0123456789ABCDEF");
            break;

        case Type::Alpha:
            set_symbol_list(" 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
            break;

        default:
            set_symbol_list("01");
            break;
        }

        set_focusable(true);
    }

    SymField::SymField(
        Point parent_pos,
        size_t length,
        std::string symbol_list,
        bool explicit_edits)
        : SymField{parent_pos, length, Type::Custom, explicit_edits}
    {
        set_symbol_list(std::move(symbol_list));
    }

    char SymField::get_symbol(size_t index) const
    {
        if (index >= value_.length())
            return 0;

        return value_[index];
    }

    void SymField::set_symbol(size_t index, char symbol)
    {
        if (index >= value_.length())
            return;

        set_symbol_internal(index, ensure_valid(symbol));
    }

    size_t SymField::get_offset(size_t index) const
    {
        if (index >= value_.length())
            return 0;

        // NB: Linear search - symbol lists are small.
        return symbols_.find(value_[index]);
    }

    void SymField::set_offset(size_t index, size_t offset)
    {
        if (index >= value_.length() || offset >= symbols_.length())
            return;

        set_symbol_internal(index, symbols_[offset]);
    }

    void SymField::set_symbol_list(std::string symbol_list)
    {
        if (symbol_list.length() == 0)
            return;

        symbols_ = std::move(symbol_list);
        ensure_all_symbols();
    }

    void SymField::set_value(uint64_t value)
    {
        auto v = value;
        uint8_t radix = get_radix();

        for (int i = value_.length() - 1; i >= 0; --i)
        {
            uint8_t temp = v % radix;
            value_[i] = uint_to_char(temp, radix);
            v /= radix;
        }

        if (on_change)
            on_change(*this);
    }

    void SymField::set_value(std::string_view value)
    {
        // Is new value too long?
        // TODO: Truncate instead? Which end?
        if (value.length() > value_.length())
            return;

        // Right-align string in field.
        auto left_padding = value_.length() - value.length();
        value_ = std::string(static_cast<size_t>(left_padding), '\0') + std::string{value};
        ensure_all_symbols();
    }

    uint64_t SymField::to_integer() const
    {
        uint64_t v = 0;
        uint64_t mul = 1;
        uint8_t radix = get_radix();

        for (int i = value_.length() - 1; i >= 0; --i)
        {
            auto temp = char_to_uint(value_[i], radix);
            v += temp * mul;
            mul *= radix;
        }

        return v;
    }

    const std::string &SymField::to_string() const
    {
        return value_;
    }

    void SymField::getAccessibilityText(std::string &result)
    {
        result = value_;
    }
    void SymField::getWidgetName(std::string &result)
    {
        result = "SymField";
    }
    void SymField::paint(Painter &painter)
    {
        Point p = screen_pos();

        for (size_t n = 0; n < value_.length(); n++)
        {
            auto c = value_[n];
            MutableStyle paint_style{style()};

            // Only highlight while focused.
            if (has_focus())
            {
                if (explicit_edits_)
                {
                    // Invert the whole field on focus if explicit edits is enabled.
                    paint_style.invert();
                }
                else if (n == selected_)
                {
                    // Otherwise only highlight the selected symbol.
                    paint_style.invert();
                }

                if (editing_ && n == selected_)
                {
                    // Use 'bg_blue' style to indicate in editing mode.
                    paint_style.foreground = Theme::getInstance()->bg_darkest->foreground;
                    paint_style.background = Theme::getInstance()->fg_blue->foreground;
                }
            }

            painter.draw_char(p, paint_style, c);
            p += {8, 0};
        }
    }

    bool SymField::on_key(KeyEvent key)
    {
        // If explicit edits are enabled, only Select is handled when not in edit mode.
        if (explicit_edits_ && !editing_)
        {
            switch (key)
            {
            case KeyEvent::Select:
                editing_ = true;
                set_dirty();
                return true;

            default:
                return false;
            }
        }

        switch (key)
        {
        case KeyEvent::Select:
            editing_ = !editing_;
            set_dirty();
            return true;

        case KeyEvent::Left:
            if (selected_ > 0)
            {
                selected_--;
                set_dirty();
                return true;
            }
            break;

        case KeyEvent::Right:
            if (selected_ < (value_.length() - 1))
            {
                selected_++;
                set_dirty();
                return true;
            }
            break;

        case KeyEvent::Up:
            if (editing_)
            {
                on_encoder(1);
                return true;
            }
            break;

        case KeyEvent::Down:
            if (editing_)
            {
                on_encoder(-1);
                return true;
            }
            break;

        default:
            break;
        }

        return false;
    }

    bool SymField::on_encoder(EncoderEvent delta)
    {
        if (explicit_edits_ && !editing_)
            return false;

        // TODO: Wrapping or carrying might be nice.
        int offset = get_offset(selected_) + delta;

        offset = clip<int>(offset, 0, symbols_.length() - 1);
        set_offset(selected_, offset);

        return true;
    }

    bool SymField::on_touch(TouchEvent event)
    {
        if (event.type == TouchEvent::Type::Start)
            focus();

        return true;
    }

    char SymField::ensure_valid(char symbol) const
    {
        // NB: Linear search - symbol lists are small.
        auto pos = symbols_.find(symbol);
        return pos != std::string::npos ? symbol : symbols_[0];
    }

    void SymField::ensure_all_symbols()
    {
        auto temp = value_;

        for (auto &c : value_)
            c = ensure_valid(c);

        if (temp != value_)
        {
            if (on_change)
                on_change(*this);
            set_dirty();
        }
    }

    void SymField::set_symbol_internal(size_t index, char symbol)
    {
        if (value_[index] == symbol)
            return;

        value_[index] = symbol;
        if (on_change)
            on_change(*this);
        set_dirty();
    }

    uint8_t SymField::get_radix() const
    {
        switch (type_)
        {
        case Type::Oct:
            return 8;
        case Type::Dec:
            return 10;
        case Type::Hex:
            return 16;
        default:
            return 0;
        }
    }

    /* Waveform **************************************************************/

    Waveform::Waveform(
        Rect parent_rect,
        int16_t *data,
        uint32_t length,
        uint32_t offset,
        bool digital,
        Color color)
        : Widget{parent_rect},
          data_{data},
          length_{length},
          offset_{offset},
          digital_{digital},
          color_{color}
    {
        // set_focusable(false);
        // previous_data.resize(length_, 0);
    }

    void Waveform::set_cursor(const uint32_t i, const int16_t position)
    {
        if (i < 2)
        {
            if (position != cursors[i])
            {
                cursors[i] = position;
                set_dirty();
            }
            show_cursors = true;
        }
    }

    void Waveform::set_offset(const uint32_t new_offset)
    {
        if (new_offset != offset_)
        {
            offset_ = new_offset;
            set_dirty();
        }
    }

    void Waveform::set_length(const uint32_t new_length)
    {
        if (new_length != length_)
        {
            length_ = new_length;
            set_dirty();
        }
    }

    void Waveform::paint(Painter &painter)
    {
        // previously it's upside down , low level is up and high level is down, which doesn't make sense,
        // if that was made for a reason, feel free to revert.
        size_t n;
        Coord y, y_offset = screen_rect().location().y();
        Coord prev_x = screen_rect().location().x(), prev_y;
        float x, x_inc;
        Dim h = screen_rect().size().height();
        const float y_scale = (float)(h - 1) / 65536.0;
        int16_t *data_start = data_ + offset_;

        // Clear
        painter.fill_rectangle_unrolled8(screen_rect(), Theme::getInstance()->bg_darkest->background);

        if (!length_)
            return;

        x_inc = (float)screen_rect().size().width() / length_;

        if (digital_)
        {
            // Digital waveform: each value is an horizontal line
            x = 0;
            h--;
            for (n = 0; n < length_; n++)
            {
                y = *(data_start++) ? 0 : h;

                if (n)
                {
                    if (y != prev_y)
                        painter.draw_vline({(Coord)x, y_offset}, h, color_);
                }

                painter.draw_hline({(Coord)x, y_offset + y}, ceil(x_inc), color_);

                prev_y = y;
                x += x_inc;
            }
        }
        else
        {
            // Analog waveform: each value is a point's Y coordinate
            x = prev_x + x_inc;
            h /= 2;

            prev_y = y_offset + h + (*(data_start++) * y_scale);
            for (n = 1; n < length_; n++)
            {
                y = y_offset + h + (*(data_start++) * y_scale);
                // TODO: wire standalone api: display.draw_line({prev_x, prev_y}, {(Coord)x, y}, color_);

                prev_x = x;
                prev_y = y;
                x += x_inc;
            }
        }

        // Cursors
        if (show_cursors)
        {
            for (n = 0; n < 2; n++)
            {
                painter.draw_vline(
                    Point(std::min(screen_rect().size().width(), (int)cursors[n]), y_offset),
                    screen_rect().size().height(),
                    cursor_colors[n]);
            }
        }
    }

    /* VuMeter **************************************************************/

    VuMeter::VuMeter(
        Rect parent_rect,
        uint32_t LEDs,
        bool show_max)
        : Widget{parent_rect},
          LEDs_{LEDs},
          show_max_{show_max}
    {
        // set_focusable(false);
        LED_height = std::max(1UL, parent_rect.size().height() / LEDs);
        split = 256 / LEDs;
    }

    void VuMeter::set_value(const uint32_t new_value)
    {
        if ((new_value != value_) && (new_value < 256))
        {
            value_ = new_value;
            set_dirty();
        }
    }

    void VuMeter::set_mark(const uint32_t new_mark)
    {
        if ((new_mark != mark) && (new_mark < 256))
        {
            mark = new_mark;
            set_dirty();
        }
    }

    void VuMeter::paint(Painter &painter)
    {
        uint32_t bar;
        Color color;
        bool lit = false;
        uint32_t bar_level;
        Point pos = screen_rect().location();
        Dim width = screen_rect().size().width() - 4;
        Dim height = screen_rect().size().height();
        Dim bottom = pos.y() + height;
        Coord marks_x = pos.x() + width;

        if (value_ != prev_value)
        {
            bar_level = LEDs_ - ((value_ + 1) / split);

            // Draw LEDs
            for (bar = 0; bar < LEDs_; bar++)
            {
                if (bar >= bar_level)
                    lit = true;

                if (bar == 0)
                    color = lit ? Theme::getInstance()->fg_red->foreground : Theme::getInstance()->bg_dark->background;
                else if (bar == 1)
                    color = lit ? Theme::getInstance()->fg_orange->foreground : Theme::getInstance()->bg_dark->background;
                else if ((bar == 2) || (bar == 3))
                    color = lit ? Theme::getInstance()->fg_yellow->foreground : Theme::getInstance()->bg_dark->background;
                else
                    color = lit ? Theme::getInstance()->fg_green->foreground : Theme::getInstance()->bg_dark->background;

                painter.fill_rectangle({pos.x(), pos.y() + (Coord)(bar * (LED_height + 1)), width, (Coord)LED_height}, color);
            }
            prev_value = value_;
        }

        // Update max level
        if (show_max_)
        {
            if (value_ > max)
            {
                max = value_;
                hold_timer = 30; // 0.5s @ 60Hz
            }
            else
            {
                if (hold_timer)
                {
                    hold_timer--;
                }
                else
                {
                    if (max)
                        max--; // Let it drop
                }
            }

            // Draw max level
            if (max != prev_max)
            {
                painter.draw_hline({marks_x, bottom - (height * prev_max) / 256}, 8, Theme::getInstance()->bg_darkest->background);
                painter.draw_hline({marks_x, bottom - (height * max) / 256}, 8, Theme::getInstance()->bg_darkest->foreground);
                if (prev_max == mark)
                    prev_mark = 0; // Force mark refresh
                prev_max = max;
            }
        }

        // Draw mark (forced refresh)
        if (mark)
        {
            painter.draw_hline({marks_x, bottom - (height * prev_mark) / 256}, 8, Theme::getInstance()->bg_darkest->background);
            painter.draw_hline({marks_x, bottom - (height * mark) / 256}, 8, Theme::getInstance()->fg_medium->foreground);
            prev_mark = mark;
        }
    }

} /* namespace ui */
