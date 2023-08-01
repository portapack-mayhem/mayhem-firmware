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

#ifndef __FIELD_BINDER_H__
#define __FIELD_BINDER_H__

/* There's a common pattern that appears in applications that use
 * field widgets. The field value is set, on_change and on_select
 * handlers are registered. on_select will read the value and
 * set the new value once changed. on_change will be called which
 * will write the new value to the backing variable. */

struct binding_traits {
};

template <typename T>
constexpr auto get_traits() {
    return {};
}

template <typename TField, typename TTraits = get_traits<TField>()>
class FieldBinding {
};

#endif /*__FIELD_BINDER_H__*/
