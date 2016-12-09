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
	return screen_rect().pos;
}

Size Widget::size() const {
	return parent_rect.size;
}

Rect Widget::screen_rect() const {
	return parent() ? (parent_rect + parent()->screen_pos()) : parent_rect;
}

void Widget::set_parent_rect(const Rect new_parent_rect) {
	parent_rect = new_parent_rect;
	set_dirty();
}

Widget* Widget::parent() const {
	return parent_;
}

void Widget::set_parent(Widget* const widget) {
	if( parent_ && !widget ) {
		// We have a parent, but are losing it. Update visible status.
		visible(false);
	}

	parent_ = widget;
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
			dirty_overlapping_children_in_rect(parent_rect);
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
		if( !child_rect.intersect(child->parent_rect).is_empty() ) {
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
			widget->set_dirty();
		}
	}
}

void View::add_children(const std::vector<Widget*>& children) {
	for(auto child : children) {
		add_child(child);
	}
}

void View::remove_child(Widget* const widget) {
	if( widget ) {
		children_.erase(std::remove(children_.begin(), children_.end(), widget), children_.end());
		dirty_overlapping_children_in_rect(widget->screen_rect());
		widget->set_parent(nullptr);
	}
}

const std::vector<Widget*>& View::children() const {
	return children_;
}

std::string View::title() const {
	return "";
};

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

Rectangle::Rectangle(
) : Widget {  }
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
		rect.pos,
		s,
		text
	);
}

/* BigFrequency **********************************************************/

