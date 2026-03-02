/*
 * Copyleft zxkmm (>) 2026
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

#ifndef __PROC_TIME_SINK_H__
#define __PROC_TIME_SINK_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "message.hpp"

#include <cstddef>

class TimeSinkProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    bool configured = false;
    size_t baseband_fs = 20000000;

    void execute_time_domain(const buffer_c8_t& buffer);
    void set_time_streaming_state(const SpectrumStreamingConfigMessage& message);
    void update_time_domain();
    ChannelSpectrum time_domain_spectrum{};
    ChannelSpectrum fifo_data[1 << ChannelSpectrumConfigMessage::fifo_k]{};
    ChannelSpectrumFIFO fifo{fifo_data, ChannelSpectrumConfigMessage::fifo_k};

    size_t phase = 0, trigger = 127;
    bool time_streaming = false;
    volatile bool time_domain_request_update = false;

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
};

#endif /*__PROC_TIME_SINK_H__*/
