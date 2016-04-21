/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "utility.hpp"

#include <memory>
#include <vector>
#include <string>

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
	FocusManager focus_manager_;
};

class Widget {
public:
	constexpr Widget(
	) : parent_rect { }
	{
	}

	constexpr Widget(
		Rect parent_rect
	) : parent_rect { parent_rect }
	{
	}

	Widget(const Widget&) = delete;
	Widget(Widget&&) = delete;

	virtual ~Widget() = default;

	Point screen_pos();
	Size size() const;
	Rect screen_rect();
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

	virtual void on_show() { };
	virtual void on_hide() { };

	virtual bool on_key(const KeyEvent event);
	virtual bool on_encoder(const EncoderEvent event);
	virtual bool on_touch(const TouchEvent event);
	virtual const std::vector<Widget*>& children() const;

	virtual Context& context() const;

	void set_style(const Style* new_style);

	const Style& style() const;

	// State management methods.
	void set_dirty();
	bool dirty() const;
	void set_clean();

	void visible(bool v);

	bool highlighted() const;
	void set_highlighted(const bool value);

protected:
	void dirty_overlapping_children_in_rect(const Rect& child_rect);

private:
	/* Widget rectangle relative to parent pos(). */
	Rect parent_rect;
	const Style* style_ { nullptr };
	Widget* parent_ { nullptr };

	struct flags_t {
		bool dirty : 1;			// Widget content has changed.
		bool hidden : 1;		// Hide widget and children.
		bool focusable : 1;		// Widget can receive focus.
		bool highlighted : 1;	// Show in a highlighted style.
		bool visible : 1;		// Object was visible during last paint.
	};

	flags_t flags {
		.dirty = true,
		.hidden = false,
		.focusable = false,
		.highlighted = false,
		.visible = false,
	};

	static const std::vector<Widget*> no_children;
};

class View : public Widget {
public:
	View() {
	}

	View(Rect parent_rect) {
		set_parent_rect(parent_rect);
	}

	// TODO: ~View() should on_hide() all children?

	void paint(Painter& painter) override;

	void add_child(Widget* const widget);
	void add_children(const std::vector<Widget*>& children);
	void remove_child(Widget* const widget);
	const std::vector<Widget*>& children() const override;

	virtual std::string title() const;

protected:
	std::vector<Widget*> children_;

	void invalidate_child(Widget* const widget);
};

class Rectangle : public Widget {
public:
	Rectangle(Rect parent_rect, Color c);

	void paint(Painter& painter) override;

	void set_color(const Color c);

private:
	Color color;
};

class Text : public Widget {
public:
	Text(
	) : text { "" } {
	}

	Text(Rect parent_rect, std::string text);
	Text(Rect parent_rect);

	void set(const std::string value);

	void paint(Painter& painter) override;

private:
	std::string text;
};

class Checkbox : public Widget {
public:
	std::function<void(Checkbox&)> on_select;

	Checkbox(Point parent_pos, size_t length, std::string text);
	
	void set_text(const std::string value);
	void set_style(const Style* new_style);
	std::string text() const;
	void set_value(const bool value);
	bool value() const;

	void paint(Painter& painter) override;
	
	bool on_key(const KeyEvent key) override;
	bool on_touch(const TouchEvent event) override;

private:
	std::string text_;
	bool value_ = false;
	const Style* style_ { nullptr };
};

class Button : public Widget {
public:
	std::function<void(Button&)> on_select;
	std::function<void(Button&,KeyEvent)> on_dir;

	Button(Rect parent_rect, std::string text);

	Button(
	) : Button { { }, { } }
	{
	}

	void set_text(const std::string value);
	std::string text() const;

	void paint(Painter& painter) override;

	bool on_key(const KeyEvent key) override;
	bool on_touch(const TouchEvent event) override;

private:
	std::string text_;
};

class Image : public Widget {
public:
	Image();
	Image(
		const Rect parent_rect,
		const Bitmap* bitmap,
		const Color foreground,
		const Color background
	);

	void set_bitmap(const Bitmap* bitmap);
	void set_foreground(const Color color);
	void set_background(const Color color);

	void paint(Painter& painter) override;

private:
	const Bitmap* bitmap_;
	Color foreground_;
	Color background_;
};

class ImageButton : public Image {
public:
	std::function<void(ImageButton&)> on_select;

	ImageButton(
		const Rect parent_rect,
		const Bitmap* bitmap,
		const Color foreground,
		const Color background
	);

	bool on_key(const KeyEvent key) override;
	bool on_touch(const TouchEvent event) override;
};

class OptionsField : public Widget {
public:
	using name_t = std::string;
	using value_t = int32_t;
	using option_t = std::pair<name_t, value_t>;
	using options_t = std::vector<option_t>;

	std::function<void(size_t, value_t)> on_change;
	std::function<void(void)> on_show_options;

	OptionsField(Point parent_pos, size_t length, options_t options);
	
	void set_options(options_t new_options);

	size_t selected_index() const;
	void set_selected_index(const size_t new_index);

	void set_by_value(value_t v);

	void paint(Painter& painter) override;

	void on_focus() override;
	bool on_encoder(const EncoderEvent delta) override;
	bool on_touch(const TouchEvent event) override;

private:
	const size_t length_;
	options_t options;
	size_t selected_index_ { 0 };
};

class NumberField : public Widget {
public:
	std::function<void(int32_t)> on_change;

	using range_t = std::pair<int32_t, int32_t>;

	NumberField(Point parent_pos, size_t length, range_t range, int32_t step, char fill_char);

	NumberField(const NumberField&) = delete;
	NumberField(NumberField&&) = delete;

	int32_t value() const;
	void set_value(int32_t new_value);

	void paint(Painter& painter) override;

	bool on_encoder(const EncoderEvent delta) override;
	bool on_touch(const TouchEvent event) override;

private:
	const range_t range;
	const int32_t step;
	const size_t length_;
	const char fill_char;
	int32_t value_;

	int32_t clip_value(int32_t value);
};

} /* namespace ui */

#endif/*__UI_WIDGET_H__*/
