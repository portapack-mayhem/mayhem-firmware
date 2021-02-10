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
#include "portapack_persistent_memory.hpp"

#include "sd_card.hpp"
#include "rtc_time.hpp"

#include "message.hpp"
#include "message_queue.hpp"

#include "irq_controls.hpp"

#include "buffer_exchange.hpp"

#include "ch.h"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <array>

#include "ui_font_fixed_8x16.hpp"

extern "C" {

CH_IRQ_HANDLER(M4Core_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	BufferExchange::handle_isr();
	EventDispatcher::check_fifo_isr();
	chSysUnlockFromIsr();

	creg::m4txevent::clear();

	CH_IRQ_EPILOGUE();
}

}

class MessageHandlerMap {
public:
	using MessageHandler = std::function<void(Message* const p)>;

	void register_handler(const Message::ID id, MessageHandler&& handler) {
		if( map_[toUType(id)] != nullptr ) {
			chDbgPanic("MsgDblReg");
		}
		map_[toUType(id)] = std::move(handler);
	}

	void unregister_handler(const Message::ID id) {
		map_[toUType(id)] = nullptr;
	}

	void send(Message* const message) {
		if( message->id < Message::ID::MAX ) {
			auto& fn = map_[toUType(message->id)];
			if( fn ) {
				fn(message);
			}
		}
	}

private:
	using MapType = std::array<MessageHandler, toUType(Message::ID::MAX)>;
	MapType map_ { };
};

static MessageHandlerMap message_map;
Thread* EventDispatcher::thread_event_loop = nullptr;
bool EventDispatcher::is_running = false;
bool EventDispatcher::display_sleep = false;

EventDispatcher::EventDispatcher(
	ui::Widget* const top_widget,
	ui::Context& context
) : top_widget { top_widget },
	painter { },
	context(context)
{
	init_message_queues();

	thread_event_loop = chThdSelf();
	is_running = true;
	touch_manager.on_event = [this](const ui::TouchEvent event) {
		this->on_touch_event(event);
	};
}

void EventDispatcher::run() {
	while(is_running) {
		const auto events = wait();
		dispatch(events);
	}
}

void EventDispatcher::request_stop() {
	is_running = false;
}

void EventDispatcher::set_display_sleep(const bool sleep) {
	// TODO: Distribute display sleep message more broadly, shut down data generation
	// on baseband side, since all that data is being discarded during sleep.
	if( sleep ) {
		portapack::backlight()->off();
		portapack::display.sleep();
	} else {
		portapack::display.wake();
		// Don't turn on backlight here.
		// Let frame sync handler turn on backlight after repaint.
	}
	EventDispatcher::display_sleep = sleep;
};

eventmask_t EventDispatcher::wait() {
	return chEvtWaitAny(ALL_EVENTS);
}

void EventDispatcher::dispatch(const eventmask_t events) {
	if( shared_memory.m4_panic_msg[0] != 0 ) {
		halt = true;
	}

	if( halt ) {
		if( shared_memory.m4_panic_msg[0] != 0 ) {
			painter.fill_rectangle(
				{ 0, 0, portapack::display.width(), portapack::display.height() },
				ui::Color::red()
			);

			constexpr int border = 8;
			painter.fill_rectangle(
				{ border, border, portapack::display.width() - (border * 2), portapack::display.height() - (border * 2) },
				ui::Color::black()
			);

			painter.draw_string({ 48, 24 }, top_widget->style(), "M4 Guru Meditation");

			shared_memory.m4_panic_msg[sizeof(shared_memory.m4_panic_msg) - 1] = 0;
			const std::string message = shared_memory.m4_panic_msg;
			const int x_offset = (portapack::display.width() - (message.size() * 8)) / 2;
			constexpr int y_offset = (portapack::display.height() - 16) / 2;
			painter.draw_string(
				{ x_offset, y_offset },
				top_widget->style(),
				message
			);

			shared_memory.m4_panic_msg[0] = 0;
		}
		return;
	}

	if( events & EVT_MASK_APPLICATION ) {
		handle_application_queue();
	}

	if( events & EVT_MASK_LOCAL ) {
		handle_local_queue();
	}

	if( events & EVT_MASK_RTC_TICK ) {
		handle_rtc_tick();
	}
	
	if( events & EVT_MASK_SWITCHES ) {
		handle_switches();
	}
	
	/*if( events & EVT_MASK_LCD_FRAME_SYNC ) {
		blink_timer();
	}*/

	if( !EventDispatcher::display_sleep ) {
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
		message_map.send(message);
	});
}

void EventDispatcher::handle_local_queue() {
	shared_memory.app_local_queue.handle([](Message* const message) {
		message_map.send(message);
	});
}

void EventDispatcher::handle_rtc_tick() {
	sd_card::poll_inserted();

	portapack::temperature_logger.second_tick();
	
	uint32_t backlight_timer = portapack::persistent_memory::config_backlight_timer();
	if (backlight_timer) {
		if (portapack::bl_tick_counter == backlight_timer)
			set_display_sleep(true);
		else
			portapack::bl_tick_counter++;
	}

	rtc_time::on_tick_second();
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
	message_map.send(&message);
	painter.paint_widget_tree(top_widget);

	portapack::backlight()->on();
}

void EventDispatcher::handle_switches() {
	const auto switches_state = get_switches_state();

	portapack::bl_tick_counter = 0;

	if( switches_state.count() == 0 ) {
		// If all keys are released, we are no longer in a key event.
		in_key_event = false;
	}

	if( in_key_event ) {
		// If we're in a key event, return. We will ignore all additional key
		// presses until the first key is released. We also want to ignore events
		// where the last key held generates a key event when other pressed keys
		// are released.
		return;
	}

	if( EventDispatcher::display_sleep ) {
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

			in_key_event = true;
		}
	}
}

void EventDispatcher::handle_encoder() {
	portapack::bl_tick_counter = 0;
	
	if( EventDispatcher::display_sleep ) {
		// Swallow event, wake up display.
		set_display_sleep(false);
		return;
	}
	
	const uint32_t encoder_now = get_encoder_position();
	const int32_t delta = static_cast<int32_t>(encoder_now - encoder_last);
	encoder_last = encoder_now;
	const auto event = static_cast<ui::EncoderEvent>(delta);
	event_bubble_encoder(event);
}

void EventDispatcher::handle_touch() {
	portapack::bl_tick_counter = 0;

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
	new (&shared_memory) SharedMemory;
}

MessageHandlerRegistration::MessageHandlerRegistration(
	const Message::ID message_id,
	MessageHandlerMap::MessageHandler&& callback
) : message_id { message_id }
{
	message_map.register_handler(message_id, std::move(callback));
}

MessageHandlerRegistration::~MessageHandlerRegistration() {
	message_map.unregister_handler(message_id);
}
