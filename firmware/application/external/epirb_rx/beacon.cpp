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

#include "beacon.hpp"

#include <cstdint>
#include <string>
#include <cstring>
#include <cctype>

namespace ui::external_app::epirb_rx {

using std::string;

/* Baudot code matrix */
static char BAUDOT_CODE[64] = {' ', '5', ' ', '9', ' ', ' ', ' ', ' ', ' ', ' ', '4', ' ', '8', '0', ' ', ' ',
                               '3', ' ', ' ', ' ', ' ', '6', ' ', '/', '-', '2', ' ', ' ', '7', '1', ' ', ' ',
                               ' ', 'T', ' ', 'O', ' ', 'H', 'N', 'M', ' ', 'L', 'R', 'G', 'I', 'P', 'C', 'V',
                               'E', 'Z', 'D', 'B', 'S', 'Y', 'F', 'X', 'A', 'W', 'J', ' ', 'U', 'Q', 'K', '\0'};

// 21 bits BCH polynomial
#define BCH_21_POLYNOMIAL 0b1001101101100111100011UL
#define BCH_21_POLY_LENGTH 22
// 12 bits BCH polynomial
#define BCH_12_POLYNOMIAL 0b1010100111001UL
#define BCH_12_POLY_LENGTH 13

// User
const Beacon::Protocol Beacon::Protocol::USER_EPIRB_MARITIME(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_EPIRB_RADIO(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_ELT(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_SERIAL(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_TEST(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_ORB(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_NAT(Beacon::Protocol::Type::USER);
const Beacon::Protocol Beacon::Protocol::USER_2G(Beacon::Protocol::Type::USER);
// Standard
const Beacon::Protocol Beacon::Protocol::STD_EPIRB(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_ELT_24(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_ELT_SERIAL(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_ELT_AIRCRAFT(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_EPIRB_SERIAL(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_PLB_SERIAL(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_SHIP(Beacon::Protocol::Type::STANDARD_LOCATION);
const Beacon::Protocol Beacon::Protocol::STD_TEST(Beacon::Protocol::Type::STANDARD_LOCATION);
// National
const Beacon::Protocol Beacon::Protocol::NAT_ELT(Beacon::Protocol::Type::NATIONAL_LOCATION);
const Beacon::Protocol Beacon::Protocol::NAT_EPIRB(Beacon::Protocol::Type::NATIONAL_LOCATION);
const Beacon::Protocol Beacon::Protocol::NAT_PLB(Beacon::Protocol::Type::NATIONAL_LOCATION);
const Beacon::Protocol Beacon::Protocol::NAT_TEST(Beacon::Protocol::Type::NATIONAL_LOCATION);
// RLS / ELT / Spare
const Beacon::Protocol Beacon::Protocol::RLS(Beacon::Protocol::Type::RLS_LOCATION);
const Beacon::Protocol Beacon::Protocol::ELT_DT(Beacon::Protocol::Type::ELT_DT_LOCATION);
const Beacon::Protocol Beacon::Protocol::SPARE(Beacon::Protocol::Type::SPARE);
const Beacon::Protocol Beacon::Protocol::UNKNOWN(Beacon::Protocol::Type::UNKNOWN);

string Beacon::Protocol::getTypeName(bool longFrame) const {
    switch (type) {
        case Type::STANDARD_LOCATION:
            return "Standard Protocol";
            break;
        case Type::NATIONAL_LOCATION:
            return "National Protocol";
            break;
        case Type::RLS_LOCATION:
            return "RLS";
            break;
        case Type::ELT_DT_LOCATION:
            return "ELT(DT)";
            break;
        case Type::USER:
            return longFrame ? "User Location Protocol" : "User Protocol";
            break;
        case Type::SPARE:
            return "Spare Protocol";
            break;
        case Type::UNKNOWN:
        default:
            return "Unknown Protocol";
    }
}

static uint64_t getBits(uint8_t* data, int startBit, int endBit) {
    uint64_t result = 0;
    // 0 bases bit count
    startBit--;
    int numBits = endBit - startBit;

    // get a pointer to the starting byte...
    const uint8_t* pData = &(data[startBit / 8]);
    uint8_t b = *pData;

    // calculate the starting bit within that byte...
    int bitOffset = 7 - (startBit % 8);

    // iterate for the desired number of bits...
    for (int i = 0; i < numBits; ++i) {
        // make room for the next bit...
        result <<= 1;
        // copy the bit...
        result |= ((b >> bitOffset) & 0x01);
        // reached the end of the current byte?
        if (--bitOffset < 0) {
            b = *(++pData);  // go to the next byte...
            bitOffset = 7;   // restart at the first bit in that byte...
        }
    }

    // all done...
    return result;
}

static uint64_t computeBCH(uint8_t* frame, int startBit, int endBit, unsigned long poly, int polyLength) {  // Length of data to be checked (not including the BCH code)
    int dataLength = endBit - startBit + 1;
    // Total lengh (including the BCH code that will be padded to zeros (BCH code length is polyLengh-1))
    int totalLength = dataLength + polyLength - 1;
    // Start with the first polyLength bits
    uint64_t result = getBits(frame, startBit, startBit + polyLength - 1);
#ifdef DEBUG_BCH
    Serial.println("BCH calculation :";
    Serial.println((unsigned long)result, 2);
#endif
    for (int i = polyLength; i <= totalLength; i++) {  // Iterate on each bit after the first polyLength batch
        bool firstBit = result >> (polyLength - 1);
#ifdef DEBUG_BCH
        Serial.print("First bit :";
        Serial.println(firstBit, 2);
#endif
        if (firstBit) {  // We have a leading 1 => xor the result with the poly
            result = result ^ poly;
#ifdef DEBUG_BCH
            for (int j = 0; j < i - polyLength; j++) Serial.print(" ";
            Serial.println((unsigned long)result, 2);
#endif
        }
        if (i < totalLength) {  // Move to next bit
            result = result << 1;
#ifdef DEBUG_BCH
            for (int j = 0; j < i - polyLength; j++) Serial.print(" ";
            Serial.println((unsigned long)result, 2);
#endif
            if (i < dataLength) {  // Append next bit
                    result |= getBits(frame, startBit + i, startBit + i);
#ifdef DEBUG_BCH
                    for (int j = 0; j < i - polyLength; j++) Serial.print(" ";
                Serial.println((unsigned long)result, 2);
#endif
            }  // else : 0 padding after data length
        }
    }
    return result;
}

static uint64_t computeBCH1(uint8_t* frame) {
    return computeBCH(frame, 25, 85, BCH_21_POLYNOMIAL, BCH_21_POLY_LENGTH);
}

static uint64_t computeBCH2(uint8_t* frame) {
    return computeBCH(frame, 107, 132, BCH_12_POLYNOMIAL, BCH_12_POLY_LENGTH);
}

static string toHexString(uint32_t data) {
    char buffer[11];  // 8 chars + '0x' + '\0'
    std::sprintf(buffer, "0x%08lX", data);
    return string(buffer);
}

/**
 * @brief Convert the provided frame to it's Hex String representation
 *
 * @param frame the frame to convert
 * @param withSpace true if bytes should be seperated by spaces
 * @param start start byte
 * @param end end byte
 * @return String
 */
static string toHexString(uint8_t* frame, bool withSpace, int start, int end) {
    char buffer[4];
    string result = "";
    for (uint8_t i = start; i < end; i++) {
        snprintf(buffer, sizeof(buffer), "%02X", frame[i]);
        if (withSpace && i > start) {
            result += " ";
        }
        result += buffer;
    }
    return result;
}

Beacon::Beacon() {
}

Beacon::Beacon(const Beacon& copy) {
    setFrame(copy.frame);
}

void Beacon::setFrame(const uint8_t* frameBuffer) {
    // Get a local copy of the original frame
    memcpy(frame, (const void*)frameBuffer, BEACON_DATA_SIZE);
    // Beacon::date = date;
    parseFrame();
}

string Beacon::getFrameTitle() {
    if (frameMode == Beacon::FrameMode::SELF_TEST) {  // Self-test message frame synchronisation byte
        return "Self-test 406";
    } else if (frameMode == Beacon::FrameMode::NORMAL) {  // Normal message frame synchronisation byte
        if (protocol == &Protocol::USER_SERIAL) {
            return "Serial 406";
        } else if (protocol == &Protocol::USER_TEST) {
            return "User Test 406";
        } else if (protocol == &Protocol::USER_ORB) {
            return "Orbitography 406";
        } else if (protocol == &Protocol::USER_NAT) {
            return "National 406";
        } else if (protocol == &Protocol::STD_TEST) {
            return "Test Std. 406";
        } else if (protocol == &Protocol::NAT_TEST) {
            return "Test Nat. 406";
        } else if (protocol == &Protocol::SPARE) {
            return "Spare 406";
        } else {
            return "Distress 406";
        }
    } else {  // Unknown frame format
        return "Unknown 406";
    }
}

string Beacon::getProtocolName() {
    return protocol->getTypeName(longFrame);
}

string Beacon::getProtocolDesciption() {
    // User
    if (protocol == &Protocol::USER_EPIRB_MARITIME) {
        return "EPIRB - Maritime";
    } else if (protocol == &Protocol::USER_EPIRB_RADIO) {
        return "EPIRB - Radio Call Sign";
    } else if (protocol == &Protocol::USER_ELT) {
        return "ELT - Aviation";
    } else if (protocol == &Protocol::USER_SERIAL) {
        return "Serial User Protocol";
    } else if (protocol == &Protocol::USER_TEST) {
        return "Test User Protocol";
    } else if (protocol == &Protocol::USER_ORB) {
        return "Orbitography Protocol";
    } else if (protocol == &Protocol::USER_NAT) {
        return "National User Protocol";
    } else if (protocol == &Protocol::USER_2G) {
        return "2nd Generation Beacons";
    }
    // Standard
    else if (protocol == &Protocol::STD_EPIRB) {
        return "EPIRB - MMSI / Location";
    } else if (protocol == &Protocol::STD_ELT_24) {
        return "ELT-24-bit Address / Location";
    } else if (protocol == &Protocol::STD_ELT_SERIAL) {
        return "ELT Serial Location";
    } else if (protocol == &Protocol::STD_ELT_AIRCRAFT) {
        return "ELT Serial Aircradt Location";
    } else if (protocol == &Protocol::STD_EPIRB_SERIAL) {
        return "EPIRB Serial Location";
    } else if (protocol == &Protocol::STD_PLB_SERIAL) {
        return "PLB Serial Location";
    } else if (protocol == &Protocol::STD_SHIP) {
        return "Ship Security Location";
    } else if (protocol == &Protocol::STD_TEST) {
        return "Test Standard Location";
    }
    // National
    else if (protocol == &Protocol::NAT_ELT) {
        return "ELT National Location";
    } else if (protocol == &Protocol::NAT_EPIRB) {
        return "EPIRB National Location";
    } else if (protocol == &Protocol::NAT_PLB) {
        return "PLB National Location";
    } else if (protocol == &Protocol::NAT_TEST) {
        return "Test National Location";
    }
    // RLS / ELT / Spare
    else if (protocol == &Protocol::RLS) {
        return "RLS Location Protocol";
    } else if (protocol == &Protocol::ELT_DT) {
        return "ELT(DT) Location Protocol";
    } else if (protocol == &Protocol::SPARE) {
        return "Spare";
    } else {
        return "Unknown Protocol";
    }
}

bool Beacon::hasMainLocatingDevice() {
    return (mainLocatingDevice != MainLocatingDevice::UNDEFINED);
}

string Beacon::getMainLocatingDeviceName() {
    switch (mainLocatingDevice) {
        case MainLocatingDevice::EXTERNAL_NAV:
            return "Exernal";
            break;
        case MainLocatingDevice::INTERNAL_NAV:
            return "Internal";
            break;
        default:
            return "Undefined";
    }
}

bool Beacon::hasAuxLocatingDevice() {
    return (auxLocatingDevice != AuxLocatingDevice::UNDEFINED);
}

string Beacon::getAuxLocatingDeviceName() {
    switch (auxLocatingDevice) {
        case AuxLocatingDevice::NONE:
            return "No device";
            break;
        case AuxLocatingDevice::NONE_OR_OTHER:
            return "Other or no device";
            break;
        case AuxLocatingDevice::OTHER:
            return "Other device";
            break;
        case AuxLocatingDevice::MHZ121_5:
            return "121.5 MHz";
            break;
        case AuxLocatingDevice::SART:
            return "Maritime locating: 9 GHz SART";
            break;
        default:
            return "Undefined";
    }
}

void Beacon::parseProtocol() {
    protocolFlag = getBits(frame, 26, 26);      // protocol flag : bit 26
    if (protocolFlag) {                         // User protocol
        protocolCode = getBits(frame, 37, 39);  // protocol code : bits 37-39
    } else {                                    // Other protocols
        protocolCode = getBits(frame, 37, 40);  // protocol code     bits 37-40
    }

    if (!longFrame || protocolFlag == 1) {  // User protocol (3 bits)
        switch (protocolCode) {
            case 0b000:
                protocol = &Protocol::USER_ORB;
                break;
            case 0b001:
                protocol = &Protocol::USER_ELT;
                break;
            case 0b010:
                protocol = &Protocol::USER_EPIRB_MARITIME;
                break;
            case 0b011:
                protocol = &Protocol::USER_SERIAL;
                break;
            case 0b100:
                protocol = &Protocol::USER_NAT;
                break;
            case 0b101:
                protocol = &Protocol::USER_2G;
                break;
            case 0b110:
                protocol = &Protocol::USER_EPIRB_RADIO;
                break;
            case 0b111:
                protocol = &Protocol::USER_TEST;
                break;
            default:
                protocol = &Protocol::UNKNOWN;
        }
    } else {  // Other protocols (4 bits)
        switch (protocolCode) {
            case 0b0000:
            case 0b0001:
                protocol = &Protocol::SPARE;
                break;
            case 0b0010:
                protocol = &Protocol::STD_EPIRB;
                break;
            case 0b0011:
                protocol = &Protocol::STD_ELT_24;
                break;
            case 0b0100:
                protocol = &Protocol::STD_ELT_SERIAL;
                break;
            case 0b0101:
                protocol = &Protocol::STD_ELT_AIRCRAFT;
                break;
            case 0b0110:
                protocol = &Protocol::STD_EPIRB_SERIAL;
                break;
            case 0b0111:
                protocol = &Protocol::STD_PLB_SERIAL;
                break;
            case 0b1000:
                protocol = &Protocol::NAT_ELT;
                break;
            case 0b1001:
                protocol = &Protocol::ELT_DT;
                break;
            case 0b1010:
                protocol = &Protocol::NAT_EPIRB;
                break;
            case 0b1011:
                protocol = &Protocol::NAT_PLB;
                break;
            case 0b1100:
                protocol = &Protocol::STD_SHIP;
                break;
            case 0b1101:
                protocol = &Protocol::RLS;
                break;
            case 0b1110:
                protocol = &Protocol::STD_TEST;
                break;
            case 0b1111:
                protocol = &Protocol::NAT_TEST;
                break;
            default:
                protocol = &Protocol::UNKNOWN;
        }
    }
}

void Beacon::setSerialNumber(uint32_t serial) {
    serialNumber = toHexString(serial);
    serialNumber = std::to_string(serial) + " (0x" + serialNumber + "";
}

void Beacon::parseAdditionalData() {
    hasAdditionalData = false;
    hasSerialNumber = false;
    if (protocolFlag) {               // User protocols
        if (protocolCode == 0b011) {  // Serial user
            uint8_t serialUserProtocol = getBits(frame, 40, 42);
            hasAdditionalData = true;
            switch (serialUserProtocol) {
                case 0b000:
                    additionalData = "ELTs/serial identification number";
                    break;
                case 0b001:
                    additionalData = "ELTs/aircraft operator & serial #";
                    break;
                case 0b010:
                    additionalData = "Float free EPIRBs/serial #";
                    break;
                case 0b011:
                    additionalData = "ELTs with aircraft 24-bit address";
                    break;
                case 0b100:
                    additionalData = "Non float free EPIRBs/serial #";
                    break;
                case 0b110:
                    additionalData = "PLBs/serial identification #";
                    break;
                default:
                    hasAdditionalData = false;
            }
            hasSerialNumber = true;
            setSerialNumber(getBits(frame, 44, 67));
        }
    } else if (longFrame) {  // Long frame other than User Protocol
        switch (protocolCode) {
            case 0b0010: {
                hasSerialNumber = true;
                setSerialNumber(getBits(frame, 61, 64));
            } break;
            case 0b1100: {
                hasAdditionalData = true;
                uint32_t mmsi = getBits(frame, 41, 60);
                additionalData = "MMSI=" + toHexString(mmsi) + " MID=" + std::to_string(mmsi);
            } break;
            case 0b0011: {
                hasAdditionalData = true;
                hasSerialNumber = true;
                setSerialNumber(getBits(frame, 41, 64));
                additionalData = "24 bits aircraft address";
            } break;
            case 0b0100:
            case 0b0110:
            case 0b0111: {
                hasAdditionalData = true;
                hasSerialNumber = true;
                uint32_t csTaNumber = getBits(frame, 41, 50);
                additionalData = "C/S TA #=" + std::to_string(csTaNumber);
                setSerialNumber(getBits(frame, 51, 64));
            } break;
            case 0b0101: {
                hasAdditionalData = true;
                hasSerialNumber = true;
                uint32_t data = getBits(frame, 41, 45);  // 1st char
                data = data + 32;
                additionalData = BAUDOT_CODE[data];

                data = getBits(frame, 46, 50);  // 2nd char
                data = data + 32;
                additionalData = additionalData + BAUDOT_CODE[data];

                data = getBits(frame, 51, 55);  // 3rd char
                data = data + 32;
                additionalData = additionalData + BAUDOT_CODE[data];

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
                additionalData = "National data=" + std::to_string(natNum);
            } break;
        }
    }
}

void Beacon::parseLocatingDevices() {
    // Main
    bool mainLocating;
    if (protocol->isStandard() || protocol->isNational()) {
        mainLocating = getBits(frame, 111, 111);
        mainLocatingDevice = mainLocating ? MainLocatingDevice::INTERNAL_NAV : MainLocatingDevice::EXTERNAL_NAV;
    } else if (protocol->isUser() || protocol->isRls()) {
        mainLocating = getBits(frame, 107, 107);
        mainLocatingDevice = mainLocating ? MainLocatingDevice::INTERNAL_NAV : MainLocatingDevice::EXTERNAL_NAV;
    }
    // Aux
    if (protocol->isStandard() || protocol->isNational()) {
        bool auxLocating = getBits(frame, 112, 112);
        auxLocatingDevice = auxLocating ? AuxLocatingDevice::MHZ121_5 : AuxLocatingDevice::NONE_OR_OTHER;
    } else if (protocol->isRls()) {
        bool auxLocating = getBits(frame, 108, 108);
        auxLocatingDevice = auxLocating ? AuxLocatingDevice::MHZ121_5 : AuxLocatingDevice::NONE_OR_OTHER;
    } else if (protocol->isUser() && protocolCode != 0b100) {  // User protocol with an aux locating device
        uint8_t auxLocating = getBits(frame, 84, 85);
        switch (auxLocating) {
            case 0b00:
                auxLocatingDevice = AuxLocatingDevice::NONE;
                break;
            case 0b01:
                auxLocatingDevice = AuxLocatingDevice::MHZ121_5;
                break;
            case 0b10:
                auxLocatingDevice = AuxLocatingDevice::SART;
                break;
            case 0b11:
                auxLocatingDevice = AuxLocatingDevice::OTHER;
                break;
        }
    }
}

void Beacon::parseFrame() {
    long latofmin, latofsec, lonofmin, lonofsec;
    bool latoffset, lonoffset;

    longFrame = getBits(frame, 25, 25);  // frame size : bit 25
    parseProtocol();
    unsigned long countryCode = getBits(frame, 27, 36);  // country code bit 27-36
    country = Country::getCountry(countryCode);

    if (frame[2] == 0xD0) {  // Self-test message frame synchronisation byte
        frameMode = FrameMode::SELF_TEST;
    } else if (frame[2] == 0x2F) {  // Normal message frame synchronisation byte
        frameMode = FrameMode::NORMAL;
    } else {
        frameMode = FrameMode::UNKNOWN;
    }

    // Hex id from bits 26-85
    identifier = getBits(frame, 26, 85);
    // Format HexId to string
    char buffer[32];
    uint32_t msb = identifier >> 32;
    uint32_t lsb = identifier;
    snprintf(buffer, sizeof(buffer), "%07lX%08lX", msb, lsb);
    hexId = string(buffer);

    if (longFrame) {
        if (protocol->isUser() && !isOrbito()) {  // User loc protocol long frame (no location for Obitography frame)
            location.latitude.orientation = (frame[13] & 0x10) >> 4;
            location.latitude.degrees = ((frame[13] & 0x0F) << 3 | (frame[14] & 0xE0) >> 5);
            location.latitude.minutes = (frame[14] & 0x1E) >> 1;
            location.latitude.minutes = (location.latitude.minutes * 4);
            location.longitude.orientation = (frame[14] & 0x01);
            location.longitude.degrees = (frame[15]);
            location.longitude.minutes = (frame[16] & 0xF0) >> 4;
            location.longitude.minutes = (location.longitude.minutes * 4);
            // Identification data in bits 26-85 (no default location values)
        } else if (protocol->isNational()) {      // Nat loc protocol
            latoffset = (frame[14] & 0x80) >> 7;  // Latitude NAT LOCATION
            location.latitude.orientation = (frame[7] & 0x20) >> 5;
            location.latitude.degrees = ((frame[7] & 0x1F) << 2 | (frame[8] & 0xC0) >> 6);
            location.latitude.minutes = (frame[8] & 0x3E) >> 1;
            location.latitude.minutes = (location.latitude.minutes * 2);
            latofmin = (frame[14] & 0x60) >> 5;
            latofsec = (frame[14] & 0x1E) >> 1;
            latofsec = (latofsec * 4);
            if (latoffset == 1) {
                location.latitude.minutes += latofmin;
                location.latitude.seconds += latofsec;
            } else if (latoffset == 0) {
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
            location.longitude.minutes = (frame[10] & 0xF8) >> 3;
            location.longitude.minutes = (location.longitude.minutes * 2);
            lonofmin = (frame[15] & 0xC0) >> 6;
            lonofsec = (frame[15] & 0x3C) >> 2;
            lonofsec = (lonofsec * 4);
            if (lonoffset == 1) {
                location.longitude.minutes += lonofmin;
                location.longitude.seconds += lonofsec;
            } else if (lonoffset == 0) {
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
            // Default location on bits 1/7-5/1/8-5 (27bits)
            identifier &= 0xFFFFFFFFF8000000ULL;
            identifier |= 0b011111110000001111111100000;
        } else if (protocol->isStandard()) {  // Std loc protocol
            latoffset = (frame[14] & 0x80) >> 7;
            location.latitude.orientation = (frame[8] & 0x80) >> 7;
            location.latitude.degrees = (frame[8] & 0x7F);
            location.latitude.minutes = (frame[9] & 0xC0) >> 6;
            location.latitude.minutes = (location.latitude.minutes * 15);
            latofmin = (frame[14] & 0x7C) >> 2;
            latofsec = ((frame[14] & 0x03) << 2 | (frame[15] & 0xC0) >> 6);
            latofsec = (latofsec * 4);
            if (latoffset == 1) {
                location.latitude.minutes += latofmin;
                location.latitude.seconds += latofsec;
            } else if (latoffset == 0) {
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

            lonoffset = (frame[15] & 0x20) >> 5;  // Longitude STD LOCATION
            location.longitude.orientation = (frame[9] & 0x20) >> 5;
            location.longitude.degrees = ((frame[9] & 0x1F) << 3 | (frame[10] & 0xE0) >> 5);
            location.longitude.minutes = (frame[10] & 0x18) >> 3;
            location.longitude.minutes = (location.longitude.minutes * 15);
            lonofmin = (frame[15] & 0x1F);
            lonofsec = (frame[16] & 0xF0) >> 4;
            lonofsec = (lonofsec * 4);
            if (lonoffset == 1) {
                location.longitude.minutes += lonofmin;
                location.longitude.seconds += lonofsec;
            } else if (lonoffset == 0) {
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
            // Default location on bits 1/9/1/10
            identifier &= 0xFFFFFFFFFFE00000ULL;
            identifier |= 0b011111111101111111111;
        } else if (protocol->isRlsOrElt()) {  // RLS Location protocol => protocolCode == 0b1101 / ELT(DT) Location protocol => protocolCode == 0b1001
            latoffset = (frame[14] & 0x20) >> 5;
            location.latitude.orientation = (frame[8] & 0x20) >> 5;
            location.latitude.degrees = ((frame[8] & 0x1F) << 2) | ((frame[9] & 0xC0) >> 6);
            location.latitude.minutes = (frame[9] & 0x20) >> 5;
            location.latitude.minutes = (location.latitude.minutes * 30);
            latofmin = (frame[14] & 0x1E) >> 1;
            latofsec = ((frame[14] & 0x01) << 3 | (frame[15] & 0xE0) >> 5);
            latofsec = (latofsec * 4);
            if (latoffset == 1) {
                location.latitude.minutes += latofmin;
                location.latitude.seconds += latofsec;
            } else if (latoffset == 0) {
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
            location.longitude.minutes = (frame[10] & 0x08) >> 3;
            location.longitude.minutes = (location.longitude.minutes * 30);
            lonofmin = (frame[15] & 0x0F);
            lonofsec = (frame[16] & 0xF0) >> 4;
            lonofsec = (lonofsec * 4);
            if (lonoffset == 1) {
                location.longitude.minutes += lonofmin;
                location.longitude.seconds += lonofsec;
            } else if (lonoffset == 0) {
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
            // Default location on bits 1/8/1/9 (19bits)
            identifier &= 0xFFFFFFFFFFF80000ULL;
            identifier |= 0b0111111110111111111;
        }
    }
    // else = User Protocol / short frame => Identification data in bits 26-85 (no default values)

    // Parse additional data
    parseAdditionalData();

    // Parse locating device information
    parseLocatingDevices();

    // Check for Empty frame
    if (isOrbito() && !longFrame) {
        isEmpty = true;
        for (size_t i = 3; i < BEACON_DATA_SIZE; i++) {
            if (frame[i] != 0) {
                isEmpty = false;
                break;
            }
        }
    } else {
        isEmpty = false;
    }

    // Actual and computed BCH1 and BCH2 values
    bch1 = getBits(frame, 86, 106);
    computedBch1 = computeBCH1(frame);
    // Special handing of Orbitography Protocol that has no BCH2 error correction code
    hasBch2 = !isOrbito();
    if (hasBch2) {
        bch2 = getBits(frame, 133, 144);
        computedBch2 = computeBCH2(frame);
    }
}

bool Beacon::isBch1Valid() {
    return (bch1 == computedBch1);
}

bool Beacon::isBch2Valid() {
    return (bch2 == computedBch2);
}

bool Beacon::isFrameValid() {
    return isBch1Valid() && ((!hasBch2) || isBch2Valid()) && (!isEmpty);
}

bool Beacon::isOrbito() {
    return (protocol == &Protocol::USER_ORB);
}

string Beacon::toKvpString() {
    string result;
    // result += "date=" + date.getDateString() + "\n";
    // result += "time=" + date.getTimeString() + "\n";
    //  Frame title
    result += "title=" + getFrameTitle() + "\n";
    // Protocol name
    result += "protocol=" + getProtocolName() + "\n";
    // Protocol description
    result += "protocolDesc=" + getProtocolDesciption() + "\n";
    // Protocol additional data
    if (hasAdditionalData) {  // Additionnal protocol data
        result += "protocolAddData=" + additionalData + "\n";
    }

    // Location
    // Country
    result += "country=" + country.toString() + "\n";
    // Coordinates
    if (!location.isUnknown()) {
        result += "lat=" + std::to_string(location.latitude.getFloatValue()) + "\n";
        result += "lon=" + std::to_string(location.longitude.getFloatValue()) + "\n";
    }
    // Control codes
    result += ("bch1=" + (isBch1Valid() ? string("ok") : string("ko")) + "\n");
    if (longFrame && hasBch2) {  // No second proteced field in short frames
        result += ("bch2=" + (isBch2Valid() ? string("ok") : string("ko")) + "\n");
    }

    // Hex ID
    result += "hexId=" + hexId + "\n";

    // Serial #
    if (hasSerialNumber) {  // Serial number
        result += "serial=" + serialNumber + "\n";
    }

    // Location devices
    if (hasMainLocatingDevice()) {  // Main locating device
        result += "mainDevice=" + getMainLocatingDeviceName() + "\n";
    }

    if (hasAuxLocatingDevice()) {  // Auxiliary locating device
        result += "auxDevice=" + getAuxLocatingDeviceName() + "\n";
    }

    // Data
    result += "data=" + hexString(false) + "\n";
    return result;
}

string Beacon::hexString(bool withHeader) {
    return toHexString(frame, false, (withHeader ? 0 : 3), (longFrame ? 18 : 14));
}

}  // namespace ui::external_app::epirb_rx