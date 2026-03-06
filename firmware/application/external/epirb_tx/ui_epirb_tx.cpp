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

#include "ui_epirb_tx.hpp"

#include "tonesets.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file_reader.hpp"
#include "file_path.hpp"
#include "binder.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui::external_app::epirb_tx {

void EPIRBTXAppView::focus() {
    options_frame.focus();
}

EPIRBTXAppView::~EPIRBTXAppView() {
    // Restore bpsk fequency
    transmitter_model.set_target_frequency(bpsk_frequency);
    transmitter_model.disable();
    baseband::shutdown();
}

static uint8_t hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static uint8_t hexToByte(char high, char low)
{
    return (hexval(high) << 4) | hexval(low);
}

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

static void maidenhead_to_latlon(const std::string& loc, float& lat, float& lon)
{
    static const double lon_step[] =
    {
        20.0,
        2.0,
        1.0/12.0,
        1.0/120.0,
        1.0/2880.0
    };

    static const double lat_step[] =
    {
        10.0,
        1.0,
        1.0/24.0,
        1.0/240.0,
        1.0/5760.0
    };

    lon = -180.0;
    lat = -90.0;

    int pairs = loc.size() / 2;
    if(pairs > 5)
        pairs = 5;

    for(int i=0;i<pairs;i++)
    {
        char c1 = std::toupper(loc[i*2]);
        char c2 = std::toupper(loc[i*2+1]);

        double lon_size = lon_step[i];
        double lat_size = lat_step[i];

        int v1, v2;

        if(i % 2 == 0) // letters
        {
            v1 = c1 - 'A';
            v2 = c2 - 'A';
        }
        else // digits
        {
            v1 = c1 - '0';
            v2 = c2 - '0';
        }

        lon += v1 * lon_size;
        lat += v2 * lat_size;
    }

    // Cell center
    lon += lon_step[pairs-1] / 2.0;
    lat += lat_step[pairs-1] / 2.0;
}

static uint16_t encode_lat(float lat)
{
    return (uint16_t)(((lat + 90.0f)/180.0f) * 32767);
}

static uint16_t encode_lon(float lon)
{
    return (uint16_t)(((lon + 180.0f)/360.0f) * 65535);
}

static void set_bit(uint8_t* buf, int bit, bool v)
{
/*    if (v)
        buf[bit/8] |=  (1 << (bit%8));
    else
        buf[bit/8] &= ~(1 << (bit%8));*/
    int byte = bit >> 3;
    int off  = 7 - (bit & 7);

    if(v)
        buf[byte] |= (1 << off);
}

static void push_bits(uint8_t* buf, int& pos, uint64_t v, int n)
{
    for(int i=n-1;i>=0;i--)
        set_bit(buf,pos++, (v>>i)&1);
}

std::string EPIRBTXAppView::frame_to_hex_string(bool start)
{
    static const char hex[] = "0123456789ABCDEF";

    std::string out;
    out.resize(18);

    int offset = start ? 0 : 9;

    for(int i = 0; i < 9; i++)
    {
        uint8_t b = epirb_tx_message.data[offset + i];

        out[i*2]     = hex[b >> 4];
        out[i*2 + 1] = hex[b & 0x0F];
    }

    return out;
}

void EPIRBTXAppView::generate_frame(BeaconParams params)
{
    memset(epirb_tx_message.data,0,18);

    int pos = 0;

    /* bit sync */
    for(int i=0;i<15;i++)
        push_bits(epirb_tx_message.data,pos,1,1);

    /* frame sync (test) */
    push_bits(epirb_tx_message.data,pos,params.is_test ? 0b011010000 : 0b000101111,9);

    /* ----------------
       PDF-1
       ---------------- */

    int pdf1_start = pos;

    push_bits(epirb_tx_message.data,pos,1,1);  // format flag (long)
    push_bits(epirb_tx_message.data,pos,1,1);  // protocol flag
    push_bits(epirb_tx_message.data,pos,227,10); // country code example
    switch(params.type)
    {
        case BeaconType::EPIRB:
            push_bits(epirb_tx_message.data,pos,0b010,3);
            break;
        case BeaconType::PLB:
            push_bits(epirb_tx_message.data,pos,0b011,3);
            break;
        case BeaconType::ELT:
        default:
            push_bits(epirb_tx_message.data,pos,0b001,3);
            break;
    }

    while(pos < pdf1_start + 61)
        push_bits(epirb_tx_message.data,pos,0,1);

    if(params.type == BeaconType::PLB)
    {
        set_bit(epirb_tx_message.data,40-1,1);
        set_bit(epirb_tx_message.data,41-1,1);
        set_bit(epirb_tx_message.data,42-1,0);
    }

    set_bit(epirb_tx_message.data,85/*-1*/,params.has_121_5);

    /* BCH1 */

    uint64_t bch1 = compute_bch1(epirb_tx_message.data);

    push_bits(epirb_tx_message.data,pos,bch1,21);

    /* ----------------
       PDF-2 position
       ---------------- */
    push_bits(epirb_tx_message.data,pos,params.is_internal,1);

    float lat,lon;
    maidenhead_to_latlon(params.locator,lat,lon);

    uint16_t lat_e = encode_lat(lat);
    uint16_t lon_e = encode_lon(lon);

    int pdf2_start = pos;

    push_bits(epirb_tx_message.data,pos,lat_e,15);
    push_bits(epirb_tx_message.data,pos,lon_e,11);  // reste des 26 bits

    while(pos < pdf2_start + 26)
        push_bits(epirb_tx_message.data,pos,0,1);

    /* BCH2 */

    uint64_t bch2 = compute_bch2(epirb_tx_message.data);

    push_bits(epirb_tx_message.data,pos,bch2,12);
    epirb_tx_message.data_len = 18;
}


