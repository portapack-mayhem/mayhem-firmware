#include <cstdint>
#include <cstring>

#include "settings_manager.hpp"
#include "scanner.hpp"
#include "file.hpp"
#include "file_path.hpp"
#include "receiver_model.hpp"
#include "portapack.hpp"

namespace drone_analyzer {

// ============================================================================
// SettingsStruct Implementation
// ============================================================================

SettingsStruct::SettingsStruct() noexcept
    : scanning_mode(DEFAULT_SCANNING_MODE)
    , scan_interval_ms(SCAN_CYCLE_INTERVAL_MS)
    , scan_sensitivity(85)
    , alert_rssi_threshold_dbm(RSSI_DETECTION_THRESHOLD_DBM)
    , threat_low_dbm(DEFAULT_THREAT_LOW_DBM)
    , threat_medium_dbm(DEFAULT_THREAT_MEDIUM_DBM)
    , threat_high_dbm(RSSI_HIGH_THREAT_THRESHOLD_DBM)
    , threat_critical_dbm(RSSI_CRITICAL_THREAT_THRESHOLD_DBM)
    , spectrum_visible(true)
    , histogram_visible(true)
    , audio_alerts_enabled(true)
    , dwell_enabled(true)
    , confirm_count_enabled(true)
    , noise_blacklist_enabled(true)
    , spectrum_detection_enabled(true)
    , median_enabled(true)
    , spectrum_margin(DEFAULT_SPECTRUM_MARGIN)
    , spectrum_min_width(DEFAULT_SPECTRUM_MIN_WIDTH)  // 5 bins = 390 kHz
    , spectrum_max_width(DEFAULT_SPECTRUM_MAX_WIDTH)
    , spectrum_peak_sharpness(DEFAULT_SPECTRUM_PEAK_SHARPNESS)  // 130 rejects noise spikes
    , spectrum_peak_ratio(DEFAULT_SPECTRUM_PEAK_RATIO)
    , spectrum_valley_depth(DEFAULT_SPECTRUM_VALLEY_DEPTH)
    , spectrum_flatness(DEFAULT_SPECTRUM_FLATNESS)
    , spectrum_symmetry(DEFAULT_SPECTRUM_SYMMETRY)
    , cfar_mode(DEFAULT_CFAR_MODE)
    , cfar_ref_cells(DEFAULT_CFAR_REF_CELLS)
    , cfar_guard_cells(DEFAULT_CFAR_GUARD_CELLS)
    , cfar_threshold_x10(DEFAULT_CFAR_THRESHOLD_X10)
    , cfar_hybrid_alpha(DEFAULT_CFAR_HYBRID_ALPHA)
    , cfar_hybrid_beta(DEFAULT_CFAR_HYBRID_BETA)
    , cfar_hybrid_gamma(DEFAULT_CFAR_HYBRID_GAMMA)
    , os_cfar_k_percent(DEFAULT_OS_CFAR_K_PERCENT)
    , vi_cfar_threshold_x10(DEFAULT_VI_CFAR_THRESHOLD_X10)
    , neighbor_margin_db(DEFAULT_NEIGHBOR_MARGIN_DB)
    , rssi_variance_enabled(true)
    , confirm_count(DEFAULT_CONFIRM_COUNT)
    , mahalanobis_enabled(true)
    , mahalanobis_threshold_x10(DEFAULT_MAHALOBIS_THRESHOLD_X10)
    , pattern_matching_enabled(true)
    , sweep_start_freq(SWEEP_DEFAULT_START_HZ)
    , sweep_end_freq(SWEEP_DEFAULT_END_HZ)
    , sweep_step_freq(17813000)
    , sweep2_start_freq(2400000000ULL)
    , sweep2_end_freq(2500000000ULL)
    , sweep2_step_freq(17813000)
    , sweep2_enabled(false)
    , sweep3_start_freq(900000000ULL)
    , sweep3_end_freq(1000000000ULL)
    , sweep3_step_freq(17813000)
    , sweep3_enabled(false)
    , sweep4_start_freq(1200000000ULL)
    , sweep4_end_freq(1300000000ULL)
    , sweep4_step_freq(17813000)
    , sweep4_enabled(false) {
}

// ============================================================================
// Unified Parser — single implementation for all key=value parsing
// ============================================================================

static void parse_settings_line(
    const uint8_t* buf,
    size_t len,
    SettingsStruct& s
) noexcept {
    if (len == 0 || buf[0] == '#') return;

    size_t eq_pos = 0;
    for (size_t i = 0; i < len; ++i) {
        if (buf[i] == '=') { eq_pos = i; break; }
    }
    if (eq_pos == 0 || eq_pos >= len - 1) return;

    char key[32];
    size_t key_len = eq_pos;
    if (key_len > 31) key_len = 31;
    for (size_t i = 0; i < key_len; ++i) {
        key[i] = static_cast<char>(buf[i]);
    }
    key[key_len] = '\0';

    const uint8_t* val_start = buf + eq_pos + 1;
    size_t val_len = len - eq_pos - 1;

    auto key_matches = [key, key_len](const char* expected) -> bool {
        const size_t elen = __builtin_strlen(expected);
        return (key_len == elen) && __builtin_memcmp(key, expected, elen) == 0;
    };

    auto parse_int = [val_start, val_len]() -> uint64_t {
        uint64_t val = 0;
        for (size_t i = 0; i < val_len; ++i) {
            if (val_start[i] >= '0' && val_start[i] <= '9')
                val = val * 10ULL + static_cast<uint64_t>(val_start[i] - '0');
        }
        return val;
    };

    auto parse_bool = [val_start, val_len]() -> bool {
        return (val_len == 4 &&
                val_start[0] == 't' && val_start[1] == 'r' &&
                val_start[2] == 'u' && val_start[3] == 'e');
    };

    auto parse_signed_int = [val_start, val_len]() -> int32_t {
        const bool negative = (val_len > 0 && val_start[0] == '-');
        const uint8_t* num_start = negative ? val_start + 1 : val_start;
        const size_t num_len = negative ? val_len - 1 : val_len;
        uint32_t val = 0;
        for (size_t i = 0; i < num_len; ++i) {
            if (num_start[i] >= '0' && num_start[i] <= '9')
                val = val * 10 + (num_start[i] - '0');
        }
        return negative ? -static_cast<int32_t>(val) : static_cast<int32_t>(val);
    };

    // --- Scanning ---
    if (key_matches("scan_interval_ms")) {
        const uint64_t v = parse_int();
        s.scan_interval_ms = static_cast<uint32_t>((v < 10) ? 10 : (v > 10000 ? 10000 : v));
    } else if (key_matches("sensitivity")) {
        const int32_t sens = static_cast<int32_t>(parse_int());
        s.scan_sensitivity = static_cast<uint8_t>(sens > 100 ? 100 : (sens < 0 ? 0 : sens));
        s.alert_rssi_threshold_dbm = -20 - s.scan_sensitivity;
    } else if (key_matches("rssi_threshold_db")) {
        s.alert_rssi_threshold_dbm = parse_signed_int();
        // Derive scan_sensitivity to stay in sync with threshold
        const int32_t sens = -20 - s.alert_rssi_threshold_dbm;
        s.scan_sensitivity = static_cast<uint8_t>(sens > 100 ? 100 : (sens < 0 ? 0 : sens));

    } else if (key_matches("threat_low_db")) {
        const int32_t v = parse_signed_int();
        s.threat_low_dbm = (v < RSSI_MIN_DBM) ? RSSI_MIN_DBM : (v > RSSI_MAX_DBM) ? RSSI_MAX_DBM : v;
    } else if (key_matches("threat_medium_db")) {
        const int32_t v = parse_signed_int();
        s.threat_medium_dbm = (v < RSSI_MIN_DBM) ? RSSI_MIN_DBM : (v > RSSI_MAX_DBM) ? RSSI_MAX_DBM : v;
    } else if (key_matches("threat_high_db")) {
        const int32_t v = parse_signed_int();
        s.threat_high_dbm = (v < RSSI_MIN_DBM) ? RSSI_MIN_DBM : (v > RSSI_MAX_DBM) ? RSSI_MAX_DBM : v;
    } else if (key_matches("threat_critical_db")) {
        const int32_t v = parse_signed_int();
        s.threat_critical_dbm = (v < RSSI_MIN_DBM) ? RSSI_MIN_DBM : (v > RSSI_MAX_DBM) ? RSSI_MAX_DBM : v;

    // --- Audio / Display ---
    } else if (key_matches("enable_audio_alerts")) {
        s.audio_alerts_enabled = parse_bool();
    } else if (key_matches("volume")) {
        const uint64_t v = parse_int();
        s.volume = static_cast<uint8_t>((v > 99) ? 99 : v);
    } else if (key_matches("show_spectrum")) {
        s.spectrum_visible = parse_bool();
    } else if (key_matches("show_histogram")) {
        s.histogram_visible = parse_bool();

    // --- Detection features ---
    } else if (key_matches("spectrum_detection")) {
        s.spectrum_detection_enabled = parse_bool();
    } else if (key_matches("dwell_enabled")) {
        s.dwell_enabled = parse_bool();
    } else if (key_matches("confirm_count_enabled")) {
        s.confirm_count_enabled = parse_bool();
    } else if (key_matches("noise_blacklist_enabled")) {
        s.noise_blacklist_enabled = parse_bool();
    } else if (key_matches("median_enabled")) {
        s.median_enabled = parse_bool();

    // --- Spectrum shape filter (clamped to valid ranges) ---
    } else if (key_matches("spectrum_margin")) {
        const uint64_t v = parse_int();
        s.spectrum_margin = static_cast<uint8_t>((v < 5) ? 5 : (v > 200 ? 200 : v));
    } else if (key_matches("spectrum_min_width")) {
        const uint64_t v = parse_int();
        s.spectrum_min_width = static_cast<uint8_t>((v < 1) ? 1 : (v > 100 ? 100 : v));
    } else if (key_matches("spectrum_max_width")) {
        const uint64_t v = parse_int();
        s.spectrum_max_width = static_cast<uint8_t>((v < 2) ? 2 : (v > 255 ? 255 : v));
    } else if (key_matches("spectrum_peak_sharpness")) {
        const uint64_t v = parse_int();
        s.spectrum_peak_sharpness = static_cast<uint8_t>((v < 50) ? 50 : (v > 250 ? 250 : v));
    } else if (key_matches("spectrum_peak_ratio")) {
        const uint64_t v = parse_int();
        s.spectrum_peak_ratio = static_cast<uint8_t>((v > 255) ? 255 : v);
    } else if (key_matches("spectrum_valley_depth")) {
        const uint64_t v = parse_int();
        s.spectrum_valley_depth = static_cast<uint8_t>((v > 255) ? 255 : v);
    } else if (key_matches("spectrum_flatness")) {
        const uint64_t v = parse_int();
        s.spectrum_flatness = static_cast<uint8_t>((v > 100) ? 100 : v);
    } else if (key_matches("spectrum_symmetry")) {
        const uint64_t v = parse_int();
        s.spectrum_symmetry = static_cast<uint8_t>((v > 100) ? 100 : v);

    // --- Anti-false-positive ---
    } else if (key_matches("neighbor_margin_db")) {
        const int32_t v = static_cast<int32_t>(parse_int());
        s.neighbor_margin_db = (v < 0) ? 0 : (v > 15 ? 15 : v);
    } else if (key_matches("rssi_variance_enabled")) {
        s.rssi_variance_enabled = parse_bool();
    } else if (key_matches("confirm_count")) {
        const uint64_t v = parse_int();
        s.confirm_count = static_cast<uint8_t>(
            (v < CONFIRM_COUNT_MIN) ? CONFIRM_COUNT_MIN
            : (v > CONFIRM_COUNT_MAX ? CONFIRM_COUNT_MAX : v));

    // --- Sweep window 1 ---
    } else if (key_matches("sweep_start_mhz")) {
        s.sweep_start_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep_end_mhz")) {
        s.sweep_end_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep_step_khz")) {
        s.sweep_step_freq = static_cast<uint64_t>(parse_int()) * 1000ULL;

    // --- Sweep window 2 ---
    } else if (key_matches("sweep2_start_mhz")) {
        s.sweep2_start_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep2_end_mhz")) {
        s.sweep2_end_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep2_step_khz")) {
        s.sweep2_step_freq = static_cast<uint64_t>(parse_int()) * 1000ULL;
    } else if (key_matches("sweep2_enabled")) {
        s.sweep2_enabled = parse_bool();

    // --- Sweep window 3 ---
    } else if (key_matches("sweep3_start_mhz")) {
        s.sweep3_start_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep3_end_mhz")) {
        s.sweep3_end_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep3_step_khz")) {
        s.sweep3_step_freq = static_cast<uint64_t>(parse_int()) * 1000ULL;
    } else if (key_matches("sweep3_enabled")) {
        s.sweep3_enabled = parse_bool();

    // --- Sweep window 4 ---
    } else if (key_matches("sweep4_start_mhz")) {
        s.sweep4_start_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep4_end_mhz")) {
        s.sweep4_end_freq = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sweep4_step_khz")) {
        s.sweep4_step_freq = static_cast<uint64_t>(parse_int()) * 1000ULL;
    } else if (key_matches("sweep4_enabled")) {
        s.sweep4_enabled = parse_bool();

    // --- Sweep exceptions (5 slots per window) ---
    } else if (key_matches("sw1_exc0_mhz")) { s.sweep_exceptions[0][0] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw1_exc1_mhz")) { s.sweep_exceptions[0][1] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw1_exc2_mhz")) { s.sweep_exceptions[0][2] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw1_exc3_mhz")) { s.sweep_exceptions[0][3] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw1_exc4_mhz")) { s.sweep_exceptions[0][4] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw2_exc0_mhz")) { s.sweep_exceptions[1][0] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw2_exc1_mhz")) { s.sweep_exceptions[1][1] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw2_exc2_mhz")) { s.sweep_exceptions[1][2] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw2_exc3_mhz")) { s.sweep_exceptions[1][3] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw2_exc4_mhz")) { s.sweep_exceptions[1][4] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw3_exc0_mhz")) { s.sweep_exceptions[2][0] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw3_exc1_mhz")) { s.sweep_exceptions[2][1] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw3_exc2_mhz")) { s.sweep_exceptions[2][2] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw3_exc3_mhz")) { s.sweep_exceptions[2][3] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw3_exc4_mhz")) { s.sweep_exceptions[2][4] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw4_exc0_mhz")) { s.sweep_exceptions[3][0] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw4_exc1_mhz")) { s.sweep_exceptions[3][1] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw4_exc2_mhz")) { s.sweep_exceptions[3][2] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw4_exc3_mhz")) { s.sweep_exceptions[3][3] = static_cast<uint64_t>(parse_int()) * 1000000ULL;
    } else if (key_matches("sw4_exc4_mhz")) { s.sweep_exceptions[3][4] = static_cast<uint64_t>(parse_int()) * 1000000ULL;

    // --- Exception radius ---
    } else if (key_matches("exception_radius_mhz")) {
        const int32_t r = static_cast<int32_t>(parse_int());
        s.exception_radius_mhz = static_cast<uint8_t>(r > 100 ? 100 : (r < 1 ? 1 : r));

    // --- RSSI decrease cycles ---
    } else if (key_matches("rssi_decrease_cycles")) {
        const int32_t c = static_cast<int32_t>(parse_int());
        s.rssi_decrease_cycles = static_cast<uint8_t>(c > 50 ? 50 : (c < 1 ? 1 : c));

    // --- CFAR detection (clamped to valid ranges) ---
    } else if (key_matches("cfar_mode")) {
        const int32_t m = static_cast<int32_t>(parse_int());
        s.cfar_mode = static_cast<CFARMode>((m < 0 || m > 6) ? 0 : m);
    } else if (key_matches("cfar_ref_cells")) {
        const uint64_t v = parse_int();
        s.cfar_ref_cells = static_cast<uint8_t>((v < CFAR_REF_CELLS_MIN) ? CFAR_REF_CELLS_MIN : (v > CFAR_REF_CELLS_MAX ? CFAR_REF_CELLS_MAX : v));
    } else if (key_matches("cfar_guard_cells")) {
        const uint64_t v = parse_int();
        s.cfar_guard_cells = static_cast<uint8_t>((v < CFAR_GUARD_CELLS_MIN) ? CFAR_GUARD_CELLS_MIN : (v > CFAR_GUARD_CELLS_MAX ? CFAR_GUARD_CELLS_MAX : v));
    } else if (key_matches("cfar_threshold_x10")) {
        const uint64_t v = parse_int();
        s.cfar_threshold_x10 = static_cast<uint8_t>((v < CFAR_THRESHOLD_MIN_X10) ? CFAR_THRESHOLD_MIN_X10 : (v > CFAR_THRESHOLD_MAX_X10 ? CFAR_THRESHOLD_MAX_X10 : v));
    } else if (key_matches("cfar_hybrid_alpha")) {
        const uint64_t v = parse_int();
        s.cfar_hybrid_alpha = static_cast<uint8_t>((v > 100) ? 100 : v);
    } else if (key_matches("cfar_hybrid_beta")) {
        const uint64_t v = parse_int();
        s.cfar_hybrid_beta = static_cast<uint8_t>((v > 100) ? 100 : v);
    } else if (key_matches("cfar_hybrid_gamma")) {
        const uint64_t v = parse_int();
        s.cfar_hybrid_gamma = static_cast<uint8_t>((v > 100) ? 100 : v);
    } else if (key_matches("os_cfar_k_percent")) {
        const uint64_t v = parse_int();
        s.os_cfar_k_percent = static_cast<uint8_t>(
            (v < OS_CFAR_K_PERCENT_MIN) ? OS_CFAR_K_PERCENT_MIN :
            (v > OS_CFAR_K_PERCENT_MAX ? OS_CFAR_K_PERCENT_MAX : v));
    } else if (key_matches("vi_cfar_threshold_x10")) {
        const uint64_t v = parse_int();
        s.vi_cfar_threshold_x10 = static_cast<uint8_t>(
            (v < VI_CFAR_THRESHOLD_MIN_X10) ? VI_CFAR_THRESHOLD_MIN_X10 :
            (v > VI_CFAR_THRESHOLD_MAX_X10 ? VI_CFAR_THRESHOLD_MAX_X10 : v));
    } else if (key_matches("mahalanobis_enabled")) {
        s.mahalanobis_enabled = parse_bool();
    } else if (key_matches("mahalanobis_threshold_x10")) {
        const uint64_t v = parse_int();
        s.mahalanobis_threshold_x10 = static_cast<uint8_t>(
            (v < MAHALANOBIS_THRESHOLD_MIN_X10) ? MAHALANOBIS_THRESHOLD_MIN_X10
            : (v > MAHALANOBIS_THRESHOLD_MAX_X10 ? MAHALANOBIS_THRESHOLD_MAX_X10 : v));
    } else if (key_matches("pattern_matching_enabled")) {
        s.pattern_matching_enabled = parse_bool();
    }
}

