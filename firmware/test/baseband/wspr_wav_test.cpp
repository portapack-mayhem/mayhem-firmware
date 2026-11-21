/*
 * Copyright (C) 2026 Ivan
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

/*
 * Standalone host validator: decodes WSPR from a WAV file with the exact
 * Goertzel matrix pipeline the baseband receiver uses, then feeds the matrix
 * to wspr_decode_window() and prints every result. Also reports the best
 * coarse sync score (a replica of the decoder's stage-1 scan) as a profile
 * sanity check.
 *
 * The capture is 176 rows of 8192 samples (a 120 s slot) taken from the file
 * starting at the given offset. A file shorter than 162 rows (110.6 s) cannot
 * hold a transmission and is skipped. Files longer than 176 rows use only the
 * first 176 rows after the start offset.
 *
 * Usage:
 *   wspr_wav_test <file.wav> [options]
 * Options:
 *   --start SEC            decode one capture starting at SEC seconds
 *   --start-samples LIST   comma-separated byte-sample offsets to decode
 *   --sweep                sweep the capture start regardless of the result
 *   --sweep-min SEC        sweep start (default 0)
 *   --sweep-max SEC        sweep end (default 5.0)
 *   --step SEC             sweep step (default 0.5)
 *   --osr2                 build the dual-bank matrix (two interleaved
 *                          Goertzel grids half a symbol apart, block-row
 *                          2*row + bank) and decode with time_osr = 2, the
 *                          receiver's v2 sub-row scheme
 * Default behavior: decode at offset 0; if nothing decodes there, sweep the
 * capture start in 0.5 s steps up to 5 s and report which offsets decode.
 */

#include "wspr_constants.h"
#include "wspr_decode.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kRows = 176;
constexpr int kNumBins = 137;  // 1400-1600 Hz in 1.4648 Hz steps
constexpr int kBin0 = 956;     // 956 * 12000/8192 = 1400.0 Hz
constexpr int kBin0Hz = kBin0 * 12000 / 8192;

// ---------------------------------------------------------------------------
// Minimal RIFF reader: 16-bit mono PCM. Handles the canonical 44-byte header
// and files with extra leading RIFF chunks (LIST/fact) by walking the chunk
// list; anything else (non-PCM codec, stereo, non-16-bit, wrong rate) is
// rejected with a clear message.
// ---------------------------------------------------------------------------

bool read_u32le(FILE* f, uint32_t* v) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return false;
    *v = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
         ((uint32_t)b[3] << 24);
    return true;
}

bool read_wav_16bit_mono(const char* path,
                         std::vector<int16_t>* samples,
                         uint32_t* rate,
                         std::string* err) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        err->assign("cannot open file");
        return false;
    }
    uint8_t riff[12];
    if (fread(riff, 1, 12, f) != 12 || memcmp(riff, "RIFF", 4) != 0 ||
        memcmp(riff + 8, "WAVE", 4) != 0) {
        err->assign("not a RIFF/WAVE file");
        fclose(f);
        return false;
    }

    bool have_fmt = false;
    uint16_t fmt_tag = 0, channels = 0, bits = 0;
    uint32_t rate_hz = 0;
    long data_off = -1;
    uint32_t data_size = 0;

    while (true) {
        uint8_t id[4];
        uint32_t size;
        if (fread(id, 1, 4, f) != 4 || !read_u32le(f, &size)) break;
        if (memcmp(id, "fmt ", 4) == 0) {
            uint8_t fmt[16];
            if (size < 16 || fread(fmt, 1, 16, f) != 16) {
                err->assign("truncated fmt chunk");
                fclose(f);
                return false;
            }
            fmt_tag = (uint16_t)(fmt[0] | (fmt[1] << 8));
            channels = (uint16_t)(fmt[2] | (fmt[3] << 8));
            rate_hz = (uint32_t)fmt[4] | ((uint32_t)fmt[5] << 8) |
                      ((uint32_t)fmt[6] << 16) | ((uint32_t)fmt[7] << 24);
            bits = (uint16_t)(fmt[14] | (fmt[15] << 8));
            have_fmt = true;
            if (size > 16 && fseek(f, (long)(size - 16) + (size & 1), SEEK_CUR) != 0) break;
        } else if (memcmp(id, "data", 4) == 0) {
            data_off = ftell(f);
            data_size = size;
            if (fseek(f, (long)size + (size & 1), SEEK_CUR) != 0) break;
        } else {
            // Skip any other chunk (LIST, fact, ...); chunks are word-aligned.
            if (fseek(f, (long)size + (size & 1), SEEK_CUR) != 0) break;
        }
    }

    if (!have_fmt) {
        err->assign("no fmt chunk");
        fclose(f);
        return false;
    }
    if (fmt_tag != 1) {
        err->assign("unsupported: not uncompressed PCM (format tag ");
        (*err) += std::to_string(fmt_tag) + ")";
        fclose(f);
        return false;
    }
    if (channels != 1) {
        err->assign("unsupported: expected mono, got ");
        (*err) += std::to_string(channels) + " channel(s)";
        fclose(f);
        return false;
    }
    if (bits != 16) {
        err->assign("unsupported: expected 16-bit samples, got ");
        (*err) += std::to_string(bits) + " bit";
        fclose(f);
        return false;
    }
    if (rate_hz != WSPR_SAMPLE_RATE) {
        err->assign("unsupported: expected 12000 Hz, got ");
        (*err) += std::to_string(rate_hz) + " Hz";
        fclose(f);
        return false;
    }
    if (data_off < 0 || data_size == 0) {
        err->assign("no data chunk");
        fclose(f);
        return false;
    }
    if (fseek(f, data_off, SEEK_SET) != 0) {
        err->assign("cannot seek to data chunk");
        fclose(f);
        return false;
    }
    const size_t n = data_size / 2;
    samples->resize(n);
    if (fread(samples->data(), 2, n, f) != n) {
        err->assign("truncated data chunk");
        fclose(f);
        return false;
    }
    fclose(f);
    *rate = rate_hz;
    return true;
}

