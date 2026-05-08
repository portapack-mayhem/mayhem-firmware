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
#include "resources.hpp"

namespace ui::external_app::epirb_rx {

// Size for beacon frame buffer
#define BEACON_DATA_SIZE 18  // Max 144 bits => 18 bytes

// Constants for BCH control code calculation
#define BCH_21_POLYNOMIAL 0b1001101101100111100011UL
#define BCH_21_POLY_LENGTH 22
#define BCH_12_POLYNOMIAL 0b1010100111001UL
#define BCH_12_POLY_LENGTH 13

// See content of sdcard/EPIRB/RES/BEACON.RES
#define ADDITIONAL_DATA_RESOURCE_START 25
#define LONG_ADDITIONAL_DATA_RESOURCE_START 33
#define EMERGENCY_RESOURCE_START 39
#define EMERGENCY_OTHER_RESOURCE_START 49

/**
 * This is the main class for beaon handling
 */
class Beacon {
   public:
    // Real beacon or test beacon ?
    enum class FrameMode { NORMAL,
                           SELF_TEST,
                           UNKNOWN };
    // Type of main location device
    enum class MainLocatingDevice { UNDEFINED,
                                    INTERNAL_NAV,
                                    EXTERNAL_NAV };
    // Type of aux location device
    enum class AuxLocatingDevice { UNDEFINED,
                                   NONE,
                                   NONE_OR_OTHER,
                                   OTHER,
                                   MHZ121_5,
                                   SART };

    // Protcol type
    enum class ProtocolType {
        UNKNOWN,
        USER,
        STANDARD_LOCATION,
        NATIONAL_LOCATION,
        RLS_LOCATION,
        ELT_DT_LOCATION,
        SPARE
    };

    // Protocol
    enum class Protocol {
        USER_EPIRB_MARITIME,
        USER_EPIRB_RADIO,
        USER_ELT,
        USER_SERIAL,
        USER_TEST,
        USER_ORB,
        USER_NAT,
        USER_2G,
        STD_EPIRB,
        STD_ELT_24,
        STD_ELT_SERIAL,
        STD_ELT_AIRCRAFT,
        STD_EPIRB_SERIAL,
        STD_PLB_SERIAL,
        STD_SHIP,
        STD_TEST,
        NAT_ELT,
        NAT_EPIRB,
        NAT_PLB,
        NAT_TEST,
        RLS,
        ELT_DT,
        SPARE,
        UNKNOWN
    };

#define UNKNOWN_LABEL "Unk."

    // Returns true for User protocol beacons
    bool protocolIsUser() { return (protocolType == ProtocolType::USER); };
    // Returns true for National Location protocol beacons
    bool protocolIsNational() { return (protocolType == ProtocolType::NATIONAL_LOCATION); };
    // Returns true for Standard Location protocol beacons
    bool protocolIsStandard() { return (protocolType == ProtocolType::STANDARD_LOCATION); };
    // Returns true for RLS location protocol beacons
    bool protocolIsRls() { return (protocolType == ProtocolType::RLS_LOCATION); };
    // Returns true for RLS or ELT Location protocol beacons
    bool protocolIsRlsOrElt() { return (protocolType == ProtocolType::RLS_LOCATION || protocolType == ProtocolType::ELT_DT_LOCATION); };
    // Returns true for unknown protocol beacons
    bool protocolIsUnknown() { return (protocolType == ProtocolType::UNKNOWN); };

    // Returns the protocol name
    const char* getProtocolName() {
        switch (protocolType) {
            case ProtocolType::STANDARD_LOCATION:
                return "Standard";
            case ProtocolType::NATIONAL_LOCATION:
                return "National";
            case ProtocolType::RLS_LOCATION:
                return "RLS";
            case ProtocolType::ELT_DT_LOCATION:
                return "ELT(DT)";
            case ProtocolType::USER:
                return longFrame ? "User Location" : "User";
            case ProtocolType::SPARE:
                return "Spare";
            default:
                return UNKNOWN_LABEL;
        }
    }

