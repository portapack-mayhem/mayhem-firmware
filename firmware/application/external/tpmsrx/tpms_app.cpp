/*
 * Copyright (C) 2026
 * Copyright (C) 2025 Speedster04 (EU + World protocol TX encoders)
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "tpms_tx_app.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "spi_image.hpp"
#include "ui_fileman.hpp"
#include "ui_freqman.hpp"
#include "manchester.hpp"
#include "rtc_time.hpp"
#include "file_path.hpp"
#include "encoders.hpp"
#include "crc.hpp"

using namespace portapack;
using namespace tpms;

namespace ui::external_app::tpmstx {

void TPMSTXView::focus() {
    options_packet_type.focus();
}

void TPMSTXView::update_signal_type_from_packet() {
    switch (packet_type_) {
        // Original OOK
        case tpms::Reading::Type::Schrader:
            signal_type_ = tpms::SignalType::OOK_8k192_Schrader;
            break;
        case tpms::Reading::Type::GMC_96:
            signal_type_ = tpms::SignalType::OOK_8k4_Schrader;
            break;
        // FSK 19k2 - standard preamble (FLM + EU shared-path)
        case tpms::Reading::Type::FLM_64:
        case tpms::Reading::Type::FLM_72:
        case tpms::Reading::Type::FLM_80:
        case tpms::Reading::Type::Ford:
        case tpms::Reading::Type::Citroen_PSA:
        case tpms::Reading::Type::Renault:
        case tpms::Reading::Type::Jansite:
        case tpms::Reading::Type::SolarTruck:
            signal_type_ = tpms::SignalType::FSK_19k2_Schrader;
            break;
        // FSK ~40k - BMW Gen4/5
        case tpms::Reading::Type::BMW_G45:
            signal_type_ = tpms::SignalType::FSK_38k4_BMW_G45;
            break;
        // FSK 19k2 NRZI - BMW G23, Porsche, Toyota
        case tpms::Reading::Type::BMW_G23:
            signal_type_ = tpms::SignalType::FSK_19k2_BMW_G23;
            break;
        case tpms::Reading::Type::Porsche:
            signal_type_ = tpms::SignalType::FSK_19k2_Porsche;
            break;
        case tpms::Reading::Type::Toyota:
            signal_type_ = tpms::SignalType::FSK_19k2_Toyota;
            break;
        // FSK 19k2 std Manchester - Elantra
        case tpms::Reading::Type::Elantra:
            signal_type_ = tpms::SignalType::FSK_19k2_Elantra;
            break;
        // FSK 19k2 inv Manchester - Jansite Solar
        case tpms::Reading::Type::JansiteSolar:
            signal_type_ = tpms::SignalType::FSK_19k2_JansiteSolar;
            break;
        default:
            signal_type_ = tpms::SignalType::OOK_8k192_Schrader;
            break;
    }
}

void TPMSTXView::switch_baseband() {
    baseband::shutdown();
    chThdSleepMilliseconds(100);
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        baseband::run_image(portapack::spi_flash::image_tag_ook);
    } else {
        baseband::run_image(portapack::spi_flash::image_tag_fsktx);
    }
    chThdSleepMilliseconds(100);
}

void TPMSTXView::update_bar_display() {
    int bar10 = pressure_kpa_ / 10;
    text_bar_tx.set("(" + to_string_dec_int(bar10 / 10, 1) + "." +
                    to_string_dec_int(bar10 % 10, 1) + " BAR)");
}

void TPMSTXView::update_packet_display() {
    update_bar_display();
    std::string status = "ID:" + to_string_hex(transponder_id_, 8);
    if (packet_type_ == tpms::Reading::Type::Schrader)
        status = "ID:" + to_string_hex(transponder_id_ & 0x00FFFFFF, 6) + " (24bit)";
    status += " " + to_string_dec_uint(pressure_kpa_) + "kPa";
    text_status.set(status);
}

void TPMSTXView::update_field_visibility() {
    bool has_temp = (packet_type_ != tpms::Reading::Type::Schrader);
    bool has_flags = (packet_type_ == tpms::Reading::Type::Schrader);
    bool id_24bit = (packet_type_ == tpms::Reading::Type::Schrader);

    label_flags.hidden(!has_flags);
    field_flags.hidden(!has_flags);
    label_temperature.hidden(!has_temp);
    field_temperature.hidden(!has_temp);
    options_temperature.hidden(!has_temp);
    field_transponder_id_24.hidden(!id_24bit);
    field_transponder_id_32.hidden(id_24bit);
    set_dirty();
}

void TPMSTXView::on_pressure_unit_change() {
    units::Pressure pressure(pressure_kpa_);
    int v = (format::pressure_unit == PRESSURE_UNIT_PSI) ? pressure.psi()
          : (format::pressure_unit == PRESSURE_UNIT_BAR) ? pressure.bar()
          : pressure.kilopascal();
    field_pressure.set_value(v);
}

void TPMSTXView::on_temperature_unit_change() {
    units::Temperature temperature(temperature_c_);
    int v = (format::temp_unit == TEMP_UNIT_FAHRENHEIT) ? temperature.fahrenheit()
          : temperature.celsius();
    field_temperature.set_value(v);
}

// ===========================================================================
// encode_and_transmit - builds binary_string for every protocol
// ===========================================================================
void TPMSTXView::encode_and_transmit() {
    if (!is_transmitting_) return;

    std::string binary_string;
    uint32_t symbol_rate;
    uint32_t sample_rate = 2280000;

    // -----------------------------------------------------------------------
    // Helper lambdas
    // -----------------------------------------------------------------------
    // Standard Manchester (sense=0): 1-"10", 0-"01"
    auto mc = [](bool b) -> const char* { return b ? "10" : "01"; };
    // Inverted Manchester (sense=1): 1-"01", 0-"10"
    auto mci = [](bool b) -> const char* { return b ? "01" : "10"; };

    // Standard FSK preamble: "01"-14 + "10" (30 raw bits)
    auto add_std_preamble = [&]() {
        for (int i = 0; i < 14; i++) binary_string += "01";
        binary_string += "10";
    };

    // Manchester-encode byte array (standard sense=0)
    auto mc_encode_bytes = [&](const uint8_t* b, size_t n) {
        for (size_t i = 0; i < n; i++)
            for (int bit = 7; bit >= 0; bit--)
                binary_string += mc((b[i] >> bit) & 1);
    };

    // Inverted-Manchester-encode byte array (sense=1)
    auto mci_encode_bytes = [&](const uint8_t* b, size_t n) {
        for (size_t i = 0; i < n; i++)
            for (int bit = 7; bit >= 0; bit--)
                binary_string += mci((b[i] >> bit) & 1);
    };

    // NRZI-encode byte array (raw bits, prev = last preamble bit)
    auto nrzi_encode_bytes = [&](const uint8_t* b, size_t n, uint_fast8_t& prev) {
        for (size_t i = 0; i < n; i++) {
            for (int bit = 7; bit >= 0; bit--) {
                uint_fast8_t d = (b[i] >> bit) & 1;
                uint_fast8_t out = (d == 1) ? prev : (1 - prev);
                binary_string += (char)('0' + out);
                prev = out;
            }
        }
    };

    // -----------------------------------------------------------------------
    // OOK: Schrader and GMC_96 (unchanged from original)
    // -----------------------------------------------------------------------
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader) {
        symbol_rate = 8192;
        binary_string = "1111";
        for (int i = 0; i < 14; i++) binary_string += "01";
        binary_string += "1110";
        uint64_t data = 0;
        uint8_t flags_3bit = flags_ & 0x07;
        data |= ((uint64_t)flags_3bit << 34);
        uint32_t id_24bit = transponder_id_ & 0x00FFFFFF;
        data |= ((uint64_t)id_24bit << 10);
        uint16_t pc = (pressure_kpa_ > 340) ? 340 : pressure_kpa_;
        data |= ((uint64_t)(pc * 3 / 4) << 2);
        uint32_t cs = (data >> 36) & 1;
        for (size_t i = 1; i < 37; i += 2) cs += (data >> (37 - i - 2)) & 3;
        data |= (3 - (cs & 3)) & 3;
        for (int i = 36; i >= 0; i--)
            binary_string += ((data >> i) & 1) ? "10" : "01";
        goto transmit;
    }

    if (signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        symbol_rate = 8400;
        for (int i = 0; i < 40; i++) binary_string += "01";
        binary_string += "01100101";
        for (int i = 0; i < 20; i++) binary_string += "01";
        for (int i = 31; i >= 0; i--)
            binary_string += ((transponder_id_ >> i) & 1) ? "10" : "01";
        uint16_t pc = (pressure_kpa_ > 701) ? 701 : pressure_kpa_;
        uint8_t pg = pc * 4 / 11;
        uint8_t tg = (temperature_c_ + 61) & 0xFF;
        for (int i = 7; i >= 0; i--) binary_string += ((pg >> i) & 1) ? "10" : "01";
        for (int i = 7; i >= 0; i--) binary_string += ((tg >> i) & 1) ? "10" : "01";
        uint8_t chk = 0x40;
        for (int b = 24; b >= 0; b -= 8) chk += (transponder_id_ >> b) & 0xFF;
        chk += pg; chk += tg;
        for (int i = 7; i >= 0; i--) binary_string += ((chk >> i) & 1) ? "10" : "01";
        goto transmit;
    }

    // All remaining are FSK
    symbol_rate = 19200;

    // -----------------------------------------------------------------------
    // FLM_64 / FLM_72 / FLM_80 (unchanged from original)
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::FLM_64 ||
        packet_type_ == Reading::Type::FLM_72 ||
        packet_type_ == Reading::Type::FLM_80) {
        add_std_preamble();
        std::array<uint8_t, 20> db = {0};
        size_t nbits = 0;
        if (packet_type_ == Reading::Type::FLM_64) {
            nbits = 64;
            db[0]=(transponder_id_>>24)&0xFF; db[1]=(transponder_id_>>16)&0xFF;
            db[2]=(transponder_id_>>8)&0xFF;  db[3]=transponder_id_&0xFF;
            uint16_t pc=(pressure_kpa_>340)?340:pressure_kpa_;
            db[4]=pc*3/4; db[5]=(temperature_c_+56)&0x7F; db[6]=0;
            uint32_t s=0; for(int i=0;i<7;i++) s+=db[i]; db[7]=s&0xFF;
        } else if (packet_type_ == Reading::Type::FLM_72) {
            nbits = 72;
            db[0]=(transponder_id_>>24)&0xFF; db[1]=(transponder_id_>>16)&0xFF;
            db[2]=(transponder_id_>>8)&0xFF;  db[3]=transponder_id_&0xFF; db[4]=0;
            uint16_t pc=(pressure_kpa_>340)?340:pressure_kpa_;
            db[5]=pc*3/4; db[6]=(temperature_c_+56)&0xFF; db[7]=0;
            CRC<8> c{0x01,0x00}; for(int i=0;i<8;i++) c.process_byte(db[i]);
            db[8]=c.checksum()&0xFF;
        } else {
            nbits = 80;
            db[0]=0; db[1]=(transponder_id_>>24)&0xFF; db[2]=(transponder_id_>>16)&0xFF;
            db[3]=(transponder_id_>>8)&0xFF; db[4]=transponder_id_&0xFF; db[5]=0;
            uint16_t pc=(pressure_kpa_>340)?340:pressure_kpa_;
            db[6]=pc*3/4; db[7]=(temperature_c_+56)&0xFF; db[8]=0; db[9]=0;
            CRC<8> c{0x01,0x00}; for(int i=1;i<=8;i++) c.process_byte(db[i]);
            db[9]=c.checksum()&0xFF;
        }
        mc_encode_bytes(db.data(), nbits/8);
        while (binary_string.length() < 190) binary_string += "01";
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Ford/VDO - standard Manchester, preamble 0x55 0x56
    // Packet: II II II II PP TT FF CC (8 bytes)
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Ford) {
        add_std_preamble();
        uint8_t b[8] = {};
        b[0]=(transponder_id_>>24)&0xFF; b[1]=(transponder_id_>>16)&0xFF;
        b[2]=(transponder_id_>>8)&0xFF;  b[3]=transponder_id_&0xFF;
        // pressure: kPa - psi_raw (9-bit). kPa = psi_raw * 172/100 - psi_raw = kPa * 100/172
        int psi_raw = (int)pressure_kpa_ * 100 / 172;
        if (psi_raw > 511) psi_raw = 511;
        b[4] = psi_raw & 0xFF;
        b[5] = (temperature_c_ + 56) & 0x7F;  // bit7=0 = temp valid
        b[6] = flags_ & 0xDF;  // clear bit5 (will set if psi_raw>=256)
        if (psi_raw >= 256) b[6] |= 0x20;
        uint8_t sum = 0;
        for (int i = 0; i < 7; i++) sum += b[i];
        b[7] = sum;
        mc_encode_bytes(b, 8);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Citro-n/PSA - standard Manchester
    // Packet: SS II II II II FR PP TT BB -- (10 bytes)
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Citroen_PSA) {
        add_std_preamble();
        uint8_t b[10] = {};
        b[0] = 0x00;  // state byte (not in checksum)
        b[1]=(transponder_id_>>24)&0xFF; b[2]=(transponder_id_>>16)&0xFF;
        b[3]=(transponder_id_>>8)&0xFF;  b[4]=transponder_id_&0xFF;
        b[5] = flags_;  // flags nibble (high) + repeat counter (low)
        // kPa = b[6] * 341/250 - b[6] = kPa * 250/341
        b[6] = static_cast<uint8_t>(pressure_kpa_ * 250 / 341);
        if (b[6] == 0) b[6] = 1;  // must be non-zero
        b[7] = static_cast<uint8_t>(temperature_c_ + 50);  // -C = b[7]-50
        if (b[7] == 0) b[7] = 1;  // must be non-zero
        b[8] = 0x00;  // battery
        // CRC: XOR(b[1..9]) = 0 - b[9] = XOR(b[1..8])
        uint8_t crc = 0;
        for (int i = 1; i < 9; i++) crc ^= b[i];
        b[9] = crc;
        mc_encode_bytes(b, 10);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Renault/Dacia - standard Manchester
    // Packet: FF PP TT II II II ?? ?? CC (9 bytes)
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Renault) {
        add_std_preamble();
        uint8_t b[9] = {};
        // pressure: 10-bit raw = kPa*4/3 - kPa = raw*3/4
        // raw = kPa * 4/3
        int pressure_raw = pressure_kpa_ * 4 / 3;
        if (pressure_raw > 1023) pressure_raw = 1023;
        b[0] = (static_cast<uint8_t>(flags_ & 0x3F) << 2) |
               static_cast<uint8_t>((pressure_raw >> 8) & 0x03);
        b[1] = pressure_raw & 0xFF;
        b[2] = static_cast<uint8_t>(temperature_c_ + 30);
        // ID: 24-bit little-endian: b[5]<<16 | b[4]<<8 | b[3]
        b[3] = transponder_id_ & 0xFF;
        b[4] = (transponder_id_ >> 8) & 0xFF;
        b[5] = (transponder_id_ >> 16) & 0xFF;
        b[6] = 0xFF; b[7] = 0xFF;  // unknown (often 0xFFFF)
        CRC<8> crc{0x07, 0x00};
        for (int i = 0; i < 8; i++) crc.process_byte(b[i]);
        b[8] = crc.checksum() & 0xFF;
        mc_encode_bytes(b, 9);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Elantra/Honda TRW GQ4-44T - standard Manchester
    // Packet: PP TT ID ID ID ID FF CC (8 bytes)
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Elantra) {
        add_std_preamble();
        uint8_t b[8] = {};
        // kPa = b[0] + 60 - b[0] = kPa - 60
        b[0] = static_cast<uint8_t>((pressure_kpa_ >= 60) ? pressure_kpa_ - 60 : 0);
        b[1] = static_cast<uint8_t>(temperature_c_ + 50);
        b[2]=(transponder_id_>>24)&0xFF; b[3]=(transponder_id_>>16)&0xFF;
        b[4]=(transponder_id_>>8)&0xFF;  b[5]=transponder_id_&0xFF;
        b[6] = flags_ & 0x07;  // bits: storage|battery_low|triggered
        CRC<8> crc{0x07, 0x00};
        for (int i = 0; i < 7; i++) crc.process_byte(b[i]);
        b[7] = crc.checksum() & 0xFF;
        mc_encode_bytes(b, 8);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Jansite TY02S - inverted Manchester, preamble 0x55 0x56 (same)
    // Packet: II II II IS PP TT CC (7 bytes, 56 bits)
    // No real CRC - receiver uses plausibility check only
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Jansite) {
        add_std_preamble();
        uint8_t b[7] = {};
        uint32_t id28 = transponder_id_ & 0x0FFFFFFF;
        b[0] = (id28 >> 20) & 0xFF;
        b[1] = (id28 >> 12) & 0xFF;
        b[2] = (id28 >> 4) & 0xFF;
        b[3] = static_cast<uint8_t>((id28 << 4) & 0xF0) | (flags_ & 0x0F);
        // kPa = p * 17/10 - p = kPa * 10/17
        b[4] = static_cast<uint8_t>(pressure_kpa_ * 10 / 17);
        b[5] = static_cast<uint8_t>(temperature_c_ + 50);
        // Checksum: not validated by receiver, set to 0
        b[6] = 0x00;
        mci_encode_bytes(b, 7);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Solar Truck - inverted Manchester, preamble 0x55 0x56
    // Packet: U(4) II II II II WW F PPP TT CC (9 bytes after skip 4 bits)
    // XOR checksum over 9 bytes
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::SolarTruck) {
        add_std_preamble();
        // State nibble (4 bits, unknown) - send as 0x0
        mci_encode_bytes((const uint8_t[]){0x00}, 1);  // only upper 4 bits used effectively
        // Actually we need to send exactly 4 inv-Manchester symbols for state nibble
        // Then 9 full bytes
        // Simpler: send state nibble as 4 bits then 9 bytes
        // We rebuild as: 4 state bits + 9 - 8 data bits = 76 bits total
        uint8_t b[9] = {};
        b[0]=(transponder_id_>>24)&0xFF; b[1]=(transponder_id_>>16)&0xFF;
        b[2]=(transponder_id_>>8)&0xFF;  b[3]=transponder_id_&0xFF;
        b[4] = flags_ & 0x0F;  // wheel position (0-255)
        uint8_t fl = (flags_ >> 4) & 0x0F;
        int pkpa = (pressure_kpa_ > 4095) ? 4095 : pressure_kpa_;
        b[5] = static_cast<uint8_t>((fl << 4) | ((pkpa >> 8) & 0x0F));
        b[6] = pkpa & 0xFF;
        b[7] = static_cast<uint8_t>(temperature_c_);  // signed
        // XOR checksum over b[0..7], place in b[8] so total XOR = 0
        uint8_t xr = 0;
        for (int i = 0; i < 8; i++) xr ^= b[i];
        b[8] = xr;
        // State nibble = 4 bits of 0
        for (int bit = 3; bit >= 0; bit--)
            binary_string += mci(0);
        mci_encode_bytes(b, 9);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // BMW Gen4/5 - inverted Manchester ~40kbps
    // Preamble: 0xAA59 (16 raw bits)
    // Packet: MM II II II II PP TT [F1 F2 F3] CC (11 bytes BMW / 8 bytes Audi)
    // CRC-8 poly=0x2F init=0xAA
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::BMW_G45) {
        symbol_rate = 40000;
        binary_string = "1010101001011001";  // 0xAA59 preamble (16 raw bits)
        uint8_t b[11] = {};
        b[0] = flags_;  // brand_id: 0x03=HUF/Beru, 0x23=Schrader, 0x80=Continental
        b[1]=(transponder_id_>>24)&0xFF; b[2]=(transponder_id_>>16)&0xFF;
        b[3]=(transponder_id_>>8)&0xFF;  b[4]=transponder_id_&0xFF;
        // PP - 2.45 kPa - PP = kPa / 2.45 = kPa * 100/245 = kPa * 20/49
        b[5] = static_cast<uint8_t>(pressure_kpa_ * 20 / 49);
        b[6] = static_cast<uint8_t>(temperature_c_ + 52);
        b[7] = 0x01; b[8] = 0x00; b[9] = 0x00;  // F1, F2, F3 (unknown/nominal)
        // CRC-8 poly=0x2F init=0xAA over bytes 0-9, place in b[10]
        // CRC over all 11 bytes (including b[10]) must residue to 0
        // So: CRC(b[0..9]) XOR generator_final - b[10]
        CRC<8> crc{0x2f, 0xaa};
        for (int i = 0; i < 10; i++) crc.process_byte(b[i]);
        b[10] = crc.checksum() & 0xFF;
        mci_encode_bytes(b, 11);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // BMW Gen2/3 - NRZI 19200 bps
    // Preamble: 0xCCCD (16 raw bits), last bit = 1
    // Packet Gen3: II II II II PP TT F1 F2 F3 CK CK (11 bytes)
    // CRC-16 poly=0x1021 init=0x0000
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::BMW_G23) {
        binary_string = "1100110011001101";  // 0xCCCD (16 raw bits)
        uint_fast8_t prev = 1;  // last preamble bit = 1
        uint8_t b[11] = {};
        b[0]=(transponder_id_>>24)&0xFF; b[1]=(transponder_id_>>16)&0xFF;
        b[2]=(transponder_id_>>8)&0xFF;  b[3]=transponder_id_&0xFF;
        // kPa = (PP-43) - 2.5 - PP = kPa/2.5 + 43 = kPa*2/5 + 43
        b[4] = static_cast<uint8_t>(pressure_kpa_ * 2 / 5 + 43);
        b[5] = static_cast<uint8_t>(temperature_c_ + 40);
        b[6] = flags_; b[7] = 0x51; b[8] = 0x03;  // typical F2/F3 values
        // CRC-16 poly=0x1021 init=0x0000 over 9 bytes - place in b[9..10]
        // Such that CRC over 11 bytes = 0 (residue)
        CRC<16> crc{0x1021, 0x0000};
        for (int i = 0; i < 9; i++) crc.process_byte(b[i]);
        uint16_t cs = crc.checksum();
        b[9]  = (cs >> 8) & 0xFF;
        b[10] = cs & 0xFF;
        nrzi_encode_bytes(b, 11, prev);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Porsche 987 Boxster/Cayman - NRZI 19200 bps
    // Preamble: 0x333320 top 20 bits, last bit = 0
    // Packet: II II II II PP TT SS SS CK CK (10 bytes)
    // CRC-16 poly=0x1021 init=0xFFFF
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Porsche) {
        binary_string = "00110011001100110010";  // 0x33332 (20 raw bits)
        uint_fast8_t prev = 0;  // last preamble bit = 0
        uint8_t b[10] = {};
        b[0]=(transponder_id_>>24)&0xFF; b[1]=(transponder_id_>>16)&0xFF;
        b[2]=(transponder_id_>>8)&0xFF;  b[3]=transponder_id_&0xFF;
        // kPa = PP-2.5 - 100 - PP = (kPa+100)/2.5 = (kPa+100)*2/5
        b[4] = static_cast<uint8_t>((pressure_kpa_ + 100) * 2 / 5);
        b[5] = static_cast<uint8_t>(temperature_c_ + 40);
        b[6] = flags_; b[7] = 0x02;  // typical status
        CRC<16> crc{0x1021, 0xffff};
        for (int i = 0; i < 8; i++) crc.process_byte(b[i]);
        uint16_t cs = crc.checksum();
        b[8]  = (cs >> 8) & 0xFF;
        b[9]  = cs & 0xFF;
        nrzi_encode_bytes(b, 10, prev);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Toyota PMV-C210 - NRZI 19200 bps
    // Preamble: 0xa9e0 (12 raw bits), last bit = 0
    // Packet: II II II II x PP TT x iP CC (9 bytes, bitfields)
    // CRC-8 poly=0x07 init=0x80
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::Toyota) {
        binary_string = "101010011110";  // 0xA9E (12 raw bits)
        uint_fast8_t prev = 0;  // last preamble bit = 0
        uint8_t b[9] = {};
        b[0]=(transponder_id_>>24)&0xFF; b[1]=(transponder_id_>>16)&0xFF;
        b[2]=(transponder_id_>>8)&0xFF;  b[3]=transponder_id_&0xFF;
        // Pressure encoding:
        // kPa = (pressure8 - 0.25 - 7.0) - 6.895
        // pressure8 = (kPa / 6.895 + 7.0) / 0.25 = kPa * 400/689 + 28
        // Integer: pressure8 = kPa * 400 / 689 + 28
        int p8 = pressure_kpa_ * 400 / 689 + 28;
        if (p8 < 0) p8 = 0;
        if (p8 > 255) p8 = 255;
        // temperature: temp8 = temp + 40
        int t8 = temperature_c_ + 40;
        if (t8 < 0) t8 = 0;
        if (t8 > 255) t8 = 255;
        // Bitfield layout: b[4]=status_bit(7)|pressure[8:1], b[5]=pressure[0]|temp[8:1], b[6]=temp[0]|...
        b[4] = static_cast<uint8_t>((flags_ & 0x01) << 7) |
               static_cast<uint8_t>((p8 >> 1) & 0x7F);
        b[5] = static_cast<uint8_t>((p8 & 0x01) << 7) |
               static_cast<uint8_t>((t8 >> 1) & 0x7F);
        b[6] = static_cast<uint8_t>((t8 & 0x01) << 7);  // filler in low 7 bits
        b[7] = static_cast<uint8_t>(p8 ^ 0xFF);  // inverted pressure cross-check
        CRC<8> crc{0x07, 0x80};
        for (int i = 0; i < 8; i++) crc.process_byte(b[i]);
        b[8] = crc.checksum() & 0xFF;
        nrzi_encode_bytes(b, 9, prev);
        goto transmit;
    }

    // -----------------------------------------------------------------------
    // Jansite Solar - inverted Manchester 19200 bps
    // Preamble: 0xa6a65a (24 raw bits)
    // Packet: SS SS II II II 00 TT PP 00 CK CK (11 bytes)
    // CRC-16/BUYPASS poly=0x8005 init=0x0000 over b[2..8]
    // -----------------------------------------------------------------------
    if (packet_type_ == Reading::Type::JansiteSolar) {
        binary_string = "101001101010011001011010";  // 0xa6a65a (24 raw bits)
        uint8_t b[11] = {};
        b[0] = 0xDD; b[1] = 0x33;  // sync word
        b[2] = (transponder_id_ >> 16) & 0xFF;
        b[3] = (transponder_id_ >> 8) & 0xFF;
        b[4] = transponder_id_ & 0xFF;
        b[5] = flags_;  // unknown byte
        b[6] = static_cast<uint8_t>(temperature_c_ + 55);  // -C = b[6]-55
        // kPa = b[7] - 1.6 = b[7] - 8/5 - b[7] = kPa - 5/8
        b[7] = static_cast<uint8_t>(pressure_kpa_ * 5 / 8);
        b[8] = 0x00;  // unknown
        CRC<16> crc{0x8005, 0x0000};
        for (int i = 2; i < 9; i++) crc.process_byte(b[i]);
        uint16_t cs = crc.checksum();
        b[9]  = (cs >> 8) & 0xFF;
        b[10] = cs & 0xFF;
        mci_encode_bytes(b, 11);
        goto transmit;
    }

    text_status.set("Unknown protocol");
    stop_tx();
    return;

transmit:
    {
        size_t bitstream_length = encoders::make_bitstream(binary_string);
        uint32_t samples_per_bit = (sample_rate + symbol_rate / 2) / symbol_rate;
        text_status.set("TX: " + to_string_dec_uint(binary_string.length()) + " bits");

        if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
            signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
            baseband::set_ook_data(bitstream_length, samples_per_bit,
                                   repeat_count_, pause_duration_);
        } else {
            baseband::set_fsk_data(bitstream_length, samples_per_bit, 38400, 256);
        }
    }
}

void TPMSTXView::handle_tx_complete() {
    if (signal_type_ != tpms::SignalType::OOK_8k192_Schrader &&
        signal_type_ != tpms::SignalType::OOK_8k4_Schrader) {
        fsk_repeat_counter_++;
        if (fsk_repeat_counter_ < repeat_count_) {
            progressbar.set_value(fsk_repeat_counter_);
            chThdSleepMilliseconds(50);
            encode_and_transmit();
        } else {
            stop_tx();
        }
    } else {
        stop_tx();
    }
}

void TPMSTXView::start_tx() {
    if (is_transmitting_) return;
    is_transmitting_ = true;
    fsk_repeat_counter_ = 0;
    progressbar.set_max(repeat_count_);
    progressbar.set_value(0);
    button_transmit.set_text("STOP TX");
    text_status.set("Transmitting...");
    switch_baseband();
    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader ||
        signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        transmitter_model.set_sampling_rate(2000000);
    } else {
        transmitter_model.set_sampling_rate(2280000);
    }
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.enable();
    encode_and_transmit();
}

void TPMSTXView::stop_tx() {
    if (!is_transmitting_) return;
    is_transmitting_ = false;
    transmitter_model.disable();
    button_transmit.set_text("START TX");
    text_status.set("Transmission complete");
    progressbar.set_value(0);
}

TPMSTXView::TPMSTXView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&labels, &label_temperature, &label_flags,
                  &options_frequency, &options_packet_type,
                  &field_transponder_id_24, &field_transponder_id_32,
                  &field_pressure, &options_pressure,
                  &text_bar_tx,
                  &field_temperature, &options_temperature,
                  &field_flags, &field_repeat,
                  &button_load, &button_save,
                  &tx_view, &button_transmit, &text_status, &progressbar});

    label_temperature.set_style(Theme::getInstance()->fg_light);
    label_flags.set_style(Theme::getInstance()->fg_light);

    options_frequency.set_by_value(transmitter_model.target_frequency());
    options_packet_type.set_selected_index(0);
    field_repeat.set_value(repeat_count_);
    update_signal_type_from_packet();
    options_pressure.set_by_value(format::pressure_unit);
    options_temperature.set_by_value(format::temp_unit);
    field_transponder_id_24.set_value(transponder_id_ & 0x00FFFFFF);
    field_transponder_id_32.set_value(transponder_id_);
    on_pressure_unit_change();
    on_temperature_unit_change();
    field_flags.set_value(flags_);
    update_field_visibility();
    update_packet_display();

    options_frequency.on_change = [this](size_t, OptionsField::value_t v) {
        transmitter_model.set_target_frequency(v);
    };

    options_packet_type.on_change = [this](size_t, int32_t value) {
        packet_type_ = static_cast<tpms::Reading::Type>(value);
        update_signal_type_from_packet();
        if (packet_type_ == tpms::Reading::Type::Schrader) {
            transponder_id_ = field_transponder_id_32.to_integer() & 0x00FFFFFF;
            field_transponder_id_24.set_value(transponder_id_);
        } else {
            transponder_id_ = field_transponder_id_24.to_integer();
            field_transponder_id_32.set_value(transponder_id_);
        }
        update_field_visibility();
        update_packet_display();
    };

    options_pressure.on_change = [this](size_t, int32_t i) {
        format::pressure_unit = (uint8_t)i;
        on_pressure_unit_change();
    };

    options_temperature.on_change = [this](size_t, int32_t i) {
        format::temp_unit = (uint8_t)i;
        on_temperature_unit_change();
    };

    field_transponder_id_24.on_change = [this](SymField&) {
        transponder_id_ = field_transponder_id_24.to_integer() & 0x00FFFFFF;
        update_packet_display();
    };

    field_transponder_id_32.on_change = [this](SymField&) {
        transponder_id_ = field_transponder_id_32.to_integer();
        update_packet_display();
    };

    field_pressure.on_change = [this](int32_t value) {
        if (format::pressure_unit == PRESSURE_UNIT_PSI)
            pressure_kpa_ = value * 6895 / 1000;
        else if (format::pressure_unit == PRESSURE_UNIT_BAR)
            pressure_kpa_ = value * 100;
        else
            pressure_kpa_ = value;
        update_packet_display();
    };

    field_temperature.on_change = [this](int32_t value) {
        temperature_c_ = (format::temp_unit == TEMP_UNIT_FAHRENHEIT)
                         ? (value - 32) * 5 / 9
                         : value;
        update_packet_display();
    };

    field_flags.on_change = [this](int32_t value) {
        flags_ = value & 0x07;
        update_packet_display();
    };

    field_repeat.on_change = [this](int32_t value) {
        repeat_count_ = value;
    };

    button_transmit.on_select = [this](Button&) {
        if (is_transmitting_) stop_tx();
        else start_tx();
    };

    button_load.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path file_path) {
            File f;
            auto error = f.open(file_path);
            if (!error.is_valid()) {
                char buffer[512];
                auto result = f.read(buffer, sizeof(buffer) - 1);
                if (result.is_ok()) {
                    buffer[result.value()] = 0;
                    std::string content(buffer);
                    size_t pos = 0;
                    auto parse = [&](const std::string& key) -> std::string {
                        size_t kp = content.find(key + "=", pos);
                        if (kp == std::string::npos) return "";
                        size_t vs = kp + key.length() + 1;
                        size_t ve = content.find('\n', vs);
                        if (ve == std::string::npos) ve = content.length();
                        return content.substr(vs, ve - vs);
                    };
                    auto ts = parse("Type");
                    auto is = parse("ID");
                    auto ps = parse("Pressure");
                    auto ts2 = parse("Temperature");
                    auto fs = parse("Flags");
                    auto ss = parse("SignalType");
                    if (!ts.empty()) {
                        int type = std::stoi(ts);
                        packet_type_ = static_cast<tpms::Reading::Type>(type);
                        options_packet_type.set_by_value(type);
                    }
                    if (!is.empty()) {
                        transponder_id_ = std::stoul(is, nullptr, 16);
                        field_transponder_id_24.set_value(transponder_id_ & 0x00FFFFFF);
                        field_transponder_id_32.set_value(transponder_id_);
                    }
                    if (!ps.empty()) { pressure_kpa_ = std::stoi(ps); on_pressure_unit_change(); }
                    if (!ts2.empty()) { temperature_c_ = std::stoi(ts2); field_temperature.set_value(temperature_c_); }
                    if (!fs.empty()) { flags_ = std::stoul(fs, nullptr, 16) & 0xFF; field_flags.set_value(flags_ & 7); }
                    if (!ss.empty()) {
                        int st = std::stoi(ss);
                        if (st >= 1 && st <= 9) signal_type_ = static_cast<tpms::SignalType>(st);
                        else update_signal_type_from_packet();
                    } else { update_signal_type_from_packet(); }
                    update_packet_display();
                    update_field_visibility();
                    text_status.set("Loaded: " + file_path.filename().string());
                }
            }
        };
    };

    button_save.on_select = [this](Button&) {
        auto timestamp = to_string_timestamp(rtc_time::now());
        std::string file_name = "TPMS_" + timestamp + ".TXT";
        ensure_directory(tpms_dir);
        auto file_path = tpms_dir / file_name;
        File f;
        auto error = f.create(file_path);
        if (!error.is_valid()) {
            std::string c = "Type=" + to_string_dec_uint(toUType(packet_type_), 1) + "\n";
            c += "ID=" + to_string_hex(transponder_id_, 8) + "\n";
            c += "Pressure=" + to_string_dec_uint(pressure_kpa_) + "\n";
            c += "Temperature=" + to_string_dec_int(temperature_c_) + "\n";
            c += "Flags=" + to_string_hex(flags_ & 0x07, 1) + "\n";
            c += "SignalType=" + to_string_dec_uint(toUType(signal_type_), 1) + "\n";
            f.write(c.c_str(), c.length());
            text_status.set("Saved: " + file_name);
        } else {
            text_status.set("Error saving file");
        }
    };
}

TPMSTXView::~TPMSTXView() {
    stop_tx();
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::tpmstx
