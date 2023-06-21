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

#ifndef __UI_FREQ_FIELD_H__
#define __UI_FREQ_FIELD_H__

#include "portapack.hpp"
#include "receiver_model.hpp"
#include "transmitter_model.hpp"
#include "ui_receiver.hpp"

namespace ui {

/* A frequency field bound directly a radio model
 * that can manage its own editing UX. */
template <typename TModel, TModel* model>
class BoundFrequencyField : public FrequencyField {
   private:
    using FrequencyField::on_change;
    using FrequencyField::on_edit;

   public:
    decltype(FrequencyField::on_change) updated{};

    BoundFrequencyField(Point parent_pos, NavigationView& nav)
        : FrequencyField(parent_pos) {
        // NB: There is no frequency_step on the tx_model.
        set_step(portapack::receiver_model.frequency_step());
        set_value(model->target_frequency());

        on_change = [this](rf::Frequency f) {
            model->set_target_frequency(f);
            if (updated)
                updated(f);
        };

        on_edit = [this, &nav]() {
            auto freq_view = nav.push<FrequencyKeypadView>(model->target_frequency());
            freq_view->on_changed = [this](rf::Frequency f) {
                set_value(f);
            };
        };
    }

    // TODO: override set_step and update the rx model then call base.
};

using RxFrequencyField = BoundFrequencyField<ReceiverModel, &portapack::receiver_model>;
using TxFrequencyField = BoundFrequencyField<TransmitterModel, &portapack::transmitter_model>;

}  // namespace ui

#endif  // __UI_FREQ_FIELD_H__