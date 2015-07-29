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
			// New widget is not focusable. Does it have a preferred child that
			// can receive focus?
			set_focus_widget(new_focus_widget->last_child_focus());
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
		focus_widget()->parent()->set_last_child_focus(focus_widget());
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

void FocusManager::update(
	Widget* const top_widget,
	const KeyEvent event
) {
	if( focus_widget() ) {
		const auto focus_screen_rect = focus_widget()->screen_rect();
		const auto center = focus_screen_rect.center();

		const auto test_fn = [&center, event](ui::Widget* const w) -> test_result_t {
			// if( w->visible() && w->focusable() ) {
			if( w->focusable() ) {
				const Point delta = w->screen_rect().center() - center;

				/* Heuristic to compute closeness. */
				/* TODO: Look at metric involving overlap of current
				 * widget rectangle in the direction of movement, with
				 * all the prospective widgets.
				 */
				switch(event) {
				case KeyEvent::Right:
					if( delta.x > 0 ) {
						if( delta.y == 0 ) {
							return { w, delta.x };
						} else {
							return { w, delta.x * abs(delta.y) + 1000 };
						}
					}
					break;

				case KeyEvent::Left:
					if( delta.x < 0 ) {
						if( delta.y == 0 ) {
							return { w, -delta.x };
						} else {
							return { w, -delta.x * abs(delta.y) + 1000 };
						}
					}
					break;

				case KeyEvent::Down:
					if( delta.y > 0 ) {
						if( delta.x == 0 ) {
							return { w, delta.y };
						} else {
							return { w, delta.y * abs(delta.x) + 1000 };
						}
					}
					break;

				case KeyEvent::Up:
					if( delta.y < 0 ) {
						if( delta.x == 0 ) {
							return { w, -delta.y };
						} else {
							return { w, -delta.y * abs(delta.x) + 1000 };
						}
					}
					break;

				default:
					break;
				}
			}

			return { nullptr, 0 };
		};

		test_collection_t collection;
		widget_collect_visible(top_widget, test_fn, collection);

		const auto compare_fn = [](const test_result_t& a, const test_result_t& b) {
			return a.second < b.second;
		};

		const auto nearest = std::min_element(collection.cbegin(), collection.cend(), compare_fn);
		if( nearest != collection.cend() ) {
			//focus->blur();
			const auto new_focus = (*nearest).first;
			set_focus_widget(new_focus);
		}
	}
}
#if 0
void FocusManager::update(
	Widget* const top_widget,
	const TouchEvent event
) {
	const auto test_fn = [event](Widget* const w) -> test_result_t {
		if( w->focusable() ) {
			const auto r = w->screen_rect();
			if( r.contains(event) ) {
				return { w, 0 };
			}
		}

		return { nullptr, { } };
	};

	test_collection_t collection;
	widget_collect_visible(
		top_widget, test_fn,
		collection
	);

	if( !collection.empty() ) {
		// Take the last object in the collection, it will be rendered last,
		// therefore appear "on top".
		const auto touched = collection.back().first;
		touched->on_touch(event);
	}
}
#endif
} /* namespace ui */