    // Get protocol type for the given protocol
    ProtocolType getProtocolType() {
        switch (protocol) {
            case Protocol::USER_EPIRB_MARITIME:
            case Protocol::USER_EPIRB_RADIO:
            case Protocol::USER_ELT:
            case Protocol::USER_SERIAL:
            case Protocol::USER_TEST:
            case Protocol::USER_ORB:
            case Protocol::USER_NAT:
            case Protocol::USER_2G:
                return ProtocolType::USER;
            case Protocol::STD_EPIRB:
            case Protocol::STD_ELT_24:
            case Protocol::STD_ELT_SERIAL:
            case Protocol::STD_ELT_AIRCRAFT:
            case Protocol::STD_PLB_SERIAL:
            case Protocol::STD_SHIP:
            case Protocol::STD_TEST:
                return ProtocolType::STANDARD_LOCATION;
            case Protocol::NAT_ELT:
            case Protocol::NAT_EPIRB:
            case Protocol::NAT_PLB:
            case Protocol::NAT_TEST:
                return ProtocolType::NATIONAL_LOCATION;
            case Protocol::RLS:
                return ProtocolType::RLS_LOCATION;
            case Protocol::ELT_DT:
                return ProtocolType::ELT_DT_LOCATION;
            case Protocol::SPARE:
                return ProtocolType::SPARE;
            case Protocol::UNKNOWN:
            default:
                return ProtocolType::UNKNOWN;
        }
    }

    bool longFrame{true};
    bool protocolFlag{false};
    FrameMode frameMode{FrameMode::UNKNOWN};
    MainLocatingDevice mainLocatingDevice{MainLocatingDevice::UNDEFINED};
    AuxLocatingDevice auxLocatingDevice{AuxLocatingDevice::UNDEFINED};
    long protocolCode{0};
    Protocol protocol{Protocol::UNKNOWN};
    ProtocolType protocolType{ProtocolType::UNKNOWN};
    Country country{};
    uint8_t frame[BEACON_DATA_SIZE];
    Location location{};
    uint64_t identifier{0};
    rtc::RTC date{};
    bool hasAdditionalData{false};
    char additionalData[48]{0};
    bool hasSerialNumber{false};
    char serialNumber[32]{0};
    char hexId[32]{0};
    uint32_t bch1{};
    uint32_t computedBch1{};
    bool bch1Corrected{false};
    bool hasBch2{false};
    uint32_t bch2{};
    uint32_t computedBch2{};
    bool bch2Corrected{false};
    bool isEmpty{true};
    bool hasEmergency{false};
    bool isAutoamticEmergency{false};
    bool isMaritime{false};
    char emergencyType[32]{0};

    uint64_t getBits(int startBit, int endBit) {
        uint64_t result = 0;
        startBit--;
        int numBits = endBit - startBit;
        const uint8_t* pData = &(frame[startBit / 8]);
        uint8_t b = *pData;
        int bitOffset = 7 - (startBit % 8);
        for (int i = 0; i < numBits; ++i) {
            result <<= 1;
            result |= ((b >> bitOffset) & 0x01);
            if (--bitOffset < 0) {
                b = *(++pData);
                bitOffset = 7;
            }
        }
        return result;
    }

    uint64_t computeBCH(int startBit, int endBit, unsigned long poly, int polyLength) {
        int dataLength = endBit - startBit + 1;
        int totalLength = dataLength + polyLength - 1;
        uint64_t result = getBits(startBit, startBit + polyLength - 1);
        for (int i = polyLength; i <= totalLength; i++) {
            bool firstBit = result >> (polyLength - 1);
            if (firstBit) result = result ^ poly;
            if (i < totalLength) {
                result = result << 1;
                if (i < dataLength) result |= getBits(startBit + i, startBit + i);
            }
        }
        return result;
    }

    uint64_t computeBCH1() { return computeBCH(25, 85, BCH_21_POLYNOMIAL, BCH_21_POLY_LENGTH); }
    uint64_t computeBCH2() { return computeBCH(107, 132, BCH_12_POLYNOMIAL, BCH_12_POLY_LENGTH); }

    static size_t toHexString(char* buffer, uint32_t data) {
        return sprintf(buffer, "0x%08lX", data);
    }

