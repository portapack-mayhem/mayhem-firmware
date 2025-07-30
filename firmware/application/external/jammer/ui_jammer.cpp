/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2025 RocketGod - Added modes from my Flipper Zero RF Jammer App - https://betaskynet.com
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

#include "ui_jammer.hpp"
#include "ui_receiver.hpp"
#include "ui_freqman.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::jammer {

void RangeView::focus() {
    check_enabled.focus();
}

void RangeView::update_start(rf::Frequency f) {
    frequency_range.min = f;
    button_start.set_text(to_string_short_freq(f));

    center = (frequency_range.min + frequency_range.max) / 2;
    width = abs(frequency_range.max - frequency_range.min);

    button_center.set_text(to_string_short_freq(center));
    button_width.set_text(to_string_short_freq(width));
}

void RangeView::update_stop(rf::Frequency f) {
    frequency_range.max = f;
    button_stop.set_text(to_string_short_freq(f));

    center = (frequency_range.min + frequency_range.max) / 2;
    width = abs(frequency_range.max - frequency_range.min);

    button_center.set_text(to_string_short_freq(center));
    button_width.set_text(to_string_short_freq(width));
}

void RangeView::update_center(rf::Frequency f) {
    center = f;
    button_center.set_text(to_string_short_freq(center));

    rf::Frequency min = center - (width / 2);
    rf::Frequency max = min + width;

    frequency_range.min = min;
    button_start.set_text(to_string_short_freq(min));

    frequency_range.max = max;
    button_stop.set_text(to_string_short_freq(max));
}

void RangeView::update_width(uint32_t w) {
    width = w;

    button_width.set_text(to_string_short_freq(width));

    rf::Frequency min = center - (width / 2);
    rf::Frequency max = min + width;

    frequency_range.min = min;
    button_start.set_text(to_string_short_freq(min));

    frequency_range.max = max;
    button_stop.set_text(to_string_short_freq(max));
}

void RangeView::paint(Painter&) {
    Rect r;
    Point p;
    Coord c;

    r = button_center.screen_rect();
    p = r.center() + Point(0, r.height() / 2);

    display.draw_line(p, p + Point(0, 10), Theme::getInstance()->fg_medium->foreground);

    r = button_width.screen_rect();
    c = r.top() + (r.height() / 2);

    p = {r.left() - 64, c};
    display.draw_line({r.left(), c}, p, Theme::getInstance()->fg_medium->foreground);
    display.draw_line(p, p + Point(10, -10), Theme::getInstance()->fg_medium->foreground);
    display.draw_line(p, p + Point(10, 10), Theme::getInstance()->fg_medium->foreground);

    p = {r.right() + 64, c};
    display.draw_line({r.right(), c}, p, Theme::getInstance()->fg_medium->foreground);
    display.draw_line(p, p + Point(-10, -10), Theme::getInstance()->fg_medium->foreground);
    display.draw_line(p, p + Point(-10, 10), Theme::getInstance()->fg_medium->foreground);
}

RangeView::RangeView(NavigationView& nav) {
    hidden(true);

    add_children({&labels,
                  &check_enabled,
                  &button_load_range,
                  &button_start,
                  &button_stop,
                  &button_center,
                  &button_width});

    check_enabled.on_select = [this](Checkbox&, bool v) {
        frequency_range.enabled = v;
    };

    button_start.on_select = [this, &nav](Button& button) {
        auto new_view = nav.push<FrequencyKeypadView>(frequency_range.min);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            update_start(f);
        };
    };

    button_stop.on_select = [this, &nav](Button& button) {
        auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            update_stop(f);
        };
    };

    button_center.on_select = [this, &nav](Button& button) {
        auto new_view = nav.push<FrequencyKeypadView>(center);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            update_center(f);
        };
    };

    button_width.on_select = [this, &nav](Button& button) {
        auto new_view = nav.push<FrequencyKeypadView>(width);
        new_view->on_changed = [this, &button](rf::Frequency f) {
            update_width(f);
        };
    };

    button_load_range.on_select = [this, &nav](Button&) {
        auto load_view = nav.push<FrequencyLoadView>();
        load_view->on_frequency_loaded = [this](rf::Frequency value) {
            update_center(value);
            update_width(100000);
        };
        load_view->on_range_loaded = [this](rf::Frequency start, rf::Frequency stop) {
            update_start(start);
            update_stop(stop);
        };
    };

    check_enabled.set_value(false);
}

void JammerView::focus() {
    tab_view.focus();
}