const uint8_t big_segment_font[11] = {
	0b00111111,	// 0: ABCDEF
	0b00000110,	// 1: AB
	0b01011011,	// 2: ABDEG
	0b01001111,	// 3: ABCDG
	0b01100110,	// 4: BCFG
	0b01101101,	// 5: ACDFG
	0b01111101,	// 6: ACDEFG
	0b00000111,	// 7: ABC
	0b01111111,	// 8: ABCDEFG
	0b01101111,	// 9: ABCDFG
	0b01000000	// -: G
};

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
	uint8_t i, digit_def;
	char digits[7];
	char digit;
	Coord digit_x, digit_y;
	ui::Color segment_color;
	
	const auto rect = screen_rect();
	
	// Erase
	painter.fill_rectangle({{0, rect.pos.y}, {240, 52}}, ui::Color::black());
	
	if (!_frequency) {
		for (i = 0; i < 7; i++)		// ----.------
			digits[i] = 10;
	} else {
		_frequency /= 1000;			// GMMM.KKKuuu
		
		for (i = 0; i < 7; i++) {
			digits[6 - i] = _frequency % 10;
			_frequency /= 10;
		}
		
		// Remove leading zeros
		for (i = 0; i < 7; i++) {
			if (!digits[i])
				digits[i] = 16;		// "Don't draw" code
			else
				break;
		}
	}
	
	segment_color = style().foreground;

	// Draw
	digit_x = rect.pos.x;		// 7 * 32 + 8 = 232 (4 px margins)
	for (i = 0; i < 7; i++) {
		digit = digits[i];
		digit_y = rect.pos.y;
		if (digit < 16) {
			digit_def = big_segment_font[(uint8_t)digit];
			if (digit_def & 0x01) painter.fill_rectangle({{digit_x + 4, 	digit_y}, 		{20, 4}}, 	segment_color);
			if (digit_def & 0x02) painter.fill_rectangle({{digit_x + 24, 	digit_y + 4}, 	{4, 20}}, 	segment_color);
			if (digit_def & 0x04) painter.fill_rectangle({{digit_x + 24, 	digit_y + 28}, 	{4, 20}}, 	segment_color);
			if (digit_def & 0x08) painter.fill_rectangle({{digit_x + 4, 	digit_y + 48}, 	{20, 4}}, 	segment_color);
			if (digit_def & 0x10) painter.fill_rectangle({{digit_x, 		digit_y + 28}, 	{4, 20}}, 	segment_color);
			if (digit_def & 0x20) painter.fill_rectangle({{digit_x, 		digit_y + 4}, 	{4, 20}}, 	segment_color);
			if (digit_def & 0x40) painter.fill_rectangle({{digit_x + 4, 	digit_y + 24}, 	{20, 4}}, 	segment_color);
		}
		if (i == 3) {
			// Dot
			painter.fill_rectangle({{digit_x + 34, digit_y + 48}, {4, 4}}, segment_color);
			digit_x += 40;
		} else {
			digit_x += 32;
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
	_value = 0;
	_max = max;
	set_dirty();
}

void ProgressBar::set_value(const uint32_t value) {
	if (value > _max)
		_value = _max;
	else
		_value = value;
	set_dirty();
}

void ProgressBar::paint(Painter& painter) {
	uint16_t v_sized;
	
	const auto rect = screen_rect();
	const auto s = style();

	v_sized = (rect.size.w * _value) / _max;

	painter.fill_rectangle({rect.pos, {v_sized, rect.size.h}}, style().foreground);
	painter.fill_rectangle({{rect.pos.x + v_sized, rect.pos.y}, {rect.size.w - v_sized, rect.size.h}}, s.background);
	
	painter.draw_rectangle(rect, s.foreground);
}

/* Console ***************************************************************/

Console::Console(
	Rect parent_rect
) : Widget { parent_rect }
{
	display.scroll_set_position(0);
}

void Console::clear() {
	display.fill_rectangle(
		screen_rect(),
		Color::black()
	);
	pos = { 0, 0 };
}

void Console::write(std::string message) {
	if (visible) {
		const Style& s = style();
		const Font& font = s.font;
		const auto rect = screen_rect();
		for(const auto c : message) {
			if( c == '\n' ) {
				crlf();
			} else {
				const auto glyph = font.glyph(c);
				const auto advance = glyph.advance();
				if( (pos.x + advance.x) > rect.width() ) {
					crlf();
				}
				const Point pos_glyph {
					rect.pos.x + pos.x,
					display.scroll_area_y(pos.y)
				};
				display.draw_glyph(pos_glyph, glyph, s.foreground, s.background);
				pos.x += advance.x;
			}
		}
		buffer = message;
	} else {
		buffer += message;
	}
}

void Console::writeln(std::string message) {
	write(message);
	crlf();
}

void Console::paint(Painter& painter) {
	(void)painter;
	write(buffer);
}

void Console::on_show() {
	const auto screen_r = screen_rect();
	display.scroll_set_area(screen_r.top(), screen_r.bottom());
	clear();
	visible = true;
}

void Console::on_hide() {
	/* TODO: Clear region to eliminate brief flash of content at un-shifted
	 * position?
	 */
	display.scroll_disable();
}

void Console::crlf() {
	const Style& s = style();
	const auto sr = screen_rect();
	const auto line_height = s.font.line_height();
	pos.x = 0;
	pos.y += line_height;
	const int32_t y_excess = pos.y + line_height - sr.height();
	if( y_excess > 0 ) {
		display.scroll(-y_excess);
		pos.y -= y_excess;

		const Rect dirty { sr.left(), display.scroll_area_y(pos.y), sr.width(), line_height };
		display.fill_rectangle(dirty, s.background);
	}
}

/* Checkbox **************************************************************/

Checkbox::Checkbox(
	Point parent_pos,
	size_t length,
	std::string text
) : Widget { { parent_pos, { static_cast<ui::Dim>((8 * length) + 24), 24 } } },
	text_ { text }
{
	set_focusable(true);
}

void Checkbox::set_text(const std::string value) {
	text_ = value;
	set_dirty();
}

/*std::string Checkbox::text() const {
	return text_;
}*/

void Checkbox::set_value(const bool value) {
	value_ = value;
	set_dirty();
}

bool Checkbox::value() const {
	return value_;
}

void Checkbox::paint(Painter& painter) {
	const auto r = screen_rect();
	
	const auto paint_style = (has_focus() || highlighted()) ? style().invert() : style();
	
	painter.draw_rectangle({ r.pos.x, r.pos.y, 24, 24 }, style().foreground);

	painter.fill_rectangle(
		{
			static_cast<Coord>(r.pos.x + 1), static_cast<Coord>(r.pos.y + 1),
			static_cast<Dim>(24 - 2), static_cast<Dim>(24 - 2)
		},
		style().background
	);
	
	painter.draw_rectangle({ r.pos.x+2, r.pos.y+2, 24-4, 24-4 }, paint_style.background);
	
	if (value_ == true) {
		// Check
		portapack::display.draw_line( {r.pos.x+2, r.pos.y+14}, {r.pos.x+6, r.pos.y+18}, ui::Color::green());
		portapack::display.draw_line( {r.pos.x+6, r.pos.y+18}, {r.pos.x+20, r.pos.y+4}, ui::Color::green());
	} else {
		// Cross
		portapack::display.draw_line( {r.pos.x+1, r.pos.y+1}, {r.pos.x+24-2, r.pos.y+24-2}, ui::Color::red());
		portapack::display.draw_line( {r.pos.x+24-2, r.pos.y+1}, {r.pos.x+1, r.pos.y+24-2}, ui::Color::red());
	}
	
	const auto label_r = paint_style.font.size_of(text_);
	painter.draw_string(
		{
			static_cast<Coord>(r.pos.x + 24 + 4),
			static_cast<Coord>(r.pos.y + (24 - label_r.h) / 2)
		},
		paint_style,
		text_
	);
}

bool Checkbox::on_key(const KeyEvent key) {
	if( key == KeyEvent::Select ) {
		value_ = not value_;
		set_dirty();
		
		if( on_select ) {
			on_select(*this);
			return true;
		}
	}

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
			on_select(*this);
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
	
	painter.draw_rectangle({r.pos, {r.size.w, 1}}, Color::light_grey());
	painter.draw_rectangle({r.pos.x, r.pos.y + r.size.h - 1, r.size.w, 1}, Color::dark_grey());
	painter.draw_rectangle({r.pos.x + r.size.w - 1, r.pos.y, 1, r.size.h}, Color::dark_grey());

	painter.fill_rectangle(
		{ r.pos.x, r.pos.y + 1, r.size.w - 1, r.size.h - 2 },
		paint_style.background
	);

	const auto label_r = paint_style.font.size_of(text_);
	painter.draw_string(
		{ r.pos.x + (r.size.w - label_r.w) / 2, r.pos.y + (r.size.h - label_r.h) / 2 },
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
			on_dir(*this, key);
			return false;
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
	Rect parent_rect,
	options_t options
) : Widget { parent_rect },
	options { options }
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
	const auto paint_style = has_focus() ? style().invert() : style();

	if( selected_index() < options.size() ) {
		const auto bmp_ptr = options[selected_index()].first;
		painter.fill_rectangle({screen_rect().pos, {screen_rect().size.w + 4, screen_rect().size.h + 4}}, ui::Color::black());
		painter.draw_rectangle({screen_rect().pos, {screen_rect().size.w + 4, screen_rect().size.h + 4}}, paint_style.background);
		portapack::display.drawBMP({screen_pos().x + 2, screen_pos().y + 1}, bmp_ptr, true);
	}
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
	size_t length,
	options_t options
) : Widget { { parent_pos, { static_cast<ui::Dim>(8 * length), 16 } } },
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
	
	painter.fill_rectangle({screen_rect().pos, {(int)length_ * 8, 16}}, ui::Color::black());
	
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
	size_t length,
	range_t range,
	int32_t step,
	char fill_char
) : Widget { { parent_pos, { static_cast<ui::Dim>(8 * length), 16 } } },
	range { range },
	step { step },
	length_ { length },
	fill_char { fill_char }
{
	set_focusable(true);
}

