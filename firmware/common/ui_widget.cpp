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
#include "portapack.hpp"

#include <cstdint>
#include <cstddef>
#include <algorithm>

#include "string_format.hpp"

using namespace portapack;

namespace ui {

static bool ui_dirty = true;

void dirty_set() {
	ui_dirty = true;
}

void dirty_clear() {
	ui_dirty = false;
}

bool is_dirty() {
	return ui_dirty;
}

/* Widget ****************************************************************/

const std::vector<Widget*> Widget::no_children { };

Point Widget::screen_pos() {
	return screen_rect().location();
}

Size Widget::size() const {
	return _parent_rect.size();
}

Rect Widget::screen_rect() const {
	return parent() ? (parent_rect() + parent()->screen_pos()) : parent_rect();
}

Rect Widget::parent_rect() const {
	return _parent_rect;
}

void Widget::set_parent_rect(const Rect new_parent_rect) {
	_parent_rect = new_parent_rect;
	set_dirty();
}

Widget* Widget::parent() const {
	return parent_;
}

void Widget::set_parent(Widget* const widget) {
	if( widget == parent_ ) {
		return;
	}

	if( parent_ && !widget ) {
		// We have a parent, but are losing it. Update visible status.
		dirty_overlapping_children_in_rect(screen_rect());
		visible(false);
	}

	parent_ = widget;

	set_dirty();
}

void Widget::set_dirty() {
	flags.dirty = true;
	dirty_set();
}

bool Widget::dirty() const {
	return flags.dirty;
}

void Widget::set_clean() {
	flags.dirty = false;
}

void Widget::hidden(bool hide) {
	if( hide != flags.hidden ) {
		flags.hidden = hide;

		// If parent is hidden, either of these is a no-op.
		if( hide ) {
			// TODO: Instead of dirtying parent entirely, dirty only children
			// that overlap with this widget.
			
			//parent()->dirty_overlapping_children_in_rect(parent_rect());
			
			/* TODO: Notify self and all non-hidden children that they're
			 * now effectively hidden?
			 */
		} else {
			set_dirty();
			/* TODO: Notify self and all non-hidden children that they're
			 * now effectively shown?
			 */
		}
	}
}

void Widget::focus() {
	context().focus_manager().set_focus_widget(this);
}

void Widget::on_focus() {
	//set_dirty();
}

void Widget::blur() {
	context().focus_manager().set_focus_widget(nullptr);
}

void Widget::on_blur() {
	//set_dirty();
}

bool Widget::focusable() const {
	return flags.focusable;
}

void Widget::set_focusable(const bool value) {
	flags.focusable = value;
}

bool Widget::has_focus() {
	return (context().focus_manager().focus_widget() == this);
}

bool Widget::on_key(const KeyEvent event) {
	(void)event;
	return false;
}

bool Widget::on_encoder(const EncoderEvent event) {
	(void)event;
	return false;
}

bool Widget::on_touch(const TouchEvent event) {
	(void)event;
	return false;
}

const std::vector<Widget*>& Widget::children() const {
	return no_children;
}

Context& Widget::context() const {
	return parent()->context();
}

void Widget::set_style(const Style* new_style) {
	if( new_style != style_ ) {
		style_ = new_style;
		set_dirty();
	}
}

const Style& Widget::style() const {
	return style_ ? *style_ : parent()->style();
}

void Widget::visible(bool v) {
	if( v != flags.visible ) {
		flags.visible = v;

		/* TODO: This on_show/on_hide implementation seems inelegant.
		 * But I need *some* way to take/configure resources when
		 * a widget becomes visible, and reverse the process when the
		 * widget becomes invisible, whether the widget (or parent) is
		 * hidden, or the widget (or parent) is removed from the tree.
		 */
		if( v ) {
			on_show();
		} else {
			on_hide();

			// Set all children invisible too.
			for(const auto child : children()) {
				child->visible(false);
			}
		}
	}
}

bool Widget::highlighted() const {
	return flags.highlighted;
}

void Widget::set_highlighted(const bool value) {
	flags.highlighted = value;
}

void Widget::dirty_overlapping_children_in_rect(const Rect& child_rect) {
	for(auto child : children()) {
		if( !child_rect.intersect(child->parent_rect()).is_empty() ) {
			child->set_dirty();
		}
	}
}

/* View ******************************************************************/

void View::paint(Painter& painter) {
	painter.fill_rectangle(
		screen_rect(),
		style().background
	);
}

void View::add_child(Widget* const widget) {
	if( widget ) {
		if( widget->parent() == nullptr ) {
			widget->set_parent(this);
			children_.push_back(widget);
		}
	}
}

void View::add_children(const std::initializer_list<Widget*> children) {
	children_.insert(std::end(children_), children);
	for(auto child : children) {
		child->set_parent(this);
	}
}

void View::remove_child(Widget* const widget) {
	if( widget ) {
		children_.erase(std::remove(children_.begin(), children_.end(), widget), children_.end());
		widget->set_parent(nullptr);
	}
}

void View::remove_children(const std::vector<Widget*>& children) {
	for(auto child : children) {
		remove_child(child);
	}
}

const std::vector<Widget*>& View::children() const {
	return children_;
}

std::string View::title() const {
	return "";
};

/* OptionTabView *********************************************************/

OptionTabView::OptionTabView(Rect parent_rect) {
	set_parent_rect(parent_rect);
	
	add_child(&check_enable);
	hidden(true);
	
	check_enable.on_select = [this](Checkbox&, bool value) {
		enabled = value;
	};
}

void OptionTabView::set_enabled(bool value) {
	check_enable.set_value(value);
}

bool OptionTabView::is_enabled() {
	return check_enable.value();
}

void OptionTabView::set_type(std::string type) {
	check_enable.set_text("Transmit " + type);
}

void OptionTabView::focus() {
	check_enable.focus();
}

/* Rectangle *************************************************************/

Rectangle::Rectangle(
	Color c
) : Widget { },
	color { c }
{
}

Rectangle::Rectangle(
	Rect parent_rect,
	Color c
) : Widget { parent_rect },
	color { c }
{
}

void Rectangle::set_color(const Color c) {
	color = c;
	set_dirty();
}

void Rectangle::set_outline(const bool outline) {
	_outline = outline;
	set_dirty();
}

void Rectangle::paint(Painter& painter) {
	if (!_outline) {
		painter.fill_rectangle(
			screen_rect(),
			color
		);
	} else {
		painter.draw_rectangle(
			screen_rect(),
			color
		);
	}
}

/* Text ******************************************************************/

Text::Text(
	Rect parent_rect,
	std::string text
) : Widget { parent_rect },
	text { text }
{
}

Text::Text(
	Rect parent_rect
) : Text { parent_rect, { } }
{
}

void Text::set(const std::string value) {
	text = value;
	set_dirty();
}

void Text::paint(Painter& painter) {
	const auto rect = screen_rect();
	const auto s = style();

	painter.fill_rectangle(rect, s.background);

	painter.draw_string(
		rect.location(),
		s,
		text
	);
}

/* Labels ****************************************************************/

Labels::Labels(
	std::initializer_list<Label> labels
) : labels_ { labels }
{
}

void Labels::set_labels(std::initializer_list<Label> labels) {
	labels_ = labels;
	set_dirty();
}

void Labels::paint(Painter& painter) {
	for (auto &label : labels_) {
		painter.draw_string(
			label.pos + screen_pos(),
			style().font,
			label.color,
			style().background,
			label.text
		);
	}
}

/* LiveDateTime **********************************************************/

void LiveDateTime::on_tick_second() {
	rtcGetTime(&RTCD1, &datetime);
	
	text = to_string_dec_uint(datetime.month(), 2, '0') + "/" + to_string_dec_uint(datetime.day(), 2, '0') + " " +
			to_string_dec_uint(datetime.hour(), 2, '0') + ":" + to_string_dec_uint(datetime.minute(), 2, '0');
	
	set_dirty();
}

LiveDateTime::LiveDateTime(
	Rect parent_rect
) : Widget { parent_rect }
{
	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
}

LiveDateTime::~LiveDateTime() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
}

