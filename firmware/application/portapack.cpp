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
#include "irq_controls.hpp"

#include "file.hpp" 
#include "sd_card.hpp"
#include "string_format.hpp"

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
uint32_t bl_tick_counter { 0 };

void set_antenna_bias(const bool v) {
	antenna_bias = v;
}

bool get_antenna_bias() {
	return antenna_bias;
}

bool speaker_mode { false };
 void set_speaker_mode(const bool v) {
 	speaker_mode = v;
 	if (speaker_mode)
 		audio::output::speaker_unmute();
 	else
 		audio::output::speaker_mute();
 }

static constexpr uint32_t systick_count(const uint32_t clock_source_f) {
	return clock_source_f / CH_FREQUENCY;
}

static constexpr uint32_t systick_load(const uint32_t clock_source_f) {
	return systick_count(clock_source_f) - 1;
}

constexpr uint32_t i2c0_bus_f			= 400000;
constexpr uint32_t i2c0_high_period_ns	= 900;

typedef struct {
	uint32_t clock_f;
	uint32_t systick_count;
	uint32_t idivb;
	uint32_t idivc;
} clock_config_t;

static constexpr uint32_t idiv_config(const cgu::CLK_SEL clk_sel, const uint32_t idiv) {
	return cgu::IDIV_CTRL { 0, idiv-1, 1, clk_sel };
}

constexpr clock_config_t clock_config_irc {
	12000000, systick_load(12000000),
	idiv_config(cgu::CLK_SEL::IRC, 1),
	idiv_config(cgu::CLK_SEL::IRC, 1),
};

constexpr clock_config_t clock_config_pll1_boot {
	96000000, systick_load(96000000),
	idiv_config(cgu::CLK_SEL::PLL1, 9),
	idiv_config(cgu::CLK_SEL::PLL1, 3),
};

constexpr clock_config_t clock_config_pll1_step {
	100000000, systick_load(100000000),
	idiv_config(cgu::CLK_SEL::PLL1, 1),
	idiv_config(cgu::CLK_SEL::PLL1, 1),
};

constexpr clock_config_t clock_config_pll1 {
	200000000, systick_load(200000000),
	idiv_config(cgu::CLK_SEL::PLL1, 2),
	idiv_config(cgu::CLK_SEL::PLL1, 1),
};

constexpr I2CClockConfig i2c_clock_config_400k_boot_clock {
	.clock_source_f = clock_config_pll1_boot.clock_f,
	.bus_f = i2c0_bus_f,
	.high_period_ns = i2c0_high_period_ns,
};

constexpr I2CClockConfig i2c_clock_config_400k_fast_clock {
	.clock_source_f = clock_config_pll1.clock_f,
	.bus_f = i2c0_bus_f,
	.high_period_ns = i2c0_high_period_ns,
};

constexpr I2CConfig i2c_config_boot_clock {
	.high_count = i2c_clock_config_400k_boot_clock.i2c_high_count(),
	.low_count = i2c_clock_config_400k_boot_clock.i2c_low_count(),
};

constexpr I2CConfig i2c_config_fast_clock {
	.high_count = i2c_clock_config_400k_fast_clock.i2c_high_count(),
	.low_count = i2c_clock_config_400k_fast_clock.i2c_low_count(),
};

enum class PortaPackModel {
	R1_20150901,
	R2_20170522,
};

static bool save_config(int8_t value){
	persistent_memory::set_config_cpld(value);
	if(sd_card::status() == sd_card::Status::Mounted){
		make_new_directory("/hardware"); 
		File file;
		auto sucess = file.create("/hardware/settings.txt");
		if(!sucess.is_valid()) {
			file.write_line(to_string_dec_uint(value));
		}
	}
	return true;
}

int read_file(std::string name) {
	std::string return_string = "";
	File file;
	auto success = file.open(name);

	if(!success.is_valid()) {
		char one_char[1];
		for(size_t pointer = 0; pointer < file.size() ; pointer++) {
			file.seek(pointer);
			file.read(one_char, 1);
			return_string += one_char[0];
		}
		return std::stoi(return_string);
	} 
	return -1; 
}

static int load_config(){
	static Optional<int> config_value;
	if(!config_value.is_valid()){
		int8_t value = portapack::persistent_memory::config_cpld();
		if((value <= 0 || value >= 5) && sd_card::status() == sd_card::Status::Mounted){
			int data = read_file("/hardware/settings.txt");
			if(data != -1) {
				config_value = data;
			}
		} else {
			config_value = value;
		}
	}
	return config_value.value();
}