    static size_t toHexString(char* buffer, uint8_t* frame, bool withSpace, int start, int end) {
        size_t result = 0;
        for (uint8_t i = start; i < end; i++) {
            if (withSpace && i > start) buffer[result++] = ' ';
            result += sprintf((buffer + result), "%02X", frame[i]);
        }
        buffer[result] = 0;
        return result;
    }

    Beacon() {}
    Beacon(const Beacon& other) = delete;
    Beacon& operator=(const Beacon& other) = delete;

    void setFrame(const uint8_t* frameBuffer) {
        frameMode = FrameMode::UNKNOWN;
        mainLocatingDevice = MainLocatingDevice::UNDEFINED;
        auxLocatingDevice = AuxLocatingDevice::UNDEFINED;
        protocolCode = 0;
        protocol = Protocol::UNKNOWN;
        protocolType = ProtocolType::UNKNOWN;
        country.code = 0;
        country.alphaCode[0] = 0;
        country.shortName[0] = 0;
        location.clear();
        identifier = 0;
        hasAdditionalData = false;
        additionalData[0] = 0;
        hasSerialNumber = false;
        serialNumber[0] = 0;
        hexId[0] = 0;
        bch1 = 0;
        computedBch1 = 0;
        hasBch2 = false;
        bch1Corrected = false;
        bch2 = 0;
        computedBch2 = 0;
        bch2Corrected = false;
        isEmpty = true;
        hasEmergency = false;
        isAutoamticEmergency = false;
        isMaritime = false;
        emergencyType[0] = 0;
        std::memcpy(frame, (const void*)frameBuffer, BEACON_DATA_SIZE);
        parseFrame();
    }

    void flipBit(int bit) {
        int byteIdx = (bit - 1) / 8;
        int bitIdx = 7 - ((bit - 1) % 8);
        frame[byteIdx] ^= (1 << bitIdx);
    }

    void simpleCorrection(bool isBCH1) {
        for (int i = (isBCH1 ? 25 : 107); i <= (isBCH1 ? 85 : 132); i++) {
            flipBit(i);
            if ((isBCH1 ? (computeBCH1() == bch1) : (computeBCH2() == bch2))) {
                if (isBCH1) {
                    computedBch1 = bch1;
                    bch1Corrected = true;
                } else {
                    computedBch2 = bch2;
                    bch2Corrected = true;
                }
                break;
            }
            flipBit(i);  // Restore bit value
        }
    }

    const char* getType() {
        if ((protocol == Protocol::USER_EPIRB_MARITIME) || (protocol == Protocol::USER_EPIRB_RADIO) || (protocol == Protocol::STD_EPIRB) || (protocol == Protocol::STD_EPIRB_SERIAL) || (protocol == Protocol::NAT_EPIRB)) return "EPIRB";
        if ((protocol == Protocol::USER_ELT) || (protocol == Protocol::STD_ELT_24) || (protocol == Protocol::STD_ELT_SERIAL) || (protocol == Protocol::STD_ELT_AIRCRAFT) || (protocol == Protocol::NAT_ELT) || (protocol == Protocol::ELT_DT)) return "ELT";
        if ((protocol == Protocol::STD_PLB_SERIAL) || (protocol == Protocol::NAT_PLB)) return "PLB";
        if ((protocol == Protocol::USER_TEST) || (protocol == Protocol::STD_TEST) || (protocol == Protocol::NAT_TEST)) return "TEST";
        if (protocol == Protocol::USER_SERIAL) return "SRIAL";
        if (protocol == Protocol::USER_ORB) return "ORB";
        if (protocol == Protocol::USER_NAT) return "NAT";
        if (protocol == Protocol::USER_2G) return "2G";
        if (protocol == Protocol::STD_SHIP) return "SHIP";
        if (protocol == Protocol::RLS) return "RLS";
        if (protocol == Protocol::SPARE) return "SPARE";
        return UNKNOWN_LABEL;
    }

    bool hasMainLocatingDevice() { return (mainLocatingDevice != MainLocatingDevice::UNDEFINED); }
    const char* getMainLocatingDeviceName() {
        if (mainLocatingDevice == MainLocatingDevice::EXTERNAL_NAV) return "Exernal";
        if (mainLocatingDevice == MainLocatingDevice::INTERNAL_NAV) return "Internal";
        return UNKNOWN_LABEL;
    }