// ============================================================================
// SettingsFileManager::load
// ============================================================================

ErrorCode SettingsFileManager::load(SettingsStruct& out) noexcept {
    File file;
    const auto error = file.open(settings_dir / u"eda_settings.txt", true, false);
    if (error) {
        return ErrorCode::DATABASE_NOT_LOADED;
    }

    constexpr size_t READ_CHUNK_SIZE = 256;
    uint8_t chunk[READ_CHUNK_SIZE];
    uint8_t line_buf[128];
    size_t line_len = 0;

    while (true) {
        const auto read_result = file.read(chunk, READ_CHUNK_SIZE);
        if (!read_result.is_ok() || read_result.value() == 0) break;

        const size_t bytes_read = read_result.value();
        for (size_t i = 0; i < bytes_read; ++i) {
            const char c = static_cast<char>(chunk[i]);
            if (c == '\r' || c == '\n') {
                parse_settings_line(line_buf, line_len, out);
                line_len = 0;
            } else if (line_len < sizeof(line_buf) - 1) {
                line_buf[line_len++] = chunk[i];
            }
        }
    }
    parse_settings_line(line_buf, line_len, out);

    // Load-time validation: fix inconsistent field relationships
    if (out.spectrum_min_width > out.spectrum_max_width) {
        out.spectrum_max_width = out.spectrum_min_width;
    }
    if (out.threat_low_dbm > out.threat_medium_dbm) {
        out.threat_medium_dbm = out.threat_low_dbm;
    }
    if (out.threat_medium_dbm > out.threat_high_dbm) {
        out.threat_high_dbm = out.threat_medium_dbm;
    }
    if (out.threat_high_dbm > out.threat_critical_dbm) {
        out.threat_critical_dbm = out.threat_high_dbm;
    }
    if (out.cfar_ref_cells < out.cfar_guard_cells + 2) {
        out.cfar_guard_cells = (out.cfar_ref_cells > 2) ? out.cfar_ref_cells - 2 : 0;
    }
    // Normalize CFAR hybrid weights to sum to 100
    const uint16_t hybrid_sum = static_cast<uint16_t>(
        out.cfar_hybrid_alpha + out.cfar_hybrid_beta + out.cfar_hybrid_gamma);
    if (hybrid_sum != 100 && hybrid_sum > 0) {
        out.cfar_hybrid_alpha = static_cast<uint8_t>(
            (out.cfar_hybrid_alpha * 100) / hybrid_sum);
        out.cfar_hybrid_beta = static_cast<uint8_t>(
            (out.cfar_hybrid_beta * 100) / hybrid_sum);
        out.cfar_hybrid_gamma = static_cast<uint8_t>(
            100 - out.cfar_hybrid_alpha - out.cfar_hybrid_beta);
    }

    file.close();
    return ErrorCode::SUCCESS;
}

