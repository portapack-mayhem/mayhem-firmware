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

#ifndef __UI_MDC_TX_H__
#define __UI_MDC_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "message.hpp"

namespace ui::external_app::mdc_tx {

class MdcTxView : public View {
   public:
    MdcTxView(NavigationView& nav);
    ~MdcTxView();
    MdcTxView(const MdcTxView&) = delete;
    MdcTxView& operator=(const MdcTxView&) = delete;

    void focus() override;
    std::string title() const override { return "MDC-1200 TX"; }

   private:
    NavigationView& nav_;

    Text text_status{{0, 2 * 16, 240, 16}, "MDC-1200 TX"};

    bool tx_active_{false};

    void start_tx();
    void stop_tx();
    void on_tx_progress(uint32_t progress, bool done);

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto msg = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(msg.progress, msg.done);
        }};
};

}  // namespace ui::external_app::mdc_tx

#endif