// ---------------------------------------------------------------------------
// Reference Goertzel bank, copied from wspr_decode_test.cpp: one power byte
// per (row, bin), the same quantization the baseband uses
// (mag_dB = byte * 0.5 - 120 on a scale where a full-scale tone lands high).
// Reads num_rows * 8192 samples beginning at start_sample.
//
// The byte map is calibrated for unit-scale input like the reference tests
// use: a full-scale tone must land around byte 212, not clamp at 255. WAV
// samples are int16, so they are normalized to +-1.0 here first — the same
// normalization the baseband ADC path applies before its Goertzel bank.
// ---------------------------------------------------------------------------

void goertzel_bank(const std::vector<int16_t>& samples,
                   int start_sample,
                   int num_rows,
                   std::vector<uint8_t>& bank) {
    bank.assign((size_t)num_rows * kNumBins, 0);
    double coeff[kNumBins];
    for (int b = 0; b < kNumBins; b++) {
        const int k = kBin0 + b;
        coeff[b] = 2.0 * std::cos(6.2831853 * (double)k / (double)WSPR_SAMPLES_PER_SYMBOL);
    }
    double s1[kNumBins], s2[kNumBins];
    for (int row = 0; row < num_rows; row++) {
        const size_t begin = (size_t)start_sample + (size_t)row * WSPR_SAMPLES_PER_SYMBOL;
        if (begin + WSPR_SAMPLES_PER_SYMBOL > samples.size()) continue;
        memset(s1, 0, sizeof(s1));
        memset(s2, 0, sizeof(s2));
        const int16_t* block = &samples[begin];
        for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL; i++) {
            const double x = (double)block[i] / 32768.0;
            for (int b = 0; b < kNumBins; b++) {
                const double s0 = x + coeff[b] * s1[b] - s2[b];
                s2[b] = s1[b];
                s1[b] = s0;
            }
        }
        for (int b = 0; b < kNumBins; b++) {
            const double power =
                (s1[b] * s1[b] + s2[b] * s2[b] - coeff[b] * s1[b] * s2[b]) /
                ((double)WSPR_SAMPLES_PER_SYMBOL * (double)WSPR_SAMPLES_PER_SYMBOL);
            if (power < 1e-20) continue;
            int byte = (int)(6.0206 * std::log2(power) + 240.0);
            if (byte < 0) byte = 0;
            if (byte > 255) byte = 255;
            bank[(size_t)row * kNumBins + b] = (uint8_t)byte;
        }
    }
}