static PortaPackModel portapack_model() {
	static Optional<PortaPackModel> model;

	if( !model.is_valid() ) {
		const auto switches_state = get_switches_state();
		if (switches_state[(size_t)ui::KeyEvent::Up]){
			save_config(1);
			// model = PortaPackModel::R2_20170522; // Commented these out as they should be set down below anyway
		}
		else if (switches_state[(size_t)ui::KeyEvent::Down]){
			save_config(2);
			// model = PortaPackModel::R1_20150901;
		}
		else if (switches_state[(size_t)ui::KeyEvent::Left]){
			save_config(3);
			// model = PortaPackModel::R1_20150901;
		}
		else if (switches_state[(size_t)ui::KeyEvent::Right]){
			save_config(4);
			// model = PortaPackModel::R2_20170522;
		}
		else if (switches_state[(size_t)ui::KeyEvent::Select]){
			save_config(0);
		}
		

		if (load_config() == 1) {
			model = PortaPackModel::R2_20170522;
		} else if (load_config() == 2) {
			model = PortaPackModel::R1_20150901;
		} else if (load_config() == 3) {
			model = PortaPackModel::R1_20150901;
		} else if (load_config() == 4) {
			model = PortaPackModel::R2_20170522;
		} else {
			if( audio_codec_wm8731.detected() ) {
				model = PortaPackModel::R1_20150901; // H1R1
			} else {
				model = PortaPackModel::R2_20170522; // H1R2, H2, H2+
			}
		}
	}

	return model.value();
}

//audio_codec_wm8731 = H1R1 & H2+
//audio_codec_ak4951 = H1R2

static audio::Codec* portapack_audio_codec() {
	/* I2C ready OK, Automatic recognition of audio chip */
	return (audio_codec_wm8731.detected())
		? static_cast<audio::Codec*>(&audio_codec_wm8731)
		: static_cast<audio::Codec*>(&audio_codec_ak4951)
		;
}

static const portapack::cpld::Config& portapack_cpld_config() {
	return (portapack_model() == PortaPackModel::R2_20170522)
			? portapack::cpld::rev_20170522::config
			: portapack::cpld::rev_20150901::config;
}

Backlight* backlight() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? static_cast<portapack::Backlight*>(&backlight_cat4004) // R2_20170522
		: static_cast<portapack::Backlight*>(&backlight_on_off); // R1_20150901
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static LPC_CGU_BASE_CLK_Type* const base_clocks_idivc[] = {
	&LPC_CGU->BASE_PERIPH_CLK,
	&LPC_CGU->BASE_M4_CLK,
	&LPC_CGU->BASE_APB1_CLK,
	&LPC_CGU->BASE_APB3_CLK,
	&LPC_CGU->BASE_SDIO_CLK,
	&LPC_CGU->BASE_SSP1_CLK,
};

static void set_idivc_base_clocks(const cgu::CLK_SEL clock_source) {
	for(uint32_t i=0; i<ARRAY_SIZE(base_clocks_idivc); i++) {
		base_clocks_idivc[i]->AUTOBLOCK = 1;
		base_clocks_idivc[i]->CLK_SEL = toUType(clock_source);
	}
}

static void set_clock_config(const clock_config_t& config) {
	LPC_CGU->IDIVB_CTRL.word = config.idivb;
	LPC_CGU->IDIVC_CTRL.word = config.idivc;
	systick_adjust_period(config.systick_count);
	halLPCSetSystemClock(config.clock_f);
}

static void shutdown_base() {
	i2c0.stop();

	set_clock_config(clock_config_irc);

	cgu::pll1::disable();

	set_idivc_base_clocks(cgu::CLK_SEL::IRC);

	cgu::pll1::ctrl({
		.pd = 1,
		.bypass = 0,
		.fbsel = 0,
		.direct = 1,
		.psel = 0,
		.autoblock = 1,
		.nsel = 0,
		.msel = 23,
		.clk_sel = cgu::CLK_SEL::IRC,
	});

	cgu::pll1::enable();
	while( !cgu::pll1::is_locked() );

	set_clock_config(clock_config_pll1_boot);

	i2c0.start(i2c_config_boot_clock);

	clock_manager.shutdown();
}

/* Clock scheme after exiting bootloader in SPIFI mode:
 * 
 * XTAL_OSC = powered down
 *
 * PLL0USB = powered down
 * PLL0AUDIO = powered down
 * PLL1 = IRC * 24 = 288 MHz
 *
 * IDIVA = IRC / 1 = 12 MHz
 * IDIVB = PLL1 / 9 = 32 MHz
 * IDIVC = PLL1 / 3 = 96 MHz
 * IDIVD = IRC / 1 = 12 MHz
 * IDIVE = IRC / 1 = 12 MHz
 *
 * BASE_USB0_CLK = PLL0USB
 * BASE_PERIPH_CLK = IRC
 * BASE_M4_CLK = IDIVC (96 MHz)
 * BASE_SPIFI_CLK = IDIVB (32 MHZ)
 *
 * everything else = IRC
 */