int32_t NumberField::value() const {
	return value_;
}

void NumberField::set_value(int32_t new_value, bool trigger_change) {
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
	size_t length
) : Widget { { parent_pos, { static_cast<ui::Dim>(8 * length), 16 } } },
	length_ { length }
{
	set_focusable(true);
}

SymField::SymField(
	Point parent_pos,
	size_t length,
	bool hex
) : Widget { { parent_pos, { static_cast<ui::Dim>(8 * length), 16 } } },
	length_ { length },
	hex_ { hex }
{
	uint8_t c;
	
	// Hex field auto-init
	for (c = 0; c < length; c++)
		set_symbol_list(c, "0123456789ABCDEF");
	
	set_focusable(true);
}

uint64_t SymField::value_hex_u64() {
	uint8_t c;
	uint64_t v = 0;
	
	if (hex_) {
		for (c = 0; c < length_; c++)
			v += values_[c] << (4 * (length_ - 1 - c));
		return v;
	} else 
		return 0;
}
	
uint32_t SymField::value(const uint32_t index) {
	if (index >= length_) return 0;

	return values_[index];
}

void SymField::set_value(const uint32_t index, const uint32_t new_value) {
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
			set_value(n, values_[n]);
		
		erase_prev_ = true;
		set_dirty();
	}
}

void SymField::set_symbol_list(const uint32_t index, const std::string symbol_list) {
	if (index >= length_) return;
	
	symbol_list_[index] = symbol_list;

	// Re-clip symbol's value
	set_value(index, values_[index]);
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
		
		pt_draw.x += 8;
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
		set_value(selected_, values_[selected_] + delta);
	
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

} /* namespace ui */
