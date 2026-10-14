/*
 * DECT RX baseband processor for the PortaPack Mayhem DECT RX.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
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
 *
 * Decode pipeline ported from DeDECTive by SarahRose, which ports gr-dect2
 * by Pavel Yazev (GPLv3).
 */

#ifndef __PROC_DECT_H__
#define __PROC_DECT_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "message.hpp"

#include "dect_channels.hpp"
#include "dect_dc_blocker.hpp"
#include "dect_packet_receiver.hpp"
#include "dect_packet_decoder.hpp"

class DECTRxProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 4608000; /* 4 x DECT symbol rate */

    bool configured{false};

    dect::DCBlocker dc_blocker{};
    dect::PhaseDiff phase_diff{};
    dect::PacketReceiver receiver{};
    dect::PacketDecoder decoder{};

    /* Periodic status reported to the app every status_interval blocks. */
    static constexpr uint32_t status_interval = 512;
    uint32_t status_blocks = 0;
    uint8_t last_parts = 0;
    uint8_t last_voice_parts = 0;

    void configure(const DECTRxConfigureMessage& message);
    void reset_pipeline();
    void publish_parts();
    void send_status();

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_DECT_H__*/
