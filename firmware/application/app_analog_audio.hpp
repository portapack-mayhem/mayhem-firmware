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

#ifndef __APP_ANALOG_AUDIO_H__
#define __APP_ANALOG_AUDIO_H__

#include "receiver_model.hpp"
#include "ui_spectrum.hpp"
#include "utility.hpp"

class AnalogAudioModel {
public:
	AnalogAudioModel(ReceiverModel::Mode mode) {
		receiver_model.set_baseband_configuration({
			.mode = toUType(mode),
			.sampling_rate = 3072000,
			.decimation_factor = 4,
		});
		receiver_model.set_baseband_bandwidth(1750000);
	}
};

namespace ui {

class AnalogAudioView : public spectrum::WaterfallWidget {
public:
	AnalogAudioView(
		ReceiverModel::Mode mode
	) : model { mode }
	{
	}

private:
	AnalogAudioModel model;
};

} /* namespace ui */

#endif/*__APP_ANALOG_AUDIO_H__*/
