// ui_detection_logger.cpp
// Clean CSV logging implementation for Enhanced Drone Analyzer

#include "ui_detection_logger.hpp"
#include <cstring>

namespace ui::external_app::enhanced_drone_analyzer {

DroneDetectionLogger::DroneDetectionLogger()
    : session_active_(false),
      session_start_(0),
      logged_count_(0),
      header_written_(false) {
    // Auto-start session
    start_session();
}

DroneDetectionLogger::~DroneDetectionLogger() {
    end_session();
}

void DroneDetectionLogger::start_session() {
    if (session_active_) return;

    session_active_ = true;
    session_start_ = chTimeNow();
    logged_count_ = 0;
    header_written_ = false;
}

void DroneDetectionLogger::end_session() {
    if (!session_active_) return;

    session_active_ = false;
}

bool DroneDetectionLogger::log_detection(const DetectionLogEntry& entry) {
    if (!session_active_) return false;

    if (!ensure_csv_header()) {
        return false;
    }

    std::string csv_entry = format_csv_entry(entry);
    auto error = csv_log_.append(generate_log_filename());

    if (error.is_error()) {
        return false;
    }

    error = csv_log_.write_raw(csv_entry);
    if (!error.is_error()) {
        logged_count_++;
        return true;
    }

    return false;
}

bool DroneDetectionLogger::ensure_csv_header() {
    if (header_written_) return true;

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

std::string DroneDetectionLogger::format_csv_entry(const DetectionLogEntry& entry) {
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    // Simple CSV format
    snprintf(buffer, sizeof(buffer) - 1,
             "%u,%u,%d,%u,%u,%u,%.2f\n",
             entry.timestamp,
             entry.frequency_hz,
             entry.rssi_db,
             static_cast<uint8_t>(entry.threat_level),
             static_cast<uint8_t>(entry.drone_type),
             entry.detection_count,
             entry.confidence_score);

    return std::string(buffer);
}

std::string DroneDetectionLogger::format_session_summary() const {
    char buffer[256];
    uint32_t duration = session_active_ ? (chTimeNow() - session_start_) : 0;

    snprintf(buffer, sizeof(buffer),
             "Enhanced Drone Analyzer session ended.\n"
             "Duration: %u ms, Detections logged: %u\n",
             duration, logged_count_);

    return std::string(buffer);
}

std::filesystem::path DroneDetectionLogger::generate_log_filename() const {
    return std::filesystem::path("EDA_LOG.CSV");
}

} // namespace ui::external_app::enhanced_drone_analyzer
