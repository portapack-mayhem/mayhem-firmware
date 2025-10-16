// ui_detection_logger.hpp
// Clean CSV detection logging for Enhanced Drone Analyzer

#ifndef __UI_DETECTION_LOGGER_HPP__
#define __UI_DETECTION_LOGGER_HPP__

#include "log_file.hpp"
#include "ui_drone_types.hpp"
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

struct DetectionLogEntry {
    uint32_t timestamp;
    uint32_t frequency_hz;
    int32_t rssi_db;
    ThreatLevel threat_level;
    DroneType drone_type;
    uint8_t detection_count;
    float confidence_score;
};

class DroneDetectionLogger {
public:
    DroneDetectionLogger();
    ~DroneDetectionLogger();
    DroneDetectionLogger(const DroneDetectionLogger&) = delete;
    DroneDetectionLogger& operator=(const DroneDetectionLogger&) = delete;

    // Session management
    void start_session();
    void end_session();

    // Detection logging
    bool log_detection(const DetectionLogEntry& entry);

    // Session info
    bool is_session_active() const { return session_active_; }
    uint32_t get_logged_count() const { return logged_count_; }
    std::filesystem::path get_log_filename() const;

private:
    // CSV file operations (LogFile pattern)
    LogFile csv_log_;

    // Session state
    bool session_active_;
    uint32_t session_start_;
    uint32_t logged_count_;
    bool header_written_;

    // Helper methods
    bool ensure_csv_header();
    std::string format_csv_entry(const DetectionLogEntry& entry);
    std::string format_session_summary() const;
    std::filesystem::path generate_log_filename() const;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DETECTION_LOGGER_HPP__
