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

/* From ChibiOS crt0.c:
 * Two stacks available for Cortex-M, main stack or process stack.
 *
 * Thread mode:		Used to execute application software. The processor
 *					enters Thread mode when it comes out of reset.
 * Handler mode:	Used to handle exceptions. The processor returns to
 *					Thread mode when it has finished all exception processing.
 *
 * ChibiOS configures the Cortex-M in dual-stack mode. (CONTROL[1]=1)
 * When CONTROL[1]=1, PSP is used when the processor is in Thread mode.
 *
 * MSP is always used when the processor is in Handler mode.
 *
 * __main_stack_size__		:	0x2000???? - 0x2000???? =
 *		Used for exception handlers. Yes, really.
 * __process_stack_size__	:	0x2000???? - 0x2000???? =
 *		Used by main().
 *
 * After chSysInit(), the current instructions stream (usually main())
 * becomes the main thread.
 */

#if 0
static const SPIConfig ssp_config_w25q80bv = {
	.end_cb = NULL,
	.ssport = ?,
	.sspad = ?,
	.cr0 =
			CR0_CLOCKRATE()
		| ?
		| ?
		,
	.cpsr = ?,
};

static spi_bus_t ssp0 = {
	.obj = &SPID1,
	.config = &ssp_config_w25q80bv,
	.start = spi_chibi_start,
	.stop = spi_chibi_stop,
	.transfer = spi_chibi_transfer,
	.transfer_gather = spi_chibi_transfer_gather,
};
#endif

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
		// touch_manager.on_started = [this](const ui::TouchEvent event) {
		// 	this->context.focus_manager.update(this->top_widget, event);
		// };

		touch_manager.on_event = [this](const ui::TouchEvent event) {
			this->on_touch_event(event);
		};
	}

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

private:
	touch::Manager touch_manager;
	ui::Widget* const top_widget;
	ui::Painter& painter;
	ui::Context& context;
	uint32_t encoder_last = 0;

	void handle_application_queue() {
		while( !shared_memory.application_queue.is_empty() ) {
			auto message = shared_memory.application_queue.pop();

			context.message_map.send(message);

			message->state = Message::State::Free;
		}
	}

	void handle_rtc_tick() {
		/*
		if( shared_memory.application_queue.push(&rssi_request) ) {
			led_rx.on();
		}
		*/
		/*
		if( callback_second_tick ) {
			rtc::RTC datetime;
			rtcGetTime(&RTCD1, &datetime);

			callback_second_tick(datetime);
		}
		*/
		//static std::function<void(size_t app_n, size_t baseband_n)> callback_fifos_state;
		//static std::function<void(systime_t ticks)> callback_cpu_ticks;
		/*
		if( callback_fifos_state ) {
			callback_fifos_state(shared_memory.application_queue.len(), baseband_queue.len());
		}
		*/
		/*
		if( callback_cpu_ticks ) {
			//const auto thread_self = chThdSelf();
			const auto thread = chSysGetIdleThread();
			//const auto ticks = chThdGetTicks(thread);

			callback_cpu_ticks(thread->total_ticks);
		}
		*/

		/*
		callback_fifos_state = [&system_view](size_t app_n, size_t baseband_n) {
			system_view.status_view.text_app_fifo_n.set(
				ui::to_string_dec_uint(app_n, 3)
			);
			system_view.status_view.text_baseband_fifo_n.set(
				ui::to_string_dec_uint(baseband_n, 3)
			);
		};
		*/
		/*
		callback_cpu_ticks = [&system_view](systime_t ticks) {
			static systime_t last_ticks = 0;
			const auto delta_ticks = ticks - last_ticks;
			last_ticks = ticks;

			const auto text_pct = ui::to_string_dec_uint(delta_ticks / 2000000, 3) + "% idle";
			system_view.status_view.text_ticks.set(
				text_pct
			);
		};
		*/
	}
/*
	void paint_widget(ui::Widget* const w) {
		if( w->visible() ) {
			if( w->dirty() ) {
				w->paint(painter);
				// Force-paint all children.
				for(const auto child : w->children()) {
					child->set_dirty();
					paint_widget(child);
				}
				w->set_clean();
			} else {
				// Selectively paint all children.
				for(const auto child : w->children()) {
					paint_widget(child);
				}
			}
		}
	}
*/
	void paint_widget(ui::Widget* const w) {
		if( w->hidden() ) {
			// Mark widget (and all children) as invisible.
			w->visible(false);
		} else {
			// Mark this widget as visible and recurse.
			w->visible(true);

			if( w->dirty() ) {
				w->paint(painter);
				// Force-paint all children.
				for(const auto child : w->children()) {
					child->set_dirty();
					paint_widget(child);
				}
				w->set_clean();
			} else {
				// Selectively paint all children.
				for(const auto child : w->children()) {
					paint_widget(child);
				}
			}
		}
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
		if( ui::is_dirty() ) {
			paint_widget(top_widget);
			ui::dirty_clear();
		}
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

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

/* Thinking things through a bit:

	main() produces UI events.
		Touch events:
			Hit test entire screen hierarchy and send to hit widget.
			If modal view shown, block UI events destined outside.
		Navigation events:
			Move from current focus widget to "nearest" focusable widget.
			If current view is modal, don't allow events to bubble outside
			of modal view.
		System events:
			Power off from WWDT provides enough time to flush changes to
				VBAT RAM?
			SD card events? Insert/eject/error.


	View stack:
		Views that are hidden should deconstruct their widgets?
		Views that are shown after being hidden will reconstruct their
			widgets from data in their model?
		Hence, hidden views will not eat up memory beyond their model?
		Beware loops where the stack can get wildly deep?
		Breaking out data models from views should allow some amount of
			power-off persistence in the VBAT RAM area. In fact, the data
			models could be instantiated there? But then, how to protect
			from corruption if power is pulled? Does WWDT provide enough
			warning to flush changes?

	Navigation...
		If you move off the left side of the screen, move to breadcrumb
			"back" item, no matter where you're coming from?
*/

int main(void) {
	portapack::init();

	if( !cpld_update_if_necessary() ) {
		chSysHalt();
	}

	init_message_queues();

	portapack::io.init();
	portapack::display.init();

	sdcStart(&SDCD1, nullptr);

	rtc_interrupt_enable();

	controls_init();

	lcd_frame_sync_configure();

 	events_initialize(chThdSelf());

	ui::Context context;
	ui::SystemView system_view {
		context,
		portapack::display.screen_rect()
	};
	ui::Painter painter;
	EventDispatcher event_dispatcher { &system_view, painter, context };

	m4txevent_interrupt_enable();

	m4_init(portapack::spi_flash::baseband, portapack::spi_flash::m4_text_ram_base);

	while(true) {
		const auto events = event_dispatcher.wait();
		event_dispatcher.dispatch(events);
	}

	return 0;
}
