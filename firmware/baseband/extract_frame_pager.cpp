#include "extract_frame_pager.hpp"
#include <algorithm>    // std::max
#include <cmath>

#define BAUD_STABLE (104)
#define MAX_CONSEC_SAME (32)
#define MAX_WITHOUT_SINGLE (64)
#define MAX_BAD_TRANS (10)

#define M_SYNC				(0x7cd215d8)
#define M_NOTSYNC			(0x832dea27)

#define M_IDLE				(0x7a89c197)

// ====================================================================
//
// ====================================================================
inline int bitsDiff(unsigned long left, unsigned long right)
{
	unsigned long xord = left ^ right;
	int count = 0;
	for (int i = 0; i < 32; i++)
	{
		if ((xord & 0x01) != 0) ++count;
		xord = xord >> 1;
	}
	return(count);

}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extract_frame_pager::extract_frame_pager()
{
	init();
}

// ====================================================================
//
// ====================================================================
extract_frame_pager::~extract_frame_pager()
{
}

// ====================================================================
//
// ====================================================================
void extract_frame_pager::init()
{
	m_averageSymbolLen_1024 = m_maxSymSamples_1024;
	m_lastStableSymbolLen_1024 = m_minSymSamples_1024;

	m_badTransitions = 0;
	m_bitsStart = 0;
	m_bitsEnd = 0;
	m_inverted = false;

	resetVals();
}

// ====================================================================
//
// ====================================================================
void extract_frame_pager::resetVals()
{
	// Set up the xtraction mode
	m_detectionMode = DM_DYNAMIC;

	// Reset the parameters
	// --------------------
	m_goodTransitions = 0;
	m_badTransitions = 0;
	m_averageSymbolLen_1024 = m_maxSymSamples_1024;
	m_shortestGoodTrans_1024 = m_maxSymSamples_1024;
	m_valMid = 0;

	// And reset the counts
	// --------------------
	m_lastTransPos_1024 = 0;
	m_lastBitPos_1024 = 0;
	m_lastSample = 0;
	m_sampleNo = 0;
	m_nextBitPos_1024 = m_maxSymSamples_1024;
	m_nextBitPosInt = (long)m_nextBitPos_1024;

	// Extraction
	m_fifo.numBits = 0;
	m_gotSync = false;
	m_numCode = 0;
}

// ====================================================================
//
// ====================================================================
void extract_frame_pager::setParams(long a_samplesPerSec, long a_maxBaud, long a_minBaud, long maxRunOfSameValue)
{

	m_samplesPerSec = a_samplesPerSec;
	m_minSymSamples_1024 = (uint32_t)(1024.0f * (float)a_samplesPerSec / (float)a_maxBaud);
	m_maxSymSamples_1024 = (uint32_t)(1024.0f*(float)a_samplesPerSec / (float)a_minBaud);
	m_maxRunOfSameValue = maxRunOfSameValue;

	m_shortestGoodTrans_1024 = m_maxSymSamples_1024;
	m_averageSymbolLen_1024 = m_maxSymSamples_1024;
	m_lastStableSymbolLen_1024 = m_minSymSamples_1024;

	m_nextBitPos_1024 = m_averageSymbolLen_1024 / 2;
	m_nextBitPosInt = m_nextBitPos_1024 >> 10;

	init();
}

