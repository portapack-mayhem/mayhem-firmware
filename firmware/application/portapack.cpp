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

#include "portapack.hpp"
#include "portapack_hal.hpp"
#include "portapack_persistent_memory.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "clock_manager.hpp"

#include "touch_adc.hpp"
#include "audio.hpp"

namespace portapack {

portapack::IO io {
	portapack::gpio_dir,
	portapack::gpio_lcd_rd,
	portapack::gpio_lcd_wr,
	portapack::gpio_io_stbx,
	portapack::gpio_addr,
	portapack::gpio_lcd_te,
	portapack::gpio_unused,
};

lcd::ILI9341 display;

I2C i2c0(&I2CD0);
SPI ssp0(&SPID1);
SPI ssp1(&SPID2);

wolfson::wm8731::WM8731 audio_codec { i2c0, portapack::wm8731_i2c_address };

si5351::Si5351 clock_generator {
	i2c0, hackrf::one::si5351_i2c_address
};

ClockManager clock_manager {
	i2c0, clock_generator
};

ReceiverModel receiver_model;

TemperatureLogger temperature_logger;

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

	void shutdown() {
		gpio_1v8_enable.clear();
		gpio_vaa_disable.set();
	}

private:
};

static Power power;

void init() {
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
	clock_manager.set_reference_ppb(persistent_memory::correction_ppb());
	clock_manager.run_at_full_speed();

	clock_manager.start_audio_pll();
	audio_codec.init();

	i2s::i2s0::configure(
		audio::i2s0_config_tx,
		audio::i2s0_config_rx,
		audio::i2s0_config_dma
	);
	i2s::i2s0::tx_start();
	i2s::i2s0::rx_start();

	clock_manager.enable_first_if_clock();
	clock_manager.enable_second_if_clock();
	clock_manager.enable_codec_clocks();
	radio::init();

	touch::adc::init();
}

void shutdown() {
	display.shutdown();
	
	radio::disable();
	audio_codec.reset();
	clock_manager.shutdown();

	power.shutdown();
	// TODO: Wait a bit for supplies to discharge?

	chSysDisable();

	systick_stop();

	hackrf::one::reset();
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

} /* namespace portapack */
