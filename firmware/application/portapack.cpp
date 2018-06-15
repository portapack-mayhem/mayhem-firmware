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
#include "portapack_dma.hpp"
#include "portapack_cpld_data.hpp"
#include "portapack_persistent_memory.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "clock_manager.hpp"
#include "event_m0.hpp"

#include "backlight.hpp"
#include "touch_adc.hpp"
#include "audio.hpp"

#include "wm8731.hpp"
using wolfson::wm8731::WM8731;

#include "ak4951.hpp"
using asahi_kasei::ak4951::AK4951;

#include "cpld_update.hpp"

#include "optional.hpp"

namespace portapack {

portapack::IO io {
	portapack::gpio_dir,
	portapack::gpio_lcd_rdx,
	portapack::gpio_lcd_wrx,
	portapack::gpio_io_stbx,
	portapack::gpio_addr,
	portapack::gpio_lcd_te,
	portapack::gpio_unused,
};

portapack::BacklightCAT4004 backlight_cat4004;
portapack::BacklightOnOff   backlight_on_off;

lcd::ILI9341 display;

I2C i2c0(&I2CD0);
SPI ssp1(&SPID2);

si5351::Si5351 clock_generator {
	i2c0, hackrf::one::si5351_i2c_address
};

ClockManager clock_manager {
	i2c0, clock_generator
};

WM8731 audio_codec_wm8731 { i2c0, 0x1a };
AK4951 audio_codec_ak4951 { i2c0, 0x12 };

ReceiverModel receiver_model;
TransmitterModel transmitter_model;

TemperatureLogger temperature_logger;

bool antenna_bias { false };
bool prev_clkin_status { false };
uint8_t bl_tick_counter { 0 };

void set_antenna_bias(const bool v) {
	antenna_bias = v;
}

bool get_antenna_bias() {
	return antenna_bias;
}

bool get_ext_clock() {
	return prev_clkin_status;
}

void poll_ext_clock() {
	auto clkin_status = clock_generator.clkin_status();
	
	if (clkin_status != prev_clkin_status) {
		prev_clkin_status = clkin_status;
		StatusRefreshMessage message { };
		EventDispatcher::send_message(message);
		clock_manager.init(clkin_status);
	}
	
}

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

enum class PortaPackModel {
	R1_20150901,
	R2_20170522,
};

static PortaPackModel portapack_model() {
	static Optional<PortaPackModel> model;

	if( !model.is_valid() ) {
		if( audio_codec_wm8731.detected() ) {
			model = PortaPackModel::R1_20150901;
		} else {
			model = PortaPackModel::R2_20170522;
		}
	}

	return model.value();
}

static audio::Codec* portapack_audio_codec() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? static_cast<audio::Codec*>(&audio_codec_ak4951)
		: static_cast<audio::Codec*>(&audio_codec_wm8731)
		;
}

static const portapack::cpld::Config& portapack_cpld_config() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? portapack::cpld::rev_20170522::config
		: portapack::cpld::rev_20150901::config
		;
}

Backlight* backlight() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? static_cast<portapack::Backlight*>(&backlight_cat4004)
		: static_cast<portapack::Backlight*>(&backlight_on_off);
}

static void configure_unused_mcu_peripherals(const bool enabled) {
	/* Disabling these peripherals reduces "idle" (PortaPack at main
	 * menu) current by 42mA.
	 */

	/* Some surprising peripherals in use by PortaPack firmware:
	 *
	 * RITIMER: M0 SysTick substitute (because M0 has no SysTick)
	 * TIMER3: M0 cycle/PCLK counter
	 */

	const uint32_t clock_run_state = enabled ? 1 : 0;

	LPC_CCU1->CLK_APB3_I2C1_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_APB3_DAC_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_APB3_CAN0_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_APB1_MOTOCON_PWM_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_APB1_CAN1_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_LCD_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_ETHERNET_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_USB0_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_EMC_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_SCT_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_USB1_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_EMCDIV_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_WWDT_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_USART0_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_UART1_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_SSP0_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_TIMER1_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_USART2_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_USART3_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_TIMER2_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_M4_QEI_CFG.RUN = clock_run_state;

	LPC_CCU1->CLK_USB1_CFG.RUN = clock_run_state;
	LPC_CCU1->CLK_SPI_CFG.RUN = clock_run_state;

	LPC_CCU2->CLK_APB2_USART3_CFG.RUN = clock_run_state;
	LPC_CCU2->CLK_APB2_USART2_CFG.RUN = clock_run_state;
	LPC_CCU2->CLK_APB0_UART1_CFG.RUN = clock_run_state;
	LPC_CCU2->CLK_APB0_USART0_CFG.RUN = clock_run_state;
	LPC_CCU2->CLK_APB0_SSP0_CFG.RUN = clock_run_state;
}

static void disable_unused_mcu_peripheral_clocks() {
	configure_unused_mcu_peripherals(false);
}

static void enable_unused_mcu_peripheral_clocks() {
	configure_unused_mcu_peripherals(true);
}

static void shutdown_base() {
	clock_manager.shutdown();

	power.shutdown();
	// TODO: Wait a bit for supplies to discharge?

	chSysDisable();

	systick_stop();

	enable_unused_mcu_peripheral_clocks();

	hackrf::one::reset();
}

bool init() {
	for(const auto& pin : pins) {
		pin.init();
	}

	/* Configure other pins */
	/* Glitch filter operates at 3ns instead of 50ns due to the WM8731
	 * returning an ACK very fast (170ns) and confusing the I2C state
	 * machine into thinking there was a bus error. It looks like the
	 * MCU sees SDA fall before SCL falls, indicating a START at the
	 * point an ACK is expected. With the glitch filter off or set to
	 * 3ns, it's probably still a bit tight timing-wise, but improves
	 * reliability on some problem units.
	 */
	LPC_SCU->SFSI2C0 =
		  (1U <<  0)	// SCL: 3ns glitch
		| (0U <<  2)	// SCL: Standard/Fast mode
		| (1U <<  3)	// SCL: Input enabled
		| (0U <<  7)	// SCL: Enable input glitch filter
		| (1U <<  8)	// SDA: 3ns glitch
		| (0U << 10)	// SDA: Standard/Fast mode
		| (1U << 11)	// SDA: Input enabled
		| (0U << 15)	// SDA: Enable input glitch filter
		;

	disable_unused_mcu_peripheral_clocks();

	LPC_CREG->CREG0 |= (1 << 5);	// Disable USB0 PHY

	power.init();

	gpio_max5864_select.set();
	gpio_max5864_select.output();

	gpio_max2837_select.set();
	gpio_max2837_select.output();

	led_usb.setup();
	led_rx.setup();
	led_tx.setup();

	clock_manager.init(false);
	clock_manager.set_reference_ppb(persistent_memory::correction_ppb());
	clock_manager.run_at_full_speed();

	if( !portapack::cpld::update_if_necessary(portapack_cpld_config()) ) {
		shutdown_base();
		return false;
	}

	if( !hackrf::cpld::load_sram() ) {
		chSysHalt();
	}

	portapack::io.init();

	audio::init(portapack_audio_codec());
	
	clock_manager.enable_first_if_clock();
	clock_manager.enable_second_if_clock();
	clock_manager.enable_codec_clocks();
	radio::init();

	touch::adc::init();

	LPC_CREG->DMAMUX = portapack::gpdma_mux;
	gpdma::controller.enable();

	return true;
}

void shutdown() {
	gpdma::controller.disable();

	backlight()->off();
	display.shutdown();
	
	radio::disable();
	audio::shutdown();

	hackrf::cpld::init_from_eeprom();

	shutdown_base();
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
