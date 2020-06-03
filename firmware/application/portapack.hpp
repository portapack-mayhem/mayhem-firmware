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

#pragma once

#include "portapack_io.hpp"

#include "receiver_model.hpp"
#include "transmitter_model.hpp"

#include "i2c_pp.hpp"
#include "spi_pp.hpp"
#include "si5351.hpp"
#include "lcd_ili9341.hpp"
#include "backlight.hpp"

#include "radio.hpp"
#include "clock_manager.hpp"
#include "temperature_logger.hpp"

namespace portapack {

extern portapack::IO io;

extern lcd::ILI9341 display;

extern bool speaker_mode;
void set_speaker_mode(const bool v);

extern I2C i2c0;
extern SPI ssp1;

extern si5351::Si5351 clock_generator;
extern ClockManager clock_manager;

extern ReceiverModel receiver_model;
extern TransmitterModel transmitter_model;

extern uint8_t bl_tick_counter;
extern bool antenna_bias;

extern TemperatureLogger temperature_logger;

void set_antenna_bias(const bool v);
bool get_antenna_bias();

bool init();
void shutdown();

Backlight* backlight();

} /* namespace portapack */