// ============================================================================
// Direct file write helpers — no snprintf (platform-independent, stack-safe)
// ============================================================================

static void ws(File& f, const char* s) noexcept {
    f.write(s, __builtin_strlen(s));
}

static void wb(File& f, bool v) noexcept {
    ws(f, v ? "true\n" : "false\n");
}

static void wi(File& f, int64_t val) noexcept {
    char buf[24];
    uint8_t pos = 0;
    if (val < 0) { buf[pos++] = '-'; val = -val; }
    char dig[20];
    uint8_t n = 0;
    uint64_t uval = static_cast<uint64_t>(val);
    if (uval == 0) { dig[n++] = '0'; }
    else { while (uval > 0) { dig[n++] = '0' + static_cast<char>(uval % 10); uval /= 10; } }
    for (uint8_t i = n; i > 0; --i) buf[pos++] = dig[i - 1];
    buf[pos++] = '\n';
    f.write(buf, pos);
}

static void wl(File& f, const char* key, int64_t val) noexcept {
    ws(f, key);
    ws(f, "=");
    wi(f, val);
}

static void wbool(File& f, const char* key, bool val) noexcept {
    ws(f, key);
    ws(f, "=");
    wb(f, val);
}

static void wexc(File& f, const char* key, uint64_t hz) noexcept {
    if (hz == 0) return;
    wl(f, key, static_cast<int64_t>(hz / 1000000ULL));
}

