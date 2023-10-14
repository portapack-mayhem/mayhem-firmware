/*
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __RADIO_STATE_H__
#define __RADIO_STATE_H__

#include <type_traits>
#include <utility>
#include "portapack.hpp"
#include "receiver_model.hpp"
#include "transmitter_model.hpp"

/* Reinitialized model with defaults so each app can start
 * with a clean, default model instance. We tried lazily
 * setting these using the set_configuration_without_update
 * function, but there are still various UI components
 * (e.g. Waterfall) that need the radio set in order to load.
 * NB: This member must be added before SettingsManager. */
template <typename TModel, TModel* model>
class RadioState {
   public:
    RadioState() {
        model->initialize();
    }

    RadioState(uint32_t new_frequency, uint32_t new_bandwidth, uint32_t new_sampling_rate) {
        model->initialize();
        if (new_frequency != 0)
            model->set_target_frequency(new_frequency);
        model->set_sampling_rate(new_sampling_rate);
        model->set_baseband_bandwidth(new_bandwidth);
    }

    // NB: only enabled for RX model.
    template <
        typename U = TModel,
        typename Mode = std::enable_if_t<sizeof(typename U::Mode), typename U::Mode> >
    RadioState(Mode new_mode) {
        model->initialize();
        model->settings().mode = new_mode;
    }

    // NB: only enabled for RX model.
    template <
        typename U = TModel,
        typename Mode = std::enable_if_t<sizeof(typename U::Mode), typename U::Mode> >
    RadioState(
        uint32_t new_frequency,
        uint32_t new_bandwidth,
        uint32_t new_sampling_rate,
        Mode new_mode) {
        model->initialize();
        if (new_frequency != 0)
            model->set_target_frequency(new_frequency);
        model->set_sampling_rate(new_sampling_rate);
        model->set_baseband_bandwidth(new_bandwidth);
        model->settings().mode = new_mode;
    }
};

using RxRadioState = RadioState<ReceiverModel, &portapack::receiver_model>;
using TxRadioState = RadioState<TransmitterModel, &portapack::transmitter_model>;

#endif  // __RADIO_STATE_H__
