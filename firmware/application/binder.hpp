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

#ifndef __BINDER_H__
#define __BINDER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"

namespace ui {

/* Default no-op binding callback. */
struct NoOp {
    NoOp() {}
    template <typename T>
    void operator()(T) const {}
};

/* The gist of the bind functions is to bind a control
 * to a value.
 *     bind(control, value)
 *
 * An optional callback can be specified which will be called
 * when the control's value is changed.
 *    bind(control, value, [](auto new_val) { ... });
 *
 * Some controls need a reference to the NavigationView so
 * edit dialogs (like text_prompt) can be launched.
 *    bind(control, value, nav, [](auto new_val) { ... });
 *
 * Be careful with lifetime of captured objects. Most things
 * are captured by reference so the caller will need to ensure
 * adequate lifetime of the referenced instances.
 */

template <typename T, typename Fn = NoOp>
void bind(Checkbox& check, T& value, Fn fn = Fn{}) {
    check.set_value(value);
    check.on_select = [&value, fn](Checkbox&, bool b) {
        value = b;
        fn(value);
    };
}

template <typename T, typename Fn = NoOp>
void bind(NumberField& field, T& value, Fn fn = Fn{}) {
    field.set_value(value);
    field.on_change = [&value, fn](int32_t v) {
        value = v;
        fn(value);
    };
}

template <typename T, typename Fn = NoOp>
void bind(OptionsField& field, T& value, Fn fn = Fn{}) {
    field.set_by_value(static_cast<int32_t>(value));
    field.on_change = [&value, fn](size_t, auto v) {
        value = static_cast<T>(v);
        fn(value);
    };
}

template <typename T, typename Fn = NoOp>
void bind(FrequencyField& field, T& value, NavigationView& nav, Fn fn = Fn{}) {
    field.set_value(value);
    field.on_change = [&value, fn](rf::Frequency f) {
        value = f;
        fn(value);
    };
    field.on_edit = [&field, &value, &nav]() {
        auto freq_view = nav.push<FrequencyKeypadView>(value);
        freq_view->on_changed = [&field](rf::Frequency f) {
            field.set_value(f);
        };
    };
}

template <typename T, typename Fn = NoOp>
void bind(TextField& field, T& value, NavigationView& nav, Fn fn = Fn{}) {
    field.set_text(value);
    field.on_change = [&value, fn](TextField& tf) {
        value = tf.get_text();
        fn(value);
    };
    // text_prompt needs a mutable working buffer.
    // Capture a new string and make the lambda mutable so it can be modified.
    field.on_select = [&nav, buf = std::string{}](TextField& tf) mutable {
        buf = tf.get_text();
        text_prompt(nav, buf, /*max_length*/ 255,
                    [&tf](std::string& str) {
                        tf.set_text(str);
                    });
    };
}

}  // namespace ui

#endif /*__BINDER_H__*/