    bool hasAuxLocatingDevice() { return (auxLocatingDevice != AuxLocatingDevice::UNDEFINED); }
    const char* getAuxLocatingDeviceName() {
        switch (auxLocatingDevice) {
            case AuxLocatingDevice::NONE:
                return "No device";
            case AuxLocatingDevice::NONE_OR_OTHER:
                return "Other/no device";
            case AuxLocatingDevice::OTHER:
                return "Other device";
            case AuxLocatingDevice::MHZ121_5:
                return "121.5 MHz";
            case AuxLocatingDevice::SART:
                return "SART";
            default:
                return UNKNOWN_LABEL;
        }
    }

    void setSerialNumber(uint32_t serial) {
        sprintf(serialNumber, "%ld (0x%08lX)", serial, serial);
    }

    bool isBch1Valid() { return (bch1 == computedBch1); }
    bool isBch2Valid() { return (bch2 == computedBch2); }
    bool isFrameValid();
    bool isOrbito() { return (protocol == Protocol::USER_ORB); }

    size_t hexString(char* buffer, bool withHeader) {
        return toHexString(buffer, frame, false, (withHeader ? 0 : 3), (longFrame ? 18 : 14));
    }

    std::string shortId();

    std::string getSatus() {
        if (isFrameValid())
            return "OK";
        else
            return "KO";
    }

    size_t formatTime(char* buffer);

    size_t formatSummary(char* buffer, bool with_time);

   private:
    /* Baudot code matrix */
    static constexpr char BAUDOT_CODE[64] = {' ', '5', ' ', '9', ' ', ' ', ' ', ' ', ' ', ' ', '4', ' ', '8', '0', ' ', ' ',
                                             '3', ' ', ' ', ' ', ' ', '6', ' ', '/', '-', '2', ' ', ' ', '7', '1', ' ', ' ',
                                             ' ', 'T', ' ', 'O', ' ', 'H', 'N', 'M', ' ', 'L', 'R', 'G', 'I', 'P', 'C', 'V',
                                             'E', 'Z', 'D', 'B', 'S', 'Y', 'F', 'X', 'A', 'W', 'J', ' ', 'U', 'Q', 'K', '\0'};

    void parseProtocol() {
        protocolFlag = getBits(26, 26);
        if (protocolFlag)
            protocolCode = getBits(37, 39);
        else
            protocolCode = getBits(37, 40);

        if (!longFrame || protocolFlag == 1) {
            switch (protocolCode) {
                case 0b000:
                    protocol = Protocol::USER_ORB;
                    break;
                case 0b001:
                    protocol = Protocol::USER_ELT;
                    break;
                case 0b010:
                    protocol = Protocol::USER_EPIRB_MARITIME;
                    isMaritime = true;
                    break;
                case 0b011:
                    protocol = Protocol::USER_SERIAL;
                    break;
                case 0b100:
                    protocol = Protocol::USER_NAT;
                    break;
                case 0b101:
                    protocol = Protocol::USER_2G;
                    break;
                case 0b110:
                    protocol = Protocol::USER_EPIRB_RADIO;
                    isMaritime = true;
                    break;
                case 0b111:
                    protocol = Protocol::USER_TEST;
                    break;
                default:
                    protocol = Protocol::UNKNOWN;
            }
        } else {
            switch (protocolCode) {
                case 0b0010:
                    protocol = Protocol::STD_EPIRB;
                    break;
                case 0b0011:
                    protocol = Protocol::STD_ELT_24;
                    break;
                case 0b0100:
                    protocol = Protocol::STD_ELT_SERIAL;
                    break;
                case 0b0101:
                    protocol = Protocol::STD_ELT_AIRCRAFT;
                    break;
                case 0b0110:
                    protocol = Protocol::STD_EPIRB_SERIAL;
                    break;
                case 0b0111:
                    protocol = Protocol::STD_PLB_SERIAL;
                    break;
                case 0b1000:
                    protocol = Protocol::NAT_ELT;
                    break;
                case 0b1001:
                    protocol = Protocol::ELT_DT;
                    break;
                case 0b1010:
                    protocol = Protocol::NAT_EPIRB;
                    break;
                case 0b1011:
                    protocol = Protocol::NAT_PLB;
                    break;
                case 0b1100:
                    protocol = Protocol::STD_SHIP;
                    break;
                case 0b1101:
                    protocol = Protocol::RLS;
                    break;
                case 0b1110:
                    protocol = Protocol::STD_TEST;
                    break;
                case 0b1111:
                    protocol = Protocol::NAT_TEST;
                    break;
                default:
                    protocol = Protocol::UNKNOWN;
            }
        }
    }

