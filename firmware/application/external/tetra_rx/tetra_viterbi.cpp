#include "tetra_viterbi.hpp"
#include <string.h>

namespace ui::external_app::tetra_rx {

int Viterbi::decode_cch(
    const uint8_t* mother,
    uint32_t n_info,
    uint8_t* bits,
    uint8_t* trace_buffer) {  // Puffer by caller

    // Store these on stack, to avoid flash space usage
    const uint8_t next_state[16][2] = {
        {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}, {14, 15}, {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}, {14, 15}};
    const uint8_t next_output[16][2] = {
        {0, 15}, {11, 4}, {6, 9}, {13, 2}, {5, 10}, {14, 1}, {3, 12}, {8, 7}, {15, 0}, {4, 11}, {9, 6}, {2, 13}, {10, 5}, {1, 14}, {12, 3}, {7, 8}};

    const uint16_t INF = 0x3fff;
    uint16_t cost[16];
    uint16_t new_cost[16];

    // Initialization
    for (int i = 0; i < 16; i++) cost[i] = INF;
    cost[0] = 0;

    for (uint32_t step = 0; step < n_info; step++) {
        for (int i = 0; i < 16; i++) new_cost[i] = INF;
        const uint8_t* rx = &mother[step * 4];

        for (int prev = 0; prev < 16; prev++) {
            if (cost[prev] >= INF) continue;

            for (int bit = 0; bit < 2; bit++) {
                const uint8_t pattern = next_output[prev][bit];
                const uint8_t next = next_state[prev][bit];

                uint16_t d = 0;
                for (int b = 0; b < 4; b++) {
                    if (rx[b] != 0xff && rx[b] != ((pattern >> (3 - b)) & 1)) d++;
                }

                const uint16_t total = cost[prev] + d;
                if (total < new_cost[next]) {
                    new_cost[next] = total;
                    trace_buffer[step * 16 + next] = (prev << 1) | bit;
                }
            }
        }
        memcpy(cost, new_cost, sizeof(cost));
    }

    // Traceback
    uint8_t best_state = 0;
    for (int i = 1; i < 16; i++) {
        if (cost[i] < cost[best_state]) best_state = i;
    }

    uint8_t state = best_state;
    memset(bits, 0, (n_info + 7) >> 3);

    for (int step = (int)n_info - 1; step >= 0; step--) {
        uint8_t packed = trace_buffer[step * 16 + state];
        uint8_t prev = (packed >> 1) & 0x0F;
        uint8_t bit = packed & 0x01;

        if (bit) bits[step >> 3] |= (1 << (7 - (step & 7)));
        state = prev;
    }

    return cost[best_state];
}

}  // namespace ui::external_app::tetra_rx