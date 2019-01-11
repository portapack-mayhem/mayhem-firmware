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
		clock_manager.init_peripherals();
	}

}


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

static void configure_unused_mcu_peripherals_power_down(const bool power_down) {
	LPC_CGU->IDIVD_CTRL.PD = power_down;
	LPC_CGU->IDIVE_CTRL.PD = power_down;

	LPC_CGU->BASE_USB1_CLK.PD = power_down;
	LPC_CGU->BASE_SPI_CLK.PD = power_down;
	LPC_CGU->BASE_PHY_RX_CLK.PD = power_down;
	LPC_CGU->BASE_PHY_TX_CLK.PD = power_down;
	LPC_CGU->BASE_LCD_CLK.PD = power_down;
	LPC_CGU->BASE_SSP0_CLK.PD = power_down;
	LPC_CGU->BASE_UART0_CLK.PD = power_down;
	LPC_CGU->BASE_UART1_CLK.PD = power_down;
	LPC_CGU->BASE_UART2_CLK.PD = power_down;
	LPC_CGU->BASE_UART3_CLK.PD = power_down;
	LPC_CGU->BASE_OUT_CLK.PD = power_down;
	LPC_CGU->BASE_CGU_OUT0_CLK.PD = power_down;
	LPC_CGU->BASE_CGU_OUT1_CLK.PD = power_down;
}

static void configure_unused_mcu_peripherals(const bool enabled) {
	/* Disabling these peripherals reduces "idle" (PortaPack at main
	 * menu) current by 42mA.
	 */

	/* Some surprising peripherals in use by PortaPack firmware:
	 *
	 * RITIMER: M0 SysTick substitute (because M0 has no SysTick)
	 * TIMER3: M0 cycle/PCLK counter
	 * IDIVB: Clock for SPI (set up in bootstrap code)
	 * IDIVC: I2S audio clock
	 */

	const uint32_t clock_run_state = enabled ? 1 : 0;
	const bool power_down = !enabled;

	if( power_down == false ) {
		// Power up peripheral clocks *before* enabling run state.
		configure_unused_mcu_peripherals_power_down(power_down);
	}

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

	if( power_down == true ) {
		// Power down peripheral clocks *after* disabling run state.
		configure_unused_mcu_peripherals_power_down(power_down);
	}
}

static void disable_unused_mcu_peripheral_clocks() {
	configure_unused_mcu_peripherals(false);
}

static void enable_unused_mcu_peripheral_clocks() {
	configure_unused_mcu_peripherals(true);
}

static void shutdown_base() {
	clock_manager.shutdown();

	chSysDisable();

	systick_stop();

	enable_unused_mcu_peripheral_clocks();

	hackrf::one::reset();
}

