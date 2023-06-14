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

/* Stashes current radio bandwidth and sampling rate.
 * Inits to defaults, and restores previous when destroyed.
 * NB: This member must be added before SettingsManager. */
template <typename TModel, TModel* model>
class RadioState {
   public:
    RadioState()
        : RadioState(max2837::filter::bandwidth_minimum, 3072000) {
    }

    RadioState(uint32_t new_bandwidth, uint32_t new_sampling_rate)
        : prev_bandwidth_{model->baseband_bandwidth()},
          prev_sampling_rate_{model->sampling_rate()} {
        model->set_baseband_bandwidth(new_bandwidth);
        model->set_sampling_rate(new_sampling_rate);
    }

    ~RadioState() {
        model->set_baseband_bandwidth(prev_bandwidth_);
        model->set_sampling_rate(prev_sampling_rate_);
    }

   private:
    const uint32_t prev_bandwidth_;
    const uint32_t prev_sampling_rate_;
};

using RxRadioState = RadioState<ReceiverModel, &portapack::receiver_model>;
using TxRadioState = RadioState<TransmitterModel, &portapack::transmitter_model>;

#endif  // __RADIO_STATE_H__