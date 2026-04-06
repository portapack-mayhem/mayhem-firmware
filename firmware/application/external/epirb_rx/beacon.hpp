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

#ifndef __BEACON_RX_H__
#define __BEACON_RX_H__

#include "country.hpp"
#include "location.hpp"
#include "rtc_time.hpp"

#include <cstdint>
#include <string>

namespace ui::external_app::epirb_rx {

#define BEACON_DATA_SIZE 18  // Max 144 bits => 18 bytes

// Enable BCH debuging
// #define DEBUG_BCH

class Beacon {
   public:
    enum class FrameMode { NORMAL,
                           SELF_TEST,
                           UNKNOWN };
    enum class MainLocatingDevice { UNDEFINED,
                                    INTERNAL_NAV,
                                    EXTERNAL_NAV };
    enum class AuxLocatingDevice { UNDEFINED,
                                   NONE,
                                   NONE_OR_OTHER,
                                   OTHER,
                                   MHZ121_5,
                                   SART };
    class Protocol {
       public:
        enum class Type { STANDARD_LOCATION,
                          NATIONAL_LOCATION,
                          RLS_LOCATION,
                          ELT_DT_LOCATION,
                          USER,
                          SPARE,
                          UNKNOWN };
        Type type;
        bool isUser() const { return (type == Type::USER); };
        bool isNational() const { return (type == Type::NATIONAL_LOCATION); };
        bool isStandard() const { return (type == Type::STANDARD_LOCATION); };
        bool isRls() const { return (type == Type::RLS_LOCATION); };
        bool isRlsOrElt() const { return (type == Type::RLS_LOCATION || type == Type::ELT_DT_LOCATION); };
        bool isUnknown() const { return (type == Type::UNKNOWN); };
        std::string getTypeName(bool longFrame) const;
        Protocol(Type type) : type(type) {}
        static const Protocol
            // User
            USER_EPIRB_MARITIME,
            USER_EPIRB_RADIO, USER_ELT, USER_SERIAL, USER_TEST, USER_ORB, USER_NAT, USER_2G,
            // Standard
            STD_EPIRB, STD_ELT_24, STD_ELT_SERIAL, STD_ELT_AIRCRAFT, STD_EPIRB_SERIAL, STD_PLB_SERIAL, STD_SHIP, STD_TEST,
            // National
            NAT_ELT, NAT_EPIRB, NAT_PLB, NAT_TEST,
            // RLS / ELT / Spare
            RLS, ELT_DT, SPARE, UNKNOWN;
    };
    bool longFrame{true};
    bool protocolFlag{false};
    FrameMode frameMode{FrameMode::UNKNOWN};
    MainLocatingDevice mainLocatingDevice{MainLocatingDevice::UNDEFINED};
    AuxLocatingDevice auxLocatingDevice{AuxLocatingDevice::UNDEFINED};
    long protocolCode{0};
    const Protocol* protocol{&Protocol::UNKNOWN};
    Country country{};
    uint8_t frame[BEACON_DATA_SIZE];
    Location location{};
    uint64_t identifier{0};
    Beacon();
    Beacon(const Beacon& other);
    Beacon& operator=(const Beacon& other) { setFrame(other.frame); return *this; };
    void setFrame(const uint8_t* frameBuffer);
    // Beacon(volatile uint8_t frameBuffer[]) : Beacon(frameBuffer/*,Rtc::getRtc()->getDate()*/) {}
    rtc::RTC date{};
    std::string getFrameTitle();
    std::string getProtocolName();
    std::string getProtocolDesciption();
    bool hasMainLocatingDevice();
    std::string getMainLocatingDeviceName();
    bool hasAuxLocatingDevice();
    std::string getAuxLocatingDeviceName();
    bool hasAdditionalData{false};
    std::string additionalData{};
    bool hasSerialNumber{false};
    std::string serialNumber{};
    std::string hexId{};
    uint32_t bch1{};
    uint32_t computedBch1{};
    bool isBch1Valid();
    bool hasBch2{false};
    uint32_t bch2{};
    uint32_t computedBch2{};
    bool isEmpty{true};
    bool isBch2Valid();
    bool isFrameValid();
    bool isOrbito();
    std::string toKvpString();
    std::string hexString(bool withHeader);

   private:
    void parseFrame();
    void parseProtocol();
    void parseAdditionalData();
    void parseLocatingDevices();
    void setSerialNumber(uint32_t serial);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __BEACON_RX_H__