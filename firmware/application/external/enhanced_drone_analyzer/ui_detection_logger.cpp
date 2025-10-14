// ui_detection_logger.cpp
// Implementation of clean CSV detection logging

#include "ui_detection_logger.hpp"
#include <cstring>

namespace ui::external_app::enhanced_drone_analyzer {

// Directory for log files (matches V0 concept)
static const char* logs_dir = "SDRPP";

DetectionLogger::DetectionLogger()
    : session_active_(false), session_start_(0),
      logged_count_(0), header_written_(false) {
    // Auto-start session
    start_session();
}

DetectionLogger::~DetectionLogger() {
    end_session();
}

void DetectionLogger::start_session() {
    if (session_active_) return;
    session_active_ = true;
    session_start_ = chTimeNow();
    logged_count_ = 0;
    header_written_ = false;
}

void DetectionLogger::end_session() {
    if (!session_active_) return;

    // Write session summary (V0 concept but simplified)
    std::string summary = format_session_summary();
    // END SESSION: Write summary to separate log file (following V0 session summaries)
    auto summary_result = csv_log_.append(generate_log_filename());
    if (summary_result.is_error()) return;

    // Session summary format (V0 concept)
    std::string summary = format_session_summary();
    auto write_result = csv_log_.write_raw(summary);
    if (write_result.is_error()) {
        // Audio feedback for logging failure
        if (global_audio_enabled) {
            baseband::request_audio_beep(200, 24000, 100);  // Long low beep = write error
        }
    }

    session_active_ = false;
    session_start_ = 0;

    // Session end beep (two short beeps)
    if (global_audio_enabled) {
        baseband::request_audio_beep(800, 24000, 50);
        chThdSleepMilliseconds(50);
        baseband::request_audio_beep(800, 24000, 50);
    }
}

bool DetectionLogger::log_detection(const DetectionLogEntry& entry) {
    if (!session_active_) return false;

    if (!ensure_csv_header()) {
        // Audio feedback for logging failure (V0 concept)
        if (global_audio_enabled) {
            baseband::request_audio_beep(200, 24000, 50);  // Low beep = error
        }
        return false;
    }

    std::string csv_entry = format_csv_entry(entry);
    auto error = csv_log_.append(generate_log_filename());

    if (error.is_error()) {
        // Audio feedback for failure
        if (global_audio_enabled) {
            baseband::request_audio_beep(300, 24000, 50);  // Medium beep = write error
        }
        return false;
    }

    error = csv_log_.write_raw(csv_entry);
    if (!error.is_error()) {
        logged_count_++;
        return true;
    } else {
        // Audio feedback for failure
        if (global_audio_enabled) {
            baseband::request_audio_beep(150, 24000, 50);  // Different tone = failure
        }
        return false;
    }
}

bool DetectionLogger::ensure_csv_header() {
    if (header_written_) return true;

    // Simplified header (no complex path operations)
    const char* header = "timestamp_ms,frequency_hz,rssi_db,threat_level,drone_type,detection_count,confidence\n";

    auto error = csv_log_.append(generate_log_filename());
    if (error.is_error()) return false;

    error = csv_log_.write_raw(header);
    if (!error.is_error()) {
        header_written_ = true;
        return true;
    }

    return false;
}

std::string DetectionLogger::format_csv_entry(const DetectionLogEntry& entry) {
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    // Use V0 format but simplified
    snprintf(buffer, sizeof(buffer) - 1,
             "%u,%u,%d,%u,%u,%u,%.2f\n",
             entry.timestamp,
             entry.frequency_hz,
             entry.rssi_db,
             entry.threat_level,
             entry.drone_type,
             entry.detection_count,
             entry.confidence_score);

    return std::string(buffer);
}

std::filesystem::path DetectionLogger::generate_log_filename() const {
    // Simplified filename generation (ERT style)
    return std::filesystem::path("EDA_LOG.CSV");
}

std::string DetectionLogger::format_session_summary() const {
    char buffer[256];
    uint32_t duration = session_active_ ? (chTimeNow() - session_start_) : 0;

    snprintf(buffer, sizeof(buffer),
             "Enhanced Drone Analyzer session ended.\n"
             "Duration: %u ms, Detections logged: %u\n",
             duration, logged_count_);

    return std::string(buffer);
}

} // namespace ui::external_app::enhanced_drone_analyzer
