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

#include "ui_widget.hpp"
#include "ui_painter.hpp"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "message.hpp"

#include "touch.hpp"

#include "ch.h"

#include <cstdint>

constexpr auto EVT_MASK_SWITCHES		= EVENT_MASK(3);
constexpr auto EVT_MASK_ENCODER			= EVENT_MASK(4);
constexpr auto EVT_MASK_TOUCH			= EVENT_MASK(5);

class EventDispatcher {
public:
	EventDispatcher(
		ui::Widget* const top_widget,
		ui::Context& context
	);

	void run();
	void request_stop();

	void set_display_sleep(const bool sleep);

	static inline void check_fifo_isr() {
		if( !shared_memory.application_queue.is_empty() ) {
			events_flag_isr(EVT_MASK_APPLICATION);
		}
	}

	static inline void event_isr_rtc_tick() {
		events_flag_isr(EVT_MASK_RTC_TICK);
	}

	static inline void event_isr_lcd_frame_sync() {
		events_flag_isr(EVT_MASK_LCD_FRAME_SYNC);
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

	template<typename T>
	static void send_message(T& message) {
		shared_memory.app_local_queue.push(message);
		events_flag(EVT_MASK_LOCAL);
	}

private:
	static constexpr auto EVT_MASK_RTC_TICK       = EVENT_MASK(0);
	static constexpr auto EVT_MASK_LCD_FRAME_SYNC = EVENT_MASK(1);
	static constexpr auto EVT_MASK_APPLICATION    = EVENT_MASK(6);
	static constexpr auto EVT_MASK_LOCAL          = EVENT_MASK(7);

	static Thread* thread_event_loop;

	touch::Manager touch_manager;
	ui::Widget* const top_widget;
	ui::Painter painter;
	ui::Context& context;
	uint32_t encoder_last = 0;
	bool is_running = true;
	bool sd_card_present = false;
	bool display_sleep = false;
	bool halt = false;

	eventmask_t wait();
	void dispatch(const eventmask_t events);

	void handle_application_queue();
	void handle_local_queue();
	void handle_rtc_tick();

	static ui::Widget* touch_widget(ui::Widget* const w, ui::TouchEvent event);

	ui::Widget* captured_widget { nullptr };

	void on_touch_event(ui::TouchEvent event);

	void handle_lcd_frame_sync();
	void handle_switches();
	void handle_encoder();
	void handle_touch();

	bool event_bubble_key(const ui::KeyEvent event);
	void event_bubble_encoder(const ui::EncoderEvent event);

	void init_message_queues();
};

class MessageHandlerRegistration {
public:
	MessageHandlerRegistration(
		const Message::ID message_id,
		std::function<void(Message* const p)>&& callback
	);

	~MessageHandlerRegistration();
	
private:
	const Message::ID message_id;
};

#endif/*__EVENT_M0_H__*/