// ====================================================================
//
// ====================================================================
int extract_frame_pager::processDemodulatedSamples(float * sampleBuff, int noOfSamples)
{
	bool transition = false;
	uint32_t samplePos_1024 = 0;
	uint32_t len_1024 = 0;

	// Loop through the block of data
	// ------------------------------
	for (int pos = 0; pos < noOfSamples; ++pos)
	{
		m_sample = sampleBuff[pos];
		m_valMid += (m_sample - m_valMid) / 1024.0f;

		++m_sampleNo;

		// Detect Transition
		// -----------------
		transition = ! ((m_lastSample < m_valMid) ^ (m_sample >= m_valMid)); // use XOR for speed

		// If this is a transition
		// -----------------------
		if (transition)
		{
			// Calculate samples since last trans
			// ----------------------------------
			int32_t fractional_1024 = (int32_t)(((m_sample - m_valMid)*1024) / (m_sample - m_lastSample));
			if (fractional_1024 < 0) { fractional_1024 = -fractional_1024; }

			samplePos_1024 = (m_sampleNo<<10)-fractional_1024;
			len_1024 = samplePos_1024 - m_lastTransPos_1024;
			m_lastTransPos_1024 = samplePos_1024;

			// If symbol is large enough to be valid
			// -------------------------------------
			if (len_1024 > m_minSymSamples_1024)
			{
				// Check for shortest good transition
				// ----------------------------------
				if ((len_1024 < m_shortestGoodTrans_1024) &&
					(m_goodTransitions < BAUD_STABLE))	// detect change of symbol size
				{
					int32_t fractionOfShortest_1024 = (len_1024<<10) / m_shortestGoodTrans_1024;

					// If currently at half the baud rate
					// ----------------------------------
					if ((fractionOfShortest_1024 > 410) && (fractionOfShortest_1024 < 614)) // 0.4 and 0.6
					{
						m_averageSymbolLen_1024 /= 2;
						m_shortestGoodTrans_1024 = len_1024;
					}
					// If currently at the wrong baud rate
					// -----------------------------------
					else if (fractionOfShortest_1024 < 768) // 0.75
					{
						m_averageSymbolLen_1024 = len_1024;
						m_shortestGoodTrans_1024 = len_1024;
						m_goodTransitions = 0;
						m_lastSingleBitPos_1024 = samplePos_1024 - len_1024;
					}
				}

				// Calc the number of bits since events
				// ------------------------------------
				int32_t halfSymbol_1024 = m_averageSymbolLen_1024 / 2;
				int bitsSinceLastTrans = max((uint32_t)1, (len_1024+halfSymbol_1024) / m_averageSymbolLen_1024 );
				int bitsSinceLastSingle = (((m_sampleNo<<10)-m_lastSingleBitPos_1024) + halfSymbol_1024) / m_averageSymbolLen_1024;

				// Check for single bit
				// --------------------
				if (bitsSinceLastTrans == 1)
				{
					m_lastSingleBitPos_1024 = samplePos_1024;
				}

				// If too long since last transition
				// ---------------------------------
				if (bitsSinceLastTrans > MAX_CONSEC_SAME)
				{
					resetVals();
				}
				// If too long sice last single bit
				// --------------------------------
				else if (bitsSinceLastSingle > MAX_WITHOUT_SINGLE)
				{
					resetVals();
				}
				else
				{
					// If this is a good transition
					// ----------------------------
					int32_t offsetFromExtectedTransition_1024 = len_1024 - (bitsSinceLastTrans*m_averageSymbolLen_1024);
					if (offsetFromExtectedTransition_1024 < 0) { offsetFromExtectedTransition_1024 = -offsetFromExtectedTransition_1024; }
					if (offsetFromExtectedTransition_1024 < (m_averageSymbolLen_1024 / 4)) // Has to be within 1/4 of symbol to be good
					{
						++m_goodTransitions;
						uint32_t bitsCount = min((uint32_t)BAUD_STABLE, m_goodTransitions);

						uint32_t propFromPrevious = m_averageSymbolLen_1024*bitsCount;
						uint32_t propFromCurrent = (len_1024 / bitsSinceLastTrans);
						m_averageSymbolLen_1024 = (propFromPrevious + propFromCurrent) / (bitsCount + 1);
						m_badTransitions = 0;
						//if ( len < m_shortestGoodTrans ){m_shortestGoodTrans = len;}
						// Store the old symbol size
						if (m_goodTransitions >= BAUD_STABLE) 
						{ 
							m_lastStableSymbolLen_1024 = m_averageSymbolLen_1024; 
						}
					}
					// Not a good transition
					// ---------------------
					else
					{
						//	m_goodTransitions = 0;
					}
				}

				// Set the point of the last bit if not yet stable
				// -----------------------------------------------
				if ((m_goodTransitions < BAUD_STABLE) || (m_badTransitions > 0))
				{
					m_lastBitPos_1024 = samplePos_1024 - (m_averageSymbolLen_1024 / 2);
				}

				// Calculate the exact positiom of the next bit
				// --------------------------------------------
				int32_t thisPlusHalfsymbol_1024 = samplePos_1024 + (m_averageSymbolLen_1024/2);
				int32_t lastPlusSymbol = m_lastBitPos_1024 + m_averageSymbolLen_1024;
				m_nextBitPos_1024 = lastPlusSymbol + ((thisPlusHalfsymbol_1024 - lastPlusSymbol) / 16);

				// Check for bad pos error
				// -----------------------
				if (m_nextBitPos_1024 < samplePos_1024) m_nextBitPos_1024 += m_averageSymbolLen_1024;

				// Calculate integer sample after next bit
				// ---------------------------------------
				m_nextBitPosInt = (m_nextBitPos_1024>>10) + 1;

			} // symbol is large enough to be valid
			else
			{
				// Bad transition, so reset the counts
				// -----------------------------------
				++m_badTransitions;
				if (m_badTransitions > MAX_BAD_TRANS)
				{
					resetVals();
				}
			}
		}  // end of if transition
		else
		{
			//TRACE("Len too small %f",len);
		}

		// Reached the point of the next bit
		// ---------------------------------
		if (m_sampleNo >= m_nextBitPosInt)
		{
			// Everything is good so extract a bit
			// -----------------------------------
			if (m_goodTransitions > 20)
			{
				// Store value at the center of bit
				// --------------------------------
				storeBit();
			}
			// Check for long 1 or zero
			// ------------------------
			int bitsSinceLastTrans = ((m_sampleNo<<10) - m_lastTransPos_1024) / m_averageSymbolLen_1024;
			if (bitsSinceLastTrans > m_maxRunOfSameValue) 
			{ 
				resetVals(); 
			}

			// Store the point of the last bit
			// -------------------------------
			m_lastBitPos_1024 = m_nextBitPos_1024;

			// Calculate the exact point of the next bit
			// -----------------------------------------
			m_nextBitPos_1024 += m_averageSymbolLen_1024;

			// Look for the bit after the next bit pos
			// ---------------------------------------
			m_nextBitPosInt = (m_nextBitPos_1024>>10) + 1;

		} // Reached the point of the next bit

		m_lastSample = m_sample;

	} // Loop through the block of data

	return getNoOfBits();
}

