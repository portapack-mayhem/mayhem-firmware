/*
 * Copyright (C) 2026 zxkmm
 * Copyright (C) 2026 gullradriel, Nilorea Studio Inc.
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

#ifndef __PROC_SPEC_AN_H__
#define __PROC_SPEC_AN_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "message.hpp"
#include "spec_an_engine.hpp"

class SpecAnProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    spec_an::Engine engine{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{20000000, this, baseband::Direction::Receive};
};

#endif /*__PROC_SPEC_AN_H__*/
