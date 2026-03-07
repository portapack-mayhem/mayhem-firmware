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
#ifndef __BEACON_H__
#define __BEACON_H__

#include "portapack.hpp"
#include "ui_epirb_tx.hpp"
#include "location.hpp"

namespace ui::external_app::epirb_tx {

static uint64_t get_bits(uint8_t *data, int startBit, int endBit)
{
    uint64_t result = 0;
    // 0 bases bit count
    startBit--;
    int numBits = endBit - startBit;

    // get a pointer to the starting byte...
    const uint8_t *pData = &(data[startBit / 8]);
    uint8_t b = *pData;

    // calculate the starting bit within that byte...
    int bitOffset = 7 - (startBit % 8);

    // iterate for the desired number of bits...
    for(int i = 0; i < numBits; ++i)
    {
        // make room for the next bit...
        result <<= 1;
        // copy the bit...
        result |= ((b >> bitOffset) & 0x01);
        // reached the end of the current byte?
        if (--bitOffset < 0)
        {
            b = *(++pData); // go to the next byte...
            bitOffset = 7; // restart at the first bit in that byte...
        }
    }

    // all done...
    return result;
}

static uint64_t compute_bch(uint8_t* frame, int startBit, int endBit, unsigned long poly, int polyLength)
{   // Length of data to be checked (not including the BCH code)
    int dataLength = endBit-startBit+1;
    // Total lengh (including the BCH code that will be padded to zeros (BCH code length is polyLengh-1))
    int totalLength = dataLength+polyLength-1;
    // Start with the first polyLength bits
    uint64_t result = get_bits(frame, startBit,startBit+polyLength-1);
    for (int i = polyLength; i <= totalLength; i++) 
    {   // Iterate on each bit after the first polyLength batch
        bool firstBit = result >> (polyLength-1);
        if(firstBit)
        {   // We have a leading 1 => xor the result with the poly
            result = result^poly;
        }
        if(i<totalLength)
        {   // Move to next bit
            result = result << 1;
            if(i<dataLength)
            {   // Append next bit
                result |= get_bits(frame,startBit+i,startBit+i);
            } // else : 0 padding after data length
        }
    }
    return result;
}

// 21 bits BCH polynomial
#define BCH_21_POLYNOMIAL   0b1001101101100111100011UL 
#define BCH_21_POLY_LENGTH  22
// 12 bits BCH polynomial
#define BCH_12_POLYNOMIAL   0b1010100111001UL
#define BCH_12_POLY_LENGTH  13

static uint64_t compute_bch1(uint8_t* frame)
{
    return compute_bch(frame,25,85,BCH_21_POLYNOMIAL,BCH_21_POLY_LENGTH);
}

static uint64_t compute_bch2(uint8_t* frame)
{
    return compute_bch(frame,107,132,BCH_12_POLYNOMIAL,BCH_12_POLY_LENGTH);
}

static void set_bit(uint8_t* buf, int bit, bool v)
{
    int byte = bit >> 3;
    int off  = 7 - (bit & 7);

    if(v)
        buf[byte] |= (1 << off);
    else
        buf[byte] &= ~(1 << off);
}

static void push_bits(uint8_t* buf, int& pos, uint64_t v, int n)
{
    for(int i=n-1;i>=0;i--)
        set_bit(buf,pos++, (v>>i)&1);
}

std::string beacon_to_hex_string(const uint8_t* frame, bool start)
{
    static const char hex[] = "0123456789ABCDEF";

    std::string out;
    out.resize(18);

    int offset = start ? 0 : 9;

    for(int i = 0; i < 9; i++)
    {
        uint8_t b = frame[offset + i];

        out[i*2]     = hex[b >> 4];
        out[i*2 + 1] = hex[b & 0x0F];
    }

    return out;
}

size_t generate_beacon(uint8_t* frame, const BeaconParams& params)
{
    memset(frame,0,18);

    int pos = 0;
    uint8_t min, sec;

    /* bit sync */
    for(int i=0;i<15;i++)
        push_bits(frame,pos,1,1);

    /* frame sync (test) */
    push_bits(frame,pos,params.is_test ? 0b011010000 : 0b000101111,9);

    /* ----------------
       PDF-1
       ---------------- */

    int pdf1_start = pos;

    push_bits(frame,pos,1,1);  // format flag (long)
    bool is_user = (params.type == BeaconType::EPIRB_USER || params.type == BeaconType::ELT_USER || params.type == BeaconType::PLB_USER);
    push_bits(frame,pos,is_user,1);  // protocol flag
    push_bits(frame,pos,227,10); // country code example
    switch(params.type)
    {
        case BeaconType::EPIRB_NATIONAL:
            push_bits(frame,pos,0b1010,4);
            break;
        case BeaconType::EPIRB_USER:
            push_bits(frame,pos,0b010,3);
            break;
        case BeaconType::PLB_NATIONAL:
            push_bits(frame,pos,0b1011,4);
            break;
        case BeaconType::PLB_USER:
            push_bits(frame,pos,0b011,3);
            break;
        case BeaconType::ELT_NATIONAL:
            push_bits(frame,pos,0b1000,4);
            break;
        case BeaconType::ELT_USER:
        default:
            push_bits(frame,pos,0b001,3);
            break;
    }

    while(pos < pdf1_start + 61)
        push_bits(frame,pos,0,1);

    if(is_user)
    {
        if(params.type == BeaconType::PLB_NATIONAL || params.type == BeaconType::PLB_USER)
        {
            set_bit(frame,40-1,1);
            set_bit(frame,41-1,1);
            set_bit(frame,42-1,0);
        }

        set_bit(frame,85-1,params.has_121_5);
    }
    else
    {   // National location
        // North / south
        set_bit(frame,59-1,params.location.south);
        // E / W
        set_bit(frame,72-1,params.location.west);
        pos = 60-1;
        // Latitude degrees (7 bits)
        for(int i = 6; i >= 0; i--)
            push_bits(frame, pos, (params.location.lat_deg >> i) & 1,1);
        // Latitude min (5 bits, 2 min increment)
        min = params.location.lat_min/2;
        for(int i = 4; i >= 0; i--)
            push_bits(frame, pos, (min >> i) & 1,1);
        pos = 73-1;
        // Longitude degrees (8 bits)
        for(int i = 7; i >= 0; i--)
            push_bits(frame, pos, (params.location.long_deg >> i) & 1,1);
        // Longiitude min (5 bits, 2 min increment)
        min = params.location.long_min/2;
        for(int i = 4; i >= 0; i--)
            push_bits(frame, pos, (min >> i) & 1,1);
        pos = pdf1_start + 61;
    }

    /* BCH1 */

    uint64_t bch1 = compute_bch1(frame);

    push_bits(frame,pos,bch1,21);

    /* ----------------
       PDF-2 position
       ---------------- */
    int pdf2_start = pos;

    if(is_user)
    {
        push_bits(frame,pos,params.is_internal,1);

        // Latitude N/S
        push_bits(frame, pos, params.location.south,1);

        // Latitude degrees (7 bits)
        for(int i = 6; i >= 0; i--)
            push_bits(frame, pos, (params.location.lat_deg >> i) & 1,1);

        // Latitude minutes (4 bits, 4 minutes precision)
        min = params.location.lat_min/4;
        for(int i = 3; i >= 0; i--)
            push_bits(frame, pos, (min >> i) & 1,1);

        // Longitude E/W
        push_bits(frame, pos, params.location.west,1);

        // Longitude degrees (8 bits)
        for(int i = 7; i >= 0; i--)
            push_bits(frame, pos, (params.location.long_deg >> i) & 1,1);

        // Longitude minutes (4 bits, 4 minutes precision)
        min = params.location.long_min/4;
        for(int i = 3; i >= 0; i--)
            push_bits(frame, pos, (min >> i) & 1,1);
    }
    else
    {   // National location
        push_bits(frame,pos,0b1101,4);
        push_bits(frame,pos,params.is_internal,1);
        push_bits(frame,pos,params.has_121_5,1);
        // Lat
        // +
        push_bits(frame,pos,1,1);
        // Min
        push_bits(frame,pos,(params.location.lat_min % 2),2);
        // Sec (4 bits, 4 sec precision)
        sec = params.location.lat_sec/4;
        for(int i = 3; i >= 0; i--)
            push_bits(frame, pos, (sec >> i) & 1,1);
        // LLon
        // +
        push_bits(frame,pos,1,1);
        // Min
        push_bits(frame,pos,(params.location.long_min % 2),2);
        // Sec (4 bits, 4 sec precision)
        sec = params.location.long_sec/4;
        for(int i = 3; i >= 0; i--)
            push_bits(frame, pos, (sec >> i) & 1,1);
    }

    while(pos < pdf2_start + 26)
        push_bits(frame,pos,0,1);

    /* BCH2 */

    uint64_t bch2 = compute_bch2(frame);

    push_bits(frame,pos,bch2,12);
    return 18;
}

}  // namespace ui::external_app::epirb_tx

#endif /*__BEACON_H__*/