    void parseAdditionalData() {
        hasAdditionalData = false;
        hasSerialNumber = false;
        if (protocolFlag) {
            if (protocolCode == 0b011) {
                uint8_t serialUserProtocol = getBits(40, 42);
                hasAdditionalData = true;
                switch (serialUserProtocol) {
                    case 0b100:
                    case 0b010:
                        isMaritime = true;
                        // fallthrough
                    case 0b000:
                    case 0b001:
                    case 0b011:
                    case 0b110:
                        strcpy(additionalData, ResourceManager::get_beacon_resource(ADDITIONAL_DATA_RESOURCE_START + serialUserProtocol));
                        break;
                    default:
                        hasAdditionalData = false;
                }
                hasSerialNumber = true;
                setSerialNumber((serialUserProtocol == 0b011) ? getBits(44, 67) : (serialUserProtocol == 0b001) ? getBits(62, 73)
                                                                                                                : getBits(44, 63));
            }
            if (!longFrame) {
                hasEmergency = getBits(107, 107);
                if (hasEmergency) {
                    isAutoamticEmergency = getBits(108, 108);
                    if (isMaritime) {
                        uint8_t emmergency = getBits(109, 112);
                        switch (emmergency) {
                            case 0b0001:
                            case 0b0010:
                            case 0b0011:
                            case 0b0100:
                            case 0b0101:
                            case 0b0110:
                            case 0b0111:
                            case 0b1000:
                                strcpy(emergencyType, ResourceManager::get_beacon_resource(EMERGENCY_RESOURCE_START + emmergency));
                                break;
                            default:
                                strcpy(emergencyType, ResourceManager::get_beacon_resource(EMERGENCY_RESOURCE_START));
                        }
                    } else {
                        bool fire = getBits(109, 109);
                        bool med = getBits(110, 110);
                        bool disabled = getBits(111, 111);
                        char* emmergency_pointer = emergencyType;
                        if (fire) emmergency_pointer += sprintf(emmergency_pointer, "%s", ResourceManager::get_beacon_resource(EMERGENCY_OTHER_RESOURCE_START));
                        if (med) {
                            if (fire) emmergency_pointer += sprintf(emmergency_pointer, "%c", '+');
                            emmergency_pointer += sprintf(emmergency_pointer, "%s", ResourceManager::get_beacon_resource(EMERGENCY_OTHER_RESOURCE_START + 1));
                        }
                        if (disabled) {
                            if (fire || med) emmergency_pointer += sprintf(emmergency_pointer, "%c", '+');
                            sprintf(emmergency_pointer, "%s", ResourceManager::get_beacon_resource(EMERGENCY_OTHER_RESOURCE_START + 2));
                        }
                    }
                }
            }
        } else if (longFrame) {
            switch (protocolCode) {
                case 0b0010:
                    hasSerialNumber = true;
                    setSerialNumber(getBits(61, 64));
                    break;
                case 0b1100: {
                    hasAdditionalData = true;
                    uint32_t mmsi = getBits(41, 60);
                    sprintf(additionalData, ResourceManager::get_beacon_resource(LONG_ADDITIONAL_DATA_RESOURCE_START), mmsi, mmsi);
                } break;
                case 0b0011: {
                    hasAdditionalData = true;
                    hasSerialNumber = true;
                    setSerialNumber(getBits(41, 64));
                    strcpy(additionalData, ResourceManager::get_beacon_resource(LONG_ADDITIONAL_DATA_RESOURCE_START + 1));
                } break;
                case 0b0100:
                case 0b0110:
                case 0b0111: {
                    hasAdditionalData = true;
                    hasSerialNumber = true;
                    uint32_t csTaNumber = getBits(41, 50);
                    sprintf(additionalData, ResourceManager::get_beacon_resource(LONG_ADDITIONAL_DATA_RESOURCE_START + 2), csTaNumber);
                    setSerialNumber(getBits(51, 64));
                } break;
                case 0b0101: {
                    hasAdditionalData = true;
                    hasSerialNumber = true;
                    uint32_t data = getBits(41, 45);
                    char char1 = BAUDOT_CODE[data + 32];
                    data = getBits(46, 50);
                    char char2 = BAUDOT_CODE[data + 32];
                    data = getBits(51, 55);
                    char char3 = BAUDOT_CODE[data + 32];
                    sprintf(additionalData, ResourceManager::get_beacon_resource(LONG_ADDITIONAL_DATA_RESOURCE_START + 3), char1, char2, char3);
                    setSerialNumber(getBits(56, 64));
                } break;
                case 0b1000:
                case 0b1010:
                case 0b1011:
                case 0b1111: {
                    hasSerialNumber = true;
                    hasAdditionalData = true;
                    setSerialNumber(getBits(41, 58));
                    uint32_t natNum = getBits(127, 132);
                    sprintf(additionalData, ResourceManager::get_beacon_resource(LONG_ADDITIONAL_DATA_RESOURCE_START + 4), natNum);
                } break;
            }
        }
    }

