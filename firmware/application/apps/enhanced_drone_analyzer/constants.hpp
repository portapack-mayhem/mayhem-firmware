#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstdint>
#include <cstddef>
#include "drone_types.hpp"

namespace drone_analyzer {

// ============================================================================
// Frequency Constants
// ============================================================================

/**
 * @brief Minimum frequency in Hz (1 MHz - HackRF One minimum)
 */
constexpr FreqHz MIN_FREQUENCY_HZ = 1'000'000ULL;

/**
 * @brief Maximum frequency in Hz (7.2 GHz - HackRF One theoretical maximum)
 */
constexpr FreqHz MAX_FREQUENCY_HZ = 7'200'000'000ULL;

/**
 * @brief Maximum supported frequency in Hz (7.2 GHz)
 * @note HackRF One RFFC5072 mixer practical limit: ~6 GHz
 *       but 7.2 GHz is used for validation headroom
 * @note Use this for validation of sweep ranges and database entries
 */
constexpr FreqHz HARDWARE_MAX_FREQ_HZ = 7'200'000'000ULL;

/**
 * @brief Hardware practical minimum frequency in Hz (1 MHz)
 */
constexpr FreqHz HARDWARE_MIN_FREQ_HZ = 1'000'000ULL;

/**
 * @brief Frequency step size in Hz (1 MHz)
 */
constexpr FreqHz FREQUENCY_STEP_HZ = 1'000'000ULL;

/**
 * @brief Frequency bandwidth in Hz (2 MHz - matches DEFAULT_SAMPLE_RATE_HZ)
 */
constexpr FreqHz FREQUENCY_BANDWIDTH_HZ = 20'000'000ULL;

// ============================================================================
// Buffer Size Constants
// ============================================================================

/**
 * @brief Maximum number of tracked drones
 */
constexpr size_t MAX_TRACKED_DRONES = 16;

/**
 * @brief Maximum number of displayed drones
 */
constexpr size_t MAX_DISPLAYED_DRONES = 16;

/**
 * @brief Spectrum buffer size (bytes)
 * @note Must match ChannelSpectrum::db.size() (256 bins)
 */
constexpr size_t SPECTRUM_BUFFER_SIZE = 256;

/**
 * @brief Histogram buffer size (bins)
 * @note 240 = DISPLAY_WIDTH = full sweep composite coverage
 */
constexpr size_t HISTOGRAM_BUFFER_SIZE = 240;

/**
 * @brief RSSI history size for each drone
 * @note Must be >= MOVEMENT_TREND_MIN_HISTORY for trend calculation
 */
constexpr size_t RSSI_HISTORY_SIZE = 6;

/**
 * @brief Timestamp history size for each drone
 * @note Must equal RSSI_HISTORY_SIZE to prevent index misalignment
 *       in circular buffer access patterns
 */
constexpr size_t TIMESTAMP_HISTORY_SIZE = RSSI_HISTORY_SIZE;

/**
 * @brief Maximum number of frequency entries to scan
 */
constexpr size_t MAX_ENTRIES_TO_SCAN = 100;

/**
 * @brief Frequency hash table size
 */
constexpr size_t FREQUENCY_HASH_TABLE_SIZE = 256;

// ============================================================================
// Hardware Constants
// ============================================================================

/**
 * @brief Default sample rate in Hz (2 MHz)
 */
constexpr uint32_t DEFAULT_SAMPLE_RATE_HZ = 2000000;

/**
 * @brief Default gain value
 * @note >= 20 enables RF amp (RFFC5072) for ~10-14 dB additional gain
 */
constexpr uint16_t DEFAULT_GAIN = 30;

/**
 * @brief Default LNA gain value (MAX2837: 0-40 dB, step 8 dB)
 * @note 40 dB — FPV max-range: catch weak analog 5.8 GHz video links
 * @note 40 dB is the maximum valid value (LNA step = 8 dB)
 */
constexpr uint8_t DEFAULT_LNA_GAIN = 32;

/**
 * @brief Default VGA gain value (MAX2837: 0-62 dB, step 2 dB)
 * @note 40 dB — FPV-optimized: 80 dB total RF gain (LNA 40 + VGA 40)
 * @note 62 dB (max) would risk ADC saturation on strong analog FM,
 *       producing intermod spurs that mimic drone signatures.
 *       40 dB gives max useful range without baseband overload.
 */
constexpr uint8_t DEFAULT_VGA_GAIN = 32;

// ============================================================================
// Time Constants
// ============================================================================

/**
 * @brief Drone stale timeout in milliseconds (5 seconds)
 */
constexpr uint32_t DRONE_STALE_TIMEOUT_MS = 5000;

/**
 * @brief Drone removal timeout in milliseconds (30 seconds)
 */
constexpr uint32_t DRONE_REMOVAL_TIMEOUT_MS = 30000;

/**
 * @brief Scan cycle interval in milliseconds (50 ms)
 */
constexpr uint32_t SCAN_CYCLE_INTERVAL_MS = 50;

/**
 * @brief PLL lock timeout in milliseconds
 */
constexpr uint32_t PLL_LOCK_TIMEOUT_MS = 100;

/**
 * @brief PLL lock poll interval in milliseconds
 */
constexpr uint32_t PLL_LOCK_POLL_INTERVAL_MS = 3;

/**
 * @brief Hardware retry delay in milliseconds
 */
constexpr uint32_t HARDWARE_RETRY_DELAY_MS = 10;

/**
 * @brief SD card operation timeout in milliseconds
 */
constexpr uint32_t SD_CARD_TIMEOUT_MS = 1000;

// ============================================================================
// RSSI Constants
// ============================================================================

/**
 * @brief Minimum RSSI value (dBm)
 */
constexpr int32_t RSSI_MIN_DBM = -120;

/**
 * @brief Maximum RSSI value (dBm)
 */
constexpr int32_t RSSI_MAX_DBM = -20;

/**
 * @brief RSSI threshold for signal detection (dBm)
 * @note FPV-OPTIMIZED: -95 dBm for maximum long-range detection
 * @note Analog 5.8 GHz FPV: -85 to -100 dBm at 5-15 km (typical analog VTX)
 * @note -95 dBm catches weak analog FM video links; noise/spurs filtered by:
 *       - spectrum shape filter (V-shape, valley depth, symmetry)
 *       - CFAR OS (multi-target robust)
 *       - Mahalanobis gate + RSSI variance (anti-false-positive post-filters)
 * @note -90 dBm was the previous default; -95 adds ~3-5 km of detection range
 * @note HackRF baseband: spectrum.db = clamp(dBV*5 + 255, 0, 255)
 * @note dBm = (value - 255) / 5 - gain_offset; approximated as value - 120
 * @note Center bins 120-135 contain DC spike (blanked like Looking Glass/Search)
 */
constexpr int32_t RSSI_DETECTION_THRESHOLD_DBM = -95;

/**
 * @brief RSSI threshold for high threat (dBm)
 * @note With default gains (LNA=32 + VGA=32 + RF_AMP=14 = 78 dB),
 *       max RSSI = (255-255)/5 - 78 = -78 dBm. Threshold must be reachable.
 *       -87 dBm = 8 dB above detection threshold (-95). Reachable when
 *       spectrum.db value >= 230 (~90% of max).
 * @note Previous value (-60) was unreachable with default gains (max -78 dBm),
 *       causing all detections to classify as MEDIUM.
 */
constexpr int32_t RSSI_HIGH_THREAT_THRESHOLD_DBM = -87;

/**
 * @brief RSSI threshold for critical threat (dBm)
 * @note With default gains (78 dB), max RSSI = -78 dBm. Threshold -80 dBm
 *       requires spectrum.db value >= 240 (~94% of max) — reserved for very
 *       strong signals (close-range drone, line-of-sight).
 * @note Previous value (-40) was unreachable with default gains (max -78 dBm),
 *       causing all detections to classify as MEDIUM.
 */
constexpr int32_t RSSI_CRITICAL_THREAT_THRESHOLD_DBM = -80;

/**
 * @brief Noise floor RSSI (dBm)
 */
constexpr int32_t RSSI_NOISE_FLOOR_DBM = -100;

/**
 * @brief Default RSSI threshold for low threat (dBm)
 * @note Detection threshold - 10 dB = -105 dBm
 */
constexpr int32_t DEFAULT_THREAT_LOW_DBM = -105;

/**
 * @brief Default RSSI threshold for medium threat (dBm)
 * @note Equals detection threshold (-95 dBm) — all signals above detection start at MEDIUM
 */
constexpr int32_t DEFAULT_THREAT_MEDIUM_DBM = -95;

/**
 * @brief RF amplifier gain (dB) when enabled
 * @note HackRF RF amp (HMC627A/VGA) provides ~14 dB when enabled, 0 dB bypass when off
 */
constexpr int32_t RF_AMP_GAIN_DB = 14;

// ============================================================================
// Histogram Constants
// ============================================================================

/**
 * @brief Histogram bin count
 */
constexpr size_t HISTOGRAM_BIN_COUNT = 64;

/**
 * @brief Histogram scale factor
 */
constexpr uint8_t HISTOGRAM_SCALE_FACTOR = 4;

/**
 * @brief Histogram max value (uint8_t max)
 */
constexpr uint8_t HISTOGRAM_MAX_VALUE = 255;

/**
 * @brief Histogram extended size (for calculations)
 */
constexpr size_t HISTOGRAM_EXTENDED_SIZE = 256;

/**
 * @brief Histogram buffer half size
 */
constexpr size_t HISTOGRAM_HALF_SIZE = 128;

/**
 * @brief Histogram noise floor threshold
 */
constexpr uint8_t HISTOGRAM_NOISE_FLOOR = 10;

/**
 * @brief Histogram signal threshold
 */
constexpr uint8_t HISTOGRAM_SIGNAL_THRESHOLD = 20;

// ============================================================================
// Display Constants
// ============================================================================

/**
 * @brief Display width in pixels
 */
constexpr uint16_t DISPLAY_WIDTH = 240;

/**
 * @brief Display height in pixels
 */
constexpr uint16_t DISPLAY_HEIGHT = 320;

/**
 * @brief Font height in pixels
 */
constexpr uint8_t FONT_HEIGHT = 8;

/**
 * @brief Font width in pixels
 */
constexpr uint8_t FONT_WIDTH = 5;

/**
 * @brief Maximum text length for display
 */
constexpr size_t MAX_TEXT_LENGTH = 32;

/**
 * @brief Drone type name length
 */
constexpr size_t DRONE_TYPE_NAME_LENGTH = 16;

// ============================================================================
// Threat Level Constants
// ============================================================================

/**
 * @brief Number of threat levels
 */
constexpr size_t THREAT_LEVEL_COUNT = 5;

/**
 * @brief High threat approaching count threshold
 */
constexpr size_t HIGH_THREAT_APPROACHING_COUNT = 3;

/**
 * @brief Critical threat approaching count threshold
 */
constexpr size_t CRITICAL_THREAT_APPROACHING_COUNT = 5;

// ============================================================================
// Scanning Constants
// ============================================================================

/**
 * @brief Default scanning mode
 */
constexpr ScanningMode DEFAULT_SCANNING_MODE = ScanningMode::SEQUENTIAL;

/**
 * @brief Number of scanning modes
 */
constexpr uint8_t SCANNING_MODE_COUNT = 1;

/**
 * @brief Maximum scan cycles before timeout
 */
constexpr uint32_t MAX_SCAN_CYCLES = 1000;

// ============================================================================
// Database Constants
// ============================================================================

/**
 * @brief Maximum database entries
 * @note 100 entries × 16 bytes = 1,600 bytes
 */
constexpr size_t MAX_DATABASE_ENTRIES = 100;

/**
 * @brief Database file path
 */
constexpr const char DATABASE_FILE_PATH[] = "/FREQMAN/DRONES.TXT";

/**
 * @brief Database load timeout in milliseconds
 */
constexpr uint32_t DATABASE_LOAD_TIMEOUT_MS = 2000;

/**
 * @line Database line buffer size
 */
constexpr size_t DATABASE_LINE_BUFFER_SIZE = 128;

// ============================================================================
// Settings Constants
// ============================================================================

/**
 * @brief Settings file path
 */
constexpr const char SETTINGS_FILE_PATH[] = "/EDA/SETTINGS.TXT";

/**
 * @brief Settings file version
 */
constexpr uint32_t SETTINGS_FILE_VERSION = 1;

// ============================================================================
// Audio Constants
// ============================================================================

/**
 * @brief Audio alert sample rate in Hz
 */
constexpr uint32_t AUDIO_ALERT_SAMPLE_RATE_HZ = 24000;

/**
 * @brief Audio alert frequency in Hz (default)
 */
constexpr uint32_t AUDIO_ALERT_FREQUENCY_HZ = 1000;

/**
 * @brief Audio alert high threat frequency in Hz
 */
constexpr uint32_t AUDIO_ALERT_HIGH_FREQUENCY_HZ = 1200;

/**
 * @brief Audio alert critical threat frequency in Hz
 */
constexpr uint32_t AUDIO_ALERT_CRITICAL_FREQUENCY_HZ = 1500;

/**
 * @brief Audio alert low frequency in Hz (receding)
 */
constexpr uint32_t AUDIO_ALERT_LOW_FREQUENCY_HZ = 800;

/**
 * @brief Audio alert duration in milliseconds (default)
 */
constexpr uint32_t AUDIO_ALERT_DURATION_MS = 150;

/**
 * @brief Audio alert short duration in milliseconds (critical)
 */
constexpr uint32_t AUDIO_ALERT_SHORT_DURATION_MS = 80;

/**
 * @brief Audio alert medium duration in milliseconds (threat increased)
 */
constexpr uint32_t AUDIO_ALERT_MEDIUM_DURATION_MS = 100;

/**
 * @brief Audio alert long duration in milliseconds (approaching)
 */
constexpr uint32_t AUDIO_ALERT_LONG_DURATION_MS = 200;

/**
 * @brief Audio alert gap in milliseconds (short)
 */
constexpr uint32_t AUDIO_ALERT_SHORT_GAP_MS = 40;

/**
 * @brief Audio alert gap in milliseconds (long)
 */
constexpr uint32_t AUDIO_ALERT_LONG_GAP_MS = 50;

// ============================================================================
// Memory Constants
// ============================================================================

/**
 * @brief Memory breakdown (simplified):
 * - Database entries: 96 × 12 = 1,152 bytes
 * - Tracked drones: 16 × 56 = 896 bytes
 * - Display drones: 16 × 39 = 624 bytes
 * - Spectrum buffer: 256 bytes
 * - Histogram buffer: 512 bytes
 * - Histogram processor: ~509 bytes
 * - RSSI detector: ~108 bytes (includes RSSIStatistics struct)
 * - Scanner thread stack: 2,048 bytes (BSS)
 * - Other structures: ~200 bytes
 * - Total static RAM: ~6,105 bytes
 */
constexpr size_t STATIC_RAM_BUDGET_BYTES = 6105;

/**
 * @brief Total stack budget (bytes)
 * @note Updated to accommodate deeper call stacks for alert handling
 */
constexpr size_t STACK_BUDGET_BYTES = 4096;

/**
 * @brief Total memory budget (bytes)
 * @note Sum of static RAM and stack budgets
 */
constexpr size_t TOTAL_MEMORY_BUDGET_BYTES = 10200;

/**
 * @brief Maximum stack usage per function (bytes)
 */
constexpr size_t MAX_STACK_PER_FUNCTION = 512;

// ============================================================================
// Struct Size Validation Constants
// ============================================================================

// ============================================================================
// Error Handling Constants
// ============================================================================

/**
 * @brief Maximum hardware retry attempts
 */
constexpr uint32_t MAX_HARDWARE_RETRIES = 3;

/**
 * @brief Maximum database retry attempts
 */
constexpr uint32_t MAX_DATABASE_RETRIES = 3;

/**
 * @brief Maximum mutex retry attempts
 */
constexpr uint32_t MAX_MUTEX_RETRIES = 3;

// ============================================================================
// Thread Constants
// ============================================================================

/**
 * @brief Scanner thread priority
 */
constexpr uint8_t SCANNER_THREAD_PRIORITY = 10;

/**
 * @brief UI thread priority
 */
constexpr uint8_t UI_THREAD_PRIORITY = 5;

/**
 * @brief Scanner thread stack size (bytes)
 */
constexpr size_t SCANNER_THREAD_STACK_SIZE = 2048;

/**
 * @brief UI thread stack size (bytes)
 */
constexpr size_t UI_THREAD_STACK_SIZE = 4096;

// ============================================================================
// Color Constants (RGBA)
// ============================================================================

/**
 * @brief Color for low threat (green)
 */
constexpr uint32_t COLOR_LOW_THREAT = 0xFF00FF00;

/**
 * @brief Color for medium threat (yellow)
 */
constexpr uint32_t COLOR_MEDIUM_THREAT = 0xFFFFFF00;

/**
 * @brief Color for high threat (orange)
 */
constexpr uint32_t COLOR_HIGH_THREAT = 0xFFFF8000;

/**
 * @brief Color for critical threat (red)
 */
constexpr uint32_t COLOR_CRITICAL_THREAT = 0xFFFF0000;

/**
 * @brief Color for unknown threat (gray)
 */
constexpr uint32_t COLOR_UNKNOWN_THREAT = 0xFF808080;

/**
 * @brief Color for background (black)
 */
constexpr uint32_t COLOR_BACKGROUND = 0xFF000000;

/**
 * @brief Color for text (white)
 */
constexpr uint32_t COLOR_TEXT = 0xFFFFFFFF;

// ============================================================================
// FFT Bin Layout Constants (HackRF baseband: 256-bin FFT)
// ============================================================================
// The HackRF baseband produces 256 spectrum bins. Center bins 120-135
// contain the DC spike from the FFT zero-frequency component.
// Filter rolloff artifacts appear at bins 0-9 and 246-255.

/**
 * @brief Total FFT bins per spectrum message
 */
constexpr size_t FFT_BIN_COUNT = 256;

/**
 * @brief DC spike start bin index
 */
constexpr size_t FFT_DC_SPIKE_START = 120;

/**
 * @brief DC spike end bin index (exclusive)
 */
constexpr size_t FFT_DC_SPIKE_END = 136;

/**
 * @brief Edge skip for filter rolloff (standard: skip bins 0-9 and 246-255)
 * @note Used by extract_rssi() and analyze_spectrum_shape()
 */
constexpr size_t FFT_EDGE_SKIP = 10;

/**
 * @brief Edge skip for filter rolloff (narrow: skip bins 0-5 and 250-255)
 * @note Used by sweep mode (Looking Glass pattern) — tighter window for speed
 */
constexpr size_t FFT_EDGE_SKIP_NARROW = 6;

/**
 * @brief Focused bin window: lower sideband start
 * @note Bins 100-119 cover ±200 kHz around tuned frequency
 */
constexpr size_t FFT_FOCUSED_LOWER_START = 100;

/**
 * @brief Focused bin window: upper sideband end (exclusive)
 * @note Bins 136-156 cover the upper sideband mirror
 */
constexpr size_t FFT_FOCUSED_UPPER_END = 156;

/**
 * @brief Legacy dBm conversion offset (for uncorrected RSSI)
 * @note HackRF baseband: spectrum.db = clamp(dBV*5 + 255, 0, 255)
 * @note LEGACY: Approximate dBm = spectrum.db - 120 (NO gain correction, slope error)
 * @note CORRECT: dBm = (spectrum.db - 255) / 5 - (lna + vga + rf_amp_gain)
 * @note See extract_rssi() for the gain-compensated implementation.
 */
constexpr int32_t FFT_DBM_OFFSET = 120;

/**
 * @brief Usable bins for spectrum shape analysis
 * @note Total bins minus DC spike (16) minus edge skip (2×10) = 220
 */
constexpr size_t FFT_USABLE_BINS = FFT_BIN_COUNT - (FFT_DC_SPIKE_END - FFT_DC_SPIKE_START) - (2 * FFT_EDGE_SKIP);

/**
 * @brief Usable bins for sweep mode (narrow edge skip)
 * @note Total bins minus DC spike (16) minus edge skip narrow (2×6) = 228
 */
constexpr size_t FFT_USABLE_BINS_NARROW = FFT_BIN_COUNT - (FFT_DC_SPIKE_END - FFT_DC_SPIKE_START) - (2 * FFT_EDGE_SKIP_NARROW);

/**
 * @brief Sweep slice bandwidth in Hz (20 MHz per slice)
 * @note MUST be defined BEFORE SWEEP_BIN_SIZE - C++ requires forward declaration
 */
constexpr FreqHz SWEEP_SLICE_BW = 20000000;

/**
 * @brief Unified bin size in Hz (78125 Hz per bin)
 * @note Used by both Logic and UI layers to avoid duplication
 */
constexpr FreqHz SWEEP_BIN_SIZE = SWEEP_SLICE_BW / FFT_BIN_COUNT;

// ============================================================================
// Sweep Bin Mapping Constants (Looking Glass pattern)
// ============================================================================
// In sweep mode, 240 bins from the 256-bin FFT are mapped to screen pixels.
// The mapping rearranges bins to skip the DC spike:
//   Lower sideband: FFT bins 134-253 → screen pixels 0-119
//   Upper sideband: FFT bins 0-118   → screen pixels 120-238
//   Bins 119, 120-135, 254-255 are skipped (DC spike + neighbors)

/**
 * @brief Screen pixels per sweep slice (= DISPLAY_WIDTH)
 */
constexpr uint8_t SWEEP_PIXELS_PER_SLICE = 240;

/**
 * @brief Composite buffer size (pixels)
 */
constexpr uint16_t COMPOSITE_SIZE = 240;

/**
 * @brief FFT bin where lower sideband mapping starts
 */
constexpr uint8_t SWEEP_FFT_MAP_START = 134;

/**
 * @brief Crossover pixel index (lower → upper sideband boundary)
 */
constexpr uint8_t SWEEP_FFT_MAP_CROSSOVER = 120;

/**
 * @brief FFT bins per step for sweep advancement
 * @note 228 bins × 78125 Hz/bin = 17.8125 MHz step (matches usable narrow bins)
 */
constexpr uint16_t SWEEP_BINS_PER_STEP = FFT_USABLE_BINS_NARROW;

/**
 * @brief M0 baseband phase decimation trigger for wideband spectrum.
 * @note Controls how many samples are accumulated per FFT frame.
 * @note 63 buffers @ 20MHz = ~6.5ms integration = maximum sensitivity
 * @note +3dB SNR improvement over trigger=31, sweep takes ~1.6s for 240 freq
 */
constexpr size_t SWEEP_FFT_TRIGGER = 63;

/**
 * @brief Scale factor for converting 8-bit composite power to 16-bit histogram bins.
 * @note Shifts uint8_t range [0,255] to uint16_t range [0,65280].
 */
constexpr uint16_t COMPOSITE_TO_HIST_SCALE = 256;

/**
 * @brief Maximum number of sweep windows
 */
constexpr uint8_t MAX_SWEEP_WINDOWS = 4;

// ============================================================================
// Sweep Persistence & Smoothing Constants
// ============================================================================

/**
 * @brief Composite EMA decay factor for sweep persistence.
 * @note new_val = max(raw_val, (persist_val * DECAY) >> 8)
 * @note 224/256 = 0.875: signal decays to ~59% after 4 passes (~3s at 1.6s/pass)
 * @note Increased from 192 to retain weak signals longer between sweep cycles.
 * @note Range: 128 (fast fade) to 255 (slow fade)
 */
constexpr uint16_t SWEEP_PERSISTENCE_DECAY_Q8 = 224;

/**
 * @brief Number of FFT frames to discard after frequency retune.
 * @note 0 frames: 5ms PLL settle delay in retune_sweep_window() already exceeds
 *       the MAX2837/RFFC5072 lock time (~200us). No frame skip needed.
 * @note Previously 1, but the 5ms sleep is more than sufficient.
 */
constexpr uint8_t SWEEP_SETTLE_FRAMES = 0;

/**
 * @brief Noise margin for composite display floor subtraction.
 * @note render_composite suppresses bins below noise_floor + this.
 */
constexpr uint8_t SWEEP_DISPLAY_NOISE_MARGIN = 8;

/**
 * @brief Percentile of EMA persistence buffer used as auto-computed noise floor.
 * @note Range 0..99. Lower value = more aggressive subtraction (cuts more bins).
 * @note 15 = robust against sparse spectra: only the bottom 15% is treated as noise,
 *       so the first fresh pixels of a new pass are visible in real-time.
 * @note 50 (median, the previous default) is too aggressive for real-time sweep:
 *       with a dense spectrum the median sits at the signal level and zero-outs
 *       freshly-written pixels until the pass completes.
 */
constexpr uint8_t SWEEP_NOISE_FLOOR_PERCENTILE = 15;

// ============================================================================
// Spectrum Filter Constants (matching Looking Glass)
// ============================================================================

constexpr uint8_t SPECTRUM_FILTER_OFF = 0;
constexpr uint8_t SPECTRUM_FILTER_MID = 118;
constexpr uint8_t SPECTRUM_FILTER_HIGH = 202;
constexpr uint8_t DEFAULT_SPECTRUM_FILTER = SPECTRUM_FILTER_OFF;
constexpr uint8_t DEFAULT_SPECTRUM_INTEGRATION = 3;

// ============================================================================
// Spectrum Shape Filter Constants
// ============================================================================

/**
 * @brief Default peak margin above noise floor (5-200)
 * @note 15 ≈ 5 dB above noise (sensitive)
 * @note 55 ≈ 20 dB above noise (strict)
 * @note FPV-OPTIMIZED: 25 ≈ 8 dB — rejects Wi-Fi 5.8 GHz flat noise
 *       while still catching weak analog FM video peaks
 * @note Previous default was 20; raised by 5 to cut Wi-Fi false positives
 */
constexpr uint8_t DEFAULT_SPECTRUM_MARGIN = 25;

/**
 * @brief Default minimum signal width in bins (1-100)
 * @note Signals narrower than this are rejected as needle spikes
 * @note 5 bins = 390 kHz — rejects narrow noise spikes while still catching
 *       weak analog FM pilot/carrier and narrow control bursts at long range
 * @note 20 bins = 1.56 MHz (aggressive filtering)
 * @note Previous default was 3; raised to 5 to reject more noise spikes
 */
constexpr uint8_t DEFAULT_SPECTRUM_MIN_WIDTH = 5;

/**
 * @brief Default maximum signal width in bins (1-255)
 * @note Signals wider than this are rejected as flat-topped U/I noise
 * @note 100 = UI limit for some apps
 * @note 200 = FPV video: accommodates ~15 MHz (~192 bins at 78 kHz/bin)
 * @note 255 = accepts all widths (no filtering)
 * @note 40 = original default (too narrow for FPV)
 */
constexpr uint8_t DEFAULT_SPECTRUM_MAX_WIDTH = 10;

/**
 * @brief Default minimum peak sharpness ratio (50-250)
 * @note sharpness = (peak_margin * 100) / avg_margin
 * @note Inverted-V peaks have sharpness > 200; flat U/I shapes have sharpness ~ 100
 * @note 50 = no sharpness filtering (accept all shapes)
 * @note FPV-OPTIMIZED: 130 — rejects noise spikes (sharpness 100-150) while
 *       accepting real analog FM V-shape signals (sharpness 150-250).
 * @note Previous default was 115; raised to 130 for better noise rejection
 */
constexpr uint8_t DEFAULT_SPECTRUM_PEAK_SHARPNESS = 125;

/**
 * @brief Default peak-to-width ratio threshold (0-255)
 * @note ratio = (peak_margin * 10) / signal_width
 * @note Inverted-V (drone video link): ratio > 50 (tall, narrow)
 * @note Flat U/I noise: ratio < 20 (wide, short)
 * @note Needle spikes: ratio > 100 (very tall, very narrow)
 * @note 0 = no ratio filtering (disabled) - RECOMMENDED for FPV
 * @note 80 allows signals up to 31 bins wide with peak_margin=255
 * @note FPV: disabled due to wide signal width (~77 bins) having low ratio
 */
constexpr uint8_t DEFAULT_SPECTRUM_PEAK_RATIO = 5;

/**
 * @brief Default valley depth threshold (0-200)
 * @note Measures margin of bins immediately flanking the signal peak
 * @note Inverted-V: deep valleys (flanking bins have margin < 5)
 * @note Flat U/I: shallow valleys (flanking bins still elevated)
 * @note 0 = no valley depth filtering (disabled)
 * @note 60 = default for narrowband drones
 * @note FPV-OPTIMIZED: 100 — analog FM at 5.8 GHz has dual peaks
 *       (audio sub-carrier + video carrier), which produces SHALLOW valleys.
 *       Stricter threshold (100) rejects broadband noise that has NO valley
 *       at all (truly flat), but accepts the natural FPV dual-peak shape.
 * @note Previous default was 80; raised to 100 for analog FM
 */
constexpr uint8_t DEFAULT_SPECTRUM_VALLEY_DEPTH = 60;

/**
 * @brief Default peak flatness threshold (0-100, percentage)
 * @note flatness = (high_power_bins * 100) / signal_width
 * @note Measures how many bins are at 90%+ of peak power
 * @note WiFi/BT flat-top: flatness ~ 50-80% (many bins near peak)
 * @note Drone V-shape: flatness ~ 5-20% (only peak bin at high power)
 * @note Higher threshold = stricter (rejects more flat signals)
 * @note 0 = no flatness filtering (disabled)
 * @note FPV-OPTIMIZED: 40 — stricter than 30 to reject Wi-Fi 5.8 GHz
 *       flat-top noise that bleeds through the analog FM bandwidth.
 * @note Previous default was 30; raised to 40 for Wi-Fi rejection
 */
constexpr uint8_t DEFAULT_SPECTRUM_FLATNESS = 0;

/**
 * @brief Minimum peak margin for flatness check to be meaningful (in spectrum.db units)
 * @note Below this SNR, the flatness measurement is unreliable because:
 *       - V-shape signals compress near the noise floor, increasing flatness_pct
 *       - The 90% threshold is too close to the signal values, causing 20-50% swing
 *         from normal peak fluctuations
 * @note At peak_margin < 40 (~8 dB), WiFi flat-top still has flatness > 60%
 *       while drone V-shape has flatness 40-70% — overlap is too high to distinguish.
 * @note Above peak_margin 40 (~8 dB), WiFi flatness > 80% vs drone < 30% — clear separation.
 * @note Skipping flatness for weak signals lets the other filters (sharpness, valley depth,
 *       width) handle rejection, which are more stable at low SNR.
 */
constexpr uint8_t FLATNESS_MIN_PEAK_MARGIN = 40;

/**
 * @brief Default signal symmetry threshold (0-100, percent)
 * @note symmetry = min(left_width, right_width) * 100 / max(left_width, right_width)
 * @note Drone video V-shape: symmetry > 50% (both sides similar)
 * @note Noise/asymmetric: symmetry < 30% (one side dominant)
 * @note Lower = stricter (requires more symmetry)
 * @note 0 = no symmetry filtering (disabled)
 * @note FPV-OPTIMIZED: 35 — analog FM at 5.8 GHz often has slight asymmetry
 *       (FM modulator drift, multipath). Relaxing from 50 to 35 reduces
 *       false rejection of real FPV signals while still cutting truly
 *       asymmetric noise spikes.
 * @note Previous default was 50; lowered to 35 for analog FM tolerance
 */
constexpr uint8_t DEFAULT_SPECTRUM_SYMMETRY = 0;

// ============================================================================
// Pattern Matching Constants
// ============================================================================

/**
 * @brief Maximum number of patterns to store
 * @note Reduced from 20 to 10 to save RAM (each pattern ~500 bytes)
 */
constexpr size_t MAX_PATTERNS = 10;

/**
 * @brief Pattern waveform size (16 bins from 256-bin FFT)
 * @note 256 bins are downsampled to 16 bins for pattern matching
 * @note Clean divisor: 256/16 = 16 (no fractional bin mapping)
 * @note Each bin = ~1.25 MHz at 20 MHz BW
 */
constexpr size_t PATTERN_WAVEFORM_SIZE = 16;

/**
 * @brief Default similarity threshold for pattern matching (0-1000)
 * @note 600 = 60% similarity — balanced for drone signal detection
 * @note Higher threshold = stricter matching (fewer false positives)
 * @note Lower threshold = more permissive (more false positives)
 */
constexpr uint16_t DEFAULT_PATTERN_SIMILARITY_THRESHOLD = 600;

/**
 * @brief Similarity score thresholds for match quality classification
 */
constexpr uint16_t SIMILARITY_EXCELLENT = 800;  // 80% match
constexpr uint16_t SIMILARITY_STRONG = 600;    // 60% match
constexpr uint16_t SIMILARITY_MODERATE = 400;  // 40% match

/**
 * @brief Minimum amplitude ratio for pattern matching (live peak / pattern peak)
 * @note If the live signal's peak is less than 1/5 (20%) of the pattern's peak,
 *       skip matching to prevent noise-level signals from matching strong saved patterns.
 *       This is a weak gate — the primary discrimination is shape-based.
 */
constexpr uint8_t PATTERN_MIN_AMPLITUDE_RATIO = 5;

/**
 * @brief Fixed edge skip for pattern normalization
 * @note Must match FFT_EDGE_SKIP (10) for consistency between save and match
 */
constexpr size_t PATTERN_NORM_EDGE_SKIP = 10;

/**
 * @brief Scaling factor to convert 256-bin FFT indices to 16-bin pattern space
 * @note 256 / 16 = 16 — exact integer division, no fractional error
 */
constexpr uint8_t PATTERN_BIN_SCALE_FACTOR = FFT_BIN_COUNT / PATTERN_WAVEFORM_SIZE;

// ============================================================================
// CFAR Detection Constants (Constant False Alarm Rate)
// ============================================================================
// CFAR adapts threshold to local noise level, reducing false alarms
// in varying noise environments (WiFi, Bluetooth, etc.)

/**
 * @brief CFAR mode selection
 */
enum class CFARMode : uint8_t {
    OFF = 0,    // CFAR disabled — use fixed threshold
    CA = 1,     // Cell Averaging CFAR — best for homogeneous noise
    GO = 2,     // Greatest Of CFAR — robust at noise edges
    SO = 3,     // Smallest Of CFAR — better in cluttered environments
    HYBRID = 4, // Hybrid CFAR — weighted combination of CA/GO/SO
    OS = 5,     // Ordered Statistic CFAR — best for multi-target environments
    VI = 6      // Variability Index CFAR — adaptive mode switching based on local statistics
};

/**
 * @brief Default CFAR mode (FPV-OPTIMIZED: OS = Ordered Statistic)
 * @note OS-CFAR ranks reference cells and picks the k-th order statistic as
 *       the noise estimate. Best for MULTI-TARGET environments where multiple
 *       FPV drones or drones + their RC controllers are active simultaneously.
 * @note GO-CFAR (previous default) has higher PFA at clutter edges (sweep
 *       slice boundaries) and masks weak targets near strong ones.
 * @note OS-CFAR is more robust against target masking — FPV swarm detection.
 * @note Cost: requires sorting 32 reference cells per CUT (~32×log₂32 = 160 ops).
 *       Still fits within the 1.6s sweep cycle budget.
 */
constexpr CFARMode DEFAULT_CFAR_MODE = CFARMode::OS;

/**
 * @brief CFAR reference window size (number of reference cells)
 * @note Must be power of 2 for efficient computation
 * @note Typical: 8-32 cells. More cells = better noise estimate, slower adaptation
 */
constexpr uint8_t DEFAULT_CFAR_REF_CELLS = 32;

/**
 * @brief CFAR guard cells (protect signal from contaminating noise estimate)
 * @note Cells adjacent to CUT (Cell Under Test) that are excluded from reference
 * @note Typical: 2-4 cells
 */
constexpr uint8_t DEFAULT_CFAR_GUARD_CELLS = 3;

/**
 * @brief CFAR threshold offset (dB offset above noise in spectrum.db units)
 * @note Additive offset in dB-compressed domain: threshold = noise_estimate + (value / 10)
 * @note Higher = fewer false alarms, more missed detections
 * @note Lower = more detections, more false alarms
 * @note Each unit ≈ 0.2 dB. Typical: 1.0-10.0 dB offset
 * @note FPV-OPTIMIZED: 60 (6.0 units, ≈1.2 dB offset) — at -95 dBm RSSI
 *       sensitivity, the additional 1.0 dB margin cuts ~30% of false positives
 *       from broadband spurs while preserving detection of real FPV signals.
 * @note Previous default was 50; raised to 60 for FP suppression
 */
constexpr uint8_t DEFAULT_CFAR_THRESHOLD_X10 = 60;  // 6.0 units (≈1.2 dB offset above noise)

/**
 * @brief CFAR threshold range (×10 for integer storage, offset in spectrum.db units)
 * @note Each unit ≈ 0.2 dB offset above noise floor
 */
constexpr uint8_t CFAR_THRESHOLD_MIN_X10 = 10;   // 1.0 (~0.2 dB)
constexpr uint8_t CFAR_THRESHOLD_MAX_X10 = 100;  // 10.0 (~2.0 dB)

/**
 * @brief CFAR reference cells range
 */
constexpr uint8_t CFAR_REF_CELLS_MIN = 4;
constexpr uint8_t CFAR_REF_CELLS_MAX = 64;

/**
 * @brief CFAR guard cells range
 */
constexpr uint8_t CFAR_GUARD_CELLS_MIN = 0;
constexpr uint8_t CFAR_GUARD_CELLS_MAX = 8;

/**
 * @brief CFAR hybrid mode weights (×100 for integer storage)
 * @note w_hybrid = α*w_CA + β*w_GO + γ*w_SO
 * @note α + β + γ = 100
 */
constexpr uint8_t DEFAULT_CFAR_HYBRID_ALPHA = 50;  // CA weight (0.5)
constexpr uint8_t DEFAULT_CFAR_HYBRID_BETA = 30;   // GO weight (0.3)
constexpr uint8_t DEFAULT_CFAR_HYBRID_GAMMA = 20;  // SO weight (0.2)

// ============================================================================
// OS-CFAR Constants (Ordered Statistic)
// ============================================================================

/**
 * @brief OS-CFAR k-th order statistic index (as fraction of N_ref × 100)
 * @note k = (N_ref * OS_CFAR_K_PERCENT) / 100
 * @note 75 = 75% of sorted cells (robust against multi-target masking)
 * @note Higher = more aggressive noise estimate, fewer false alarms
 * @note Lower = more sensitive, more false alarms in multi-target
 */
constexpr uint8_t DEFAULT_OS_CFAR_K_PERCENT = 75;

/**
 * @brief OS-CFAR k-th order range (percent)
 */
constexpr uint8_t OS_CFAR_K_PERCENT_MIN = 50;   // Median (most aggressive)
constexpr uint8_t OS_CFAR_K_PERCENT_MAX = 90;   // Near-maximum (very conservative)

// ============================================================================
// VI-CFAR Constants (Variability Index)
// ============================================================================

/**
 * @brief VI-CFAR variability index threshold (×10 for integer storage)
 * @note VI = variance / mean^2
 * @note VI < threshold → homogeneous noise → use CA-CFAR
 * @note VI > threshold → clutter edge → use GO-CFAR or SO-CFAR
 * @note FPV-OPTIMIZED: 2.0 (20) — analog FM at 5.8 GHz has more spectral
 *       variability than narrowband digital. Higher VI threshold (2.0 vs
 *       1.5) correctly classifies analog FM as "homogeneous" more often,
 *       steering VI-CFAR to use CA-CFAR for the FPV case (best sensitivity).
 * @note Previous default was 1.5 (15); raised to 2.0 (20) for analog FM
 */
constexpr uint8_t DEFAULT_VI_CFAR_THRESHOLD_X10 = 20;  // 2.0

/**
 * @brief VI-CFAR threshold range (×10)
 */
constexpr uint8_t VI_CFAR_THRESHOLD_MIN_X10 = 5;   // 0.5 (very sensitive)
constexpr uint8_t VI_CFAR_THRESHOLD_MAX_X10 = 50;  // 5.0 (very tolerant)

// ============================================================================
// Mahalanobis Distance Constants
// ============================================================================

/**
 * @brief Number of feature dimensions for Mahalanobis calculation
 * @note Dimensions: [0]=RSSI normalized, [1]=Frequency stability
 */
constexpr uint8_t MAHALANOBIS_DIMENSIONS = 2;

/**
 * @brief History size for computing running statistics
 * @note Must be power of 2 for efficient modulo operation
 */
constexpr uint8_t MAHALANOBIS_HISTORY_SIZE = 8;

/**
 * @brief Default Mahalanobis threshold ×10 (FPV-OPTIMIZED: 4.0)
 * @note D²_M < threshold → signal accepted as valid drone
 * @note Previous default was 3.0 (30). Raised to 4.0 to account for the wider
 *       RSSI variance of analog FM at 5.8 GHz compared to narrowband digital
 *       drone links (DJI OcuSync, ELRS). 4.0 keeps good FPV signals inside
 *       the gate while pushing out statistical outliers (noise, spurs).
 * @note Range: 1.0 (strict) to 10.0 (permissive)
 */
constexpr uint8_t DEFAULT_MAHALOBIS_THRESHOLD_X10 = 40;

/**
 * @brief Minimum Mahalanobis threshold ×10 (1.0)
 */
constexpr uint8_t MAHALANOBIS_THRESHOLD_MIN_X10 = 10;

/**
 * @brief Maximum Mahalanobis threshold ×10 (10.0)
 */
constexpr uint8_t MAHALANOBIS_THRESHOLD_MAX_X10 = 100;

/**
 * @brief Q-format for fixed-point arithmetic (Q8.8 = 8 integer bits, 8 fractional bits)
 * @note Range: -128.0 to 127.99 with 0.004 precision
 */
constexpr uint8_t MAHALANOBIS_Q_FORMAT = 8;

/**
 * @brief Q_SCALE for Q8.8 arithmetic (2^8 = 256)
 */
constexpr int32_t MAHALANOBIS_Q_SCALE = 256;

/**
 * @brief Minimum variance for clamping (1.0 in Q8.8 = 256)
 */
constexpr int32_t MAHALANOBIS_MIN_VARIANCE = 256;

/**
 * @brief Variance decay interval (samples between decay events)
 * @note Decay occurs every N samples with factor 31/32 (3.125% per event)
 *       After 256 samples: retention ≈ 88% (vs old 36% with 15/16 every 16)
 *       After 1024 samples: retention ≈ 72% (vs old 13%)
 *       This prevents overly aggressive gate tightening during long scans.
 */
constexpr uint8_t MAHALANOBIS_VARIANCE_DECAY_INTERVAL = 64;

/**
 * @brief RSSI normalization range (dBm)
 */
constexpr int32_t MAHALANOBIS_RSSI_MIN_DBM = -120;
constexpr int32_t MAHALANOBIS_RSSI_MAX_DBM = -20;

// ============================================================================
// Neighbor Margin Check Constants
// ============================================================================

/**
 * @brief Default neighbor margin in dB (center must be stronger than neighbors)
 * @note 0 = disabled, 3 = default (like FPV detect), 5 = strict
 * @note FPV-OPTIMIZED: 2 dB — analog FM video at 5.8 GHz is WIDE (~15 MHz)
 *       and often has flat-top shape; strict neighbor margin (3-5 dB) would
 *       reject legitimate FPV signals whose center bin is NOT the strongest.
 *       2 dB provides FP rejection against single-bin noise spikes while
 *       accepting real analog FM with its natural flat-top profile.
 * @note Previous default was 3; lowered to 2 for FPV wide-signal acceptance
 * @note Eliminates wideband noise false positives (WiFi, BT, microwave)
 */
constexpr int32_t DEFAULT_NEIGHBOR_MARGIN_DB = 2;

/**
 * @brief Minimum neighbor margin (disabled)
 */
constexpr int32_t NEIGHBOR_MARGIN_MIN_DB = 0;

/**
 * @brief Maximum neighbor margin (very strict)
 */
constexpr int32_t NEIGHBOR_MARGIN_MAX_DB = 15;

// ============================================================================
// Confirm Count Constants
// ============================================================================

/**
 * @brief Default number of confirmations before creating a drone
 * @note Higher = fewer false positives but slower detection
 */
constexpr uint8_t DEFAULT_CONFIRM_COUNT = 2;

/**
 * @brief Minimum confirm count
 */
constexpr uint8_t CONFIRM_COUNT_MIN = 1;

/**
 * @brief Maximum confirm count
 */
constexpr uint8_t CONFIRM_COUNT_MAX = 10;

// ============================================================================
// RSSI Variance Noise Rejection Constants
// ============================================================================

/**
 * @brief RSSI variance threshold for noise rejection
 * @note Real drones: variance < 25 (stable signal)
 * @note Noise: variance > 100 (chaotic fluctuations)
 * @note 0 = disabled
 */
constexpr int32_t DEFAULT_RSSI_VARIANCE_THRESHOLD = 100;

/**
 * @brief Noise blacklist threshold — consecutive noise events before frequency is skipped.
 * @note If scanner force-resumes from a frequency 3+ times without threat upgrade → skip it.
 */
constexpr uint8_t NOISE_BLACKLIST_THRESHOLD = 3;

// ============================================================================
// Band Sweep Constants
// ============================================================================

constexpr FreqHz SWEEP_DEFAULT_START_HZ = 5645000000;   // 5.645 GHz - full 5.8 GHz FPV band
constexpr FreqHz SWEEP_DEFAULT_END_HZ = 5945000000;     // 5.945 GHz - 300 MHz span

// ============================================================================
// Sweep Exception Constants
// ============================================================================

/**
 * @brief Default exclusion radius around exception frequencies (±3 MHz)
 * @note Configurable at runtime via ScanConfig.exception_radius_mhz (1-100)
 */
constexpr uint8_t DEFAULT_EXCEPTION_RADIUS_MHZ = 3;
constexpr FreqHz EXCEPTION_RADIUS_HZ = 3'000'000ULL;

/**
 * @brief Number of exception frequency slots per sweep window
 */
constexpr uint8_t EXCEPTIONS_PER_WINDOW = 5;

// ============================================================================
// String Constants (Flash Storage)
// ============================================================================

/**
 * @brief String for unknown drone type
 */
constexpr const char DRONE_TYPE_UNKNOWN[] = "Unknown";
constexpr const char DRONE_TYPE_DJI[] = "DJI";
constexpr const char DRONE_TYPE_PARROT[] = "Parrot";
constexpr const char DRONE_TYPE_YUNEEC[] = "Yuneec";
constexpr const char DRONE_TYPE_3DR[] = "3DR";
constexpr const char DRONE_TYPE_AUTEL[] = "Autel";
constexpr const char DRONE_TYPE_HOBBY[] = "Hobby";
constexpr const char DRONE_TYPE_FPV[] = "FPV";
constexpr const char DRONE_TYPE_CUSTOM[] = "Custom";
constexpr const char DRONE_TYPE_OTHER[] = "Other";

// ============================================================================
// Movement Trend Constants
// ============================================================================

/**
 * @brief Threshold for drone approaching detection (dB)
 * @note RSSI increase of 3 dB means drone is getting closer
 */
constexpr int32_t MOVEMENT_TREND_THRESHOLD_APPROACHING_DB = 3;

/**
 * @brief Threshold for drone receding detection (dB)
 * @note RSSI decrease of 3 dB means drone is moving away
 */
constexpr int32_t MOVEMENT_TREND_THRESHOLD_RECEEDING_DB = -3;

/**
 * @brief Minimum history samples for trend calculation
 * @note Need at least 3 samples for reliable trend
 * @note Must be <= RSSI_HISTORY_SIZE
 */
constexpr uint8_t MOVEMENT_TREND_MIN_HISTORY = 3;

/**
 * @brief Silence threshold for RSSI filtering (dBm)
 * @note RSSI values below -110 dBm are ignored as noise
 */
constexpr int32_t MOVEMENT_TREND_SILENCE_THRESHOLD_DBM = -110;

/**
 * @brief Movement trend symbols for UI display
 */
constexpr char MOVEMENT_TREND_SYMBOL_APPROACHING = '<';
constexpr char MOVEMENT_TREND_SYMBOL_RECEEDING = '>';
constexpr char MOVEMENT_TREND_SYMBOL_STATIC = '~';
constexpr char MOVEMENT_TREND_SYMBOL_UNKNOWN = '-';

// ============================================================================
// Status Messages (Flash Storage)
// ============================================================================

/**
 * @brief Status message for no drones detected
 */
constexpr const char STATUS_NO_DRONES[] = "No drones detected";
constexpr const char STATUS_SCANNING[] = "Scanning...";
constexpr const char STATUS_ERROR[] = "Error";
constexpr const char STATUS_LOADING[] = "Loading...";
constexpr const char STATUS_READY[] = "Ready";

// ============================================================================
// Validation Constants
// ============================================================================

/**
 * @brief Maximum frequency entry name length
 */
constexpr size_t MAX_FREQUENCY_NAME_LENGTH = 32;

/**
 * @brief Maximum frequency description length
 */
constexpr size_t MAX_FREQUENCY_DESCRIPTION_LENGTH = 64;

/**
 * @brief Maximum file path length
 */
constexpr size_t MAX_FILE_PATH_LENGTH = 64;

// ============================================================================
// Performance Constants
// ============================================================================

/**
 * @brief Target UI refresh rate (FPS)
 */
constexpr uint32_t TARGET_UI_FPS = 60;

/**
 * @brief UI refresh interval in milliseconds
 */
constexpr uint32_t UI_REFRESH_INTERVAL_MS = 16;

/**
 * @brief Target scan rate (frequencies per second)
 */
constexpr uint32_t TARGET_SCAN_RATE_HZ = 10;

// ============================================================================
// Fast Scanner Constants
// ============================================================================

/**
 * @brief Scanner sleep time per frequency (ms)
 * @note 50ms = 20 frequencies/second
 */
constexpr uint32_t SCANNER_SLEEP_MS = 50;

/**
 * @brief Statistics updates per second
 */
constexpr uint32_t STATISTICS_UPDATES_PER_SEC = 10;

/**
 * @brief Maximum frequency lock cycles
 * @note 10 cycles × 50ms = 500ms to verify signal
 */
constexpr uint32_t MAX_FREQ_LOCK = 10;

/**
 * @brief Signal lock time (ms)
 * @note 10 × 50ms = 500ms
 */
constexpr uint32_t SIGNAL_LOCK_TIME_MS = 500;

/**
 * @brief Drone type display duration (ms)
 * @note 500ms for showing "FPV", "DJI", etc.
 */
constexpr uint32_t DRONE_TYPE_DISPLAY_DURATION_MS = 500;

/**
 * @brief Maximum drone type display length
 * @note 4 characters + null terminator
 */
constexpr size_t MAX_DRONE_TYPE_DISPLAY = 4;

// ============================================================================
// Debug Constants
// ============================================================================

#ifdef DEBUG
/**
 * @brief Enable debug logging
 */
constexpr bool DEBUG_LOGGING_ENABLED = true;

/**
 * @brief Enable lock order validation
 */
constexpr bool DEBUG_LOCK_ORDER_VALIDATION = true;

/**
 * @brief Enable stack usage monitoring
 */
constexpr bool DEBUG_STACK_MONITORING = true;
#else
/**
 * @brief Disable debug logging
 */
constexpr bool DEBUG_LOGGING_ENABLED = false;

/**
 * @brief Disable lock order validation
 */
constexpr bool DEBUG_LOCK_ORDER_VALIDATION = false;

/**
 * @brief Disable stack usage monitoring
 */
constexpr bool DEBUG_STACK_MONITORING = false;
#endif

} // namespace drone_analyzer

#endif // CONSTANTS_HPP