// ============================================================================
// SettingsFileManager::save
// ============================================================================

ErrorCode SettingsFileManager::save(
    DroneScanner* scanner_ptr,
    const SettingsStruct& s
) noexcept {
    ScanConfig sweep_cfg;
    if (scanner_ptr != nullptr) {
        sweep_cfg = scanner_ptr->get_config();
    } else {
        sweep_cfg.sweep_start_freq = s.sweep_start_freq;
        sweep_cfg.sweep_end_freq = s.sweep_end_freq;
        sweep_cfg.sweep_step_freq = s.sweep_step_freq;
        sweep_cfg.sweep2_start_freq = s.sweep2_start_freq;
        sweep_cfg.sweep2_end_freq = s.sweep2_end_freq;
        sweep_cfg.sweep2_step_freq = s.sweep2_step_freq;
        sweep_cfg.sweep2_enabled = s.sweep2_enabled;
        sweep_cfg.sweep3_start_freq = s.sweep3_start_freq;
        sweep_cfg.sweep3_end_freq = s.sweep3_end_freq;
        sweep_cfg.sweep3_step_freq = s.sweep3_step_freq;
        sweep_cfg.sweep3_enabled = s.sweep3_enabled;
        sweep_cfg.sweep4_start_freq = s.sweep4_start_freq;
        sweep_cfg.sweep4_end_freq = s.sweep4_end_freq;
        sweep_cfg.sweep4_step_freq = s.sweep4_step_freq;
        sweep_cfg.sweep4_enabled = s.sweep4_enabled;
        for (uint8_t w = 0; w < 4; ++w) {
            for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
                sweep_cfg.sweep_exceptions[w][i] = s.sweep_exceptions[w][i];
            }
        }
    }

    File file;
    ensure_directory(settings_dir);
    // Write to temp file first, then rename — prevents corruption on power loss
    const auto create_error = file.create(settings_dir / u"eda_settings.txt.tmp");
    if (create_error) {
        return ErrorCode::INITIALIZATION_FAILED;
    }

    ws(file, "# Enhanced Drone Analyzer Settings\n");
    ws(file, "# Auto-generated - do not edit while app is running\n\n");

    // Scanning
    ws(file, "spectrum_mode=SEQUENTIAL\n");
    wl(file, "scan_interval_ms", static_cast<int64_t>(s.scan_interval_ms));
    wl(file, "sensitivity", static_cast<int64_t>(s.scan_sensitivity));
    wl(file, "rssi_threshold_db", static_cast<int64_t>(s.alert_rssi_threshold_dbm));
    wl(file, "threat_low_db", static_cast<int64_t>(s.threat_low_dbm));
    wl(file, "threat_medium_db", static_cast<int64_t>(s.threat_medium_dbm));
    wl(file, "threat_high_db", static_cast<int64_t>(s.threat_high_dbm));
    wl(file, "threat_critical_db", static_cast<int64_t>(s.threat_critical_dbm));

    // Audio / Display
    wbool(file, "enable_audio_alerts", s.audio_alerts_enabled);
    wl(file, "volume", static_cast<int64_t>(s.volume));
    wbool(file, "show_spectrum", s.spectrum_visible);
    wbool(file, "show_histogram", s.histogram_visible);

    // Detection features
    wbool(file, "spectrum_detection", s.spectrum_detection_enabled);
    wbool(file, "dwell_enabled", s.dwell_enabled);
    wbool(file, "confirm_count_enabled", s.confirm_count_enabled);
    wbool(file, "noise_blacklist_enabled", s.noise_blacklist_enabled);
    wbool(file, "median_enabled", s.median_enabled);

    // Spectrum shape filter
    wl(file, "spectrum_margin", static_cast<int64_t>(s.spectrum_margin));
    wl(file, "spectrum_min_width", static_cast<int64_t>(s.spectrum_min_width));
    wl(file, "spectrum_max_width", static_cast<int64_t>(s.spectrum_max_width));
    wl(file, "spectrum_peak_sharpness", static_cast<int64_t>(s.spectrum_peak_sharpness));
    wl(file, "spectrum_peak_ratio", static_cast<int64_t>(s.spectrum_peak_ratio));
    wl(file, "spectrum_valley_depth", static_cast<int64_t>(s.spectrum_valley_depth));
    wl(file, "spectrum_flatness", static_cast<int64_t>(s.spectrum_flatness));
    wl(file, "spectrum_symmetry", static_cast<int64_t>(s.spectrum_symmetry));

    // Anti-false-positive
    wl(file, "neighbor_margin_db", static_cast<int64_t>(s.neighbor_margin_db));
    wbool(file, "rssi_variance_enabled", s.rssi_variance_enabled);
    wl(file, "confirm_count", static_cast<int64_t>(s.confirm_count));

    // Sweep window 1
    wl(file, "sweep_start_mhz", static_cast<int64_t>(sweep_cfg.sweep_start_freq / 1000000ULL));
    wl(file, "sweep_end_mhz", static_cast<int64_t>(sweep_cfg.sweep_end_freq / 1000000ULL));
    wl(file, "sweep_step_khz", static_cast<int64_t>(sweep_cfg.sweep_step_freq / 1000ULL));

    // Sweep window 2
    wl(file, "sweep2_start_mhz", static_cast<int64_t>(sweep_cfg.sweep2_start_freq / 1000000ULL));
    wl(file, "sweep2_end_mhz", static_cast<int64_t>(sweep_cfg.sweep2_end_freq / 1000000ULL));
    wl(file, "sweep2_step_khz", static_cast<int64_t>(sweep_cfg.sweep2_step_freq / 1000ULL));
    wbool(file, "sweep2_enabled", sweep_cfg.sweep2_enabled);

    // Sweep window 3
    wl(file, "sweep3_start_mhz", static_cast<int64_t>(sweep_cfg.sweep3_start_freq / 1000000ULL));
    wl(file, "sweep3_end_mhz", static_cast<int64_t>(sweep_cfg.sweep3_end_freq / 1000000ULL));
    wl(file, "sweep3_step_khz", static_cast<int64_t>(sweep_cfg.sweep3_step_freq / 1000ULL));
    wbool(file, "sweep3_enabled", sweep_cfg.sweep3_enabled);

    // Sweep window 4
    wl(file, "sweep4_start_mhz", static_cast<int64_t>(sweep_cfg.sweep4_start_freq / 1000000ULL));
    wl(file, "sweep4_end_mhz", static_cast<int64_t>(sweep_cfg.sweep4_end_freq / 1000000ULL));
    wl(file, "sweep4_step_khz", static_cast<int64_t>(sweep_cfg.sweep4_step_freq / 1000ULL));
    wbool(file, "sweep4_enabled", sweep_cfg.sweep4_enabled);

    // Sweep exceptions (4 windows x 5 slots)
    static const char* exc_keys[4][EXCEPTIONS_PER_WINDOW] = {
        {"sw1_exc0_mhz", "sw1_exc1_mhz", "sw1_exc2_mhz", "sw1_exc3_mhz", "sw1_exc4_mhz"},
        {"sw2_exc0_mhz", "sw2_exc1_mhz", "sw2_exc2_mhz", "sw2_exc3_mhz", "sw2_exc4_mhz"},
        {"sw3_exc0_mhz", "sw3_exc1_mhz", "sw3_exc2_mhz", "sw3_exc3_mhz", "sw3_exc4_mhz"},
        {"sw4_exc0_mhz", "sw4_exc1_mhz", "sw4_exc2_mhz", "sw4_exc3_mhz", "sw4_exc4_mhz"},
    };
    for (uint8_t w = 0; w < 4; ++w) {
        for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
            wexc(file, exc_keys[w][i], sweep_cfg.sweep_exceptions[w][i]);
        }
    }

    // Exception radius
    wl(file, "exception_radius_mhz", static_cast<int64_t>(s.exception_radius_mhz));

    // RSSI decrease cycles
    wl(file, "rssi_decrease_cycles", static_cast<int64_t>(s.rssi_decrease_cycles));

    // CFAR detection
    wl(file, "cfar_mode", static_cast<int64_t>(static_cast<uint8_t>(s.cfar_mode)));
    wl(file, "cfar_ref_cells", static_cast<int64_t>(s.cfar_ref_cells));
    wl(file, "cfar_guard_cells", static_cast<int64_t>(s.cfar_guard_cells));
    wl(file, "cfar_threshold_x10", static_cast<int64_t>(s.cfar_threshold_x10));
    wl(file, "cfar_hybrid_alpha", static_cast<int64_t>(s.cfar_hybrid_alpha));
    wl(file, "cfar_hybrid_beta", static_cast<int64_t>(s.cfar_hybrid_beta));
    wl(file, "cfar_hybrid_gamma", static_cast<int64_t>(s.cfar_hybrid_gamma));
    wl(file, "os_cfar_k_percent", static_cast<int64_t>(s.os_cfar_k_percent));
    wl(file, "vi_cfar_threshold_x10", static_cast<int64_t>(s.vi_cfar_threshold_x10));

    // Mahalanobis gate
    wbool(file, "mahalanobis_enabled", s.mahalanobis_enabled);
    wl(file, "mahalanobis_threshold_x10", static_cast<int64_t>(s.mahalanobis_threshold_x10));

    // Pattern matching
    wbool(file, "pattern_matching_enabled", s.pattern_matching_enabled);

    // Metadata
    ws(file, "freqman_path=DRONES\n");
    ws(file, "settings_version=1.2\n");

    (void)file.sync();
    file.close();

    // Atomic rename: delete old file, then rename temp to final
    (void)delete_file(settings_dir / u"eda_settings.txt");
    (void)rename_file(settings_dir / u"eda_settings.txt.tmp", settings_dir / u"eda_settings.txt");

    return ErrorCode::SUCCESS;
}

