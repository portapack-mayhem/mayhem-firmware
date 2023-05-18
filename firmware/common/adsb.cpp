/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "adsb.hpp"
#include "sine_table.hpp"
#include "utility.hpp"

#include <math.h>

namespace adsb {

void make_frame_adsb(ADSBFrame& frame, const uint32_t ICAO_address) {
    frame.clear();
    frame.push_byte((DF_ADSB << 3) | 5);  // DF=17 and CA
    frame.push_byte(ICAO_address >> 16);
    frame.push_byte(ICAO_address >> 8);
    frame.push_byte(ICAO_address & 0xFF);
}

// Civil aircraft ADS-B message type starts with Dowlink Format (DF=17) and frame is 112 bits long.
// All known DF's >=16 are long (112 bits). All known DF's <=15 are short (56 bits).(In this case 112 bits)
// Msg structure consists of five main parts :|DF=17 (5 bits)|CA (3 bits)|ICAO (24 bits)|ME (56 bits)|CRC (24 bits)
// Aircraft identification and category  message structure, the ME (56 bits) =  TC,5 bits | CA,3 bits | C1,6 bits | C2,6 bits | C3,6 | C4,6 | C5,6 | C6,6 | C7,6 | C8,6
// TC : (1..4) : Aircraft identification Type Code . // TC : 9 to 18: Airbone postion // TC : 19 Airbone velocity .
// In this encode_frame_identification function  we are using DF = 17 (112 bits long)  and  TC=4)

void encode_frame_id(ADSBFrame& frame, const uint32_t ICAO_address, const std::string& callsign) {
    std::string callsign_formatted(8, '_');
    uint64_t callsign_coded = 0;
    uint32_t c, s;
    char ch;

    make_frame_adsb(frame, ICAO_address);  // Header DF=17 Downlink Format = ADS-B message (frame 112 bits)

    frame.push_byte(TC_IDENT << 3);  // 5 top bits ME = TC = we use fix 4  , # Type aircraft Identification Category = TC_IDENT = 4,

    // Translate and encode callsign
    for (c = 0; c < 8; c++) {
        ch = callsign[c];

        for (s = 0; s < 64; s++)
            if (ch == icao_id_lut[s]) break;

        if (s == 64) {
            ch = ' ';  // Invalid character
            s = 32;
        }

        callsign_coded <<= 6;
        callsign_coded |= s;

        // callsign[c] = ch;
    }

    // Insert callsign in frame
    for (c = 0; c < 6; c++)
        frame.push_byte((callsign_coded >> ((5 - c) * 8)) & 0xFF);

    frame.make_CRC();
}

std::string decode_frame_id(ADSBFrame& frame) {
    std::string callsign = "";
    uint8_t* raw_data = frame.get_raw_data();
    uint64_t callsign_coded = 0;
    uint32_t c;

    // Frame bytes to long
    for (c = 5; c < 11; c++) {
        callsign_coded <<= 8;
        callsign_coded |= raw_data[c];
    }

    // Long to 6-bit characters
    for (c = 0; c < 8; c++) {
        callsign.append(1, icao_id_lut[(callsign_coded >> 42) & 0x3F]);
        callsign_coded <<= 6;
    }

    return callsign;
}

/*void generate_frame_emergency(ADSBFrame& frame, const uint32_t ICAO_address, const uint8_t code) {
        make_frame_mode_s(frame, ICAO_address);

        frame.push_byte((28 << 3) + 1);	// TC = 28 (Emergency), subtype = 1 (Emergency)
        frame.push_byte(code << 5);

        frame.make_CRC();
}*/

// Mode S services. (Mode Select Beacon System). There are two types of Mode S interrogations: The short (56 bits) . and the long (112 bits )
// All known DF's >=16 are long frame msg  (112 bits). All known DF's <=15 are short frame msgs (56 bits).(In this case 112 bits)
// Identity squawk replies can be DF=5 (Surveillance Identity reply)(56 bits)  / DF 21 (Comm-B with Identity reply) (112 bits)
// DF 21: Comm-B with identity reply structure = |DF=21(5 bits)|FS (3 bits)|DR (5 bits)|UM (6 bits) |Identity squawk code (13 bits) |MB (56 bits) |CRC (24 bits) (total 112 bits)
// Comm-B messages count for a large portion of the Mode S selective interrogation responses.(means, only transmitted information upon selective request)
// Comm-B messages protocol supports many different types of msg's (up to 255).The three more popular ones are the following ones:
// (a) Mode S ELementary Surveillance (ELS) / (b) Mode S EnHanced Surveillance (EHS) / (c) Meteorological information
// Comm-B Data Selector (BDS) is an 8-bit code that determines which information to be included in the MB fields

void encode_frame_squawk(ADSBFrame& frame, const uint16_t squawk) {
    uint16_t squawk_coded;
    uint8_t UM_field = 0b111101, FS = 0b010, DR = 0b00001;

    // To be sent those fields, (56 bits). We should store byte by byte into the frame , and  It will be transmitted byte to byte same  FIFO order.
    // DF 			  5 bits    5		DF=21 (5 top bits) Downlink Format
    // FS 			  3 bits    0b000, 	FS (3 bottom bits) (Flight status ) = 000 : no alert, no SPI, aircraft is airborne
    // DR 			  5 bits    0b00001  DR (Downlink request) (5 top bits)  = 00000 : no downlink request (In surveillance replies, only values 0, 1, 4, and 5 are used.)
    // UM 			  6 bits    0b000010 UM (Utility message)= 000000, Utility message (UM): 6 bits, contains transponder communication status information.(IIS + IDS)
    // Identity_code 13 bits	squawk id_code in special interleaved format.
    // MB			 56 bits
    // CRC partity 	 24 bits    parity checksum , cyclic redundancy chek.

    frame.clear();
    frame.push_byte((DF_EHS_SQUAWK << 3) | FS);    // DF=21 (5bits) + FS (3bits,  010 : alert, NO SPI, aircraft is airborne)
    frame.push_byte((DR << 3) | (UM_field >> 3));  // DR  (5bits, 00001 : downlink request + 3 top bits of UM , let's use 0b111000

    // 12 11 10  9  8  7  6  5  4  3  2  1  0         (Original notes) bit weight position----------------------
    // 32 31 30 29 28 27 26 25 24 23 22 21 20         (it was wrong , now corrected) bit order inside frame msg
    // D4 B4 D2 B2 D1 B1 __ A4 C4 A2 C2 A1 C1         standard spec order of the 13 bits,  to be sent , each octal digit = 3 bits , (example A=7 binary A4 A2 A0 = 111
    // ABCD = code (octal, 0000~7777)

    // FEDCBA9876543210
    // xAAAxBBBxCCCxDDD   4 x 3 bits (each octal digit)
    // x421x421x421x421   binary weight of each binary position,  example AAA = 7 = 111 -------------------------

    // Additional , expanded  notes -------------------------------
    // Identity squawk code ABCD = code (octal, 0000~7777) , input concatenated squawk : 4 octal digits ,A4 A2 A1-B4 B2 B1-C4 C2 C1-D4 D2 D1.
    // 17	18	19	20	21	22	23	24	25	26	 27	28	29	30	31	32	 	bit position of the frame msg, (Squawk id is bit 20-32, from C1..D4).
    // UM4	UM2	UM1	C1	A1	C2	A2	C4	A4 	X    B1	D1	B2	D2	B4	D4      3 lower bit UM4,UM2,UM1 of the UM (6bits), and we should re-order the 13 bits ABCD changing 12 bit poistion based on std.
    // 15	14	13	12	11	10	9	8	7	6	 5	4	3	2	1	0       Two bytes , bit position  to be send.

    squawk_coded = (((UM_field & (0b111)) << 13) | ((squawk << 9) & 0x1000)) |  // C1	It also leaves in the top 3 lower bottom bitd part of UM field.
                   ((squawk << 2) & 0x0800) |                                   // A1
                   ((squawk << 6) & 0x0400) |                                   // C2
                   ((squawk >> 1) & 0x0200) |                                   // A2
                   ((squawk << 3) & 0x0100) |                                   // C4
                   ((squawk >> 4) & 0x0080) |                                   // A4

                   ((squawk >> 1) & 0x0020) |  // B1
                   ((squawk << 4) & 0x0010) |  // D1
                   ((squawk >> 4) & 0x0008) |  // B2
                   ((squawk << 1) & 0x0004) |  // D2
                   ((squawk >> 7) & 0x0002) |  // B4
                   ((squawk >> 2) & 0x0001);   // D4

    frame.push_byte(squawk_coded >> 8);  // UM4	UM2	 UM1 C1	A1	C2	A2	C4  that is the correct order, confirmed with dump1090
    frame.push_byte(squawk_coded);       // A4   X(1) B1	 D1	B2	D2	B4	D4  that is the correct order, confirmed with dupm1090

    // DF 21 messages , has  56 bits more after 13 bits of squawk, we should add MB (56 bits)
    // In this example, we are adding fixed MB = Track and turn report (BDS 5,0) decoding MB example = "F9363D3BBF9CE9" (56 bits)
    // # -9.7, roll angle (deg)
    // # 140.273, track angle (deg)
    // # -0.406, track angle rate (deg/s)
    // # 476, ground speed (kt)
    // # 466, TAS (kt)

    frame.push_byte(0xF9);
    frame.push_byte(0x36);
    frame.push_byte(0x3D);
    frame.push_byte(0x3B);  // If we deltele those two lines,  to send this fixed MB (56 bits),
    frame.push_byte(0xBF);
    frame.push_byte(0x9C);
    frame.push_byte(0xE9);  // current fw is padding with 56 x 0's to complete 112 bits msg.

    frame.make_CRC();
}

float cpr_mod(float a, float b) {
    return a - (b * floor(a / b));
}

int cpr_NL_precise(float lat) {
    return (int)floor(2 * PI / acos(1 - ((1 - cos(PI / (2 * NZ))) / pow(cos(PI * lat / 180), 2))));
}

int cpr_NL_approx(float lat) {
    if (lat < 0)
        lat = -lat;  // Symmetry

    for (size_t c = 0; c < 58; c++) {
        if (lat < adsb_lat_lut[c])
            return 59 - c;
    }

    return 1;
}

int cpr_NL(float lat) {
    // TODO prove that the approximate function is good
    // enough for the precision we need. Uncomment if
    // that is true. No performance penalty was noticed
    // from testing, but if you find it might be an issue,
    // switch to cpr_NL_approx() instead:

    // return cpr_NL_approx(lat);

    return cpr_NL_precise(lat);
}

int cpr_N(float lat, int is_odd) {
    int nl = cpr_NL(lat) - is_odd;

    if (nl < 1)
        nl = 1;

    return nl;
}

float cpr_Dlon(float lat, int is_odd) {
    return 360.0 / cpr_N(lat, is_odd);
}

// An ADS-B frame Civil aircraft message type starts with Dowlink Format (DF=17) and frame is 112 bits long.
// All known DF's >=16 are long (112 bits). All known DF's <=15 are short (56 bits). (In this case 112 bits)
// Msg structure consists of five main parts :|DF=17 (5 bits)|CA (3 bits)|ICAO (24 bits)|ME (56 bits)|CRC (24 bits)
// Airborne position msg  struct, the ME (56 bits) = |TC,5 bits| SS, 2 bits | SAF, 1 | ALT, 12 | T, 1 | F, 1 | LAT-CPR, 17 | LON-CPR, 17
// TC : (1..4) : Aircraft identification Type Code. // TC : 9 to 18: Airbone postion and altitude // TC : 19 Airbone velocity .
// Airborne position message is used to broadcast the position and altitude of the aircraft. It has the Type Code 9–18 and 20–22. (here , we use TC=11)

void encode_frame_pos(ADSBFrame& frame, const uint32_t ICAO_address, const int32_t altitude, const float latitude, const float longitude, const uint32_t time_parity) {
    uint32_t altitude_coded;
    uint32_t lat, lon;
    float delta_lat, yz, rlat, delta_lon, xz;

    make_frame_adsb(frame, ICAO_address);  // Header DF=17 (long frame 112 bits)

    frame.push_byte(TC_AIRBORNE_POS << 3);  // Bits 2~1: Surveillance Status, bit 0: NICsb

    altitude_coded = (altitude + 1000) / 25;  // 25ft precision, insert Q-bit (1)
    altitude_coded = ((altitude_coded & 0x7F0) << 1) | 0x10 | (altitude_coded & 0x0F);

    frame.push_byte(altitude_coded >> 4);  // Top-most altitude bits

    // CPR encoding
    // Info from: http://antena.fe.uni-lj.si/literatura/Razno/Avionika/modes/CPRencoding.pdf

    delta_lat = 360.0 / ((4.0 * NZ) - time_parity);  // NZ = 15
    yz = floor(CPR_MAX_VALUE * (cpr_mod(latitude, delta_lat) / delta_lat) + 0.5);
    rlat = delta_lat * ((yz / CPR_MAX_VALUE) + floor(latitude / delta_lat));

    if ((cpr_NL(rlat) - time_parity) > 0)
        delta_lon = 360.0 / cpr_N(rlat, time_parity);
    else
        delta_lon = 360.0;
    xz = floor(CPR_MAX_VALUE * (cpr_mod(longitude, delta_lon) / delta_lon) + 0.5);

    lat = cpr_mod(yz, CPR_MAX_VALUE);
    lon = cpr_mod(xz, CPR_MAX_VALUE);

    frame.push_byte((altitude_coded << 4) | ((uint32_t)time_parity << 2) | (lat >> 15));  // T = 0
    frame.push_byte(lat >> 7);
    frame.push_byte((lat << 1) | (lon >> 16));
    frame.push_byte(lon >> 8);
    frame.push_byte(lon);

    frame.make_CRC();
}

// Decoding method from dump1090
adsb_pos decode_frame_pos(ADSBFrame& frame_even, ADSBFrame& frame_odd) {
    uint8_t* raw_data;
    uint32_t latcprE, latcprO, loncprE, loncprO;
    float latE, latO, m, Dlon, cpr_lon_odd, cpr_lon_even, cpr_lat_odd, cpr_lat_even;
    int ni;
    adsb_pos position{false, 0, 0, 0};

    uint32_t time_even = frame_even.get_rx_timestamp();
    uint32_t time_odd = frame_odd.get_rx_timestamp();
    uint8_t* frame_data_even = frame_even.get_raw_data();
    uint8_t* frame_data_odd = frame_odd.get_raw_data();

    // Return most recent altitude
    if (time_even > time_odd)
        raw_data = frame_data_even;
    else
        raw_data = frame_data_odd;

    // Q-bit must be present
    if (raw_data[5] & 1)
        position.altitude = ((((raw_data[5] & 0xFE) << 3) | ((raw_data[6] & 0xF0) >> 4)) * 25) - 1000;

    // Position
    latcprE = ((frame_data_even[6] & 3) << 15) | (frame_data_even[7] << 7) | (frame_data_even[8] >> 1);
    loncprE = ((frame_data_even[8] & 1) << 16) | (frame_data_even[9] << 8) | frame_data_even[10];

    latcprO = ((frame_data_odd[6] & 3) << 15) | (frame_data_odd[7] << 7) | (frame_data_odd[8] >> 1);
    loncprO = ((frame_data_odd[8] & 1) << 16) | (frame_data_odd[9] << 8) | frame_data_odd[10];

    // Calculate the coefficients
    cpr_lon_even = loncprE / CPR_MAX_VALUE;
    cpr_lon_odd = loncprO / CPR_MAX_VALUE;

    cpr_lat_odd = latcprO / CPR_MAX_VALUE;
    cpr_lat_even = latcprE / CPR_MAX_VALUE;

    // Compute latitude index
    float j = floor(((59.0 * cpr_lat_even) - (60.0 * cpr_lat_odd)) + 0.5);
    latE = (360.0 / 60.0) * (cpr_mod(j, 60) + cpr_lat_even);
    latO = (360.0 / 59.0) * (cpr_mod(j, 59) + cpr_lat_odd);

    if (latE >= 270) latE -= 360;
    if (latO >= 270) latO -= 360;

    // Both frames must be in the same latitude zone
    if (cpr_NL(latE) != cpr_NL(latO))
        return position;

    // Compute longitude
    if (time_even > time_odd) {
        // Use even frame2
        ni = cpr_N(latE, 0);
        Dlon = 360.0 / ni;

        m = floor((cpr_lon_even * (cpr_NL(latE) - 1)) - (cpr_lon_odd * cpr_NL(latE)) + 0.5);

        position.longitude = Dlon * (cpr_mod(m, ni) + cpr_lon_even);

        position.latitude = latE;
    } else {
        // Use odd frame
        ni = cpr_N(latO, 1);
        Dlon = 360.0 / ni;

        m = floor((cpr_lon_even * (cpr_NL(latO) - 1)) - (cpr_lon_odd * cpr_NL(latO)) + 0.5);

        position.longitude = Dlon * (cpr_mod(m, ni) + cpr_lon_odd);

        position.latitude = latO;
    }

    if (position.longitude >= 180) position.longitude -= 360;

    position.valid = true;

    return position;
}

// An ADS-B frame is 112 bits long. Civil aircraft ADS-B message starts with the Downlink Format ,DF=17.
// Msg structure  consists of five main parts :|DF=17 (5 bits)|CA (3 bits)|ICAO (24 bits)|ME (56 bits)|CRC (24 bits)
// Airborne velocities are all transmitted with Type Code 19 ( TC=19 ) inside ME (56 bits)
// [units] : speed is in knots,    vertical rate climb / descend  is in ft/min

void encode_frame_velo(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t speed, const float angle, const int32_t v_rate) {
    int32_t velo_ew, velo_ns;
    uint32_t velo_ew_abs, velo_ns_abs, v_rate_coded_abs;

    // To get NS and EW speeds from speed and bearing, a polar to cartesian conversion is enough
    velo_ew = static_cast<int32_t>(sin_f32(DEG_TO_RAD(angle)) * speed);             // East direction, is the projection from West -> East is directly sin(angle=Compas Bearing) , (90º is the max +1, EAST) max velo_EW
    velo_ns = static_cast<int32_t>(sin_f32((pi / 2 - DEG_TO_RAD(angle))) * speed);  // North direction,is the projection of North = cos(angle=Compas Bearing), cos(angle)= sen(90-angle) (0º is the max +1 NORTH) max velo_NS

    v_rate_coded_abs = (abs(v_rate) / 64) + 1;  // encoding vertical rate source.  (Decoding, VR ft/min = (Decimal v_rate_value - 1)* 64)

    velo_ew_abs = abs(velo_ew) + 1;  // encoding Velo speed EW , when sign Direction is 0 (+): West->East, (-) 1: East->West
    velo_ns_abs = abs(velo_ns) + 1;  // encoding Velo speed NS , when sign Direction is 0 (+): South->North , (-) 1: North->South

    make_frame_adsb(frame, ICAO_address);  // Header DF=17 (long frame 112 bits)

    // Airborne velocities are all transmitted with Type Code 19 ( TC=19, using 5 bits ,TC=19 [Binary: 10011]), the following 3 bits are Subt-type Code ,SC= 1,2,3,4
    // SC Subtypes code  1 and 2 are used to report ground speeds of aircraft. (SC 3,4 to used to report true airspeed. SC 2,4 are for supersonic aircraft (not used in commercial airline).
    frame.push_byte((TC_AIRBORNE_VELO << 3) | 1);  // 1st byte , top 5 bits Type Code TC=19, and lower 3 bits (38-40 bits), SC=001 Subtype Code SC: 1 (subsonic) ,

    // Message A, (ME bits from 14-35) , 22 bits = Sign ew(1 bit) + V_ew (10 bits) + Sign_ns (1 bit)  + V_ns (10 bits)
    // Vertical rate source bit VrSrc (ME bit 36) indicates source of the altitude measurements. GNSS altitude(0) /  , barometric altitude(1).
    // Vertical rate source direction,(ME bit 37)  movement can be read from Svr bit , with 0 and 1 referring to climb and descent, respectively (ft/min)
    // The encoded vertical rate value VR can be computed using message (ME bits 38 to 46). If the 9-bit block contains all zeros, the vertical rate information is not available.
    // + Sign VrSrc (vert rate src)  (1 bit)+ VrSrc (9 bits).
    frame.push_byte(((velo_ew < 0 ? 1 : 0) << 2) | (velo_ew_abs >> 8));
    frame.push_byte(velo_ew_abs);
    frame.push_byte(((velo_ns < 0 ? 1 : 0) << 7) | (velo_ns_abs >> 3));
    frame.push_byte((velo_ns_abs << 5) | ((v_rate < 0 ? 1 : 0) << 3) | (v_rate_coded_abs >> 6));  // VrSrc = 0
    frame.push_byte(v_rate_coded_abs << 2);
    frame.push_byte(0);

    frame.make_CRC();
}

// Decoding method from dump1090
adsb_vel decode_frame_velo(ADSBFrame& frame) {
    adsb_vel velo{false, 0, 0, 0};

    uint8_t* frame_data = frame.get_raw_data();
    uint8_t velo_type = frame.get_msg_sub();

    if (velo_type >= 1 && velo_type <= 4) {  // vertical rate is always present

        velo.v_rate = (((frame_data[8] & 0x07) << 6) | ((frame_data[9] >> 2) - 1)) * 64;

        if ((frame_data[8] & 0x8) >> 3) velo.v_rate *= -1;  // check v_rate sign
    }

    if (velo_type == 1 || velo_type == 2) {  // Ground Speed
        int32_t raw_ew = ((frame_data[5] & 0x03) << 8) | frame_data[6];
        int32_t velo_ew = raw_ew - 1;  // velocities are all offset by one (this is part of the spec)

        int32_t raw_ns = ((frame_data[7] & 0x7f) << 3) | (frame_data[8] >> 5);
        int32_t velo_ns = raw_ns - 1;

        if (velo_type == 2) {  // supersonic indicator so multiply by 4
            velo_ew = velo_ew << 2;
            velo_ns = velo_ns << 2;
        }

        if (frame_data[5] & 0x04) velo_ew *= -1;  // check ew direction sign
        if (frame_data[7] & 0x80) velo_ns *= -1;  // check ns direction sign

        velo.speed = fast_int_magnitude(velo_ns, velo_ew);

        if (velo.speed) {
            // calculate heading in degrees from ew/ns velocities
            int16_t heading_temp = (int16_t)(int_atan2(velo_ew, velo_ns));  // Nearest degree
            // We don't want negative values but a 0-360 scale.
            if (heading_temp < 0) heading_temp += 360.0;
            velo.heading = (uint16_t)heading_temp;
        }

    } else if (velo_type == 3 || velo_type == 4) {  // Airspeed
        velo.valid = frame_data[5] & (1 << 2);
        velo.heading = ((((frame_data[5] & 0x03) << 8) | frame_data[6]) * 45) << 7;
    }

    return velo;
}

} /* namespace adsb */
