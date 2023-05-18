/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_RDS_H__
#define __PROC_RDS_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#define SAMPLES_PER_BIT 192
#define FILTER_SIZE 576
#define SAMPLE_BUFFER_SIZE SAMPLES_PER_BIT + FILTER_SIZE

class RDSProcessor : public BasebandProcessor {
 public:
  void execute(const buffer_c8_t& buffer) override;

  void on_message(const Message* const msg) override;

 private:
  uint32_t* rdsdata{};

  BasebandThread baseband_thread{2280000, this, NORMALPRIO + 20, baseband::Direction::Transmit};

  uint16_t message_length{0};
  int8_t re{0}, im{0};
  uint8_t mphase{0}, s{0};
  uint32_t bit_pos{0};
  int32_t sample_buffer[SAMPLE_BUFFER_SIZE] = {0};
  int32_t val{0};
  uint8_t prev_output{0};
  uint8_t cur_output{0};
  uint8_t cur_bit{0};
  int sample_count = SAMPLES_PER_BIT;
  int in_sample_index = 0;
  int32_t sample{0};
  int out_sample_index = SAMPLE_BUFFER_SIZE - 1;
  uint32_t phase{0}, sphase{0};
  int32_t delta{0};

  bool configured{false};

  const int32_t waveform_biphase[576] = {
      165, 167, 168, 168, 167, 166, 163, 160,
      157, 152, 147, 141, 134, 126, 118, 109,
      99, 88, 77, 66, 53, 41, 27, 14,
      0, -14, -29, -44, -59, -74, -89, -105,
      -120, -135, -150, -165, -179, -193, -206, -218,
      -231, -242, -252, -262, -271, -279, -286, -291,
      -296, -299, -301, -302, -302, -300, -297, -292,
      -286, -278, -269, -259, -247, -233, -219, -202,
      -185, -166, -145, -124, -101, -77, -52, -26,
      0, 27, 56, 85, 114, 144, 175, 205,
      236, 266, 296, 326, 356, 384, 412, 439,
      465, 490, 513, 535, 555, 574, 590, 604,
      616, 626, 633, 637, 639, 638, 633, 626,
      616, 602, 586, 565, 542, 515, 485, 451,
      414, 373, 329, 282, 232, 178, 121, 62,
      0, -65, -132, -202, -274, -347, -423, -500,
      -578, -656, -736, -815, -894, -973, -1051, -1128,
      -1203, -1276, -1347, -1415, -1479, -1540, -1596, -1648,
      -1695, -1736, -1771, -1799, -1820, -1833, -1838, -1835,
      -1822, -1800, -1767, -1724, -1670, -1605, -1527, -1437,
      -1334, -1217, -1087, -943, -785, -611, -423, -219,
      0, 235, 487, 755, 1040, 1341, 1659, 1994,
      2346, 2715, 3101, 3504, 3923, 4359, 4811, 5280,
      5764, 6264, 6780, 7310, 7856, 8415, 8987, 9573,
      10172, 10782, 11404, 12036, 12678, 13329, 13989, 14656,
      15330, 16009, 16694, 17382, 18074, 18767, 19461, 20155,
      20848, 21539, 22226, 22909, 23586, 24256, 24918, 25571,
      26214, 26845, 27464, 28068, 28658, 29231, 29787, 30325,
      30842, 31339, 31814, 32266, 32694, 33097, 33473, 33823,
      34144, 34437, 34699, 34931, 35131, 35299, 35434, 35535,
      35602, 35634, 35630, 35591, 35515, 35402, 35252, 35065,
      34841, 34579, 34279, 33941, 33566, 33153, 32702, 32214,
      31689, 31128, 30530, 29897, 29228, 28525, 27788, 27017,
      26214, 25379, 24513, 23617, 22693, 21740, 20761, 19755,
      18725, 17672, 16597, 15501, 14385, 13251, 12101, 10935,
      9755, 8563, 7360, 6148, 4927, 3701, 2470, 1235,
      0, -1235, -2470, -3701, -4927, -6148, -7360, -8563,
      -9755, -10935, -12101, -13251, -14385, -15501, -16597, -17672,
      -18725, -19755, -20761, -21740, -22693, -23617, -24513, -25379,
      -26214, -27017, -27788, -28525, -29228, -29897, -30530, -31128,
      -31689, -32214, -32702, -33153, -33566, -33941, -34279, -34579,
      -34841, -35065, -35252, -35402, -35515, -35591, -35630, -35634,
      -35602, -35535, -35434, -35299, -35131, -34931, -34699, -34437,
      -34144, -33823, -33473, -33097, -32694, -32266, -31814, -31339,
      -30842, -30325, -29787, -29231, -28658, -28068, -27464, -26845,
      -26214, -25571, -24918, -24256, -23586, -22909, -22226, -21539,
      -20848, -20155, -19461, -18767, -18074, -17382, -16694, -16009,
      -15330, -14656, -13989, -13329, -12678, -12036, -11404, -10782,
      -10172, -9573, -8987, -8415, -7856, -7310, -6780, -6264,
      -5764, -5280, -4811, -4359, -3923, -3504, -3101, -2715,
      -2346, -1994, -1659, -1341, -1040, -755, -487, -235,
      0, 219, 423, 611, 785, 943, 1087, 1217,
      1334, 1437, 1527, 1605, 1670, 1724, 1767, 1800,
      1822, 1835, 1838, 1833, 1820, 1799, 1771, 1736,
      1695, 1648, 1596, 1540, 1479, 1415, 1347, 1276,
      1203, 1128, 1051, 973, 894, 815, 736, 656,
      578, 500, 423, 347, 274, 202, 132, 65,
      0, -62, -121, -178, -232, -282, -329, -373,
      -414, -451, -485, -515, -542, -565, -586, -602,
      -616, -626, -633, -638, -639, -637, -633, -626,
      -616, -604, -590, -574, -555, -535, -513, -490,
      -465, -439, -412, -384, -356, -326, -296, -266,
      -236, -205, -175, -144, -114, -85, -56, -27,
      0, 26, 52, 77, 101, 124, 145, 166,
      185, 202, 219, 233, 247, 259, 269, 278,
      286, 292, 297, 300, 302, 302, 301, 299,
      296, 291, 286, 279, 271, 262, 252, 242,
      231, 218, 206, 193, 179, 165, 150, 135,
      120, 105, 89, 74, 59, 44, 29, 14,
      0, -14, -27, -41, -53, -66, -77, -88,
      -99, -109, -118, -126, -134, -141, -147, -152,
      -157, -160, -163, -166, -167, -168, -168, -167};
};

#endif