// ============================================================================
// SettingsFileManager::apply_to_config
// ============================================================================

void SettingsFileManager::apply_to_config(
    const SettingsStruct& s,
    ScanConfig& config
) noexcept {
    config.mode = s.scanning_mode;
    config.scan_interval_ms = s.scan_interval_ms;
    config.rssi_threshold_dbm = s.alert_rssi_threshold_dbm;
    config.threat_low_dbm = s.threat_low_dbm;
    config.threat_medium_dbm = s.threat_medium_dbm;
    config.threat_high_dbm = s.threat_high_dbm;
    config.threat_critical_dbm = s.threat_critical_dbm;
    config.dwell_enabled = s.dwell_enabled;
    config.confirm_count_enabled = s.confirm_count_enabled;
    config.noise_blacklist_enabled = s.noise_blacklist_enabled;
    config.spectrum_detection_enabled = s.spectrum_detection_enabled;
    config.median_enabled = s.median_enabled;

    // Mahalanobis gate
    config.mahalanobis_enabled = s.mahalanobis_enabled;
    config.mahalanobis_threshold_x10 = s.mahalanobis_threshold_x10;

    // Pattern matching
    config.pattern_matching_enabled = s.pattern_matching_enabled;

    config.neighbor_margin_db = s.neighbor_margin_db;
    config.rssi_variance_enabled = s.rssi_variance_enabled;
    config.confirm_count = s.confirm_count;

    // Spectrum shape filter parameters
    config.spectrum_margin = s.spectrum_margin;
    config.spectrum_min_width = s.spectrum_min_width;
    config.spectrum_max_width = s.spectrum_max_width;
    config.spectrum_peak_sharpness = s.spectrum_peak_sharpness;
    config.spectrum_peak_ratio = s.spectrum_peak_ratio;
    config.spectrum_valley_depth = s.spectrum_valley_depth;
    config.spectrum_flatness = s.spectrum_flatness;
    config.spectrum_symmetry = s.spectrum_symmetry;

    // CFAR detection
    config.cfar_mode = s.cfar_mode;
    config.cfar_ref_cells = s.cfar_ref_cells;
    config.cfar_guard_cells = s.cfar_guard_cells;
    config.cfar_threshold_x10 = s.cfar_threshold_x10;

    // CFAR extended parameters
    config.cfar_hybrid_alpha = s.cfar_hybrid_alpha;
    config.cfar_hybrid_beta = s.cfar_hybrid_beta;
    config.cfar_hybrid_gamma = s.cfar_hybrid_gamma;
    config.os_cfar_k_percent = s.os_cfar_k_percent;
    config.vi_cfar_threshold_x10 = s.vi_cfar_threshold_x10;

    // Sweep window 1
    config.sweep_start_freq = s.sweep_start_freq;
    config.sweep_end_freq = s.sweep_end_freq;
    config.sweep_step_freq = s.sweep_step_freq;

    // Sweep window 2
    config.sweep2_start_freq = s.sweep2_start_freq;
    config.sweep2_end_freq = s.sweep2_end_freq;
    config.sweep2_step_freq = s.sweep2_step_freq;
    config.sweep2_enabled = s.sweep2_enabled;

    // Sweep window 3
    config.sweep3_start_freq = s.sweep3_start_freq;
    config.sweep3_end_freq = s.sweep3_end_freq;
    config.sweep3_step_freq = s.sweep3_step_freq;
    config.sweep3_enabled = s.sweep3_enabled;

    // Sweep window 4
    config.sweep4_start_freq = s.sweep4_start_freq;
    config.sweep4_end_freq = s.sweep4_end_freq;
    config.sweep4_step_freq = s.sweep4_step_freq;
    config.sweep4_enabled = s.sweep4_enabled;

    // Sweep exceptions
    for (uint8_t w = 0; w < 4; ++w) {
        for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
            config.sweep_exceptions[w][i] = s.sweep_exceptions[w][i];
        }
    }
    config.exception_radius_mhz = s.exception_radius_mhz;
    config.rssi_decrease_cycles = s.rssi_decrease_cycles;
}

