/*
 * Copyright (C) 2026 Ivan
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

/* HOST TEST SHIM: the ui::Rect members declared in the real
 * firmware/common/ui.hpp but defined in firmware/common/ui.cpp, which is
 * not host-compilable (irq_controls, sine tables). The implementations
 * below are copied verbatim from ui.cpp so the on_touch path behaves
 * exactly like the device. */

#include "ui.hpp"

#include <algorithm>

namespace ui {

bool Rect::contains(const Point p) const {
    return (p.x() >= left()) && (p.y() >= top()) &&
           (p.x() < right()) && (p.y() < bottom());
}

Rect Rect::intersect(const Rect& o) const {
    const auto x1 = std::max(left(), o.left());
    const auto x2 = std::min(right(), o.right());
    const auto y1 = std::max(top(), o.top());
    const auto y2 = std::min(bottom(), o.bottom());
    if ((x2 >= x1) && (y2 > y1)) {
        return {x1, y1, x2 - x1, y2 - y1};
    } else {
        return {};
    }
}

Rect& Rect::operator+=(const Rect& p) {
    if (is_empty()) {
        *this = p;
    }
    if (!p.is_empty()) {
        const auto x1 = std::min(left(), p.left());
        const auto y1 = std::min(top(), p.top());
        const auto x2 = std::max(right(), p.right());
        const auto y2 = std::max(bottom(), p.bottom());
        *this = Rect{x1, y1, x2 - x1, y2 - y1};
    }
    return *this;
}

Rect& Rect::operator+=(const Point& p) {
    *this = Rect{left() + p.x(), top() + p.y(), width(), height()};
    return *this;
}

} /* namespace ui */
