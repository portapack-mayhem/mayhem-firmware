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

/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
 *
 * This file is part of PortaPack.
 */

#ifndef __BEACON_RX_H__
#define __BEACON_RX_H__

#include "country.hpp"
#include "location.hpp"
#include "rtc_time.hpp"

namespace ui::external_app::epirb_rx {

#define BEACON_DATA_SIZE 18  // Max 144 bits => 18 bytes

#define BCH_21_POLYNOMIAL 0b1001101101100111100011UL
#define BCH_21_POLY_LENGTH 22
#define BCH_12_POLYNOMIAL 0b1010100111001UL
#define BCH_12_POLY_LENGTH 13

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

    enum class ProtocolType {
        UNKNOWN,
        USER,
        STANDARD_LOCATION,
        NATIONAL_LOCATION,
        RLS_LOCATION,
        ELT_DT_LOCATION,
        SPARE
    };

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

    static bool protocolIsUser(Protocol protocol) { return (getProtocolType(protocol) == ProtocolType::USER); };
    static bool protocolIsNational(Protocol protocol) { return (getProtocolType(protocol) == ProtocolType::NATIONAL_LOCATION); };
    static bool protocolIsStandard(Protocol protocol) { return (getProtocolType(protocol) == ProtocolType::STANDARD_LOCATION); };
    static bool protocolIsRls(Protocol protocol) { return (getProtocolType(protocol) == ProtocolType::RLS_LOCATION); };
    static bool protocolIsRlsOrElt(Protocol protocol) { return (getProtocolType(protocol) == ProtocolType::RLS_LOCATION || getProtocolType(protocol) == ProtocolType::ELT_DT_LOCATION); };
    static bool protocolIsUnknown(Protocol protocol) { return (getProtocolType(protocol) == ProtocolType::UNKNOWN); };

    static inline const char* getProtocolTypeName(Protocol protocol, bool longFrame) {
        switch (getProtocolType(protocol)) {
            case ProtocolType::STANDARD_LOCATION:
                return "Standard Protocol";
            case ProtocolType::NATIONAL_LOCATION:
                return "National Protocol";
            case ProtocolType::RLS_LOCATION:
                return "RLS";
            case ProtocolType::ELT_DT_LOCATION:
                return "ELT(DT)";
            case ProtocolType::USER:
                return longFrame ? "User Location Protocol" : "User Protocol";
            case ProtocolType::SPARE:
                return "Spare Protocol";
            default:
                return UNKNOWN_LABEL;
        }
    }

