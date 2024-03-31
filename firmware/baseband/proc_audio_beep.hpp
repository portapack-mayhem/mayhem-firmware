/*
 * Copyright (C) 2024 Mark Thompson
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

#ifndef __PROC_AUDIO_BEEP_H__
#define __PROC_AUDIO_BEEP_H__

#include "baseband_processor.hpp"
#include "message.hpp"

class AudioBeepProcessor : public BasebandProcessor {
   public:
    AudioBeepProcessor();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const msg);

   private:
    void on_signal_message(const RequestSignalMessage& message);
    void on_beep_message(const AudioBeepMessage& message);
};

#endif /*__PROC_AUDIO_BEEP_H__*/