void LiveDateTime::paint(Painter& painter) {
	const auto rect = screen_rect();
	const auto s = style();
	
	on_tick_second();

	painter.fill_rectangle(rect, s.background);

	painter.draw_string(
		rect.location(),
		s,
		text
	);
}

/* BigFrequency **********************************************************/

BigFrequency::BigFrequency(
	Rect parent_rect,
	rf::Frequency frequency
) : Widget { parent_rect },
	_frequency { frequency }
{
}

void BigFrequency::set(const rf::Frequency frequency) {
	_frequency = frequency;
	set_dirty();
}

void BigFrequency::paint(Painter& painter) {
	uint32_t i, digit_def;
	std::array<char, 7> digits;
	char digit;
	Point digit_pos;
	ui::Color segment_color;
	
	const auto rect = screen_rect();
	
	// Erase
	painter.fill_rectangle({ { 0, rect.location().y() }, { 240, 52 } }, ui::Color::black());
	
	// Prepare digits
	if (!_frequency) {
		digits.fill(10);			// ----.---
		digit_pos = { 0, rect.location().y() };
	} else {
		_frequency /= 1000;			// GMMM.KKK(uuu)
		
		for (i = 0; i < 7; i++) {
			digits[6 - i] = _frequency % 10;
			_frequency /= 10;
		}
		
		// Remove leading zeros
		for (i = 0; i < 3; i++) {
			if (!digits[i])
				digits[i] = 16;		// "Don't draw" code
			else
				break;
		}
		
		digit_pos = { (Coord)(240 - ((7 * digit_width) + 8) - (i * digit_width)) / 2, rect.location().y() };
	}
	
	segment_color = style().foreground;

	// Draw
	for (i = 0; i < 7; i++) {
		digit = digits[i];
		
		if (digit < 16) {
			digit_def = segment_font[(uint8_t)digit];
			
			for (size_t s = 0; s < 7; s++) {
				if (digit_def & 1)
					painter.fill_rectangle({ digit_pos + segments[s].location(), segments[s].size() }, segment_color);
				digit_def >>= 1;
			}
		}
		
		if (i == 3) {
			// Dot
			painter.fill_rectangle({ digit_pos + Point(34, 48), { 4, 4 } }, segment_color);
			digit_pos += { (digit_width + 8), 0 };
		} else {
			digit_pos += { digit_width, 0 };
		}
	}
}