void goertzel_matrix(const std::vector<int16_t>& samples,
                     int start_sample,
                     int num_rows,
                     std::vector<uint8_t>& mag) {
    goertzel_bank(samples, start_sample, num_rows, mag);
}

// Dual-bank matrix, the layout the receiver glue produces: block-row index
// 2*row + bank, bank 0 rows closing at capture sample 0 (mod 8192), bank 1
// starting its first full row at capture sample 4096 and lagging half a
// symbol. Rows whose span runs past the end of the file stay zero, like the
// receiver's window end.
void goertzel_matrix_dual(const std::vector<int16_t>& samples,
                          int start_sample,
                          int num_rows,
                          std::vector<uint8_t>& mag) {
    std::vector<uint8_t> bank0, bank1;
    goertzel_bank(samples, start_sample, num_rows, bank0);
    goertzel_bank(samples, start_sample + WSPR_SAMPLES_PER_SYMBOL / 2, num_rows, bank1);
    mag.assign((size_t)2 * num_rows * kNumBins, 0);
    for (int row = 0; row < num_rows; row++) {
        memcpy(&mag[(size_t)(2 * row) * kNumBins], &bank0[(size_t)row * kNumBins], kNumBins);
        memcpy(&mag[(size_t)(2 * row + 1) * kNumBins], &bank1[(size_t)row * kNumBins],
               kNumBins);
    }
}

// ---------------------------------------------------------------------------
// Replica of the decoder's stage-1 sync score (wspr_decode.c sync_score) so
// the profile can be sanity-checked outside the decoder: sum of the better
// bin of every symbol's sync-selected pair, scanned over the whole matrix.
// ---------------------------------------------------------------------------

int32_t sync_score_at(const uint8_t* mag, int num_bins, int row0, int col0) {
    int32_t score = 0;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int col = col0 + wspr_sync_vector[n];
        const uint8_t e0 = mag[(size_t)row0 * num_bins + col];
        const uint8_t e1 = mag[(size_t)row0 * num_bins + col + 2];
        score += (e0 > e1) ? e0 : e1;
    }
    return score;
}

struct SyncBest {
    int32_t score;
    int row;
    int col;
};

