/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_battinfo.hpp"

#include "event_m0.hpp"
#include "portapack.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

bool BattinfoView::needRun = true;
void BattinfoView::focus() {
    button_exit.focus();
}

// called each 1/60th of second, so 6 = 100ms
void BattinfoView::on_timer() {
    if (++timer_counter == timer_period) {
        timer_counter = 0;
        update_result();
    }
}

void BattinfoView::update_result() {
    if (!battery::BatteryManagement::isDetected()) {
        text_percent.set("UNKNOWN");
        text_voltage.set("UNKNOWN");
        text_current.set("-");
        text_charge.set("-");
        return;
    }
    bool uichg = false;
    battery::BatteryManagement::getBatteryInfo(percent, voltage, current);
    // update text fields
    if (percent <= 100)
        text_percent.set(to_string_dec_uint(percent) + " %");
    else
        text_percent.set("UNKNOWN");
    if (voltage > 1) {
        text_voltage.set(to_string_decimal(voltage / 1000.0, 3) + " V");
    } else {
        text_voltage.set("UNKNOWN");
    }
    if (current != 0) {
        if (labels_opt.hidden()) uichg = true;
        labels_opt.hidden(false);
        text_current.hidden(false);
        text_charge.hidden(false);
        text_current.set(to_string_decimal(current / 100000.0, 3) + " mA");
        text_charge.set(current >= 0 ? "Charging" : "Discharging");
        labels_opt.hidden(false);
    } else {
        if (!labels_opt.hidden()) uichg = true;
        labels_opt.hidden(true);
        text_current.hidden(true);
        text_charge.hidden(true);
    }
    if (uichg) set_dirty();
    // to update status bar too, send message in behalf of batt manager
    BatteryStateMessage msg{percent, current >= 0, voltage};
    EventDispatcher::send_message(msg);
}

BattinfoView::BattinfoView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &labels_opt,
                  &text_percent,
                  &text_voltage,
                  &text_current,
                  &text_charge,
                  &button_exit});

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };

    update_result();
    needRun = true;
    thread = chThdCreateFromHeap(NULL, 512, NORMALPRIO + 10, BattinfoView::static_fn, this);
}

msg_t BattinfoView::static_fn(void* arg) {
    auto obj = static_cast<BattinfoView*>(arg);
    while (needRun) {
        chThdSleepMilliseconds(16);
        obj->on_timer();
    }
    return 0;
}

BattinfoView::~BattinfoView() {
    needRun = false;
    if (thread) {
        chThdTerminate(thread);
        chThdWait(thread);
        thread = nullptr;
    }
}
}  // namespace ui