/* ProgressBar ***********************************************************/

ProgressBar::ProgressBar(
	Rect parent_rect
) : Widget { parent_rect }
{
}

void ProgressBar::set_max(const uint32_t max) {
	if (max == _max) return;
	
	if (_value > _max)
		_value = _max;
	
	_max = max;
	set_dirty();
}

void ProgressBar::set_value(const uint32_t value) {
	if (value == _value) return;
	
	if (value > _max)
		_value = _max;
	else
		_value = value;
	set_dirty();
}

void ProgressBar::paint(Painter& painter) {
	int v_scaled;
	
	const auto sr = screen_rect();
	const auto s = style();

	v_scaled = (sr.size().width() * _value) / _max;

	painter.fill_rectangle({sr.location(), {v_scaled, sr.size().height()}}, style().foreground);
	painter.fill_rectangle({{sr.location().x() + v_scaled, sr.location().y()}, {sr.size().width() - v_scaled, sr.size().height()}}, s.background);
	
	painter.draw_rectangle(sr, s.foreground);
}

/* Console ***************************************************************/

Console::Console(
	Rect parent_rect
) : Widget { parent_rect }
{
}

void Console::clear(bool clear_buffer = false) {
	if(clear_buffer)
		buffer.clear();
		
	display.fill_rectangle(
		screen_rect(),
		Color::black()
	);
	pos = { 0, 0 };
}

void Console::write(std::string message) {
	bool escape = false;
	
	if (!hidden() && visible()) {
		const Style& s = style();
		const Font& font = s.font;
		const auto rect = screen_rect();
		ui::Color pen_color = s.foreground;
		
		for (const auto c : message) {
			if (escape) {
				if (c <= 15)
					pen_color = term_colors[(uint8_t)c];
				else
					pen_color = s.foreground;
				escape = false;
			} else {
				if (c == '\n') {
					crlf();
				} else if (c == '\x1B') {
					escape = true;
				} else {
					const auto glyph = font.glyph(c);
					const auto advance = glyph.advance();
					if( (pos.x() + advance.x()) > rect.width() ) {
						crlf();
					}
					const Point pos_glyph {
						rect.left() + pos.x(),
						display.scroll_area_y(pos.y())
					};
					display.draw_glyph(pos_glyph, glyph, pen_color, s.background);
					pos += { advance.x(), 0 };
				}
			}
		}
		buffer = message;
	} else {
		if (buffer.size() < 256) buffer += message;
	}
}

void Console::writeln(std::string message) {
	write(message);
	crlf();
}

void Console::paint(Painter&) {
	write(buffer);
}

void Console::on_show() {
	const auto screen_r = screen_rect();
	display.scroll_set_area(screen_r.top(), screen_r.bottom());
	display.scroll_set_position(0);
	clear();
	//visible = true;
}

void Console::on_hide() {
	/* TODO: Clear region to eliminate brief flash of content at un-shifted
	 * position?
	 */
	display.scroll_disable();
	//visible = false;
}

void Console::crlf() {
	if (hidden() || !visible()) return;
	
	const Style& s = style();
	const auto sr = screen_rect();
	const auto line_height = s.font.line_height();
	pos = { 0, pos.y() + line_height };
	const int32_t y_excess = pos.y() + line_height - sr.height();
	if( y_excess > 0 ) {
		display.scroll(-y_excess);
		pos = { pos.x(), pos.y() - y_excess };

		const Rect dirty { sr.left(), display.scroll_area_y(pos.y()), sr.width(), line_height };
		display.fill_rectangle(dirty, s.background);
	}
}

/* Checkbox **************************************************************/

Checkbox::Checkbox(
	Point parent_pos,
	size_t length,
	std::string text,
	bool small
) : Widget {  },
	text_ { text },
	small_ { small }
{
	if (!small_)
		set_parent_rect({ parent_pos, { static_cast<ui::Dim>((8 * length) + 24), 24 } });
	else
		set_parent_rect({ parent_pos, { static_cast<ui::Dim>((8 * length) + 16), 16 } });
	
	set_focusable(true);
}

void Checkbox::set_text(const std::string value) {
	text_ = value;
	set_dirty();
}

bool Checkbox::set_value(const bool value) {
	value_ = value;
	set_dirty();
	
	if( on_select ) {
		on_select(*this, value_);
		return true;
	}
	
	return false;
}

bool Checkbox::value() const {
	return value_;
}