bool init() {
	clock_manager.init_peripherals();


	if( !portapack::cpld::update_if_necessary(portapack_cpld_config()) ) {
		shutdown_base();
		return false;
	}

	if( !hackrf::cpld::load_sram() ) {
		chSysHalt();
	}

	portapack::io.init();

	clock_manager.init_clock_generator();
	clock_manager.set_reference_ppb(persistent_memory::correction_ppb());

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

/* Bootstrap runs from SPIFI on the M4, immediately after the LPC43xx built-in
 * boot ROM runs.
 */

/* After boot ROM executes:
 * PLL1 is at 288MHz (IRC * 24)
 * IDIVB_CTRL = PLL1 / 9 = 32MHz
 * IDIVC_CTRL = PLL1 / 3 = 96MHz
 * BASE_SPIFI_CLK.CLK_SEL = IDIVB
 * BASE_M4_CLK.CLK_SEL = IDIVC?
 */

static void configure_spifi(void) {
	constexpr Pin pins_spifi[] = {
		{  3,  3, PinConfig::spifi_sck(3) }, /* SPIFI_SCK: W25Q80BV.CLK(I), enable input buffer for timing feedback */
		{  3,  4, PinConfig::spifi_inout(3) }, /* SPIFI_SIO3/P82: W25Q80BV.HOLD(IO) */
		{  3,  5, PinConfig::spifi_inout(3) }, /* SPIFI_SIO2/P81: W25Q80BV.WP(IO) */
		{  3,  6, PinConfig::spifi_inout(3) }, /* SPIFI_MISO: W25Q80BV.DO(IO) */
		{  3,  7, PinConfig::spifi_inout(3) }, /* SPIFI_MOSI: W25Q80BV.DI(IO) */
		{  3,  8, PinConfig::spifi_cs(3) }, /* SPIFI_CS/P68: W25Q80BV.CS(I) */
	};

	for(const auto& pin : pins_spifi) {
		pin.init();
	}

	/* Tweak SPIFI mode */
	LPC_SPIFI->CTRL =
		  (0xffff <<  0)	/* Timeout */
		| (0x1    << 16)	/* CS high time in "clocks - 1" */
		| (0      << 21)	/* 0: Attempt speculative prefetch on data accesses */
		| (0      << 22)	/* 0: No interrupt on command ended */
		| (0      << 23)	/* 0: SCK driven low after rising edge at which last bit of command is captured. Stays low while CS# is high. */
		| (0      << 27)	/* 0: Cache prefetching enabled */
		| (0      << 28)	/* 0: Quad protocol, IO3:0 */
		| (1      << 29)	/* 1: Read data sampled on falling edge of clock */
		| (1      << 30)	/* 1: Read data is sampled using feedback clock from SCK pin */
		| (0      << 31)	/* 0: DMA request disabled */
		;

	/* Throttle up the SPIFI interface to 96MHz (IDIVA=PLL1 / 3) */
	LPC_CGU->IDIVB_CTRL.word =
		  ( 0 <<  0)	/* PD */
		| ( 2 <<  2)	/* IDIV (/3) */
		| ( 1 << 11)	/* AUTOBLOCK */
		| ( 9 << 24)	/* PLL1 */
		;
}

extern "C" {

void __early_init(void) {
	/*
	 * Upon exit from bootloader into SPIFI boot mode:
	 *
	 * Enabled:
	 *   PLL1: IRC, M=/24, N=/1, P=/1, autoblock, direct = 288 MHz
	 *   IDIVA: IRC /1 = 12 MHz
	 *   IDIVB: PLL1 /9, autoblock = 32 MHz
	 *   IDIVC: PLL1 /3, autoblock = 96 MHz
	 *   IDIVD: IRC /1 = 12 MHz
	 *   IDIVE: IRC /1 = 12 MHz
	 *   BASE_M4_CLK: IDIVC, autoblock
	 *   BASE_SPIFI_CLK: IDIVB, autoblock
	 *
	 * Disabled:
	 *   XTAL_OSC
	 *   PLL0USB
	 *   PLL0AUDIO
	 */
	/* LPC43xx M4 takes about 500 usec to get to __early_init
	 * Before __early_init, LPC bootloader runs and starts our code. In user code, the process stack
	 * is initialized, hardware floating point is initialized, and stacks are zeroed,
	 */
	const uint32_t CORTEX_M4_CPUID      = 0x410fc240;
	const uint32_t CORTEX_M4_CPUID_MASK = 0xff0ffff0;

	if( (SCB->CPUID & CORTEX_M4_CPUID_MASK) == CORTEX_M4_CPUID ) {
		/* Enable unaligned exception handler */
		SCB_CCR |= (1 << 3);

		/* Enable MemManage, BusFault, UsageFault exception handlers */
		SCB_SHCSR |= (1 << 18) | (1 << 17) | (1 << 16);

		reset();

		// disable_unused_mcu_peripheral_clocks();
		configure_spifi();

		LPC_CCU1->CLK_M4_M0APP_CFG.RUN = true;
		LPC_CREG->M0APPMEMMAP = LPC_SPIFI_DATA_CACHED_BASE + 0x0;
		LPC_RGU->RESET_CTRL[1] = 0;

		/* Prevent the M4 from doing any more initializing by sleep-waiting forever...
		 * ...until the M0 resets the M4 with some code to run.
		 */
		while(1) {
			__WFE();
		}
	}
}

void __late_init(void) {
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