SyncBest best_sync_score(const uint8_t* mag, int num_rows) {
    SyncBest best = {-1, -1, -1};
    for (int t = 0; t + WSPR_NUM_SYMBOLS <= num_rows; t++) {
        for (int c = 0; c + 3 < kNumBins; c++) {
            const int32_t s = sync_score_at(mag, kNumBins, t, c);
            if (s > best.score) best = {s, t, c};
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// One capture: build the matrix, decode, print. Returns the result count.
// ---------------------------------------------------------------------------

int run_capture(const std::vector<int16_t>& samples,
                int start_sample,
                int time_osr,
                const char* label) {
    if (start_sample < 0 || (size_t)start_sample >= samples.size()) {
        printf("  %s: start offset past end of file - skipped\n", label);
        return -1;
    }
    const int avail_rows = (int)((samples.size() - (size_t)start_sample) /
                                 WSPR_SAMPLES_PER_SYMBOL);
    if (avail_rows < WSPR_NUM_SYMBOLS) {
        printf("  %s: only %d rows available (need >= %d) - skipped\n",
               label, avail_rows, WSPR_NUM_SYMBOLS);
        return -1;
    }
    const int num_rows = avail_rows < kRows ? avail_rows : kRows;

    std::vector<uint8_t> mag;
    if (time_osr == 2) {
        goertzel_matrix_dual(samples, start_sample, num_rows, mag);
    } else {
        goertzel_matrix(samples, start_sample, num_rows, mag);
    }

    const SyncBest sync = best_sync_score(mag.data(), num_rows);
    printf("  %s: osr%d, %d rows, best sync score %d at row=%d col=%d (%.1f Hz)\n",
           label, time_osr, num_rows, sync.score, sync.row, sync.col,
           kBin0Hz + sync.col * 12000.0 / 8192.0);

    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count = wspr_decode_window(mag.data(), num_rows, kNumBins,
                                         kBin0Hz, results, WSPR_MAX_RESULTS, time_osr);
    if (count <= 0) {
        printf("    decode: no results\n");
    } else {
        printf("    decode: %d result(s)\n", count);
        for (int i = 0; i < count; i++) {
            printf("      %s %s %d  snr=%d dB  freq=%d Hz  drift=%d  row_offset=%d  bank=%d\n",
                   results[i].callsign, results[i].grid, results[i].dbm,
                   results[i].snr_db, results[i].freq_hz,
                   results[i].drift_bins, results[i].row_offset, results[i].bank);
        }
    }
    return count;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s <file.wav> [--start SEC] [--start-samples LIST]"
                " [--sweep] [--sweep-min SEC] [--sweep-max SEC] [--step SEC]\n",
                argv[0]);
        return 2;
    }
    const char* path = argv[1];

    bool force_sweep = false;
    double sweep_min = 0.0, sweep_max = 5.0, step = 0.5;
    bool have_start = false;
    double start_sec = 0.0;
    int time_osr = 1;
    std::vector<int> sample_starts;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--sweep") == 0) {
            force_sweep = true;
        } else if (strcmp(argv[i], "--osr2") == 0) {
            time_osr = 2;
        } else if (strcmp(argv[i], "--sweep-min") == 0 && i + 1 < argc) {
            sweep_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--sweep-max") == 0 && i + 1 < argc) {
            sweep_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--step") == 0 && i + 1 < argc) {
            step = atof(argv[++i]);
        } else if (strcmp(argv[i], "--start") == 0 && i + 1 < argc) {
            have_start = true;
            start_sec = atof(argv[++i]);
        } else if (strcmp(argv[i], "--start-samples") == 0 && i + 1 < argc) {
            const char* p = argv[++i];
            while (*p) {
                char* end = NULL;
                const long v = strtol(p, &end, 10);
                if (end == p) break;
                sample_starts.push_back((int)v);
                p = (*end == ',') ? end + 1 : end;
            }
        } else {
            fprintf(stderr, "unknown or incomplete option: %s\n", argv[i]);
            return 2;
        }
    }

    std::vector<int16_t> samples;
    uint32_t rate = 0;
    std::string err;
    if (!read_wav_16bit_mono(path, &samples, &rate, &err)) {
        fprintf(stderr, "%s: WAV error: %s\n", path, err.c_str());
        return 1;
    }
    printf("file: %s\n", path);
    printf("  %zu samples (%.2f s), 16-bit mono %u Hz, max %d rows\n",
           samples.size(), (double)samples.size() / rate, rate, kRows);
    printf("  capture: %d rows x %d samples, time_osr=%d, columns bin %d..%d (%.1f..%.1f Hz)\n",
           kRows, WSPR_SAMPLES_PER_SYMBOL, time_osr, kBin0, kBin0 + kNumBins - 1,
           (double)kBin0Hz, kBin0Hz + (kNumBins - 1) * 12000.0 / 8192.0);

    // Explicit offsets given on the command line.
    if (!sample_starts.empty() || have_start) {
        if (sample_starts.empty()) {
            sample_starts.push_back((int)(start_sec * WSPR_SAMPLE_RATE + 0.5));
        }
        for (size_t i = 0; i < sample_starts.size(); i++) {
            char label[64];
            snprintf(label, sizeof(label), "start sample %d (%.3f s)",
                     sample_starts[i], (double)sample_starts[i] / WSPR_SAMPLE_RATE);
            run_capture(samples, sample_starts[i], time_osr, label);
        }
        return 0;
    }

    // Default: offset 0, then (unless --sweep) an automatic 0.5 s-step sweep
    // when offset 0 yields nothing.
    const int count0 = run_capture(samples, 0, time_osr, "start 0.000 s (offset 0)");
    if (count0 > 0 && !force_sweep) return 0;

    printf("\n  -- capture-offset sweep, %.2f s steps --\n", step);
    std::vector<int> good;
    for (double t = sweep_min; t <= sweep_max + 1e-9; t += step) {
        const int s = (int)(t * WSPR_SAMPLE_RATE + 0.5);
        char label[64];
        snprintf(label, sizeof(label), "start %.3f s", t);
        const int c = run_capture(samples, s, time_osr, label);
        if (c > 0) good.push_back(s);
    }
    printf("\n  sweep summary: %zu offset(s) decoded:",
           good.size());
    for (size_t i = 0; i < good.size(); i++) {
        printf(" %.3fs(%ds)", (double)good[i] / WSPR_SAMPLE_RATE, good[i]);
    }
    printf("\n");
    return 0;
}