JammerView::~JammerView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void JammerView::on_retune(const rf::Frequency freq, const uint32_t range) {
    if (freq) {
        transmitter_model.set_target_frequency(freq);
        text_range_number.set(to_string_dec_uint(range, 2));
    }
}

void JammerView::set_jammer_channel(uint32_t i, uint32_t width, uint64_t center, uint32_t duration) {
    jammer_channels[i].enabled = true;
    jammer_channels[i].width = (width * 0xFFFFFFULL) / 1536000;
    jammer_channels[i].center = center;
    jammer_channels[i].duration = duration ? 30720 * duration : 3000;
}

void JammerView::start_tx() {
    if (update_config()) {
        jamming = true;
        button_transmit.set_style(&style_cancel);
        button_transmit.set_text("STOP");

        transmitter_model.set_rf_amp(field_amp.value());
        transmitter_model.set_tx_gain(field_gain.value());
        transmitter_model.set_baseband_bandwidth(28'000'000);
        transmitter_model.enable();

        baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());
        mscounter = 0;
    }
}

void JammerView::stop_tx() {
    button_transmit.set_style(&style_val);
    button_transmit.set_text("START");
    transmitter_model.disable();
    baseband::set_jammer(false, JammerType::TYPE_FSK, 0);
    jamming = false;
    cooling = false;
    seconds = 0;
    mscounter = 0;
}

bool JammerView::update_config() {
    uint32_t c, i = 0;
    size_t num_channels;
    rf::Frequency start_freq, range_bw, range_bw_sub, ch_width;
    bool out_of_ranges = false;

    size_t hop_value = options_hop.selected_index_value();

    for (c = 0; c < JAMMER_MAX_CH; c++)
        jammer_channels[c].enabled = false;

    for (size_t r = 0; r < 3; r++) {
        if (range_views[r]->frequency_range.enabled) {
            range_bw = abs(range_views[r]->frequency_range.max - range_views[r]->frequency_range.min);

            if (range_views[r]->frequency_range.min < range_views[r]->frequency_range.max)
                start_freq = range_views[r]->frequency_range.min;
            else
                start_freq = range_views[r]->frequency_range.max;

            if (range_bw >= JAMMER_CH_WIDTH) {
                num_channels = 0;
                range_bw_sub = range_bw;

                do {
                    range_bw_sub -= JAMMER_CH_WIDTH;
                    num_channels++;
                } while (range_bw_sub >= JAMMER_CH_WIDTH);

                ch_width = range_bw / num_channels;

                for (c = 0; c < num_channels; c++) {
                    if (i >= JAMMER_MAX_CH) {
                        out_of_ranges = true;
                        break;
                    }
                    set_jammer_channel(i, ch_width, start_freq + (ch_width / 2) + (ch_width * c), hop_value);
                    i++;
                }
            } else {
                if (i >= JAMMER_MAX_CH) {
                    out_of_ranges = true;
                } else {
                    set_jammer_channel(i, range_bw, start_freq + (range_bw / 2), hop_value);
                    i++;
                }
            }
        }
    }

    if (!out_of_ranges && i) {
        text_range_total.set("/" + to_string_dec_uint(i, 2));
        return true;
    } else {
        if (out_of_ranges)
            nav_.display_modal("Error", "Jamming bandwidth too large.\nMust be 80MHz or less.");
        else
            nav_.display_modal("Error", "No range enabled.");
        return false;
    }
}

void JammerView::on_timer() {
    if (++mscounter >= 60) {
        mscounter = 0;
        if (jamming) {
            int32_t timepause = field_timepause.value();
            if (cooling) {
                if (timepause == 0 || ++seconds >= timepause) {
                    transmitter_model.set_baseband_bandwidth(28'000'000);
                    transmitter_model.enable();
                    button_transmit.set_text("STOP");
                    baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());

                    int32_t jitter_amount = field_jitter.value();
                    if (jitter_amount) {
                        lfsr_v = lfsr_iterate(lfsr_v);
                        jitter_amount = (jitter_amount / 2) - (lfsr_v & jitter_amount);
                        mscounter += jitter_amount;
                    }

                    cooling = false;
                    seconds = 0;
                }
            } else {
                if (timepause && ++seconds >= field_timetx.value()) {
                    transmitter_model.disable();
                    button_transmit.set_text("PAUSED");
                    baseband::set_jammer(false, JammerType::TYPE_FSK, 0);

                    int32_t jitter_amount = field_jitter.value();
                    if (jitter_amount) {
                        lfsr_v = lfsr_iterate(lfsr_v);
                        jitter_amount = (jitter_amount / 2) - (lfsr_v & jitter_amount);
                        mscounter += jitter_amount;
                    }

                    cooling = true;
                    seconds = 0;
                }
            }
        }
    }
}

