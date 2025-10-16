#ifndef __UI_DRONE_SETTINGS_HPP__
#define __UI_DRONE_SETTINGS_HPP__

#include "app_settings.hpp"
#include "ui_drone_types.hpp"
#include "ui_drone_config.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// Перегруппированные настройки для лучшей организации UI
struct DroneSpectrumSettings {
    uint32_t min_scan_interval_ms = 1000;  // Скорректировано с 750ms для энергоэффективности
    uint32_t stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;
    int32_t default_rssi_threshold = DEFAULT_RSSI_THRESHOLD_DB;
    float rssi_smoothing_alpha = 0.6f;  // Скорректировано с 0.3 для лучшей фильтрации шума
};

struct DroneDetectionSettings {
    uint8_t min_detection_count = 5;  // Скорректировано с 3 для стабильности
    int32_t hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
    int16_t trend_threshold_db = 5;   // Скорректировано с 3 для уменьшения false positives
    uint32_t detection_reset_interval = 100;  // Скорректировано с 50 для производительности
};

struct DroneAudioSettings {
    uint16_t alert_frequency_hz = SOS_FREQUENCY_HZ;
    uint32_t beep_duration_ms = DETECTION_BEEP_DURATION_MS;
    int32_t alert_squelch_db = -80;  // Новый параметр для контроля аудио тревог
};

struct DroneDisplaySettings {
    bool show_rssi_graph = true;      // Показывать мини-спектр
    bool show_trends = true;          // Показывать тренды движения
    size_t max_display_drones = MAX_DISPLAYED_DRONES;
    bool color_scheme = 0;            // 0=стандартная, 1=высокий контраст
};

struct DroneAnalyzerSettings {
public:
    SpectrumMode spectrum_mode = SpectrumMode::MEDIUM;

    DroneSpectrumSettings spectrum;
    DroneDetectionSettings detection;
    DroneAudioSettings audio;
    DroneDisplaySettings display;

    // Метод сброса к значениям по умолчанию
    void reset_to_defaults();

    // Сериализация для SettingsManager
    std::string serialize() const;
    bool deserialize(const std::string& data);
};

// Глобальная функция для получения настроек по умолчанию
const DroneAnalyzerSettings& get_default_settings();

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SETTINGS_HPP__
