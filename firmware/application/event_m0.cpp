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

#include "event_m0.hpp"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "sd_card.hpp"

#include "message.hpp"
#include "message_queue.hpp"

#include "irq_controls.hpp"

#include "audio_thread.hpp"

#include "ch.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <array>

extern "C" {

CH_IRQ_HANDLER(M4Core_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	AudioThread::check_fifo_isr();
	EventDispatcher::events_flag_isr(EVT_MASK_APPLICATION);
	chSysUnlockFromIsr();

	creg::m4txevent::clear();

	CH_IRQ_EPILOGUE();
}

}

MessageHandlerMap EventDispatcher::message_map_;
Thread* EventDispatcher::thread_event_loop = nullptr;
Thread* EventDispatcher::thread_record = nullptr;

EventDispatcher::EventDispatcher(
	ui::Widget* const top_widget,
	ui::Painter& painter,
	ui::Context& context
) : top_widget { top_widget },
	painter(painter),
	context(context)
{
	init_message_queues();

	thread_event_loop = chThdSelf();
	touch_manager.on_event = [this](const ui::TouchEvent event) {
		this->on_touch_event(event);
	};
}

void EventDispatcher::run() {
	creg::m4txevent::enable();

	while(is_running) {
		const auto events = wait();
		dispatch(events);
	}

	creg::m4txevent::disable();
}

void EventDispatcher::request_stop() {
	is_running = false;
}

void EventDispatcher::set_display_sleep(const bool sleep) {
	// TODO: Distribute display sleep message more broadly, shut down data generation
	// on baseband side, since all that data is being discarded during sleep.
	if( sleep ) {
		portapack::io.lcd_backlight(false);
		portapack::display.sleep();
	} else {
		portapack::display.wake();
		portapack::io.lcd_backlight(true);
	}
	display_sleep = sleep;
};

eventmask_t EventDispatcher::wait() {
	return chEvtWaitAny(ALL_EVENTS);
}

void EventDispatcher::dispatch(const eventmask_t events) {
	if( events & EVT_MASK_APPLICATION ) {
		handle_application_queue();
	}

	if( events & EVT_MASK_RTC_TICK ) {
		handle_rtc_tick();
	}
	
	if( events & EVT_MASK_SWITCHES ) {
		handle_switches();
	}

	if( !display_sleep ) {
		if( events & EVT_MASK_LCD_FRAME_SYNC ) {
			handle_lcd_frame_sync();
		}

		if( events & EVT_MASK_ENCODER ) {
			handle_encoder();
		}

		if( events & EVT_MASK_TOUCH ) {
			handle_touch();
		}
	}
}

void EventDispatcher::handle_application_queue() {
	shared_memory.application_queue.handle([](Message* const message) {
		message_map().send(message);
	});
}

void EventDispatcher::handle_rtc_tick() {
	sd_card::poll_inserted();

	portapack::temperature_logger.second_tick();
}

ui::Widget* EventDispatcher::touch_widget(ui::Widget* const w, ui::TouchEvent event) {
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

void EventDispatcher::on_touch_event(ui::TouchEvent event) {
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

void EventDispatcher::handle_lcd_frame_sync() {
	DisplayFrameSyncMessage message;
	message_map().send(&message);
	painter.paint_widget_tree(top_widget);
}

void EventDispatcher::handle_switches() {
	const auto switches_state = get_switches_state();

	if( display_sleep ) {
		// Swallow event, wake up display.
		if( switches_state.any() ) {
			set_display_sleep(false);
		}
		return;
	}

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

void EventDispatcher::handle_encoder() {
	const uint32_t encoder_now = get_encoder_position();
	const int32_t delta = static_cast<int32_t>(encoder_now - encoder_last);
	encoder_last = encoder_now;
	const auto event = static_cast<ui::EncoderEvent>(delta);
	event_bubble_encoder(event);
}

void EventDispatcher::handle_touch() {
	touch_manager.feed(get_touch_frame());
}

bool EventDispatcher::event_bubble_key(const ui::KeyEvent event) {
	auto target = context.focus_manager().focus_widget();
	while( (target != nullptr) && !target->on_key(event) ) {
		target = target->parent();
	}

	/* Return true if event was consumed. */
	return (target != nullptr);
}

void EventDispatcher::event_bubble_encoder(const ui::EncoderEvent event) {
	auto target = context.focus_manager().focus_widget();
	while( (target != nullptr) && !target->on_encoder(event) ) {
		target = target->parent();
	}
}

void EventDispatcher::init_message_queues() {
	new (&shared_memory.baseband_queue) MessageQueue(
		shared_memory.baseband_queue_data, SharedMemory::baseband_queue_k
	);
	new (&shared_memory.application_queue) MessageQueue(
		shared_memory.application_queue_data, SharedMemory::application_queue_k
	);

	shared_memory.FIFO_HACK = nullptr;
}