    void parseLocatingDevices() {
        bool mainLoc;
        if (protocolIsStandard() || protocolIsNational()) {
            mainLoc = getBits(111, 111);
            mainLocatingDevice = mainLoc ? MainLocatingDevice::INTERNAL_NAV : MainLocatingDevice::EXTERNAL_NAV;
        } else if (protocolIsUser() || protocolIsRls()) {
            mainLoc = getBits(107, 107);
            mainLocatingDevice = mainLoc ? MainLocatingDevice::INTERNAL_NAV : MainLocatingDevice::EXTERNAL_NAV;
        }
        if (protocolIsStandard() || protocolIsNational()) {
            auxLocatingDevice = getBits(112, 112) ? AuxLocatingDevice::MHZ121_5 : AuxLocatingDevice::NONE_OR_OTHER;
        } else if (protocolIsRls()) {
            auxLocatingDevice = getBits(108, 108) ? AuxLocatingDevice::MHZ121_5 : AuxLocatingDevice::NONE_OR_OTHER;
        } else if (protocolIsUser() && protocolCode != 0b100) {
            uint8_t aux = getBits(84, 85);
            if (aux == 0b00)
                auxLocatingDevice = AuxLocatingDevice::NONE;
            else if (aux == 0b01)
                auxLocatingDevice = AuxLocatingDevice::MHZ121_5;
            else if (aux == 0b10)
                auxLocatingDevice = AuxLocatingDevice::SART;
            else
                auxLocatingDevice = AuxLocatingDevice::OTHER;
        }
    }