void Checkbox::paint(Painter& painter) {
	const auto r = screen_rect();
	
	const auto paint_style = (has_focus() || highlighted()) ? style().invert() : style();
	
	const auto x = r.location().x();
	const auto y = r.location().y();
	
	const auto label_r = paint_style.font.size_of(text_);
	
	if (!small_) {
		painter.draw_rectangle({ { r.location() }, { 24, 24 } }, style().foreground);
		
		painter.fill_rectangle({ x + 1, y + 1, 24 - 2, 24 - 2 }, style().background);
		
		// Highlight
		painter.draw_rectangle({ x + 2, y + 2, 24 - 4, 24 - 4 }, paint_style.background);
		
		if (value_ == true) {
			// Check
			portapack::display.draw_line( {x + 2, y + 14}, {x + 6, y + 18}, ui::Color::green());
			portapack::display.draw_line( {x + 6, y + 18}, {x + 20, y + 4}, ui::Color::green());
		} else {
			// Cross
			portapack::display.draw_line( {x + 1, y + 1}, {x + 24 - 2, y + 24 - 2}, ui::Color::red());
			portapack::display.draw_line( {x + 24 - 2, y + 1}, {x + 1, y + 24 - 2}, ui::Color::red());
		}
		
		painter.draw_string(
			{
				static_cast<Coord>(x + 24 + 4),
				static_cast<Coord>(y + (24 - label_r.height()) / 2)
			},
			paint_style,
			text_
		);
	} else {
		painter.draw_rectangle({ { r.location() }, { 16, 16 } }, style().foreground);
		
		painter.fill_rectangle({ x + 1, y + 1, 16 - 2, 16 - 2 }, style().background);
		
		// Highlight
		painter.draw_rectangle({ x + 1, y + 1, 16 - 2, 16 - 2 }, paint_style.background);
		
		if (value_ == true) {
			// Check
			portapack::display.draw_line( {x + 2, y + 8}, {x + 6, y + 12}, ui::Color::green());
			portapack::display.draw_line( {x + 6, y + 12}, {x + 13, y + 5}, ui::Color::green());
		} else {
			// Cross
			portapack::display.draw_line( {x + 1, y + 1}, {x + 16 - 2, y + 16 - 2}, ui::Color::red());
			portapack::display.draw_line( {x + 16 - 2, y + 1}, {x + 1, y + 16 - 2}, ui::Color::red());
		}
		
		painter.draw_string(
			{
				static_cast<Coord>(x + 16 + 2),
				static_cast<Coord>(y + (16 - label_r.height()) / 2)
			},
			paint_style,
			text_
		);
	}
}

bool Checkbox::on_key(const KeyEvent key) {
	if( key == KeyEvent::Select )
		return set_value(not value_);

	return false;
}

