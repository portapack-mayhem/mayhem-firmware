/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifndef __COUNTRY_RX_H__
#define __COUNTRY_RX_H__

#include <cstdint>
#include <string>

namespace ui::external_app::epirb_rx {

class Country
{
    public:
        int code{0};
        std::string alphaCode{};
        std::string longName{};
        std::string shortName{};
        void setValues(int code, std::string alphaCode, std::string longName, std::string shortName);
        std::string toString();
        static Country getCountry(int code);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __COUNTRY_RX_H__