// ============================================================================
// SettingsFileManager::extract_from_config
// ============================================================================

void SettingsFileManager::extract_from_config(
    const ScanConfig& config,
    SettingsStruct& s
) noexcept {
    s.scanning_mode = config.mode;
    s.scan_interval_ms = config.scan_interval_ms;
    s.alert_rssi_threshold_dbm = config.rssi_threshold_dbm;
    s.threat_low_dbm = config.threat_low_dbm;
    s.threat_medium_dbm = config.threat_medium_dbm;
    s.threat_high_dbm = config.threat_high_dbm;
    s.threat_critical_dbm = config.threat_critical_dbm;
    s.scan_sensitivity = static_cast<uint8_t>(
        (config.rssi_threshold_dbm > -20) ? 0 :
        (config.rssi_threshold_dbm < -120) ? 100 :
        (-20 - config.rssi_threshold_dbm)
    );
    s.dwell_enabled = config.dwell_enabled;
    s.confirm_count_enabled = config.confirm_count_enabled;
    s.noise_blacklist_enabled = config.noise_blacklist_enabled;
    s.spectrum_detection_enabled = config.spectrum_detection_enabled;
    s.median_enabled = config.median_enabled;
    s.spectrum_margin = config.spectrum_margin;
    s.spectrum_min_width = config.spectrum_min_width;
    s.spectrum_max_width = config.spectrum_max_width;
    s.spectrum_peak_sharpness = config.spectrum_peak_sharpness;
    s.spectrum_peak_ratio = config.spectrum_peak_ratio;
    s.spectrum_valley_depth = config.spectrum_valley_depth;
    s.spectrum_flatness = config.spectrum_flatness;
    s.spectrum_symmetry = config.spectrum_symmetry;
    s.neighbor_margin_db = config.neighbor_margin_db;
    s.rssi_variance_enabled = config.rssi_variance_enabled;
    s.confirm_count = config.confirm_count;

    // CFAR detection
    s.cfar_mode = config.cfar_mode;
    s.cfar_ref_cells = config.cfar_ref_cells;
    s.cfar_guard_cells = config.cfar_guard_cells;
    s.cfar_threshold_x10 = config.cfar_threshold_x10;

    // CFAR extended parameters
    s.cfar_hybrid_alpha = config.cfar_hybrid_alpha;
    s.cfar_hybrid_beta = config.cfar_hybrid_beta;
    s.cfar_hybrid_gamma = config.cfar_hybrid_gamma;
    s.os_cfar_k_percent = config.os_cfar_k_percent;
    s.vi_cfar_threshold_x10 = config.vi_cfar_threshold_x10;

    // Sweep window 1
    s.sweep_start_freq = config.sweep_start_freq;
    s.sweep_end_freq = config.sweep_end_freq;
    s.sweep_step_freq = config.sweep_step_freq;

    // Sweep window 2
    s.sweep2_start_freq = config.sweep2_start_freq;
    s.sweep2_end_freq = config.sweep2_end_freq;
    s.sweep2_step_freq = config.sweep2_step_freq;
    s.sweep2_enabled = config.sweep2_enabled;

    // Sweep window 3
    s.sweep3_start_freq = config.sweep3_start_freq;
    s.sweep3_end_freq = config.sweep3_end_freq;
    s.sweep3_step_freq = config.sweep3_step_freq;
    s.sweep3_enabled = config.sweep3_enabled;

    // Sweep window 4
    s.sweep4_start_freq = config.sweep4_start_freq;
    s.sweep4_end_freq = config.sweep4_end_freq;
    s.sweep4_step_freq = config.sweep4_step_freq;
    s.sweep4_enabled = config.sweep4_enabled;

    // Sweep exceptions
    for (uint8_t w = 0; w < 4; ++w) {
        for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW; ++i) {
            s.sweep_exceptions[w][i] = config.sweep_exceptions[w][i];
        }
    }
    s.exception_radius_mhz = config.exception_radius_mhz;
    s.rssi_decrease_cycles = config.rssi_decrease_cycles;

    // Mahalanobis gate
    s.mahalanobis_enabled = config.mahalanobis_enabled;
    s.mahalanobis_threshold_x10 = config.mahalanobis_threshold_x10;

    // Pattern matching
    s.pattern_matching_enabled = config.pattern_matching_enabled;
}
} // namespace drone_analyzer