bool Checkbox::on_touch(const TouchEvent event) {
	switch(event.type) {
	case TouchEvent::Type::Start:
		set_highlighted(true);
		set_dirty();
		return true;

	case TouchEvent::Type::End:
		set_highlighted(false);
		value_ = not value_;
		set_dirty();
		if( on_select ) {
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
	std::string text
) : Widget { parent_rect },
	text_ { text }
{
	set_focusable(true);
}

void Button::set_text(const std::string value) {
	text_ = value;
	set_dirty();
}

std::string Button::text() const {
	return text_;
}

void Button::paint(Painter& painter) {
	Color bg, fg;
	const auto r = screen_rect();

	if (has_focus() || highlighted()) {
		bg = style().foreground;
		fg = Color::black();
	} else {
		bg = Color::grey();
		fg = style().foreground;
	}
	
	const Style paint_style = { style().font, bg, fg };
	
	painter.draw_rectangle({r.location(), {r.size().width(), 1}}, Color::light_grey());
	painter.draw_rectangle({r.location().x(), r.location().y() + r.size().height() - 1, r.size().width(), 1}, Color::dark_grey());
	painter.draw_rectangle({r.location().x() + r.size().width() - 1, r.location().y(), 1, r.size().height()}, Color::dark_grey());

	painter.fill_rectangle(
		{ r.location().x(), r.location().y() + 1, r.size().width() - 1, r.size().height() - 2 },
		paint_style.background
	);

	const auto label_r = paint_style.font.size_of(text_);
	painter.draw_string(
		{ r.location().x() + (r.size().width() - label_r.width()) / 2, r.location().y() + (r.size().height() - label_r.height()) / 2 },
		paint_style,
		text_
	);
}

void Button::on_focus() {
	if( on_highlight )
		on_highlight(*this);
}

bool Button::on_key(const KeyEvent key) {
	if( key == KeyEvent::Select ) {
		if( on_select ) {
			on_select(*this);
			return true;
		}
	} else {
		if( on_dir ) {
			return on_dir(*this, key);
		}
	}

	return false;
}

bool Button::on_touch(const TouchEvent event) {
	switch(event.type) {
	case TouchEvent::Type::Start:
		set_highlighted(true);
		set_dirty();
		return true;


	case TouchEvent::Type::End:
		set_highlighted(false);
		set_dirty();
		if( on_select ) {
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

/* NewButton ****************************************************************/

NewButton::NewButton(
	Rect parent_rect,
	std::string text,
	const Bitmap* bitmap
) : Widget { parent_rect },
	text_ { text },
	bitmap_ (bitmap)
{
	set_focusable(true);
}

void NewButton::set_text(const std::string value) {
	text_ = value;
	set_dirty();
}

std::string NewButton::text() const {
	return text_;
}

void NewButton::set_bitmap(const Bitmap* bitmap) {
	bitmap_ = bitmap;
	set_dirty();
}

const Bitmap* NewButton::bitmap() {
	return bitmap_;
}

void NewButton::set_color(Color color) {
  color_ = color;
	set_dirty();
}

ui::Color NewButton::color() {
	return color_;
}

void NewButton::paint(Painter& painter) {

	if (!bitmap_ && text_.empty())
		return;

	Color bg, fg;
	const auto r = screen_rect();

	if (has_focus() || highlighted()) {
		bg = style().foreground;
		fg = Color::black();
	} else {
		bg = Color::grey();
		fg = style().foreground;
	}

	const Style paint_style = { style().font, bg, fg };

	painter.draw_rectangle({r.location(), {r.size().width(), 1}}, Color::light_grey());
	painter.draw_rectangle({r.location().x(), r.location().y() + r.size().height() - 1, r.size().width(), 1}, Color::dark_grey());
	painter.draw_rectangle({r.location().x() + r.size().width() - 1, r.location().y(), 1, r.size().height()}, Color::dark_grey());

	painter.fill_rectangle(
		{ r.location().x(), r.location().y() + 1, r.size().width() - 1, r.size().height() - 2 },
		paint_style.background
	);

	int y = r.location().y();
	if (bitmap_) {
		painter.draw_bitmap(
			{r.location().x() + (r.size().width() / 2) - 8, r.location().y() + 6},
			*bitmap_,
			color_, //Color::green(), //fg,
			bg
		);
		y += 10;
	}
	const auto label_r = paint_style.font.size_of(text_);
	painter.draw_string(
		{ r.location().x() + (r.size().width() - label_r.width()) / 2, y + (r.size().height() - label_r.height()) / 2 },
		paint_style,
		text_
	);
}

void NewButton::on_focus() {
	if( on_highlight )
		on_highlight(*this);
}

bool NewButton::on_key(const KeyEvent key) {
	if( key == KeyEvent::Select ) {
		if( on_select ) {
			on_select();
			return true;
		}
	} else {
		if( on_dir ) {
			return on_dir(*this, key);
		}
	}

	return false;
}

bool NewButton::on_touch(const TouchEvent event) {
	switch(event.type) {
	case TouchEvent::Type::Start:
		set_highlighted(true);
		set_dirty();
		return true;


	case TouchEvent::Type::End:
		set_highlighted(false);
		set_dirty();
		if( on_select ) {
			on_select();
		}
		return true;

	default:
		return false;
	}
}

/* Image *****************************************************************/

Image::Image(
) : Image { { }, nullptr, Color::white(), Color::black() }
{
}

Image::Image(
	const Rect parent_rect,
	const Bitmap* bitmap,
	const Color foreground,
	const Color background
) : Widget { parent_rect },
	bitmap_ { bitmap },
	foreground_ { foreground },
	background_ { background }
{
}

void Image::set_bitmap(const Bitmap* bitmap) {
	bitmap_ = bitmap;
	set_dirty();
}

void Image::set_foreground(const Color color) {
	foreground_ = color;
	set_dirty();
}

void Image::set_background(const Color color) {
	background_ = color;
	set_dirty();
}

void Image::invert_colors() {
	Color temp;
	
	temp = background_;
	background_ = foreground_;
	foreground_ = temp;
	set_dirty();
}

void Image::paint(Painter& painter) {
	if( bitmap_ ) {
		// Code also handles ImageButton behavior.
		const bool selected = (has_focus() || highlighted());
		painter.draw_bitmap(
			screen_pos(),
			*bitmap_,
			selected ? background_ : foreground_,
			selected ? foreground_ : background_
		);
	}
}

/* ImageButton ***********************************************************/

// TODO: Virtually all this code is duplicated from Button. Base class?

ImageButton::ImageButton(
	const Rect parent_rect,
	const Bitmap* bitmap,
	const Color foreground,
	const Color background
) : Image { parent_rect, bitmap, foreground, background }
{
	set_focusable(true);
}

bool ImageButton::on_key(const KeyEvent key) {
	if( key == KeyEvent::Select ) {
		if( on_select ) {
			on_select(*this);
			return true;
		}
	}

	return false;
}

bool ImageButton::on_touch(const TouchEvent event) {
	switch(event.type) {
	case TouchEvent::Type::Start:
		set_highlighted(true);
		set_dirty();
		return true;


	case TouchEvent::Type::End:
		set_highlighted(false);
		set_dirty();
		if( on_select ) {
			on_select(*this);
		}
		return true;

	default:
		return false;
	}
}

/* ImageOptionsField *****************************************************/

ImageOptionsField::ImageOptionsField(
	const Rect parent_rect,
	const Color foreground,
	const Color background,
	const options_t options
) : Widget { parent_rect },
	options { options },
	foreground_ { foreground },
	background_ { background }
{
	set_focusable(true);
}

size_t ImageOptionsField::selected_index() const {
	return selected_index_;
}

size_t ImageOptionsField::selected_index_value() const {
	return options[selected_index_].second;
}

void ImageOptionsField::set_selected_index(const size_t new_index) {
	if ( new_index < options.size() ) {
		if ( new_index != selected_index() ) {
			selected_index_ = new_index;
			if ( on_change ) {
				on_change(selected_index(), options[selected_index()].second);
			}
			set_dirty();
		}
	}
}

void ImageOptionsField::set_by_value(value_t v) {
	size_t new_index { 0 };
	for(const auto& option : options) {
		if( option.second == v ) {
			set_selected_index(new_index);
			break;
		}
		new_index++;
	}
}

void ImageOptionsField::set_options(options_t new_options) {
	options = new_options;
	set_by_value(0);
	set_dirty();
}

void ImageOptionsField::paint(Painter& painter) {
	const bool selected = (has_focus() || highlighted());
	const auto paint_style = selected ? style().invert() : style();
	
	painter.draw_rectangle(
		{ screen_rect().location(), { screen_rect().size().width() + 4, screen_rect().size().height() + 4 } },
		paint_style.background
	);
	
	painter.draw_bitmap(
		{screen_pos().x() + 2, screen_pos().y() + 2},
		*options[selected_index_].first,
		foreground_,
		background_
	);
}

void ImageOptionsField::on_focus() {
	if( on_show_options ) {
		on_show_options();
	}
}

bool ImageOptionsField::on_encoder(const EncoderEvent delta) {
	set_selected_index(selected_index() + delta);
	return true;
}

bool ImageOptionsField::on_touch(const TouchEvent event) {
	if( event.type == TouchEvent::Type::Start ) {
		focus();
	}
	return true;
}

/* OptionsField **********************************************************/

OptionsField::OptionsField(
	Point parent_pos,
	int length,
	options_t options
) : Widget { { parent_pos, { 8 * length, 16 } } },
	length_ { length },
	options { options }
{
	set_focusable(true);
}

size_t OptionsField::selected_index() const {
	return selected_index_;
}

size_t OptionsField::selected_index_value() const {
	return options[selected_index_].second;
}

void OptionsField::set_selected_index(const size_t new_index) {
	if( new_index < options.size() ) {
		if( new_index != selected_index() ) {
			selected_index_ = new_index;
			if( on_change ) {
				on_change(selected_index(), options[selected_index()].second);
			}
			set_dirty();
		}
	}
}

void OptionsField::set_by_value(value_t v) {
	size_t new_index { 0 };
	for(const auto& option : options) {
		if( option.second == v ) {
			set_selected_index(new_index);
			break;
		}
		new_index++;
	}
}

void OptionsField::set_options(options_t new_options) {
	options = new_options;
	set_by_value(0);
	set_dirty();
}

void OptionsField::paint(Painter& painter) {
	const auto paint_style = has_focus() ? style().invert() : style();
	
	painter.fill_rectangle({screen_rect().location(), {(int)length_ * 8, 16}}, ui::Color::black());
	
	if( selected_index() < options.size() ) {
		const auto text = options[selected_index()].first;
		painter.draw_string(
			screen_pos(),
			paint_style,
			text
		);
	}
}

void OptionsField::on_focus() {
	if( on_show_options ) {
		on_show_options();
	}
}

bool OptionsField::on_encoder(const EncoderEvent delta) {
	set_selected_index(selected_index() + delta);
	return true;
}

bool OptionsField::on_touch(const TouchEvent event) {
	if( event.type == TouchEvent::Type::Start ) {
		focus();
	}
	return true;
}

/* NumberField ***********************************************************/

NumberField::NumberField(
	Point parent_pos,
	int length,
	range_t range,
	int32_t step,
	char fill_char,
	bool can_loop
) : Widget { { parent_pos, { 8 * length, 16 } } },
	range { range },
	step { step },
	length_ { length },
	fill_char { fill_char },
	can_loop { can_loop }
{
	set_focusable(true);
}

int32_t NumberField::value() const {
	return value_;
}

void NumberField::set_value(int32_t new_value, bool trigger_change) {
	if (can_loop) {
		if (new_value >= 0)
			new_value = new_value % (range.second + 1);
		else
			new_value = range.second + new_value + 1;
	}
	
	new_value = clip_value(new_value);

	if( new_value != value() ) {
		value_ = new_value;
		if( on_change && trigger_change ) {
			on_change(value_);
		}
		set_dirty();
	}
}

void NumberField::set_range(const int32_t min, const int32_t max) {
	range.first = min;
	range.second = max;
	set_value(value(), false);
}

void NumberField::set_step(const int32_t new_step) {
	step = new_step;
}

void NumberField::paint(Painter& painter) {
	const auto text = to_string_dec_int(value_, length_, fill_char);

	const auto paint_style = has_focus() ? style().invert() : style();

	painter.draw_string(
		screen_pos(),
		paint_style,
		text
	);
}

bool NumberField::on_key(const KeyEvent key) {
	if( key == KeyEvent::Select ) {
		if( on_select ) {
			on_select(*this);
			return true;
		}
	}

	return false;
}

bool NumberField::on_encoder(const EncoderEvent delta) {
	set_value(value() + (delta * step));
	return true;
}

bool NumberField::on_touch(const TouchEvent event) {
	if( event.type == TouchEvent::Type::Start ) {
		focus();
	}
	return true;
}

int32_t NumberField::clip_value(int32_t value) {
	if( value > range.second ) {
		value = range.second;
	}
	if( value < range.first ) {
		value = range.first;
	}
	return value;
}

/* SymField **************************************************************/

SymField::SymField(
	Point parent_pos,
	size_t length,
	symfield_type type
) : Widget { { parent_pos, { static_cast<ui::Dim>(8 * length), 16 } } },
	length_ { length },
	type_ { type }
{
	uint32_t c;
	
	// Auto-init
	if (type == SYMFIELD_OCT) {
		for (c = 0; c < length; c++)
			set_symbol_list(c, "01234567");
	} else if (type == SYMFIELD_DEC) {
		for (c = 0; c < length; c++)
			set_symbol_list(c, "0123456789");
	} else if (type == SYMFIELD_HEX) {
		for (c = 0; c < length; c++)
			set_symbol_list(c, "0123456789ABCDEF");
	} else if (type == SYMFIELD_ALPHANUM) {
		for (c = 0; c < length; c++)
			set_symbol_list(c, " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	}
	
	set_focusable(true);
}

uint32_t SymField::value_dec_u32() {
	uint32_t mul = 1, v = 0;
	
	if (type_ == SYMFIELD_DEC) {
		for (uint32_t c = 0; c < length_; c++) {
			v += values_[(length_ - 1 - c)] * mul;
			mul *= 10;
		}
		return v;
	} else 
		return 0;
}

uint64_t SymField::value_hex_u64() {
	uint64_t v = 0;
	
	if (type_ != SYMFIELD_DEF) {
		for (uint32_t c = 0; c < length_; c++)
			v += (uint64_t)(values_[c]) << (4 * (length_ - 1 - c));
		return v;
	} else 
		return 0;
}

std::string SymField::value_string() {
	std::string return_string { "" };
	
	if (type_ == SYMFIELD_ALPHANUM) {
		for (uint32_t c = 0; c < length_; c++) {
			return_string += symbol_list_[0][values_[c]];
		}
	}
	
	return return_string;
}
	
uint32_t SymField::get_sym(const uint32_t index) {
	if (index >= length_) return 0;

	return values_[index];
}

void SymField::set_sym(const uint32_t index, const uint32_t new_value) {
	if (index >= length_) return;
	
	uint32_t clipped_value = clip_value(index, new_value);

	if (clipped_value != values_[index]) {
		values_[index] = clipped_value;
		if( on_change ) {
			on_change();
		}
		set_dirty();
	}
}

void SymField::set_length(const uint32_t new_length) {
	if ((new_length <= 32) && (new_length != length_)) {
		prev_length_ = length_;
		length_ = new_length;
		
		// Clip eventual garbage from previous shorter word
		for (size_t n = 0; n < length_; n++)
			set_sym(n, values_[n]);
		
		erase_prev_ = true;
		set_dirty();
	}
}

void SymField::set_symbol_list(const uint32_t index, const std::string symbol_list) {
	if (index >= length_) return;
	
	symbol_list_[index] = symbol_list;

	// Re-clip symbol's value
	set_sym(index, values_[index]);
}

void SymField::paint(Painter& painter) {
	Point pt_draw = screen_pos();
	
	if (erase_prev_) {
		painter.fill_rectangle( { pt_draw, { (int)prev_length_ * 8, 16 } }, Color::black() );
		erase_prev_ = false;
	}
	
	for (size_t n = 0; n < length_; n++) {
		const auto text = symbol_list_[n].substr(values_[n], 1);

		const auto paint_style = (has_focus() && (n == selected_)) ? style().invert() : style();

		painter.draw_string(
			pt_draw,
			paint_style,
			text
		);
		
		pt_draw += { 8, 0 };
	}
}

bool SymField::on_key(const KeyEvent key) {
	switch (key) {
		case KeyEvent::Select:
			if( on_select ) {
				on_select(*this);
				return true;
			}
			break;
			
		case KeyEvent::Left:
			if (selected_ > 0) {
				selected_--;
				set_dirty();
				return true;
			}
			break;
			
		case KeyEvent::Right:
			if (selected_ < (length_ - 1)) {
				selected_++;
				set_dirty();
				return true;
			}
			break;
			
		default:
			break;
	}

	return false;
}

bool SymField::on_encoder(const EncoderEvent delta) {
	int32_t new_value = (int)values_[selected_] + delta;
	
	if (new_value >= 0)	
		set_sym(selected_, values_[selected_] + delta);
	
	return true;
}

bool SymField::on_touch(const TouchEvent event) {
	if (event.type == TouchEvent::Type::Start) {
		focus();
	}
	return true;
}

int32_t SymField::clip_value(const uint32_t index, const uint32_t value) {
	size_t symbol_count = symbol_list_[index].length() - 1;
	
	if (value > symbol_count)
		return symbol_count;
	else
		return value;
}

/* Waveform **************************************************************/

Waveform::Waveform(
	Rect parent_rect,
	int16_t * data,
	uint32_t length,
	uint32_t offset,
	bool digital,
	Color color
) : Widget { parent_rect },
	data_ { data },
	length_ { length },
	offset_ { offset },
	digital_ { digital },
	color_ { color }
{
	//set_focusable(false);
	//previous_data.resize(length_, 0);
}

void Waveform::set_cursor(const uint32_t i, const int16_t position) {
	if (i < 2) {
		if (position != cursors[i]) {
			cursors[i] = position;
			set_dirty();
		}
		show_cursors = true;
	}
}

void Waveform::set_offset(const uint32_t new_offset) {
	if (new_offset != offset_) {
		offset_ = new_offset;
		set_dirty();
	}
}

void Waveform::set_length(const uint32_t new_length) {
	if (new_length != length_) {
		length_ = new_length;
		set_dirty();
	}
}

void Waveform::paint(Painter& painter) {
	size_t n;
	Coord y, y_offset = screen_rect().location().y();
	Coord prev_x = screen_rect().location().x(), prev_y;
	float x, x_inc;
	Dim h = screen_rect().size().height();
	const float y_scale = (float)(h - 1) / 65536.0;
	int16_t * data_start = data_ + offset_;
	
	if (!length_) return;
	
	x_inc = (float)screen_rect().size().width() / length_;
	
	// Clear
	painter.fill_rectangle_unrolled8(screen_rect(), Color::black());
	
	if (digital_) {
		// Digital waveform: each value is an horizontal line
		x = 0;
		h--;
		for (n = 0; n < length_; n++) {
			y = *(data_start++) ? h : 0;
			
			if (n) {
				if (y != prev_y)
					painter.draw_vline( {(Coord)x, y_offset}, h, color_);
			}
			
			painter.draw_hline( {(Coord)x, y_offset + y}, ceil(x_inc), color_);
			
			prev_y = y;
			x += x_inc;
		}
	} else {
		// Analog waveform: each value is a point's Y coordinate
		x = prev_x + x_inc;
		h /= 2;
		prev_y = y_offset + h - (*(data_start++) * y_scale);
		for (n = 1; n < length_; n++) {
			y = y_offset + h - (*(data_start++) * y_scale);
			display.draw_line( {prev_x, prev_y}, {(Coord)x, y}, color_);
			
			prev_x = x;
			prev_y = y;
			x += x_inc;
		}
	}
	
	// Cursors
	if (show_cursors) {
		for (n = 0; n < 2; n++) {
			painter.draw_vline(
				Point(std::min(screen_rect().size().width(), (int)cursors[n]), y_offset),
				screen_rect().size().height(),
				cursor_colors[n]
				);
		}
	}
}


/* VuMeter **************************************************************/

VuMeter::VuMeter(
	Rect parent_rect,
	uint32_t LEDs,
	bool show_max
) : Widget { parent_rect },
	LEDs_ { LEDs },
	show_max_ { show_max }
{
	//set_focusable(false);
	LED_height = std::max(1UL, parent_rect.size().height() / LEDs);
	split = 256 / LEDs;
}

void VuMeter::set_value(const uint32_t new_value) {
	if ((new_value != value_) && (new_value < 256)) {
		value_ = new_value;
		set_dirty();
	}
}

void VuMeter::set_mark(const uint32_t new_mark) {
	if ((new_mark != mark) && (new_mark < 256)) {
		mark = new_mark;
		set_dirty();
	}
}

void VuMeter::paint(Painter& painter) {
	uint32_t bar;
	Color color;
	bool lit = false;
	uint32_t bar_level;
	Point pos = screen_rect().location();
	Dim width = screen_rect().size().width() - 4;
	Dim height = screen_rect().size().height();
	Dim bottom = pos.y() + height;
	Coord marks_x = pos.x() + width;
	
	if (value_ != prev_value) {
		bar_level = LEDs_ - ((value_ + 1) / split);
		
		// Draw LEDs
		for (bar = 0; bar < LEDs_; bar++) {
			if (bar >= bar_level)
				lit = true;
			
			if (bar == 0)
				color = lit ? Color::red() : Color::dark_red();
			else if (bar == 1)
				color = lit ? Color::orange() : Color::dark_orange();
			else if ((bar == 2) || (bar == 3))
				color = lit ? Color::yellow() : Color::dark_yellow();
			else
				color = lit ? Color::green() : Color::dark_green();
			
			painter.fill_rectangle({ pos.x(), pos.y() + (Coord)(bar * (LED_height + 1)), width, (Coord)LED_height }, color);
		}
		prev_value = value_;
	}
	
	// Update max level
	if (show_max_) {
		if (value_ > max) {
			max = value_;
			hold_timer = 30;	// 0.5s @ 60Hz
		} else {
			if (hold_timer) {
				hold_timer--;
			} else {
				if (max) max--;	// Let it drop
			}
		}
		
		// Draw max level
		if (max != prev_max) {
			painter.draw_hline({ marks_x, bottom - (height * prev_max) / 256 }, 8, Color::black());
			painter.draw_hline({ marks_x, bottom - (height * max) / 256 }, 8, Color::white());
			if (prev_max == mark)
				prev_mark = 0;	// Force mark refresh
			prev_max = max;
		}
	}
	
	// Draw mark (forced refresh)
	if (mark) {
		painter.draw_hline({ marks_x, bottom - (height * prev_mark) / 256 }, 8, Color::black());
		painter.draw_hline({ marks_x, bottom - (height * mark) / 256 }, 8, Color::grey());
		prev_mark = mark;
	}
}

} /* namespace ui */