    void parseFrame() {
        long latofmin, latofsec, lonofmin, lonofsec;
        bool latoffset, lonoffset;

        longFrame = getBits(25, 25);
        parseProtocol();
        protocolType = getProtocolType();
        CountryManager::get_country(getBits(27, 36), country);

        if (frame[2] == 0xD0)
            frameMode = FrameMode::SELF_TEST;
        else if (frame[2] == 0x2F)
            frameMode = FrameMode::NORMAL;
        else
            frameMode = FrameMode::UNKNOWN;

        if (longFrame) {
            if (protocolIsUser() && !isOrbito()) {
                location.latitude.orientation = (frame[13] & 0x10) >> 4;
                location.latitude.degrees = ((frame[13] & 0x0F) << 3 | (frame[14] & 0xE0) >> 5);
                location.latitude.minutes = ((frame[14] & 0x1E) >> 1) * 4;
                location.longitude.orientation = (frame[14] & 0x01);
                location.longitude.degrees = (frame[15]);
                location.longitude.minutes = ((frame[16] & 0xF0) >> 4) * 4;
            } else if (protocolIsNational()) {
                latoffset = (frame[14] & 0x80) >> 7;
                location.latitude.orientation = (frame[7] & 0x20) >> 5;
                location.latitude.degrees = ((frame[7] & 0x1F) << 2 | (frame[8] & 0xC0) >> 6);
                location.latitude.minutes = ((frame[8] & 0x3E) >> 1) * 2;
                latofmin = (frame[14] & 0x60) >> 5;
                latofsec = ((frame[14] & 0x1E) >> 1) * 4;
                if (latoffset) {
                    location.latitude.minutes += latofmin;
                    location.latitude.seconds += latofsec;
                } else {
                    location.latitude.minutes -= latofmin;
                    if (location.latitude.minutes < 0) {
                        location.latitude.minutes += 60;
                        location.latitude.degrees -= 1;
                    }
                    location.latitude.seconds -= latofsec;
                    if (location.latitude.seconds < 0) {
                        location.latitude.seconds += 60;
                        location.latitude.minutes -= 1;
                    }
                }
                lonoffset = (frame[14] & 0x01);
                location.longitude.orientation = (frame[8] & 0x01);
                location.longitude.degrees = (frame[9]);
                location.longitude.minutes = ((frame[10] & 0xF8) >> 3) * 2;
                lonofmin = (frame[15] & 0xC0) >> 6;
                lonofsec = ((frame[15] & 0x3C) >> 2) * 4;
                if (lonoffset) {
                    location.longitude.minutes += lonofmin;
                    location.longitude.seconds += lonofsec;
                } else {
                    location.longitude.minutes -= lonofmin;
                    if (location.longitude.minutes < 0) {
                        location.longitude.minutes += 60;
                        location.longitude.degrees -= 1;
                    }
                    location.longitude.seconds -= lonofsec;
                    if (location.longitude.seconds < 0) {
                        location.longitude.seconds += 60;
                        location.longitude.minutes -= 1;
                    }
                }
            } else if (protocolIsStandard()) {
                latoffset = (frame[14] & 0x80) >> 7;
                location.latitude.orientation = (frame[8] & 0x80) >> 7;
                location.latitude.degrees = (frame[8] & 0x7F);
                location.latitude.minutes = ((frame[9] & 0xC0) >> 6) * 15;
                latofmin = (frame[14] & 0x7C) >> 2;
                latofsec = ((frame[14] & 0x03) << 2 | (frame[15] & 0xC0) >> 6) * 4;
                if (latoffset) {
                    location.latitude.minutes += latofmin;
                    location.latitude.seconds += latofsec;
                } else {
                    location.latitude.minutes -= latofmin;
                    if (location.latitude.minutes < 0) {
                        location.latitude.minutes += 60;
                        location.latitude.degrees -= 1;
                    }
                    location.latitude.seconds -= latofsec;
                    if (location.latitude.seconds < 0) {
                        location.latitude.seconds += 60;
                        location.latitude.minutes -= 1;
                    }
                }
                lonoffset = (frame[15] & 0x20) >> 5;
                location.longitude.orientation = (frame[9] & 0x20) >> 5;
                location.longitude.degrees = ((frame[9] & 0x1F) << 3 | (frame[10] & 0xE0) >> 5);
                location.longitude.minutes = ((frame[10] & 0x18) >> 3) * 15;
                lonofmin = (frame[15] & 0x1F);
                lonofsec = ((frame[16] & 0xF0) >> 4) * 4;
                if (lonoffset) {
                    location.longitude.minutes += lonofmin;
                    location.longitude.seconds += lonofsec;
                } else {
                    location.longitude.minutes -= lonofmin;
                    if (location.longitude.minutes < 0) {
                        location.longitude.minutes += 60;
                        location.longitude.degrees -= 1;
                    }
                    location.longitude.seconds -= lonofsec;
                    if (location.longitude.seconds < 0) {
                        location.longitude.seconds += 60;
                        location.longitude.minutes -= 1;
                    }
                }
            } else if (protocolIsRlsOrElt()) {
                latoffset = (frame[14] & 0x20) >> 5;
                location.latitude.orientation = (frame[8] & 0x20) >> 5;
                location.latitude.degrees = ((frame[8] & 0x1F) << 2) | ((frame[9] & 0xC0) >> 6);
                location.latitude.minutes = ((frame[9] & 0x20) >> 5) * 30;
                latofmin = (frame[14] & 0x1E) >> 1;
                latofsec = ((frame[14] & 0x01) << 3 | (frame[15] & 0xE0) >> 5) * 4;
                if (latoffset) {
                    location.latitude.minutes += latofmin;
                    location.latitude.seconds += latofsec;
                } else {
                    location.latitude.minutes -= latofmin;
                    if (location.latitude.minutes < 0) {
                        location.latitude.minutes += 60;
                        location.latitude.degrees -= 1;
                    }
                    location.latitude.seconds -= latofsec;
                    if (location.latitude.seconds < 0) {
                        location.latitude.seconds += 60;
                        location.latitude.minutes -= 1;
                    }
                }
                lonoffset = (frame[15] & 0x10) >> 4;
                location.longitude.orientation = (frame[9] & 0x10) >> 4;
                location.longitude.degrees = ((frame[9] & 0x0F) << 4 | (frame[10] & 0xF0) >> 4);
                location.longitude.minutes = ((frame[10] & 0x08) >> 3) * 30;
                lonofmin = (frame[15] & 0x0F);
                lonofsec = ((frame[16] & 0xF0) >> 4) * 4;
                if (lonoffset) {
                    location.longitude.minutes += lonofmin;
                    location.longitude.seconds += lonofsec;
                } else {
                    location.longitude.minutes -= lonofmin;
                    if (location.longitude.minutes < 0) {
                        location.longitude.minutes += 60;
                        location.longitude.degrees -= 1;
                    }
                    location.longitude.seconds -= lonofsec;
                    if (location.longitude.seconds < 0) {
                        location.longitude.seconds += 60;
                        location.longitude.minutes -= 1;
                    }
                }
            }
        }

        parseAdditionalData();
        parseLocatingDevices();

        identifier = getBits(26, 85);

        uint32_t hexIdHigh = (uint32_t)(identifier >> 32);
        uint32_t hexIdLow = (uint32_t)identifier;

        if (protocolIsStandard()) {
            // default value of bits 65 to 74 = 0 111111111 / default value of bits 75 to 85 = 0 1111111111
            hexIdLow &= 0b11111111'11100000'00000000'00000000;
            hexIdLow |= 0b00000000'00001111'11111011'11111111;
        } else if (protocolIsNational()) {
            // default value of bits 59 to 71 = 0 1111111 00000 / default value of bits 72 to 85 = 0 11111111 00000
            hexIdLow &= 0b11111000'00000000'00000000'00000000;
            hexIdLow |= 0b00000011'11111000'00011111'11100000;
        } else if (protocolIsRlsOrElt()) {
            // default value of bits 67 to 75 = 0 11111111 / default value of bits 76 to 85 = 0 111111111
            hexIdLow &= 0b11111111'11111000'00000000'00000000;
            hexIdLow |= 0b00000000'00000011'11111101'11111111;
        }
        std::sprintf(hexId, "%07lX%08lX", hexIdHigh, hexIdLow);

        if (isOrbito() && !longFrame) {
            isEmpty = true;
            for (size_t i = 3; i < BEACON_DATA_SIZE; i++) {
                if (frame[i] != 0) {
                    isEmpty = false;
                    break;
                }
            }
        } else
            isEmpty = false;

        bch1 = getBits(86, 106);
        computedBch1 = computeBCH1();
        // Try and correct single bit error on  BCH1 (bits 25-85)
        if (computedBch1 != bch1) {
            simpleCorrection(true);
        }

        hasBch2 = longFrame && !isOrbito();
        if (hasBch2) {
            bch2 = getBits(133, 144);
            computedBch2 = computeBCH2();
            // Try and correct single bit error on  BCH2 (bits 107 - 132)
            if (computedBch2 != bch2) {
                simpleCorrection(false);
            }
        }
        static bool isCorrecting = false;
        // If BCH1 or BCH2 has been corrected, we need to parse frame again to update beacon properties
        if (!isCorrecting && (bch1Corrected || bch2Corrected)) {
            // Prevent recursion
            isCorrecting = true;
            parseFrame();
            isCorrecting = false;
        }
    }
};
}  // namespace ui::external_app::epirb_rx

#endif  // __BEACON_RX_H__