JammerView::JammerView(NavigationView& nav)
    : nav_{nav} {
    Rect view_rect = {0, 3 * 8, screen_width, 80};
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&tab_view,
                  &view_range_a,
                  &view_range_b,
                  &view_range_c,
                  &labels,
                  &options_type,
                  &text_range_number,
                  &text_range_total,
                  &options_speed,
                  &options_hop,
                  &field_timetx,
                  &field_timepause,
                  &field_jitter,
                  &field_gain,
                  &field_amp,
                  &button_transmit});

    view_range_a.set_parent_rect(view_rect);
    view_range_b.set_parent_rect(view_rect);
    view_range_c.set_parent_rect(view_rect);

    view_range_a.check_enabled.set_value(true);
    view_range_a.frequency_range.enabled = true;
    view_range_a.update_center(315'000'000);
    view_range_a.update_width(1'000'000);

    options_type.set_selected_index(3);
    options_speed.set_selected_index(3);
    options_hop.set_selected_index(0);
    field_timetx.set_value(30);
    field_timepause.set_value(0);
    field_jitter.set_value(0);
    field_gain.set_value(transmitter_model.tx_gain());
    field_amp.set_value(transmitter_model.rf_amp());
    button_transmit.set_style(&style_val);

    options_type.on_change = [this](size_t, OptionsField::value_t) {
        if (jamming) update_config();
        if (jamming && !cooling) baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());
    };

    options_speed.on_change = [this](size_t, OptionsField::value_t) {
        if (jamming) update_config();
        if (jamming && !cooling) baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());
    };

    options_hop.on_change = [this](size_t, OptionsField::value_t) {
        if (jamming) update_config();
    };

    field_timetx.on_change = [this](int32_t) {
        if (jamming) update_config();
    };

    field_timepause.on_change = [this](int32_t) {
        if (jamming) update_config();
    };

    field_jitter.on_change = [this](int32_t) {
        if (jamming) update_config();
    };

    field_gain.on_change = [this](int32_t v) {
        if (jamming) transmitter_model.set_tx_gain(v);
    };

    field_amp.on_change = [this](int32_t v) {
        if (jamming) transmitter_model.set_rf_amp(v);
    };

    for (auto range_view : range_views) {
        range_view->check_enabled.on_select = [this](Checkbox&, bool) {
            if (jamming) update_config();
        };
        range_view->button_start.on_select = [this, range_view](Button&) {
            auto new_view = nav_.push<FrequencyKeypadView>(range_view->frequency_range.min);
            new_view->on_changed = [this, range_view](rf::Frequency f) {
                range_view->update_start(f);
                if (jamming) update_config();
            };
        };
        range_view->button_stop.on_select = [this, range_view](Button&) {
            auto new_view = nav_.push<FrequencyKeypadView>(range_view->frequency_range.max);
            new_view->on_changed = [this, range_view](rf::Frequency f) {
                range_view->update_stop(f);
                if (jamming) update_config();
            };
        };
        range_view->button_center.on_select = [this, range_view](Button&) {
            auto new_view = nav_.push<FrequencyKeypadView>(range_view->center);
            new_view->on_changed = [this, range_view](rf::Frequency f) {
                range_view->update_center(f);
                if (jamming) update_config();
            };
        };
        range_view->button_width.on_select = [this, range_view](Button&) {
            auto new_view = nav_.push<FrequencyKeypadView>(range_view->width);
            new_view->on_changed = [this, range_view](rf::Frequency f) {
                range_view->update_width(f);
                if (jamming) update_config();
            };
        };
        range_view->button_load_range.on_select = [this, range_view](Button&) {
            auto load_view = nav_.push<FrequencyLoadView>();
            load_view->on_frequency_loaded = [this, range_view](rf::Frequency value) {
                range_view->update_center(value);
                range_view->update_width(100000);
                if (jamming) update_config();
            };
            load_view->on_range_loaded = [this, range_view](rf::Frequency start, rf::Frequency stop) {
                range_view->update_start(start);
                range_view->update_stop(stop);
                if (jamming) update_config();
            };
        };
    }

    button_transmit.on_select = [this](Button&) {
        if (jamming || cooling)
            stop_tx();
        else
            start_tx();
    };
}

}  // namespace ui::external_app::jammer
