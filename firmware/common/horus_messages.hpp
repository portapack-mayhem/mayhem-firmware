/*
 * Copyright (C) 2026 OE1LBS
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

#ifndef __HORUS_MESSAGES_H__
#define __HORUS_MESSAGES_H__

#include "message.hpp"

/* Messages between the Horus RX baseband image and its app.
 *
 * They ride on the FT8 RX message IDs instead of adding new ones. The M0's handler
 * table is sized by Message::ID::MAX inside the main firmware, so new IDs would only
 * work with a rebuilt firmware; reusing IDs keeps the app installable from the SD card
 * alone on a stock build. That is safe because a message only goes to the handlers of
 * the app that is running, and FT8 RX and Horus RX never run at the same time. */

/* A Horus Binary frame that passed its CRC. The payload is passed on raw (CRC
 * included); the app decodes the v1/v2 struct or the v3 ASN.1 content. */
class HorusPacketMessage : public Message {
   public:
    static constexpr ID message_id = ID::FT8Packet;
    static constexpr size_t max_size = 128;

    constexpr HorusPacketMessage()
        : Message{message_id} {
    }

    uint8_t data[max_size]{};
    uint8_t size{0};
    uint8_t version{0};         // 1, 2 or 3
    uint8_t bits_corrected{0};  // Golay-corrected bit errors
    int16_t snr_x10{0};         // per-symbol SNR, dB * 10
    int16_t freq_hz{0};         // centre of the four tones, relative to the tuned frequency
};

class HorusRxStatusMessage : public Message {
   public:
    static constexpr ID message_id = ID::FT8RxStatus;

    constexpr HorusRxStatusMessage()
        : Message{message_id} {
    }

    int16_t tone_hz[4]{};     // estimated tones relative to the tuned frequency
    int16_t snr_x10{0};       // per-symbol SNR, dB * 10
    int16_t signal_x10{0};    // tone power over the spectrum noise floor, dB * 10
    bool tones_valid{false};  // four tones have been found at least once

    // diagnostics
    uint16_t symbols{0};           // symbols decided in the last status interval
    uint16_t uw_hits{0};           // unique words seen since start, normal tone order
    uint16_t uw_hits_mirrored{0};  // the same for the mirrored tone order
    uint16_t load_pct{0};          // slowest baseband buffer of the interval, % of the buffer period
};

#endif /*__HORUS_MESSAGES_H__*/
