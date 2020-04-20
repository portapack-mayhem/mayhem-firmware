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

#include "proc_nrfrx.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

void NRFRxProcessor::execute(const buffer_c8_t& buffer) {
	if (!configured) return;
	
	// FM demodulation

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	feed_channel_stats(decim_0_out);
	
	auto audio_oversampled = demod.execute(decim_0_out, work_audio_buffer);
	// Audio signal processing
	for (size_t c = 0; c < audio_oversampled.count; c++) {
		int result;
                int g_srate = 4; //4 for 250KPS
		//int g_srate = 1; //1 for 1MPS, not working yet
		int32_t current_sample = audio_oversampled.p[c]; //if I directly use this, some results can pass crc but not correct.
                rb_head++;
	        rb_head=(rb_head)%RB_SIZE;

                rb_buf[rb_head] = current_sample;

		skipSamples = skipSamples - 1;


		if (skipSamples<1)
		{
				

			int32_t threshold_tmp=0;
			for (int c=0;c<8*g_srate;c++)
			{
				threshold_tmp = threshold_tmp + (int32_t)rb_buf[(rb_head+c)%RB_SIZE];
			}

			g_threshold = (int32_t)threshold_tmp/(8*g_srate);
	
			int transitions=0;
			if (rb_buf[(rb_head + 9*g_srate)%RB_SIZE] > g_threshold)
			{
				for (int c=0;c<8;c++)
				{
					if (rb_buf[(rb_head + c*g_srate)%RB_SIZE] > rb_buf[(rb_head + (c+1)*g_srate)%RB_SIZE])
						transitions = transitions + 1;
				}
			}
			else
			{
				for (int c=0;c<8;c++)
				{
					if (rb_buf[(rb_head + c*g_srate)%RB_SIZE] < rb_buf[(rb_head + (c+1)*g_srate)%RB_SIZE])
						transitions = transitions + 1;
				}
			}
			
			bool packet_detected=false;
			//if ( transitions==4 && abs(g_threshold)<15500)
			if ( transitions==4 && abs(g_threshold)<15500)
			{
				int packet_length = 0;
				uint8_t tmp_buf[10];
				uint8_t packet_data[500];
				uint8_t packet_packed[50];
				uint16_t pcf;
				uint32_t packet_crc;
				uint32_t calced_crc;
				uint64_t packet_addr_l;

				/* extract address */
				packet_addr_l=0;

				for (int t=0;t<5;t++)
				{
					bool current_bit;
					uint8_t byte=0;
					for (int c=0;c<8;c++) 
					{
						if (rb_buf[(rb_head+(1*8+t*8+c)*g_srate)%RB_SIZE] > g_threshold)
							current_bit = true;
						else
							current_bit = false;
						byte |= current_bit << (7-c);
					}
					tmp_buf[t]=byte;
				}

				for (int t=0;t<5;t++) packet_addr_l|=((uint64_t)tmp_buf[t])<<(4-t)*8;

				//channel_number = 26;

				
				/* extract pcf */
				for (int t=0;t<2;t++)
				{
					bool current_bit;
					uint8_t byte=0;
					for (int c=0;c<8;c++) 
					{
						if (rb_buf[(rb_head+(6*8+t*8+c)*g_srate)%RB_SIZE] > g_threshold)
							current_bit = true;
						else
							current_bit = false;
						byte |= current_bit << (7-c);
					}
					tmp_buf[t]=byte;
				}

				pcf = tmp_buf[0]<<8 | tmp_buf[1];
				pcf >>=7;

				/* extract packet length, avoid excessive length packets */
				if(packet_length == 0)
					packet_length=(int)pcf>>3;
				if (packet_length>32) 
					packet_detected = false;

				/* extract data */
				for (int t=0;t<packet_length;t++)
				{
					bool current_bit;
					uint8_t byte=0;
					for (int c=0;c<8;c++) 
					{
						if (rb_buf[(rb_head+(6*8+9+t*8+c)*g_srate)%RB_SIZE] > g_threshold)
							current_bit = true;
						else
							current_bit = false;
						byte |= current_bit << (7-c);
					}
					packet_data[t]=byte;
				}

				/* Prepare packed bit stream for CRC calculation */
				uint64_t packet_header=packet_addr_l;
				packet_header<<=9;
				packet_header|=pcf;
				for (int c=0;c<7;c++){
					packet_packed[c]=(packet_header>>((6-c)*8))&0xFF;
				}

				for (int c=0;c<packet_length;c++){
					packet_packed[c+7]=packet_data[c];
				}

				/* calculate packet crc */
				const uint8_t* data = packet_packed; 
				size_t data_len =  7+packet_length;
				bool bit;
				uint8_t cc;
				uint_fast16_t crc=0x3C18;
				while (data_len--) {
				cc = *data++;
				for (uint8_t i = 0x80; i > 0; i >>= 1) 
				{
					bit = crc & 0x8000;
					if (cc & i) 
					{
						bit = !bit;
					}
					crc <<= 1;
					if (bit) 
					{
						crc ^= 0x1021;
					}
				}
					crc &= 0xffff;
				}
				calced_crc = (uint16_t)(crc & 0xffff);

				/* extract crc */
				for (int t=0;t<2;t++)
				{
					bool current_bit;
					uint8_t byte=0;
					for (int c=0;c<8;c++) 
					{
						if (rb_buf[(rb_head+((6+packet_length)*8+9+t*8+c)*g_srate)%RB_SIZE] > g_threshold)
							current_bit = true;
						else
							current_bit = false;
						byte |= current_bit << (7-c);
					}
					tmp_buf[t]=byte;
				}
				packet_crc = tmp_buf[0]<<8 | tmp_buf[1];

				/* NRF24L01+ packet found, dump information */
				//if (packet_addr_l==0xE7E7E7E7)
				if (packet_crc==calced_crc)
				{
					data_message.is_data = false;
					data_message.value = 'A';
					shared_memory.application_queue.push(data_message);

					data_message.is_data = true;
					data_message.value = packet_addr_l;
					shared_memory.application_queue.push(data_message);

					for (int c=0;c<7;c++)
					{
						data_message.is_data = true;
						data_message.value = packet_addr_l >> 8;
						shared_memory.application_queue.push(data_message);
					}
					/*data_message.is_data = true;
					data_message.value = packet_addr_l;
					shared_memory.application_queue.push(data_message);

					data_message.is_data = true;
					data_message.value = packet_addr_l >> 8;
					shared_memory.application_queue.push(data_message);*/

					data_message.is_data = false;
					data_message.value = 'B';
					shared_memory.application_queue.push(data_message);

					for (int c=0;c<packet_length;c++)
					{
						data_message.is_data = true;
						data_message.value = packet_data[c];
						shared_memory.application_queue.push(data_message);
					}

					data_message.is_data = false;
					data_message.value = 'C';
					shared_memory.application_queue.push(data_message);


					packet_detected = true;
				}
				else
					packet_detected = false;
			}

			if (packet_detected) 
			{
				skipSamples=20;
			}
		}
	}
}

void NRFRxProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::NRFRxConfigure)
		configure(*reinterpret_cast<const NRFRxConfigureMessage*>(message));
}

void NRFRxProcessor::configure(const NRFRxConfigureMessage& message) {	
	decim_0.configure(taps_200k_wfm_decim_0.taps, 33554432);
	decim_1.configure(taps_200k_wfm_decim_1.taps, 131072);
	demod.configure(audio_fs, 5000);

	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<NRFRxProcessor>() };
	event_dispatcher.run();
	return 0;
}
