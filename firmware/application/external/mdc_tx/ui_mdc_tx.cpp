/*
 * Copyright (C) 2025 Sarah Rose
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

#include "ui_mdc_tx.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "transmitter_model.hpp"

using namespace portapack;

namespace ui::external_app::mdc_tx {

MdcTxView::MdcTxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_afsk);

    add_children({
        &text_status,
    });
}

MdcTxView::~MdcTxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void MdcTxView::focus() {
    text_status.focus();
}

void MdcTxView::start_tx() {
    tx_active_ = true;
}

void MdcTxView::stop_tx() {
    transmitter_model.disable();
    tx_active_ = false;
}

void MdcTxView::on_tx_progress(uint32_t, bool done) {
    if (done) stop_tx();
}

}  // namespace ui::external_app::mdc_tx