void EPIRBTXAppView::on_timer() {
    if(loop)
    {
        if(checkbox_loop.value())
        {
            auto now = chTimeNow();
            std::string timeout = std::to_string((uint32_t)(field_delay.value() - ((now - last_frame_time)/1000)));
            if(timeout != text_timeout.get())
            {
                text_timeout.set(timeout);
            }
            if(now > (last_frame_time + (field_delay.value()*1000)))
            {
                start_tx();
            }
        }
        else
        {
            loop = false;
        }
    }
}

void EPIRBTXAppView::update_frame() {
    if(mode_file)
    {
        Beacon& beacon = beacons[selected_beacon];
        text_description.set(beacon.description.substr(0,max_text_width_ext));
        text_description_end.set(beacon.description.size()>max_text_width_ext ? "-" + beacon.description.substr(max_text_width_ext,max_text_width_ext+max_text_width_ext-1) : "");
        text_frame.set(beacon.frame.substr(0,18));
        text_frame_end.set(beacon.frame.size()>18 ? beacon.frame.substr(18,36) : "");
        epirb_tx_message.data_len = std::min<size_t>((beacon.frame.size()/2),18);
        for(uint8_t i = 0 ; i < epirb_tx_message.data_len ; i++)
        {
            epirb_tx_message.data[i] = hexToByte(
                beacon.frame[2*i],
                beacon.frame[2*i + 1]);
        }
    }
    else
    {
        generate_frame(beacon_params);
        text_frame.set(frame_to_hex_string(true));
        text_frame_end.set(frame_to_hex_string(false));
    }
    update_config();
}

void EPIRBTXAppView::update_config() {
    if(epirb_tx_message.mode_bpsk)
    {   // Backup bpsk frequency
        bpsk_frequency = transmitter_model.target_frequency();
    }
    else
    {   // Restore bpsk frequency
        transmitter_model.set_target_frequency(bpsk_frequency);
    }
    epirb_tx_message.mode_bpsk = true;
    epirb_tx_message.pre_count = (500 * TONES_SAMPLERATE)/1000; // 500 ms
    epirb_tx_message.post_count = (100 * TONES_SAMPLERATE)/1000; // 100 ms
    baseband::set_epirb_tx_config(epirb_tx_message);
}

void EPIRBTXAppView::set_tx_button_state(bool active)
{
    button_tx.set_text(active ? "START" : "STOP");
    button_tx.set_style(active ? &style_tx_start : &style_tx_stop);
}


void EPIRBTXAppView::start_tx() {
    last_frame_time = chTimeNow();
    update_config();
    loop = checkbox_loop.value();
    transmitter_model.enable();
    tx_view.set_transmitting(true);
    set_tx_button_state(false);
    transmitting = true;
}

void EPIRBTXAppView::stop_tx() {
    loop = false;
    transmitter_model.disable();
    tx_view.set_transmitting(false);
    set_tx_button_state(true);
    transmitting = false;
}

void EPIRBTXAppView::on_tx_progress(const uint32_t progress, const bool done) {
    (void)progress;

    if (done) {
        if(checkbox_am.value())
        {   // BPSK frame sent, switch back to 121.5 signal
            epirb_tx_message.mode_bpsk = false;
            // Backup bpsk frequency 
            bpsk_frequency = transmitter_model.target_frequency();
            transmitter_model.set_target_frequency(am_frequency);
            baseband::set_epirb_tx_config(epirb_tx_message);
        }
        else
        {
            transmitter_model.disable();
            tx_view.set_transmitting(false);
            if(!loop)
            {
                set_tx_button_state(true);
                transmitting = false;  
            }
        }
    }
}

