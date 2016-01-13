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

#ifndef __EVENT_M0_H__
#define __EVENT_M0_H__

#include "event.hpp"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "ui_widget.hpp"
#include "ui_painter.hpp"

#include "touch.hpp"

#include "irq_lcd_frame.hpp"
#include "irq_controls.hpp"
#include "irq_rtc.hpp"

#include "message.hpp"
#include "message_queue.hpp"

#include "sd_card.hpp"

#include "ch.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstdint>
#include <array>

constexpr auto EVT_MASK_RTC_TICK		= EVENT_MASK(0);
constexpr auto EVT_MASK_LCD_FRAME_SYNC	= EVENT_MASK(1);
constexpr auto EVT_MASK_SWITCHES		= EVENT_MASK(3);
constexpr auto EVT_MASK_ENCODER			= EVENT_MASK(4);
constexpr auto EVT_MASK_TOUCH			= EVENT_MASK(5);
constexpr auto EVT_MASK_APPLICATION		= EVENT_MASK(6);

class EventDispatcher {
public:
	EventDispatcher(
		ui::Widget* const top_widget,
		ui::Painter& painter,
		ui::Context& context
	) : top_widget { top_widget },
		painter(painter),
		context(context)
	{
		thread_event_loop = chThdSelf();
		touch_manager.on_event = [this](const ui::TouchEvent event) {
			this->on_touch_event(event);
		};
	}

	void run() {
		creg::m4txevent::enable();

		while(is_running) {
			const auto events = wait();
			dispatch(events);
		}

		creg::m4txevent::disable();
	}

	void request_stop() {
		is_running = false;
	}

	static inline void events_flag(const eventmask_t events) {
		if( thread_event_loop ) {
			chEvtSignal(thread_event_loop, events);
		}
	}

	static inline void events_flag_isr(const eventmask_t events) {
		if( thread_event_loop ) {
			chEvtSignalI(thread_event_loop, events);
		}
	}

private:
	static Thread* thread_event_loop;

	touch::Manager touch_manager;
	ui::Widget* const top_widget;
	ui::Painter& painter;
	ui::Context& context;
	uint32_t encoder_last = 0;
	bool is_running = true;
	bool sd_card_present = false;

	eventmask_t wait() {
		return chEvtWaitAny(ALL_EVENTS);
	}

	void dispatch(const eventmask_t events) {
		if( events & EVT_MASK_APPLICATION ) {
			handle_application_queue();
		}

		if( events & EVT_MASK_RTC_TICK ) {
			handle_rtc_tick();
		}

		if( events & EVT_MASK_LCD_FRAME_SYNC ) {
			handle_lcd_frame_sync();
		}

		if( events & EVT_MASK_SWITCHES ) {
			handle_switches();
		}

		if( events & EVT_MASK_ENCODER ) {
			handle_encoder();
		}

		if( events & EVT_MASK_TOUCH ) {
			handle_touch();
		}
	}

	void handle_application_queue() {
		std::array<uint8_t, Message::MAX_SIZE> message_buffer;
		while(Message* const message = shared_memory.application_queue.pop(message_buffer)) {
			context.message_map().send(message);
		}
	}

	void handle_rtc_tick() {
		sd_card::poll_inserted();

		portapack::temperature_logger.second_tick();
	}

	static ui::Widget* touch_widget(ui::Widget* const w, ui::TouchEvent event) {
		if( !w->hidden() ) {
			// To achieve reverse depth ordering (last object drawn is
			// considered "top"), descend first.
			for(const auto child : w->children()) {
				const auto touched_widget = touch_widget(child, event);
				if( touched_widget ) {
					return touched_widget;
				}
			}

			const auto r = w->screen_rect();
			if( r.contains(event.point) ) {
				if( w->on_touch(event) ) {
					// This widget responded. Return it up the call stack.
					return w;
				}
			}
		}
		return nullptr;
	}

	ui::Widget* captured_widget { nullptr };

	void on_touch_event(ui::TouchEvent event) {
		/* TODO: Capture widget receiving the Start event, send Move and
		 * End events to the same widget.
		 */
		/* Capture Start widget.
		 * If touch is over Start widget at Move event, then the widget
		 * should be highlighted. If the touch is not over the Start
		 * widget at Move event, widget should un-highlight.
		 * If touch is over Start widget at End event, then the widget
		 * action should occur.
		 */
		if( event.type == ui::TouchEvent::Type::Start ) {
			captured_widget = touch_widget(this->top_widget, event);
		}

		if( captured_widget ) {
			captured_widget->on_touch(event);
		}
	}

	void handle_lcd_frame_sync() {
		DisplayFrameSyncMessage message;
		context.message_map().send(&message);
		painter.paint_widget_tree(top_widget);
	}

	void handle_switches() {
		const auto switches_state = get_switches_state();
		for(size_t i=0; i<switches_state.size(); i++) {
			// TODO: Ignore multiple keys at the same time?
			if( switches_state[i] ) {
				const auto event = static_cast<ui::KeyEvent>(i);
				if( !event_bubble_key(event) ) {
					context.focus_manager().update(top_widget, event);
				}
			}
		}
	}

	void handle_encoder() {
		const uint32_t encoder_now = get_encoder_position();
		const int32_t delta = static_cast<int32_t>(encoder_now - encoder_last);
		encoder_last = encoder_now;
		const auto event = static_cast<ui::EncoderEvent>(delta);
		event_bubble_encoder(event);
	}

	void handle_touch() {
		touch_manager.feed(get_touch_frame());
	}

	bool event_bubble_key(const ui::KeyEvent event) {
		auto target = context.focus_manager().focus_widget();
		while( (target != nullptr) && !target->on_key(event) ) {
			target = target->parent();
		}

		/* Return true if event was consumed. */
		return (target != nullptr);
	}

	void event_bubble_encoder(const ui::EncoderEvent event) {
		auto target = context.focus_manager().focus_widget();
		while( (target != nullptr) && !target->on_encoder(event) ) {
			target = target->parent();
		}
	}
};

#endif/*__EVENT_M0_H__*/
