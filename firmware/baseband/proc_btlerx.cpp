/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
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

#include "proc_btlerx.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

void BTLERxProcessor::execute(const buffer_c8_t& buffer) 
{
    if (!configured) return;

    //4Mhz 2048 samples

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);

    feed_channel_stats(decim_0_out);

    static const uint8_t SAMPLE_PER_SYMBOL = 4;
    static const uint8_t LEN_DEMOD_BUF_ACCESS = 32;
    static const uint32_t DEFAULT_ACCESS_ADDR = 0x8E89BED6;
    static const uint32_t NUM_ACCESS_ADDR_BYTE = 4;

    int i, sp, j = 0;
    int I0, Q0, I1, Q1 = 0;
    int k, p, phase_idx = 0;
    int num_demod_byte = 0;

    bool unequal_flag;

    const int demod_buf_len = LEN_DEMOD_BUF_ACCESS; //For AA
    int demod_buf_offset = 0;
    int num_symbol_left = buffer.count / SAMPLE_PER_SYMBOL; //One buffer sample consist of I and Q.
    int symbols_eaten = 0;
    int hit_idx = (-1);

    static uint8_t demod_buf_access[SAMPLE_PER_SYMBOL][LEN_DEMOD_BUF_ACCESS];

    uint32_t uint32_tmp = DEFAULT_ACCESS_ADDR;
    uint8_t accessAddrBits[LEN_DEMOD_BUF_ACCESS];

    uint32_t accesssAddress = 0;

    for(i = 0; i < 32; i++) 
    {
        accessAddrBits[i] = 0x01 & uint32_tmp;
        uint32_tmp = (uint32_tmp >> 1);
    }
    
    memset(demod_buf_access, 0, SAMPLE_PER_SYMBOL * demod_buf_len);

    for (i = 0; i < num_symbol_left * SAMPLE_PER_SYMBOL; i += SAMPLE_PER_SYMBOL) 
    {
        sp = ((demod_buf_offset - demod_buf_len + 1) & (demod_buf_len - 1));
    
        for (j = 0; j < SAMPLE_PER_SYMBOL; j++) 
        {
            //Sample and compare with the adjascent next sample.
            I0 = buffer.p[i+j].real();
            Q0 = buffer.p[i+j].imag();
            I1 = buffer.p[i+j + 1].real();
            Q1 = buffer.p[i+j + 1].imag();
            
            phase_idx = j;

            demod_buf_access[phase_idx][demod_buf_offset] = (I0 * Q1 - I1 * Q0) > 0 ? 1: 0;
            
            k = sp;
            unequal_flag = false;

            accesssAddress = 0;
            
            for (p = 0; p < demod_buf_len; p++) 
            {
                if (demod_buf_access[phase_idx][k] != accessAddrBits[p]) 
                {
                    unequal_flag = true;
                    hit_idx = (-1);
                    break;
                }

                accesssAddress = (accesssAddress & (~(1 << p))) | (demod_buf_access[phase_idx][k] << p);

                k = ((k + 1) & (demod_buf_len - 1));
            }
            
            if(unequal_flag == false) 
            {
                hit_idx = (i + j - (demod_buf_len - 1) * SAMPLE_PER_SYMBOL);
                break;
            }
        }

        if (unequal_flag == false)
        {
            break;
        }

        demod_buf_offset = ((demod_buf_offset + 1) & (demod_buf_len - 1));
    }

    if (hit_idx == -1)
    {
        //Process more samples.
        return;
    }

    if ((accesssAddress & DEFAULT_ACCESS_ADDR) == DEFAULT_ACCESS_ADDR)
    {
        //Send AA as test.
        data_message.is_data = false;
        data_message.value = 'A';
        shared_memory.application_queue.push(data_message);

        data_message.is_data = true;
        data_message.value = accesssAddress;
        shared_memory.application_queue.push(data_message);    
    }

    symbols_eaten += hit_idx;

    symbols_eaten += (8 * NUM_ACCESS_ADDR_BYTE * SAMPLE_PER_SYMBOL); // move to beginning of PDU header
    
    num_symbol_left = num_symbol_left - symbols_eaten;

    num_demod_byte = 2; // PDU header has 2 octets
      
    symbols_eaten += 8 * num_demod_byte * SAMPLE_PER_SYMBOL;

    if (symbols_eaten > (int)buffer.count) 
    {
        return;
    }

    //Demod the PDU Header
    uint8_t bit_decision;
    int sample_idx = symbols_eaten - num_demod_byte;

     for (i = 0; i < num_demod_byte ; i++) 
     {
        rb_buf[i] = 0;

        for (j = 0; j < 8; j++) 
        {
            I0 = buffer.p[sample_idx].real();
            Q0 = buffer.p[sample_idx].imag();
            I1 = buffer.p[sample_idx + 1].real();
            Q1 = buffer.p[sample_idx + 1].imag();

            bit_decision = (I0 * Q1 - I1 * Q0) > 0 ? 1 : 0;
            rb_buf[i] = rb_buf[i] | (bit_decision << j);

            sample_idx += SAMPLE_PER_SYMBOL;;
        }
     }

    //Scramble Bytes
    for (i = 0; i < num_demod_byte; i++)
    {
        rb_buf[i] = rb_buf[i] ^ scramble_table[channel_number][i];
    }

    uint8_t pdu_type = (ADV_PDU_TYPE)(rb_buf[0] & 0x0F);
    uint8_t tx_add = ((rb_buf[0] & 0x40) != 0);
    uint8_t rx_add = ((rb_buf[0] & 0x80) != 0);
    uint8_t payload_len = (rb_buf[1] & 0x3F);

    //Send PDU Header as test.
    data_message.is_data = false;
    data_message.value = 'T';
    shared_memory.application_queue.push(data_message);

    data_message.is_data = true;
    data_message.value = pdu_type;
    shared_memory.application_queue.push(data_message);   
    
    data_message.is_data = false;
    data_message.value = 'S';
    shared_memory.application_queue.push(data_message);

    data_message.is_data = true;
    data_message.value = payload_len;
    shared_memory.application_queue.push(data_message);    
}

void BTLERxProcessor::on_message(const Message* const message) 
{
    if (message->id == Message::ID::BTLERxConfigure)
        configure(*reinterpret_cast<const BTLERxConfigureMessage*>(message));
}

void BTLERxProcessor::configure(const BTLERxConfigureMessage& message) 
{
    (void)message;  // avoid warning
    decim_0.configure(taps_200k_decim_0.taps);
    decim_1.configure(taps_200k_decim_1.taps);
    demod.configure(48000, 5000);

    configured = true;
}

int main() 
{
    EventDispatcher event_dispatcher{std::make_unique<BTLERxProcessor>()};
    event_dispatcher.run();
    return 0;
}
