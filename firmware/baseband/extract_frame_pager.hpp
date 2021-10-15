/*
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

#ifndef __EXTRACT_FRAME_PAGER_H__
#define __EXTRACT_FRAME_PAGER_H__

#include <bitset> 
using namespace std;

#define MAX_CODEWORDS (16)
class extract_frame_pager
{
public:
	struct FIFOStruct {
		unsigned long	codeword;
		int				numBits;
	};

#define BIT_BUF_SIZE (64)
	enum BitsState { BS_SEARCHING, BS_SYNC, BS_LOCKED };
	enum DetectionMode { DM_STATIC, DM_DYNAMIC, DM_DIFFERENTIAL };

	extract_frame_pager();
	virtual ~extract_frame_pager();


	void resetVals();
	void setParams(long a_samplesPerSec, long a_maxBaud = 8000, long a_minBaud = 200, long maxRunOfSameValue = 32);

	int	 processDemodulatedSamples(float * sampleBuff, int noOfSamples);
	int  extractFrames();
	virtual int OnDataFrame(int len, int baud) = 0;
	virtual int OnDataWord(uint32_t word, int pos) = 0;

	void	init();
	void	storeBit();
	short	getBit();

	int		getNoOfBits();
	uint32_t getRate();

protected:
	uint32_t	m_averageSymbolLen_1024;
	uint32_t	m_lastStableSymbolLen_1024;

	uint32_t m_samplesPerSec;
	uint32_t m_goodTransitions;
	uint32_t m_badTransitions;

private:
	uint32_t m_sampleNo;
	float  m_sample;
	float  m_valMid;
	float  m_lastSample;

	BitsState	m_state;
	uint32_t  m_lastTransPos_1024;
	uint32_t  m_lastSingleBitPos_1024;

	uint32_t m_nextBitPosInt; // Integer rounded up version to save on ops
	uint32_t m_nextBitPos_1024;
	uint32_t m_lastBitPos_1024;

	uint32_t m_shortestGoodTrans_1024;
	uint32_t m_minSymSamples_1024;
	uint32_t m_maxSymSamples_1024;
	uint32_t m_maxRunOfSameValue;

	bitset<(size_t)BIT_BUF_SIZE> m_bits;
	long		m_bitsStart;
	long		m_bitsEnd;
	DetectionMode	m_detectionMode;

	FIFOStruct		m_fifo;
	bool			m_gotSync;
	int				m_numCode;
	bool			m_inverted;
};

#endif/*__EXTRACT_FRAME_PAGER_H__*/