    static inline ProtocolType getProtocolType(Protocol protocol) {
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
    Country country{};
    uint8_t frame[BEACON_DATA_SIZE];
    Location location{};
    uint64_t identifier{0};
    rtc::RTC date{};
    bool hasAdditionalData{false};
    std::string additionalData{};
    bool hasSerialNumber{false};
    std::string serialNumber{};
    std::string hexId{};
    uint32_t bch1{};
    uint32_t computedBch1{};
    bool hasBch2{false};
    uint32_t bch2{};
    uint32_t computedBch2{};
    bool isEmpty{true};

    static inline uint64_t getBits(uint8_t* data, int startBit, int endBit) {
        uint64_t result = 0;
        startBit--;
        int numBits = endBit - startBit;
        const uint8_t* pData = &(data[startBit / 8]);
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

    static inline uint64_t computeBCH(uint8_t* frame, int startBit, int endBit, unsigned long poly, int polyLength) {
        int dataLength = endBit - startBit + 1;
        int totalLength = dataLength + polyLength - 1;
        uint64_t result = getBits(frame, startBit, startBit + polyLength - 1);
        for (int i = polyLength; i <= totalLength; i++) {
            bool firstBit = result >> (polyLength - 1);
            if (firstBit) result = result ^ poly;
            if (i < totalLength) {
                result = result << 1;
                if (i < dataLength) result |= getBits(frame, startBit + i, startBit + i);
            }
        }
        return result;
    }

    static inline uint64_t computeBCH1(uint8_t* frame) { return computeBCH(frame, 25, 85, BCH_21_POLYNOMIAL, BCH_21_POLY_LENGTH); }
    static inline uint64_t computeBCH2(uint8_t* frame) { return computeBCH(frame, 107, 132, BCH_12_POLYNOMIAL, BCH_12_POLY_LENGTH); }

    static inline std::string toHexString(uint32_t data) {
        char buffer[11];
        std::sprintf(buffer, "0x%08lX", data);
        return std::string(buffer);
    }

    static inline std::string toHexString(uint8_t* frame, bool withSpace, int start, int end) {
        char buffer[4];
        std::string result = "";
        for (uint8_t i = start; i < end; i++) {
            std::sprintf(buffer, "%02X", frame[i]);
            if (withSpace && i > start) result += " ";
            result += buffer;
        }
        return result;
    }

    Beacon() {}
    Beacon(const Beacon& other) = delete;
    Beacon& operator=(const Beacon& other) = delete;

    inline void setFrame(const uint8_t* frameBuffer) {
        frameMode = FrameMode::UNKNOWN;
        mainLocatingDevice = MainLocatingDevice::UNDEFINED;
        auxLocatingDevice = AuxLocatingDevice::UNDEFINED;
        protocolCode = 0;
        protocol = Protocol::UNKNOWN;
        country.code = 0;
        country.alphaCode = "";
        // country.longName = "";
        country.shortName = "";
        location.clear();
        identifier = 0;
        hasAdditionalData = false;
        additionalData = "";
        hasSerialNumber = false;
        serialNumber = "";
        hexId = "";
        bch1 = 0;
        computedBch1 = 0;
        hasBch2 = false;
        bch2 = 0;
        computedBch2 = 0;
        isEmpty = true;
        std::memcpy(frame, (const void*)frameBuffer, BEACON_DATA_SIZE);
        parseFrame();
    }

    inline const char* getFrameTitle() {
        if (frameMode == FrameMode::SELF_TEST) return "Self-test 406";
        if (frameMode == FrameMode::NORMAL) {
            if (protocol == Protocol::USER_SERIAL) return "Serial 406";
            if (protocol == Protocol::USER_TEST) return "User Test 406";
            if (protocol == Protocol::USER_ORB) return "Orbitography 406";
            if (protocol == Protocol::USER_NAT) return "National 406";
            if (protocol == Protocol::STD_TEST) return "Test Std. 406";
            if (protocol == Protocol::NAT_TEST) return "Test Nat. 406";
            if (protocol == Protocol::SPARE) return "Spare 406";
            return "Distress 406";
        }
        return "Unknown 406";
    }

    inline const char* getProtocolName() { return getProtocolTypeName(protocol, longFrame); }

    inline const char* getProtocolDesciption() {
        if (protocol == Protocol::USER_EPIRB_MARITIME) return "EPIRB - Maritime";
        if (protocol == Protocol::USER_EPIRB_RADIO) return "EPIRB - Radio Call Sign";
        if (protocol == Protocol::USER_ELT) return "ELT - Aviation";
        if (protocol == Protocol::USER_SERIAL) return "Serial User Protocol";
        if (protocol == Protocol::USER_TEST) return "Test User Protocol";
        if (protocol == Protocol::USER_ORB) return "Orbitography Protocol";
        if (protocol == Protocol::USER_NAT) return "National User Protocol";
        if (protocol == Protocol::USER_2G) return "2nd Generation Beacons";
        if (protocol == Protocol::STD_EPIRB) return "EPIRB - MMSI / Location";
        if (protocol == Protocol::STD_ELT_24) return "ELT-24-bit Address / Location";
        if (protocol == Protocol::STD_ELT_SERIAL) return "ELT Serial Location";
        if (protocol == Protocol::STD_ELT_AIRCRAFT) return "ELT Serial Aircradt Location";
        if (protocol == Protocol::STD_EPIRB_SERIAL) return "EPIRB Serial Location";
        if (protocol == Protocol::STD_PLB_SERIAL) return "PLB Serial Location";
        if (protocol == Protocol::STD_SHIP) return "Ship Security Location";
        if (protocol == Protocol::STD_TEST) return "Test Standard Location";
        if (protocol == Protocol::NAT_ELT) return "ELT National Location";
        if (protocol == Protocol::NAT_EPIRB) return "EPIRB National Location";
        if (protocol == Protocol::NAT_PLB) return "PLB National Location";
        if (protocol == Protocol::NAT_TEST) return "Test National Location";
        if (protocol == Protocol::RLS) return "RLS Location Protocol";
        if (protocol == Protocol::ELT_DT) return "ELT(DT) Location Protocol";
        if (protocol == Protocol::SPARE) return "Spare";
        return UNKNOWN_LABEL;
    }

    inline const char* getType() {
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

    inline bool hasMainLocatingDevice() { return (mainLocatingDevice != MainLocatingDevice::UNDEFINED); }
    inline const char* getMainLocatingDeviceName() {
        if (mainLocatingDevice == MainLocatingDevice::EXTERNAL_NAV) return "Exernal";
        if (mainLocatingDevice == MainLocatingDevice::INTERNAL_NAV) return "Internal";
        return UNKNOWN_LABEL;
    }

    inline bool hasAuxLocatingDevice() { return (auxLocatingDevice != AuxLocatingDevice::UNDEFINED); }
    inline const char* getAuxLocatingDeviceName() {
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

    inline void setSerialNumber(uint32_t serial) {
        serialNumber = to_string_dec_uint(serial) + " (0x" + toHexString(serial) + ")";
    }

    inline bool isBch1Valid() { return (bch1 == computedBch1); }
    inline bool isBch2Valid() { return (bch2 == computedBch2); }
    inline bool isFrameValid() { return isBch1Valid() && ((!hasBch2) || isBch2Valid()) && (!isEmpty); }
    inline bool isOrbito() { return (protocol == Protocol::USER_ORB); }

    inline std::string hexString(bool withHeader) {
        return toHexString(frame, false, (withHeader ? 0 : 3), (longFrame ? 18 : 14));
    }

    inline std::string shortId() {
        return (hexId.size() >= 4) ? hexId.substr(0, 4) : hexId;
    }

    inline std::string getSatus() {
        if (isFrameValid())
            return "OK";
        else
            return "KO";
    }

    inline ui::Color getColor() {
        if (isFrameValid())
            return ui::Color::green();
        else
            return ui::Color::red();
    }

    inline std::string formatTime() {
        std::string time;
        time = to_string_dec_uint(date.hour(), 2, '0') + ":" +
                  to_string_dec_uint(date.minute(), 2, '0') + ":" +
                  to_string_dec_uint(date.second(), 2, '0');
        return time;
    }

    inline std::string formatSummary(bool with_time) {
        std::string summary;
        if (with_time) {
            summary = formatTime()+ "-";
        }
        std::string id = shortId();
        while (id.size() < 4) id = "_" + id;
        summary += id;
        summary += "-";
        std::string type = getType();
        while (type.size() < 5) type = "_" + type;
        summary += type;
        if (!location.isUnknown()) {
            summary += "-" + location.toString(Location::LocationFormat::MAIDENHEAD_LOCATOR);
        } else {
            summary += "-      ";
        }
        std::string status_color;
        if (isFrameValid())
            status_color = STR_COLOR_GREEN;
        else
            status_color = STR_COLOR_RED;

        summary += "[" + status_color + getSatus() + STR_COLOR_WHITE + "]";
        return summary;
    }

   private:
    /* Baudot code matrix */
    static constexpr char BAUDOT_CODE[64] = {' ', '5', ' ', '9', ' ', ' ', ' ', ' ', ' ', ' ', '4', ' ', '8', '0', ' ', ' ',
                                             '3', ' ', ' ', ' ', ' ', '6', ' ', '/', '-', '2', ' ', ' ', '7', '1', ' ', ' ',
                                             ' ', 'T', ' ', 'O', ' ', 'H', 'N', 'M', ' ', 'L', 'R', 'G', 'I', 'P', 'C', 'V',
                                             'E', 'Z', 'D', 'B', 'S', 'Y', 'F', 'X', 'A', 'W', 'J', ' ', 'U', 'Q', 'K', '\0'};

    inline void parseProtocol() {
        protocolFlag = getBits(frame, 26, 26);
        if (protocolFlag)
            protocolCode = getBits(frame, 37, 39);
        else
            protocolCode = getBits(frame, 37, 40);

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

    inline void parseAdditionalData() {
        hasAdditionalData = false;
        hasSerialNumber = false;
        if (protocolFlag) {
            if (protocolCode == 0b011) {
                uint8_t serialUserProtocol = getBits(frame, 40, 42);
                hasAdditionalData = true;
                switch (serialUserProtocol) {
                    case 0b000:
                        additionalData = "ELTs/SN";
                        break;
                    case 0b001:
                        additionalData = "ELTs/AC op & SN";
                        break;
                    case 0b010:
                        additionalData = "FF EPIRBs/SN";
                        break;
                    case 0b011:
                        additionalData = "ELTs+AC 24-bit ad.";
                        break;
                    case 0b100:
                        additionalData = "NFF EPIRBs/SN";
                        break;
                    case 0b110:
                        additionalData = "PLBs/SN";
                        break;
                    default:
                        hasAdditionalData = false;
                }
                hasSerialNumber = true;
                setSerialNumber(getBits(frame, 44, 67));
            }
            // TODO Get emergency info out of bits 107-112 (see spec page 54: "Non protected data fields")
            /*switch (EmergencyType::Fire) {
                case EmergencyType::Fire:
                    return "Fire";
                case EmergencyType::Flooding:
                    return "Flooding";
                case EmergencyType::Collision:
                    return "Collision";
                case EmergencyType::Grounding:
                    return "Grounding";
                case EmergencyType::Sinking:
                    return "Sinking";
                case EmergencyType::Disabled:
                    return "Disabled";
                case EmergencyType::Abandoning:
                    return "Abandoning";
                case EmergencyType::Piracy:
                    return "Piracy";
                case EmergencyType::Man_Overboard:
                    return "MOB";
                default:
                    return "Other";
            }*/
        } else if (longFrame) {
            switch (protocolCode) {
                case 0b0010:
                    hasSerialNumber = true;
                    setSerialNumber(getBits(frame, 61, 64));
                    break;
                case 0b1100: {
                    hasAdditionalData = true;
                    uint32_t mmsi = getBits(frame, 41, 60);
                    additionalData = "MMSI=" + toHexString(mmsi) + " MID=" + to_string_dec_uint(mmsi);
                } break;
                case 0b0011: {
                    hasAdditionalData = true;
                    hasSerialNumber = true;
                    setSerialNumber(getBits(frame, 41, 64));
                    additionalData = "24 bits AC ad.";
                } break;
                case 0b0100:
                case 0b0110:
                case 0b0111: {
                    hasAdditionalData = true;
                    hasSerialNumber = true;
                    uint32_t csTaNumber = getBits(frame, 41, 50);
                    additionalData = "C/S TA #=" + to_string_dec_uint(csTaNumber);
                    setSerialNumber(getBits(frame, 51, 64));
                } break;
                case 0b0101: {
                    hasAdditionalData = true;
                    hasSerialNumber = true;
                    uint32_t data = getBits(frame, 41, 45);
                    additionalData = BAUDOT_CODE[data + 32];
                    data = getBits(frame, 46, 50);
                    additionalData += BAUDOT_CODE[data + 32];
                    data = getBits(frame, 51, 55);
                    additionalData += BAUDOT_CODE[data + 32];
                    additionalData = "Op Design.=" + additionalData;
                    setSerialNumber(getBits(frame, 56, 64));
                } break;
                case 0b1000:
                case 0b1010:
                case 0b1011:
                case 0b1111: {
                    hasSerialNumber = true;
                    hasAdditionalData = true;
                    setSerialNumber(getBits(frame, 41, 58));
                    uint32_t natNum = getBits(frame, 127, 132);
                    additionalData = "Nati. data=" + to_string_dec_uint(natNum);
                } break;
            }
        }
    }

    inline void parseLocatingDevices() {
        bool mainLoc;
        if (protocolIsStandard(protocol) || protocolIsNational(protocol)) {
            mainLoc = getBits(frame, 111, 111);
            mainLocatingDevice = mainLoc ? MainLocatingDevice::INTERNAL_NAV : MainLocatingDevice::EXTERNAL_NAV;
        } else if (protocolIsUser(protocol) || protocolIsRls(protocol)) {
            mainLoc = getBits(frame, 107, 107);
            mainLocatingDevice = mainLoc ? MainLocatingDevice::INTERNAL_NAV : MainLocatingDevice::EXTERNAL_NAV;
        }
        if (protocolIsStandard(protocol) || protocolIsNational(protocol)) {
            auxLocatingDevice = getBits(frame, 112, 112) ? AuxLocatingDevice::MHZ121_5 : AuxLocatingDevice::NONE_OR_OTHER;
        } else if (protocolIsRls(protocol)) {
            auxLocatingDevice = getBits(frame, 108, 108) ? AuxLocatingDevice::MHZ121_5 : AuxLocatingDevice::NONE_OR_OTHER;
        } else if (protocolIsUser(protocol) && protocolCode != 0b100) {
            uint8_t aux = getBits(frame, 84, 85);
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

    inline void parseFrame() {
        long latofmin, latofsec, lonofmin, lonofsec;
        bool latoffset, lonoffset;

        longFrame = getBits(frame, 25, 25);
        parseProtocol();
        country = Country::getCountry(getBits(frame, 27, 36));

        if (frame[2] == 0xD0)
            frameMode = FrameMode::SELF_TEST;
        else if (frame[2] == 0x2F)
            frameMode = FrameMode::NORMAL;
        else
            frameMode = FrameMode::UNKNOWN;

        identifier = getBits(frame, 26, 85);
        char buffer[32];
        std::sprintf(buffer, "%07lX%08lX", (uint32_t)(identifier >> 32), (uint32_t)identifier);
        hexId = std::string(buffer);

        if (longFrame) {
            if (protocolIsUser(protocol) && !isOrbito()) {
                location.latitude.orientation = (frame[13] & 0x10) >> 4;
                location.latitude.degrees = ((frame[13] & 0x0F) << 3 | (frame[14] & 0xE0) >> 5);
                location.latitude.minutes = ((frame[14] & 0x1E) >> 1) * 4;
                location.longitude.orientation = (frame[14] & 0x01);
                location.longitude.degrees = (frame[15]);
                location.longitude.minutes = ((frame[16] & 0xF0) >> 4) * 4;
            } else if (protocolIsNational(protocol)) {
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
            } else if (protocolIsStandard(protocol)) {
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
            } else if (protocolIsRlsOrElt(protocol)) {
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

        bch1 = getBits(frame, 86, 106);
        computedBch1 = computeBCH1(frame);
        hasBch2 = !isOrbito();
        if (hasBch2) {
            bch2 = getBits(frame, 133, 144);
            computedBch2 = computeBCH2(frame);
        }
    }
};
}  // namespace ui::external_app::epirb_rx

#endif  // __BEACON_RX_H__