// ====================================================================
//
// ====================================================================
void extract_frame_pager::storeBit()
{
	if (++m_bitsStart >= BIT_BUF_SIZE) { m_bitsStart = 0; }

	// Calculate the bit value
	float sample = (m_sample + m_lastSample) / 2;
	//int32_t sample_1024 = m_sample_1024;
	bool bit = sample > m_valMid;

	// If buffer not full
	if (m_bitsStart != m_bitsEnd)
	{
		// Decide on output val
		if (bit)
		{
			m_bits[m_bitsStart] = 0;
		}
		else
		{
			m_bits[m_bitsStart] = 1;
		}
	}
	// Throw away bits if the buffer is full
	else
	{
		if (--m_bitsStart <= -1) 
		{ 
			m_bitsStart = BIT_BUF_SIZE - 1; 
		}
	}
}

// ====================================================================
//
// ====================================================================
int extract_frame_pager::extractFrames()
{
	int msgCnt = 0;
	// While there is unread data in the bits buffer
	//----------------------------------------------
	while (getNoOfBits() > 0)
	{
		m_fifo.codeword = (m_fifo.codeword << 1) + getBit();
		m_fifo.numBits++;

		// If number of bits in fifo equals 32
		//------------------------------------
		if (m_fifo.numBits >= 32)
		{
			// Not got sync
			// ------------
			if (!m_gotSync)
			{
				if (bitsDiff(m_fifo.codeword, M_SYNC) <= 2)
				{
					m_inverted = false;
					m_gotSync = true;
					m_numCode = -1;
					m_fifo.numBits = 0;
					//TRACE("SYNC %x %d\n", m_fifo.codeword, m_numCode);
				}
				else if (bitsDiff(m_fifo.codeword, M_NOTSYNC) <= 2)
				{
					m_inverted = true;
					m_gotSync = true;
					m_numCode = -1;
					m_fifo.numBits = 0;
					//TRACE("ISYNC %x %d\n", m_fifo.codeword, m_numCode);
				}
				else
				{
					// Cause it to load one more bit
					m_fifo.numBits = 31;
				}
			} // Not got sync
			else
			{
				// Increment the word count
				// ------------------------
				++m_numCode; // It got set to -1 when a sync was found, now count the 16 words
				uint32_t val = m_inverted ? ~m_fifo.codeword : m_fifo.codeword;
				OnDataWord(val, m_numCode);
				//TRACE("WORD %x %d\n", m_fifo.codeword, m_numCode);

				// If at the end of a 16 word block
				// --------------------------------
				if (m_numCode >= 15)
				{
					msgCnt += OnDataFrame(m_numCode+1, (m_samplesPerSec<<10) / m_lastStableSymbolLen_1024);
					m_gotSync = false;
					m_numCode = -1;
				}
				m_fifo.numBits = 0;
			}
		} // If number of bits in fifo equals 32
	} // While there is unread data in the bits buffer
	return msgCnt;
} // extractFrames

// ====================================================================
//
// ====================================================================
//
// ====================================================================
short extract_frame_pager::getBit()
{
	if (m_bitsEnd != m_bitsStart)
	{
		if (++m_bitsEnd >= BIT_BUF_SIZE) 
		{ 
			m_bitsEnd = 0; 
		}
		return m_bits[m_bitsEnd];
	}
	else
	{
		return -1;
	}
}

// ====================================================================
//
// ====================================================================
int extract_frame_pager::getNoOfBits()
{
	int bits = m_bitsEnd - m_bitsStart;
	if (bits < 0) { bits += BIT_BUF_SIZE; }
	return bits;
}

// ====================================================================
//
// ====================================================================
uint32_t extract_frame_pager::getRate()
{
	return ((m_samplesPerSec<<10)+512) / m_lastStableSymbolLen_1024;
}
