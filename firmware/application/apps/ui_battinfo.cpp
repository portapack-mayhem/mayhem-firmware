/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 HTotoo
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
#include "battery.hpp"
#include <cstring>

using namespace portapack;

namespace ui {

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
        // text_cycles.set("-");
        text_ttef.set("-");
        text_method.set("-");
        // text_warn.set("");
        return;
    }
    bool uichg = false;
    uint8_t valid_mask = 0;
    battery::BatteryManagement::getBatteryInfo(valid_mask, percent, voltage, current);
    // update text fields
    if (percent <= 100 && (valid_mask & battery::BatteryManagement::BATT_VALID_VOLTAGE) == battery::BatteryManagement::BATT_VALID_VOLTAGE)
        text_percent.set(to_string_dec_uint(percent) + " %");
    else
        text_percent.set("UNKNOWN");
    if (voltage > 1 && (valid_mask & battery::BatteryManagement::BATT_VALID_VOLTAGE) == battery::BatteryManagement::BATT_VALID_VOLTAGE) {
        text_voltage.set(to_string_decimal(voltage / 1000.0, 3) + " V");
    } else {
        text_voltage.set("UNKNOWN");
    }
    if ((valid_mask & battery::BatteryManagement::BATT_VALID_CURRENT) == battery::BatteryManagement::BATT_VALID_CURRENT) {
        if (labels_opt.hidden()) uichg = true;
        labels_opt.hidden(false);
        text_current.hidden(false);
        text_charge.hidden(false);
        text_current.set(to_string_dec_int(current) + " mA");
        if (current >= 25)  // when >25mA it is charging in any scenario
            text_charge.set("Charging");
        else {
            if (current > -25) {                         // between -25 and 25
                if (voltage > 4060 || percent == 100) {  // the voltage is high enough, so it is full, that's why no mA
                    text_charge.set("Full");
                } else {
                    text_charge.set("Holding");  // not enough voltage, so should charge. maybe the batttery is off, or USB power is not enough
                }
            } else {
                text_charge.set("Discharging");  // less then -25mA, so it is discharging
            }
        }
        labels_opt.hidden(false);

        text_ttef.hidden(false);
    } else {
        if (!labels_opt.hidden()) uichg = true;
        labels_opt.hidden(true);
        text_current.hidden(true);
        text_charge.hidden(true);
        // text_cycles.hidden(true);
        text_ttef.hidden(true);
        // text_warn.set("");
    }
    /* if ((valid_mask & battery::BatteryManagement::BATT_VALID_CYCLES) == battery::BatteryManagement::BATT_VALID_CYCLES) {
        // text_cycles.hidden(false);
        uint16_t cycles = 0;  // battery::BatteryManagement::get_cycles();
        if (cycles < 2)
            text_warn.set("SoC improves after each cycles");
        else
            text_warn.set("");
        // text_cycles.set(to_string_dec_uint(cycles));
    } else {
        // text_cycles.hidden(true);
        text_warn.set("");
    } */
    if ((valid_mask & battery::BatteryManagement::BATT_VALID_TTEF) == battery::BatteryManagement::BATT_VALID_TTEF) {
        text_ttef.hidden(false);
        float ttef = 0;
        if (current <= 0) {  // we keep this yet
            ttef = battery::BatteryManagement::get_tte();
        } else {
            ttef = battery::BatteryManagement::get_ttf();
        }

        // Convert ttef to hours and minutes
        uint8_t hours = static_cast<uint8_t>(ttef);
        uint8_t minutes = static_cast<uint8_t>((ttef - hours) * 60 + 0.5);  // +0.5 for rounding

        // Create the formatted string
        std::string formatted_time;
        if (hours > 0) {
            formatted_time += to_string_dec_uint(hours) + "h ";
        }
        formatted_time += to_string_dec_uint(minutes) + "m";

        text_ttef.set(formatted_time);
    } else {
        text_ttef.hidden(true);
    }
    if ((valid_mask & battery::BatteryManagement::BATT_VALID_PERCENT) == battery::BatteryManagement::BATT_VALID_PERCENT) {
        text_method.set("IC");
        button_mode.set_text("Volt");
    } else {
        text_method.set("Voltage");
        button_mode.set_text("IC");
    }
    if (uichg) set_dirty();
    // to update status bar too, send message in behalf of batt manager
    BatteryStateMessage msg{valid_mask, percent, current >= 25, voltage};
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
                  &text_method,
                  &button_mode,
                  &button_exit,
                  // &text_cycles,
                  // &text_warn,
                  &text_ttef});

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };
    button_mode.on_select = [this, &nav](Button&) {
        if (button_mode.text() == "IC") {
            battery::BatteryManagement::set_calc_override(false);
            persistent_memory::set_ui_override_batt_calc(false);
            button_mode.set_text("Volt");
        } else {
            battery::BatteryManagement::set_calc_override(true);
            persistent_memory::set_ui_override_batt_calc(true);
            button_mode.set_text("IC");
        }
    };
    update_result();
    if (thread == nullptr) thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, BattinfoView::static_fn, this);
}

msg_t BattinfoView::static_fn(void* arg) {
    auto obj = static_cast<BattinfoView*>(arg);
    while (!chThdShouldTerminate()) {
        chThdSleepMilliseconds(16);
        obj->on_timer();
    }
    return 0;
}

BattinfoView::~BattinfoView() {
    if (thread) {
        chThdTerminate(thread);
        chThdWait(thread);
        thread = nullptr;
    }
}
}  // namespace ui