/*
 * Copyright (C) 1996 Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 * Copyright (C) 2012-2014 Elias Oenal (multimon-ng@eliasoenal.com)
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

#ifndef __PROC_POCSAG_H__
#define __PROC_POCSAG_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "pocsag_packet.hpp"

#include "pocsag.hpp"
#include "message.hpp"
#include "audio_output.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <bitset> 
using namespace std;

// Class used to smooth demodulated waveform prior to decoding
// -----------------------------------------------------------
template <class ValType, class CalcType>
class SmoothVals
{
protected:
	ValType *	m_lastVals;	// Previoius N values
	int			m_size;		// The size N
	CalcType	m_sumVal;	// Running sum of lastVals
	int			m_pos;		// Current position in last vals ring buffer
	int			m_count;    //

public:
	SmoothVals() : m_lastVals(NULL), m_size(1), m_sumVal(0), m_pos(0), m_count(0)
	{
		m_lastVals = new ValType[m_size];
	}

	// --------------------------------------------------
	// --------------------------------------------------
	virtual ~SmoothVals()
	{
		delete[] m_lastVals;
	}

    SmoothVals(const SmoothVals<float, float>&)
    {

    }

    SmoothVals & operator=(const SmoothVals<float, float>&)
    {
        return *this ;
    }

	// --------------------------------------------------
	// Set size of smoothing
	// --------------------------------------------------
	void SetSize(int size)
	{
		m_size = std::max(size, 1);
		m_pos = 0;
		delete[] m_lastVals;
		m_lastVals = new ValType[m_size];
		m_sumVal = 0;
	}

	// --------------------------------------------------
	// Get size of smoothing
	// --------------------------------------------------
	int Size() { return m_size; }

	// --------------------------------------------------
	// In place processing
	// --------------------------------------------------
	void Process(ValType * valBuff, int numVals)
	{
		ValType tmpVal;

		if (m_count > (1024*10))
		{
			// Recalculate the sum value occasionaly, stops accumulated errors when using float
			m_count = 0;
			m_sumVal = 0;
			for (int i = 0; i < m_size; ++i) { m_sumVal += (CalcType)m_lastVals[i]; }
		}

		// Use a rolling smoothed value while processing the buffer
		for (int buffPos = 0; buffPos < numVals; ++buffPos)
		{
			m_pos = (m_pos + 1);								// Increment the position in the stored values
			if (m_pos >= m_size) { m_pos = 0; }					// loop if reached the end of the stored values 

			m_sumVal -= (CalcType)m_lastVals[m_pos];			// Subtract the oldest value
			m_lastVals[m_pos] = valBuff[buffPos];				// Store the new value
			m_sumVal += (CalcType)m_lastVals[m_pos];			// Add on the new value

			tmpVal = (ValType)(m_sumVal / m_size);			    // Scale by number of values smoothed
			valBuff[buffPos] = tmpVal;
		}

		m_count += numVals;
	}
};



// --------------------------------------------------
// Class to process base band data to pocsag frames
// --------------------------------------------------
class POCSAGProcessor : public BasebandProcessor{
public:

	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const message) override;

	int OnDataFrame(int len, int baud);
	int OnDataWord(uint32_t word, int pos);

private:
	static constexpr size_t baseband_fs = 3072000;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };
	
	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	std::array<float, 32> audio { };
	const buffer_f32_t audio_buffer {
		audio.data(),
		audio.size()
	};

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::decimate::FIRAndDecimateComplex channel_filter { };
	dsp::demodulate::FM demod { };
	SmoothVals<float, float> smooth = { };
	
	AudioOutput audio_output { };

	bool configured = false;
	pocsag::POCSAGPacket packet { };

	void configure();

	// ----------------------------------------
	// Frame extractraction methods and members
	// ----------------------------------------
private:
	void initFrameExtraction();
	struct FIFOStruct {
		unsigned long	codeword;
		int				numBits;
	};

	#define BIT_BUF_SIZE (64)

	void resetVals();
	void setFrameExtractParams(long a_samplesPerSec, long a_maxBaud = 8000, long a_minBaud = 200, long maxRunOfSameValue = 32);

	int	 processDemodulatedSamples(float * sampleBuff, int noOfSamples);
	int  extractFrames();

	void	storeBit();
	short	getBit();

	int		getNoOfBits();
	uint32_t getRate();

	uint32_t	m_averageSymbolLen_1024{0};
	uint32_t	m_lastStableSymbolLen_1024{0};

	uint32_t m_samplesPerSec{0};
	uint32_t m_goodTransitions{0};
	uint32_t m_badTransitions{0};

	uint32_t m_sampleNo{0};
	float  m_sample{0};
	float  m_valMid{0.0f};
	float  m_lastSample{0.0f};

	uint32_t  m_lastTransPos_1024{0};
	uint32_t  m_lastSingleBitPos_1024{0};

	uint32_t m_nextBitPosInt{0}; // Integer rounded up version to save on ops
	uint32_t m_nextBitPos_1024{0};
	uint32_t m_lastBitPos_1024{0};

	uint32_t m_shortestGoodTrans_1024{0};
	uint32_t m_minSymSamples_1024{0};
	uint32_t m_maxSymSamples_1024{0};
	uint32_t m_maxRunOfSameValue{0};

	bitset<(size_t)BIT_BUF_SIZE> m_bits{0};
	long		m_bitsStart{0};
	long		m_bitsEnd{0};

	FIFOStruct		m_fifo{0,0};
	bool			m_gotSync{false};
	int				m_numCode{0};
	bool			m_inverted{false};
	
};

#endif/*__PROC_POCSAG_H__*/
