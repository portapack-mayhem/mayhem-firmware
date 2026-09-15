#include "crc.h"
#include "constants.h"

#define TOPBIT (1u << (FT8_CRC_WIDTH - 1))

// Compute 14-bit CRC for a sequence of given number of bits
// Adapted from https://barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
// [IN] message  - byte sequence (MSB first)
// [IN] num_bits - number of bits in the sequence
uint16_t ftx_compute_crc(const uint8_t message[], int num_bits)
{
    uint16_t remainder = 0;
    int idx_byte = 0;

    // Perform modulo-2 division, a bit at a time.
    for (int idx_bit = 0; idx_bit < num_bits; ++idx_bit)
    {
        if (idx_bit % 8 == 0)
        {
            // Bring the next byte into the remainder.
            remainder ^= (message[idx_byte] << (FT8_CRC_WIDTH - 8));
            ++idx_byte;
        }

        // Try to divide the current data bit.
        if (remainder & TOPBIT)
        {
            remainder = (remainder << 1) ^ FT8_CRC_POLYNOMIAL;
        }
        else
        {
            remainder = (remainder << 1);
        }
    }

    return remainder & ((TOPBIT << 1) - 1u);
}

uint16_t ftx_extract_crc(const uint8_t a91[])
{
    uint16_t chksum = ((a91[9] & 0x07) << 11) | (a91[10] << 3) | (a91[11] >> 5);
    return chksum;
}

void ftx_add_crc(const uint8_t payload[], uint8_t a91[])
{
    // Copy 77 bits of payload data
    for (int i = 0; i < 10; i++)
        a91[i] = payload[i];

    // Clear 3 bits after the payload to make 82 bits
    a91[9] &= 0xF8u;
    a91[10] = 0;

    // Calculate CRC of 82 bits (77 + 5 zeros)
    // 'The CRC is calculated on the source-encoded message, zero-extended from 77 to 82 bits'
    uint16_t checksum = ftx_compute_crc(a91, 96 - 14);

    // Store the CRC at the end of 77 bit message
    a91[9] |= (uint8_t)(checksum >> 11);
    a91[10] = (uint8_t)(checksum >> 3);
    a91[11] = (uint8_t)(checksum << 5);
}