EPIRBTXAppView::EPIRBTXAppView(
    NavigationView& nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&labels,
                  &options_mode,
                  &text_beacon,
                  &text_description_label,
                  &text_beacon_type,
                  &options_beacon_type,
                  &text_beacon_locator,
                  &text_field_beacon_locator,
                  &options_frame,
                  &text_description,
                  &text_description_end,
                  &text_frame,
                  &text_frame_end,
                  &text_timeout,
                  &checkbox_loop,
                  &field_delay,
                  &button_tx,
                  &checkbox_am,
                  &field_am_frequency,
                  &tx_view});

    text_beacon.set_style(Theme::getInstance()->fg_light);
    text_description_label.set_style(Theme::getInstance()->fg_light);
    text_beacon_type.set_style(Theme::getInstance()->fg_light);
    text_beacon_locator.set_style(Theme::getInstance()->fg_light);

    options_mode.on_change = [this](size_t index, OptionsField::value_t) {
        mode_file = (index == 0);
        text_beacon.hidden(!mode_file);
        text_description_label.hidden(!mode_file);
        options_frame.hidden(!mode_file);
        text_description.hidden(!mode_file);
        text_description_end.hidden(!mode_file);
        text_beacon_type.hidden(mode_file);
        text_beacon_locator.hidden(mode_file);
        options_beacon_type.hidden(mode_file);
        text_field_beacon_locator.hidden(mode_file);
        update_frame();
        set_dirty();
    };
    // Default to file mode
    options_mode.set_by_value(mode_file ? 0 : 1);

    options_beacon_type.on_change = [this](size_t index, OptionsField::value_t) {
        beacon_params.type = (BeaconType)index;
        update_frame();
        set_dirty();
    };

    text_field_beacon_locator.on_change = [this](TextField&) {
        beacon_params.locator = text_field_beacon_locator.get_text();
        update_frame();
        set_dirty();
    };

    std::string locator = text_field_beacon_locator.get_text();
    bind(text_field_beacon_locator, locator, nav);

    field_am_frequency.set_value(am_frequency);
    field_am_frequency.on_change = [this](rf::Frequency freq) {
        am_frequency = freq;
    };

    load_beacons();  // Load available beacons from TXT files (or default).

    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t entries;

    for (const auto& beacon : beacons)
        entries.emplace_back(beacon.title, entries.size());

    options_frame.set_options(std::move(entries));
    options_frame.on_change = [this](size_t index, OptionsField::value_t) {
        selected_beacon = index;
        update_frame();
        set_dirty();
    };
    options_frame.set_selected_index(selected_beacon);
    update_frame();

    field_delay.set_value(30);
    checkbox_loop.set_value(true);

    checkbox_am.on_select = [this](Checkbox&, bool v) {
        beacon_params.has_121_5 = v;
        if(!mode_file) update_frame();
    };

    // AM frequency field edit
    field_am_frequency.on_edit = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(field_am_frequency.value());
        new_view->on_changed = [this](rf::Frequency f) {
            field_am_frequency.set_value(f);
            update_config();
        };
    };


    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        start_tx();
    };

    tx_view.on_stop = [this]() {
        stop_tx();
    };

    button_tx.on_select = [this](Button&) {
        if (!transmitting)
            start_tx();
        else
            stop_tx();
    };    
}

void EPIRBTXAppView::load_beacons() {
    File beacons_file;
    auto error = beacons_file.open(epirb_dir / u"BEACONS.TXT");
    beacons.clear();

    if (!error) {
        auto reader = FileLineReader(beacons_file);
        for (const auto& line : reader) {
            if (line.length() == 0 || line[0] == '#')
                continue;

            auto cols = split_string(line, ';');
            if (cols.size() != 3)
                continue;

            Beacon beacon{};
            beacon.title = trim(cols[0]);
            beacon.description = trim(cols[1]);
            // Make sure frame is not longer tha 18 bytes / 36 hex character
            beacon.frame = trim(cols[2]).substr(0,36);
            size_t size = beacon.frame.size();
            if (size <= 0)
                continue;  // Invalid line.
            beacons.emplace_back(std::move(beacon));
        }
    }
    if(beacons.empty())
    {   // No beacons file or empty flile: just add default beacon
        beacons.push_back(default_beacon);
    }
}


}  // namespace ui::external_app::epirb_tx