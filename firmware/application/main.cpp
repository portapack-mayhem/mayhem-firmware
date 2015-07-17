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

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "portapack_shared_memory.hpp"
#include "portapack_hal.hpp"
#include "portapack_io.hpp"

#include "cpld_update.hpp"

#include "message_queue.hpp"

#include "si5351.hpp"
#include "clock_manager.hpp"

#include "wm8731.hpp"
#include "radio.hpp"
#include "touch.hpp"
#include "touch_adc.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"

#include "receiver_model.hpp"

#include "irq_ipc.hpp"
#include "irq_lcd_frame.hpp"
#include "irq_controls.hpp"

#include "event.hpp"

#include "i2c_pp.hpp"
#include "spi_pp.hpp"

#include "m4_startup.hpp"

#include "debug.hpp"
#include "led.hpp"

#include "gcc.hpp"

#include <string.h>

I2C i2c0(&I2CD0);
SPI ssp0(&SPID1);
SPI ssp1(&SPID2);

wolfson::wm8731::WM8731 audio_codec { i2c0, portapack::wm8731_i2c_address };

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

/* ChibiOS initialization sequence:
 * ResetHandler:
 *		Initialize FPU (if present)
 *		Initialize stacks (fill with pattern)
 *		__early_init()
 *			Enable extra processor exceptions for debugging
 *		Init data segment (flash -> data)
 *		Initialize BSS (fill with 0)
 *		__late_init()
 *			reset_peripherals()
 *			halInit()
 *				hal_lld_init()
 *					Init timer 3 as cycle counter
 *					Init RIT as SysTick
 *				palInit()
 *				gptInit()
 *				i2cInit()
 *				sdcInit()
 *				spiInit()
 *				rtcInit()
 *				boardInit()
 *			chSysInit()
 *		Constructors
 *		main()
 *		Destructors
 *		_default_exit() (default is infinite loop)
 */

si5351::Si5351 clock_generator {
	i2c0, si5351_i2c_address
};

ClockManager clock_manager {
	i2c0, clock_generator
};

ReceiverModel receiver_model {
	clock_manager
};

class Power {
public:
	void init() {
		/* VAA powers:
		 * MAX5864 analog section.
		 * MAX2837 registers and other functions.
		 * RFFC5072 analog section.
		 *
		 * Beware that power applied to pins of the MAX2837 may
		 * show up on VAA and start powering other components on the
		 * VAA net. So turn on VAA before driving pins from MCU to
		 * MAX2837.
		 */
		/* Turn on VAA */
		gpio_vaa_disable.clear();
		gpio_vaa_disable.output();

		/* 1V8 powers CPLD internals.
		 */
		/* Turn on 1V8 */
		gpio_1v8_enable.set();
		gpio_1v8_enable.output();

		/* Set VREGMODE for switching regulator on HackRF One */
		gpio_vregmode.set();
		gpio_vregmode.output();
	}

private:
};

static Power power;

static void init() {
	for(const auto& pin : pins) {
		pin.init();
	}

	/* Configure other pins */
	LPC_SCU->SFSI2C0 =
		  (1U <<  3)
		| (1U << 11)
		;

	power.init();

	gpio_max5864_select.set();
	gpio_max5864_select.output();

	gpio_max2837_select.set();
	gpio_max2837_select.output();

	led_usb.setup();
	led_rx.setup();
	led_tx.setup();

	clock_manager.init();
	clock_manager.run_at_full_speed();

	clock_manager.start_audio_pll();
	audio_codec.init();

	clock_manager.enable_first_if_clock();
	clock_manager.enable_second_if_clock();
	clock_manager.enable_codec_clocks();
	radio::init();

	touch::adc::init();
}

extern "C" {

void __late_init(void) {

	reset();

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();

	/* After this call, scheduler, systick, heap, etc. are available. */
	/* By doing chSysInit() here, it runs before C++ constructors, which may
	 * require the heap.
	 */
	chSysInit();
}

}

extern "C" {

CH_IRQ_HANDLER(RTC_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	events_flag_isr(EVT_MASK_RTC_TICK);
	chSysUnlockFromIsr();

	rtc::interrupt::clear_all();

	CH_IRQ_EPILOGUE();
}

}

static bool ui_dirty = true;

void ui::dirty_event() {
	ui_dirty = true;
}

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

			auto& fn = context.message_map[message->id];
			if( fn ) {
				fn(message);
			}

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
		if( ui_dirty ) {
			paint_widget(top_widget);
			ui_dirty = false;
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

/*
message_handlers[Message::ID::FSKPacket] = [](const Message* const p) {
	const auto message = static_cast<const FSKPacketMessage*>(p);
	fsk_packet(message);
};

message_handlers[Message::ID::TestResults] = [&system_view](const Message* const p) {
	const auto message = static_cast<const TestResultsMessage*>(p);
	char c[10];
	c[0] = message->results.translate_by_fs_over_4_and_decimate_by_2_cic3 ? '+' : '-';
	c[1] = message->results.fir_cic3_decim_2_s16_s16 ? '+' : '-';
	c[2] = message->results.fir_64_and_decimate_by_2_complex ? '+' : '-';
	c[3] = message->results.fxpt_atan2 ? '+' : '-';
	c[4] = message->results.multiply_conjugate_s16_s32 ? '+' : '-';
	c[5] = 0;
	system_view.status_view.portapack.set(c);
};
*/

portapack::IO portapack::io {
	portapack::gpio_dir,
	portapack::gpio_lcd_rd,
	portapack::gpio_lcd_wr,
	portapack::gpio_io_stbx,
	portapack::gpio_addr,
	portapack::gpio_lcd_te,
	portapack::gpio_unused,
};

int main(void) {
	init();

	if( !cpld_update_if_necessary() ) {
		chSysHalt();
	}

	init_message_queues();

	portapack::io.init();
	ui::Context context;
	context.display.init();

	sdcStart(&SDCD1, nullptr);

	rtc::interrupt::enable_second_inc();
	nvicEnableVector(RTC_IRQn, CORTEX_PRIORITY_MASK(LPC_RTC_IRQ_PRIORITY));

	controls_init();

	lcd_frame_sync_configure();

 	events_initialize(chThdSelf());

	ui::SystemView system_view {
		context,
		{ 0, 0, 240, 320 }
	};
	ui::Painter painter { context.display };
	EventDispatcher event_dispatcher { &system_view, painter, context };

context.message_map[Message::ID::FSKPacket] = [](const Message* const p) {
	const auto message = static_cast<const FSKPacketMessage*>(p);
	(void)message;
	led_usb.toggle();
};

	m4txevent_interrupt_enable();
	m4_init();

	while(true) {
		const auto events = event_dispatcher.wait();
		event_dispatcher.dispatch(events);
	}

	return 0;
}