/* Clock scheme during PortaPack operation:
 * 
 * XTAL_OSC = powered down
 *
 * PLL0USB = powered down
 * PLL0AUDIO = GP_CLKIN, Fcco=491.52 MHz, Fout=12.288 MHz
 * PLL1 = GP_CLKIN * 10 = 200 MHz
 *
 * IDIVA = IRC / 1 = 12 MHz
 * IDIVB = PLL1 / 2 = 100 MHz
 * IDIVC = PLL1 / 1 = 200 MHz
 * IDIVD = PLL0AUDIO / N (where N is varied depending on decimation factor)
 * IDIVE = IRC / 1 = 12 MHz
 *
 * BASE_USB0_CLK = PLL0USB
 * BASE_PERIPH_CLK = IRC
 * BASE_M4_CLK = IDIVC (200 MHz)
 * BASE_SPIFI_CLK = IDIVB (100 MHZ)
 * BASE_AUDIO_CLK = IDIVD
 *
 * everything else = IRC
 */

bool init() {
	set_idivc_base_clocks(cgu::CLK_SEL::IDIVC);

	i2c0.start(i2c_config_boot_clock);

	// Keeping this here for now incase we need to revert
	// if( !portapack::cpld::update_if_necessary(portapack_cpld_config()) ) {
	// 	shutdown_base();
	// 	return false;
	// }

	// if( !hackrf::cpld::load_sram() ) {
	// 	chSysHalt();
	// }
	chThdSleepMilliseconds(100);

	configure_pins_portapack();
	
	portapack::io.init();

	clock_manager.init_clock_generator();

	i2c0.stop();

	chThdSleepMilliseconds(10);

	set_clock_config(clock_config_irc);
	cgu::pll1::disable();

	/* Incantation from LPC43xx UM10503 section 12.2.1.1, to bring the M4
	 * core clock speed to the 110 - 204MHz range.
	 */

	/* Step into the 90-110MHz M4 clock range */
	/* Fclkin = 40M
	 * 	/N=2 = 20M = PFDin
	 * Fcco = PFDin * (M=10) = 200M
	 * Fclk = Fcco / (2*(P=1)) = 100M
	 */
	cgu::pll1::ctrl({
		.pd = 1,
		.bypass = 0,
		.fbsel = 0,
		.direct = 0,
		.psel = 0,
		.autoblock = 1,
		.nsel = 1,
		.msel = 9,
		.clk_sel = cgu::CLK_SEL::GP_CLKIN,
	});

	cgu::pll1::enable();
	while( !cgu::pll1::is_locked() );

	set_clock_config(clock_config_pll1_step);

	/* Delay >50us at 90-110MHz clock speed */
	volatile uint32_t delay = 1400;
	while(delay--);

	set_clock_config(clock_config_pll1);

	/* Remove /2P divider from PLL1 output to achieve full speed */
	cgu::pll1::direct();

	i2c0.start(i2c_config_fast_clock);
	chThdSleepMilliseconds(10);

	/* Cache some configuration data from persistent memory. */
	persistent_memory::cache::init();

	touch::adc::init();
	controls_init();
	chThdSleepMilliseconds(10);

	clock_manager.set_reference_ppb(persistent_memory::correction_ppb());
	clock_manager.enable_first_if_clock();
	clock_manager.enable_second_if_clock();
	clock_manager.enable_codec_clocks();
	radio::init();

	sdcStart(&SDCD1, nullptr);
	sd_card::poll_inserted();
	
	chThdSleepMilliseconds(10);

	if( !portapack::cpld::update_if_necessary(portapack_cpld_config()) ) {
		chThdSleepMilliseconds(10);
		// If using a "2021/12 QFP100", press and hold the left button while booting. Should only need to do once.
		if (load_config() != 3 && load_config() != 4){
			shutdown_base();
			return false;
		}
	}

	if( !hackrf::cpld::load_sram() ) {
		chSysHalt();
	}

	chThdSleepMilliseconds(10); // This delay seems to solve white noise audio issues

	LPC_CREG->DMAMUX = portapack::gpdma_mux;
	gpdma::controller.enable();

	chThdSleepMilliseconds(10);

	audio::init(portapack_audio_codec());
	

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

} /* namespace portapack */
