/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_focus.hpp"
#include "ui_widget.hpp"

#include <cstdint>

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace ui {

Widget* FocusManager::focus_widget() const {
	return focus_widget_;
}

void FocusManager::set_focus_widget(Widget* const new_focus_widget) {
	// Widget already has focus.
	if( new_focus_widget == focus_widget() ) {
		return;
	}

	if( new_focus_widget ) {
		// if( !new_focus_widget->visible() ) {
		// if( new_focus_widget->hidden() ) {
		// 	// New widget is not visible. Do nothing.
		// 	// TODO: Should this be a debug assertion?
		// 	return;
		// }
		if( !new_focus_widget->focusable() ) {
			return;
		}
	}

	// Blur old widget.
	if( focus_widget() ) {
		focus_widget()->on_blur();
		focus_widget()->set_dirty();
	}

	focus_widget_ = new_focus_widget;

	if( focus_widget() ) {
		focus_widget()->on_focus();
		focus_widget()->set_dirty();
	}
}

using test_result_t = std::pair<Widget* const, const uint32_t>;
using test_fn = std::function<test_result_t(Widget* const)>;
using test_collection_t = std::vector<test_result_t>;

/* Walk all visible widgets in hierarchy, collecting those that pass test */
template<typename TestFn>
static void widget_collect_visible(Widget* const w, TestFn test, test_collection_t& collection) {
	for(auto child : w->children()) {
		if( !child->hidden() ) {
			const auto result = test(child);
			if( result.first ) {
				collection.push_back(result);
			}
			widget_collect_visible(child, test, collection);
		}
	}
}

static int32_t rect_distances(
	const KeyEvent direction,
	const Rect& rect_start,
	const Rect& rect_end
) {
	Coord on_axis_max, on_axis_min;

	switch(direction) {
	case KeyEvent::Right:
		on_axis_max = rect_end.left();
		on_axis_min = rect_start.right();
		break;

	case KeyEvent::Left:
		on_axis_max = rect_start.left();
		on_axis_min = rect_end.right();
		break;

	case KeyEvent::Down:
		on_axis_max = rect_end.top();
		on_axis_min = rect_start.bottom();
		break;

	case KeyEvent::Up:
		on_axis_max = rect_start.top();
		on_axis_min = rect_end.bottom();
		break;

	default:
		return -1;
	}

	Coord on_axis_distance = on_axis_max - on_axis_min;
	if( on_axis_distance < 0 ) {
		return -1;
	}

	Coord perpendicular_axis_start, perpendicular_axis_end;

	switch(direction) {
	case KeyEvent::Right:
	case KeyEvent::Left:
		perpendicular_axis_start = rect_start.center().y();
		perpendicular_axis_end = rect_end.center().y();
		break;

	case KeyEvent::Up:
	case KeyEvent::Down:
		perpendicular_axis_start = rect_start.center().x();
		perpendicular_axis_end = rect_end.center().x();
		break;

	default:
		return -1;
	}


	switch(direction) {
	case KeyEvent::Right:
	case KeyEvent::Left:
		return ((std::abs(perpendicular_axis_end - perpendicular_axis_start) + 1) ^ 3) * sqrt((on_axis_distance + 1));
		break;

	case KeyEvent::Up:
	case KeyEvent::Down:
		return (sqrt(std::abs(perpendicular_axis_end - perpendicular_axis_start) + 1)) * ((on_axis_distance + 1) ^ 3);
		break;

	default:
		return 0;
	}

	
}

void FocusManager::update(
	Widget* const top_widget,
	const KeyEvent event
) {
	if( focus_widget() ) {
		const auto focus_screen_rect = focus_widget()->screen_rect();

		const auto test_fn = [&focus_screen_rect, event](ui::Widget* const w) -> test_result_t {
			// if( w->visible() && w->focusable() ) {
			if( w->focusable() ) {
				const auto distance = rect_distances(event, focus_screen_rect, w->screen_rect());
				if( distance >= 0 ) {
					return { w, distance };
				}
			}

			return { nullptr, 0 };
		};
		
		const auto find_back_fn = [](ui::Widget* const w) -> test_result_t {
			if( w->focusable() && (w->id == -1) )
				return { w, 0 };
			else
				return { nullptr, 0 };
		};

		test_collection_t collection;
		widget_collect_visible(top_widget, test_fn, collection);

		const auto compare_fn = [](const test_result_t& a, const test_result_t& b) {
			return a.second < b.second;
		};

		const auto nearest = std::min_element(collection.cbegin(), collection.cend(), compare_fn);
		// Up and left to indicate back
		if (event == KeyEvent::Back) {
			collection.clear();
			widget_collect_visible(top_widget, find_back_fn, collection);
			if (!collection.empty())
				set_focus_widget(collection[0].first);
		}
		else if( nearest != collection.cend() ) {
			//focus->blur();
			const auto new_focus = (*nearest).first;
			set_focus_widget(new_focus);
		} else {
			if ((focus_widget()->id >= 0) && (event == KeyEvent::Left)) {
				// Stuck left, move to back button
				collection.clear();
				widget_collect_visible(top_widget, find_back_fn, collection);
				if (!collection.empty())
					set_focus_widget(collection[0].first);
			}
		}
	}
}

} /* namespace ui */
