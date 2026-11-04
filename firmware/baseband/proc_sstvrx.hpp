/*
 * Copyright (C) 2025 StarVore Labs
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

#ifndef __PROC_SSTV_RX__
#define __PROC_SSTV_RX__

#include "portapack_shared_memory.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "sstv.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"
#include "audio_output.hpp"

#include <array>

using namespace sstv;

class SSTVRXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const p) override;

   private:
    enum state_t {
        STATE_SYNC_SEARCH = 0,
        STATE_VIS_DECODE,
        STATE_SEPARATOR,  // Wait for separator pulse (1500Hz)
        STATE_IMAGE_DATA
    };

    static constexpr uint32_t MAX_SAMPLES_PER_LINE = 4096;
    static constexpr uint16_t MAX_PIXELS_PER_LINE = 640;

    // Frequency ranges for SSTV (in Hz)
    static constexpr int32_t FREQ_BLACK = 1500;
    static constexpr int32_t FREQ_WHITE = 2300;
    static constexpr int32_t FREQ_SYNC = 1200;
    static constexpr int32_t FREQ_VIS_BIT0 = 1300;
    static constexpr int32_t FREQ_VIS_BIT1 = 1100;

    state_t state{STATE_SYNC_SEARCH};
    bool configured{false};
    uint8_t vis_code{0};
    const sstv_mode* active_mode{nullptr};
    uint16_t mode_total_lines{256};
    uint16_t pixels_per_line{320};

    static constexpr size_t baseband_fs = 3072000;

    // DSP chain components (using NFM-style decimation for SSTV)
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};         // Decimate by 8 (NFM style)
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};           // Decimate by 8
    dsp::decimate::FIRAndDecimateComplex channel_filter{};  // Decimate by 2 -> 24kHz
    dsp::demodulate::FM demod{};                            // FM demodulator
    AudioOutput audio_output{};

    // Buffers
    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};
    // work_audio_buffer and dst_buffer use the same data pointer
    const buffer_s16_t work_audio_buffer{
        (int16_t*)dst.data(),
        sizeof(dst) / sizeof(int16_t)};

    // Tone frequency, a value per audio sample: the audio is mixed down around the middle of
    // the SSTV tones, low passed, and the phase step between samples gives the frequency
    int32_t current_freq{1200};  // Current frequency in Hz
    int32_t sync_freq{1200};     // Smoothed over ~1 ms, for the sync detection

    struct Biquad {
        float b0{1}, b1{0}, b2{0}, a1{0}, a2{0};
        float z1{0}, z2{0};
        void low_pass(float cutoff, float q, float sample_rate);
        float execute(float x) {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
    static constexpr float MIX_FREQ = 1900.0f;          // middle of 1100..2300 Hz
    static constexpr float TONE_LOW_PASS = 1500.0f;     // keeps the tones, drops the image at 2x1900 Hz
    static constexpr float SYNC_SMOOTHING = 1.0f / 32;  // ~0.7 ms at 48 kHz
    Biquad lpf_i[2]{}, lpf_q[2]{};                      // 4th order Butterworth
    float mix_re{1}, mix_im{0}, mix_step_re{1}, mix_step_im{0};
    uint32_t mix_count{0};
    float prev_i{0}, prev_q{0};
    float hz_per_radian{0};
    float sync_freq_avg{1200};
    uint32_t sync_detect_lag{0};  // samples the smoothing makes the end of a sync late

    // Line decoding state
    uint8_t line_buffer_r[MAX_PIXELS_PER_LINE];
    uint8_t line_buffer_g[MAX_PIXELS_PER_LINE];
    uint8_t line_buffer_b[MAX_PIXELS_PER_LINE];

    // PD modes store Y / (R-Y) / (B-Y) / Y (double-line)
    uint8_t pd_y0[MAX_PIXELS_PER_LINE];
    uint8_t pd_ry[MAX_PIXELS_PER_LINE];
    uint8_t pd_by[MAX_PIXELS_PER_LINE];
    uint8_t pd_y1[MAX_PIXELS_PER_LINE];

    // Finished lines as RGB, handed to the M0 a chunk per execute()
    static constexpr uint8_t MAX_OUT_LINES = 2;  // PD finishes two at once
    uint8_t out_rgb[MAX_OUT_LINES][MAX_PIXELS_PER_LINE * 3];
    uint16_t out_line[MAX_OUT_LINES]{};
    uint8_t out_count{0};
    uint16_t out_next_chunk{0};

    // Robot 36: Y in pd_y0, the chroma of this and the line before in pd_ry/pd_by.
    // The separator before the chroma says which one it is (1500 Hz R-Y, 2300 Hz B-Y).
    bool robot36{false};
    bool robot36_chroma_by{false};
    int32_t robot36_separator_sum{0};
    uint32_t robot36_separator_count{0};
    uint32_t robot36_separator_from{0};  // part of the gap measured, in samples
    uint32_t robot36_separator_to{0};

    uint32_t sample_count{0};
    uint32_t pixel_index{0};
    uint32_t channel_index{0};
    uint8_t channel_count{3};
    std::array<uint8_t, 3> color_order{{1, 2, 0}};
    uint16_t current_line{0};
    bool waiting_for_first_line{true};

    // Pixel accumulation for averaging
    int32_t pixel_accumulator{0};
    uint32_t pixel_sample_count{0};

    // Fractional pixel timing for accuracy
    float pixel_time_frac{0.0f};  // Fractional samples per pixel
    float pixel_phase{0.0f};      // Accumulated phase for current pixel

    // Phase and slant adjustments
    int16_t phase_offset{0};   // Horizontal offset in pixels
    int16_t slant_rate{0};     // Timing adjustment in 0.1% units
    float slant_factor{1.0f};  // Calculated slant multiplier

    // Timing parameters (will be set based on mode)
    uint32_t samples_per_pixel{7};     // Integer part for quick checks
    uint32_t samples_per_sync{216};    // 9ms
    uint32_t samples_per_gap{36};      // Gap after sync or between channels
    uint32_t channel_gap_samples{36};  // Separators between color sections
    uint32_t separator_target{0};

    // Sync detection
    uint32_t sync_sample_count{0};
    bool in_sync{false};
    int32_t sync_freq_sum{0};       // Accumulated frequency during sync pulse
    uint32_t sync_freq_samples{0};  // Number of samples in sync pulse for averaging

    // Sync pulse timing tracking for auto-calibration
    static constexpr uint32_t MAX_SYNC_HISTORY = 256;  // Track all syncs in image
    uint32_t sync_positions[MAX_SYNC_HISTORY];         // Sample positions when sync detected
    uint16_t sync_history_count{0};
    uint32_t expected_sync_interval{0};  // Expected samples between syncs
    int32_t accumulated_phase_error{0};  // Accumulated phase offset in samples
    int32_t accumulated_slant_error{0};  // Accumulated timing drift
    uint32_t global_sample_count{0};     // Never-reset counter for timing calibration

    // Frequency offset compensation (auto-calibrated from sync pulses)
    int32_t freq_offset{0};
    bool freq_offset_calibrated{false};
    int32_t sync_freq_accumulator{0};
    uint32_t sync_freq_count{0};

    // Helper functions
    int32_t freq_to_pixel(int32_t freq);
    void process_pixel_sample(int32_t freq);
    void process_line();
    void detect_sync(int32_t freq);
    void calculate_calibration();
    uint32_t compute_nominal_line_interval() const;
    void estimate_frequency(int32_t audio_sample);
    void capture_config(const CaptureConfigMessage& message);
    void reset_pixel_state();
    void start_gap(uint32_t duration);
    void begin_line_after_sync();
    void clear_line_buffers(bool keep_chroma = false);
    void ycbcr_to_line(const uint8_t* y, const uint8_t* cr, const uint8_t* cb);
    void queue_line(uint16_t line);
    void send_pending_chunk();
    void measure_robot36_separator();
    void store_pixel_value(uint32_t channel, uint16_t pixel, uint8_t value);

    RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
};

#endif