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
			parent()->dirty_overlapping_children_in_rect(parent_rect);
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

void Rectangle::paint(Painter& painter) {
	painter.fill_rectangle(
		screen_rect(),
		color
	);
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

/* ProgressBar ***********************************************************/

ProgressBar::ProgressBar(
	Rect parent_rect
) : Widget { parent_rect }
{
}

void ProgressBar::set_value(const uint16_t value) {
	if (value > 100)
		_value = 100;
	else
		_value = value;
	set_dirty();
}

void ProgressBar::paint(Painter& painter) {
	uint16_t v_sized;
	
	const auto rect = screen_rect();
	const auto s = style();

	v_sized = (rect.size.w * _value) / 100;

	painter.fill_rectangle({rect.pos, {v_sized, rect.size.h}}, ui::Color::green());
	painter.fill_rectangle({{rect.pos.x + v_sized, rect.pos.y}, {rect.size.w - v_sized, rect.size.h}}, s.background);
	
	painter.draw_rectangle(rect, s.foreground);
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

std::string Checkbox::text() const {
	return text_;
}

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
	const auto r = screen_rect();

	const auto paint_style = (has_focus() || highlighted()) ? style().invert() : style();

	painter.draw_rectangle(r, style().foreground);

	painter.fill_rectangle(
		{ r.pos.x + 1, r.pos.y + 1, r.size.w - 2, r.size.h - 2 },
		paint_style.background
	);

    //char *token = strtok(text_.c_str(), "\n");
    //while(token) {
		const auto label_r = paint_style.font.size_of(text_);
		painter.draw_string(
			{ r.pos.x + (r.size.w - label_r.w) / 2, r.pos.y + (r.size.h - label_r.h) / 2 },
			paint_style,
			text_
		);
    //    token = strtok(NULL, " ");
    //}
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

void NumberField::set_value(int32_t new_value) {
	new_value = clip_value(new_value);

	if( new_value != value() ) {
		value_ = new_value;
		if( on_change ) {
			on_change(value_);
		}
		set_dirty();
	}
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

} /* namespace ui */
