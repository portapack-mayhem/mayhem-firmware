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

#include "ch.h"
#include "test.h"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "cpld_update.hpp"

#include "message_queue.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"

#include "irq_ipc.hpp"
#include "irq_lcd_frame.hpp"
#include "irq_controls.hpp"
#include "irq_rtc.hpp"

#include "event.hpp"

#include "m4_startup.hpp"
#include "spi_image.hpp"

#include "debug.hpp"
#include "led.hpp"

#include "gcc.hpp"

#include <string.h>

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
		touch_manager.on_event = [this](const ui::TouchEvent event) {
			this->on_touch_event(event);
		};
	}

	void run() {
		while(true) {
			const auto events = wait();
			dispatch(events);
		}
	}

private:
	touch::Manager touch_manager;
	ui::Widget* const top_widget;
	ui::Painter& painter;
	ui::Context& context;
	uint32_t encoder_last = 0;

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

		if( events & EVT_MASK_SD_CARD_PRESENT ) {
			handle_sd_card_detect();
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
		while( !shared_memory.application_queue.is_empty() ) {
			std::array<uint8_t, Message::MAX_SIZE> message_buffer;
			const Message* const message = reinterpret_cast<Message*>(message_buffer.data());
			const auto message_size = shared_memory.application_queue.pop(message_buffer.data(), message_buffer.size());
			if( message_size ) {
				context.message_map.send(message);
			}
		}
	}

	void handle_rtc_tick() {

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
		painter.paint_widget_tree(top_widget);
	}

	void handle_sd_card_detect() {

	}

	void handle_switches() {
		const auto switches_state = get_switches_state();
		for(size_t i=0; i<switches_state.size(); i++) {
			// TODO: Ignore multiple keys at the same time?
			if( switches_state[i] ) {
				const auto event = static_cast<ui::KeyEvent>(i);
				if( !event_bubble_key(event) ) {
					context.focus_manager.update(top_widget, event);
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
		auto target = context.focus_manager.focus_widget();
		while( (target != nullptr) && !target->on_key(event) ) {
			target = target->parent();
		}

		/* Return true if event was consumed. */
		return (target != nullptr);
	}

	void event_bubble_encoder(const ui::EncoderEvent event) {
		auto target = context.focus_manager.focus_widget();
		while( (target != nullptr) && !target->on_encoder(event) ) {
			target = target->parent();
		}
	}
};

int main(void) {
	portapack::init();

	if( !cpld_update_if_necessary() ) {
		chSysHalt();
	}

	portapack::io.init();
	portapack::display.init();

	sdcStart(&SDCD1, nullptr);

	events_initialize(chThdSelf());
	init_message_queues();

	ui::Context context;
	ui::SystemView system_view {
		context,
		portapack::display.screen_rect()
	};
	ui::Painter painter;
	EventDispatcher event_dispatcher { &system_view, painter, context };

	m4_init(portapack::spi_flash::baseband, portapack::spi_flash::m4_text_ram_base);

	controls_init();
	lcd_frame_sync_configure();
	rtc_interrupt_enable();
	m4txevent_interrupt_enable();

	event_dispatcher.run();

	return 0;
}
