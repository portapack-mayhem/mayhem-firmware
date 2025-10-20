// ui_drone_settings_complete.hpp - Consolidated settings management for Enhanced Drone Analyzer
// Migration Session 6: ui_drone_settings.hpp + ui_drone_settings.cpp + ui_drone_settings_view.hpp + ui_drone_settings_view.cpp → ui_drone_settings_complete.hpp

#ifndef __UI_DRONE_SETTINGS_COMPLETE_HPP__
#define __UI_DRONE_SETTINGS_COMPLETE_HPP__

#include "app_settings.hpp"
#include "ui_drone_config.hpp"

#include "ui.hpp"
#include "ui_tabview.hpp"
#include "ui_button.hpp"
#include "ui_checkbox.hpp"
#include "ui_number_field.hpp"
#include "ui_options_field.hpp"
#include "ui_text.hpp"

#include "portapack.hpp"

#include <string>
#include <sstream>
#include <functional>

namespace ui::external_app::enhanced_drone_analyzer {

<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> 8ed76d5e76761e9c544152cd14263c2abc260169
>>>>>>> 12abf0fef69304265a361b1845ddcbcb5b32279b
// Language support
enum class Language {
    ENGLISH,
    RUSSIAN
};

// Translation system (simplified inline implementation)
class Translator {
public:
    static void set_language(Language lang) { current_language_ = lang; }
    static Language get_language() { return current_language_; }

    static const char* translate(const std::string& key) {
        static const std::map<Language, std::map<std::string, std::string>> translations = {
            {Language::ENGLISH, {
                // Menu items
                {"app_title", "Enhanced Drone Analyzer"},
                {"start_stop", "START/STOP"},
                {"settings", "settings"},
                {"author", "About"},
                {"ready", "Ready"},
                {"scanning", "SCANNING..."},
                {"load_database", "Load Database"},
                {"save_frequency", "Save Frequency"},
                {"toggle_audio", "Toggle Audio Alerts"},
                {"audio_settings", "Audio Settings"},
                {"add_preset", "Add Preset"},
                {"manage_freq", "Manage Frequencies"},
                {"create_db", "Create Database"},
                {"advanced", "Advanced"},
                {"constant_settings", "Constant Settings"},
                {"frequency_warning", "Frequency Warning"},
                {"about_author", "About Author"},

                // Status messages
                {"ready_status", "Ready - Enhanced Drone Analyzer"},
                {"threat_none", "THREAT: NONE | All clear"},
                {"audio_enabled", "Audio alerts ENABLED"},
                {"audio_disabled", "Audio alerts DISABLED"},
                {"audio_enabled_full", "Audio alerts ENABLED\n\nDetection beeps will sound\nfor threat detections"},
                {"audio_disabled_full", "Audio alerts DISABLED\n\nNo audio feedback will\nbe provided"},
                {"scanning_mode_db", "Database Scan"},
                {"scanning_mode_wband", "Wideband Monitor"},
                {"scanning_mode_hybrid", "Hybrid Discovery"},
                {"real_mode", "REAL"},
                {"demo_mode", "DEMO"},

                // Settings labels
                {"drone_analyzer_settings", "Drone Analyzer Settings"},
                {"spectrum_analysis", "Spectrum Analysis"},
                {"mode", "Mode"},
                {"ultra_narrow", "ULTRA NARROW"},
                {"narrow", "NARROW"},
                {"medium", "MEDIUM"},
                {"wide", "WIDE"},
                {"ultra_wide", "ULTRA WIDE"},
                {"scan_interval", "Scan Int.:"},
                {"rssi_threshold", "RSSI Thresh.:"},
                {"rssi_smooth", "RSSI Smooth:"},
                {"signal_detection", "Signal Detection"},
                {"min_detections", "Min Detections:"},
                {"hysteresis", "Hysteresis:"},
                {"trend_thresh", "Trend Thresh.:"},
                {"reset_int", "Reset Int.:"},
                {"audio_alerts", "Audio Alerts"},
                {"alert_freq", "Alert Freq.:"},
                {"beep_duration", "Beep Duration:"},
                {"alert_squelch", "Alert Squelch:"},
                {"display_settings", "Display Settings"},
                {"show_freq_chart", "Show frequency chart"},
                {"show_trends", "Show movement trends"},
                {"max_display_drones", "Max display drones:"},
                {"color_scheme", "Color scheme:"},
                {"standard", "Standard"},
                {"high_contrast", "High contrast"},
                {"save", "Save"},
                {"cancel", "Cancel"},
                {"reset", "Reset"},
                {"validation_error", "Validation Error"},
                {"validation_success", "All values valid"},

                // System status
                {"system_status", "System Status"},
                {"mode_label", "Mode:"},
                {"thread_active", "Thread Active:"},
                {"spectrum_streaming", "Spectrum Streaming:"},
                {"database", "DATABASE:"},
                {"entries_loaded", "Entries loaded:"},
                {"active_tracking", "Active tracking:"},
                {"current_threat", "Current threat:"},
                {"audio_alerts_status", "STATUS:"},
                {"detection_beeps", "Detection beeps:"},
                {"sos_signals", "SOS signals:"},
                {"enabled", "ENABLED"},
                {"yes", "YES"},
                {"no", "NO"},

                // Performance
                {"performance_stats", "Performance Stats"},
                {"cycles_completed", "Cycles completed:"},
                {"avg_cycle_time", "Avg. cycle time:"},
                {"total_detections", "Total detections:"},
                {"detection_rate", "Detection rate:"},
                {"memory_used", "Used:"},
                {"available", "Available:"},
                {"efficiency", "Efficiency:"},

                // Debug
                {"debug_info", "Debug Info"},
                {"current_freq", "Frequency:"},
                {"threat_level", "Threat Level:"},
                {"spectrum_active", "Spectrum Active:"},
                {"rssi", "RSSI:"},
                {"temperature", "Temperature:"},
                {"running", "RUNNING"},
                {"stopped", "STOPPED"},

                // Frequency ranges
                {"freq_ranges", "Frequency Ranges (MHz)"},
                {"min_drone_freq", "Min Drone Freq:"},
                {"max_drone_freq", "Max Drone Freq:"},

                // Other strings
                {"select_language", "Select Language"},
                {"english", "English"},
                {"russian", "Russian"}
            }},
            {Language::RUSSIAN, {
                // Menu items
                {"app_title", "Расширенный Анализатор Дронов"},
                {"start_stop", "СТАРТ/СТОП"},
                {"settings", "настройки"},
                {"author", "О программе"},
                {"ready", "Готов"},
                {"scanning", "СКАНИРОВАНИЕ..."},
                {"load_database", "Загрузить Базу"},
                {"save_frequency", "Сохранить Частоту"},
                {"toggle_audio", "Переключить Аудио"},
                {"audio_settings", "Настройки Аудио"},
                {"add_preset", "Добавить Пресет"},
                {"manage_freq", "Управление Частотами"},
                {"create_db", "Создать Базу"},
                {"advanced", "Дополнительно"},
                {"constant_settings", "Постоянные Настройки"},
                {"frequency_warning", "Предупреждение о Частоте"},
                {"about_author", "Об Авторе"},

                // Status messages
                {"ready_status", "Готов - Расширенный Анализатор Дронов"},
                {"threat_none", "УГРОЗА: НЕТ | Все чисто"},
                {"audio_enabled", "Аудио оповещения ВКЛЮЧЕНЫ"},
                {"audio_disabled", "Аудио оповещения ОТКЛЮЧЕНЫ"},
                {"audio_enabled_full", "Аудио оповещения ВКЛЮЧЕНЫ\n\nЗвуковые сигналы будут звучать\nпри обнаружении угроз"},
                {"audio_disabled_full", "Аудио оповещения ОТКЛЮЧЕНЫ\n\nЗвуковая обратная связь\nне предоставляется"},
                {"scanning_mode_db", "Сканирование Базы"},
                {"scanning_mode_wband", "Широкополосный Монитор"},
                {"scanning_mode_hybrid", "Гибридное Обнаружение"},
                {"real_mode", "РЕАЛЬНЫЙ"},
                {"demo_mode", "ДЕМО"},

                // Settings labels
                {"drone_analyzer_settings", "Настройки Анализатора Дронов"},
                {"spectrum_analysis", "Анализ Спектра"},
                {"mode", "Режим:"},
                {"ultra_narrow", "УЛЬТРА УЗКИЙ"},
                {"narrow", "УЗКИЙ"},
                {"medium", "СРЕДНИЙ"},
                {"wide", "ШИРОКИЙ"},
                {"ultra_wide", "УЛЬТРА ШИРОКИЙ"},
                {"scan_interval", "Инт. сканир.:"},
                {"rssi_threshold", "Порог RSSI:"},
                {"rssi_smooth", "Сглаж. RSSI:"},
                {"signal_detection", "Обнаружение Сигнала"},
                {"min_detections", "Мин. обнаруж.:"},
                {"hysteresis", "Гистерезис:"},
                {"trend_thresh", "Порог тренда:"},
                {"reset_int", "Инт. сброса:"},
                {"audio_alerts", "Аудио Оповещения"},
                {"alert_freq", "Част. опов.:"},
                {"beep_duration", "Длит. сигн.:"},
                {"alert_squelch", "Подавл. опов.:"},
                {"display_settings", "Настройки Отображения"},
                {"show_freq_chart", "Показывать частотный график"},
                {"show_trends", "Показывать тренды движения"},
                {"max_display_drones", "Макс. дронов на экране:"},
                {"color_scheme", "Цветовая схема:"},
                {"standard", "Стандартная"},
                {"high_contrast", "Высокий контраст"},
                {"save", "Сохранить"},
                {"cancel", "Отмена"},
                {"reset", "Сброс"},
                {"validation_error", "Ошибка Валидации"},
                {"validation_success", "Все значения корректны"},

                // System status
                {"system_status", "Состояние Системы"},
                {"mode_label", "Режим:"},
                {"thread_active", "Поток активен:"},
                {"spectrum_streaming", "Поток спектра:"},
                {"database", "БАЗА ДАННЫХ:"},
                {"entries_loaded", "Записей загружено:"},
                {"active_tracking", "Активное отслеживание:"},
                {"current_threat", "Текущая угроза:"},
                {"audio_alerts_status", "СОСТОЯНИЕ:"},
                {"detection_beeps", "Сигналы обнаружения:"},
                {"sos_signals", "Сигналы SOS:"},
                {"enabled", "ВКЛЮЧЕНО"},
                {"yes", "ДА"},
                {"no", "НЕТ"},

                // Performance
                {"performance_stats", "Статистика Производительности"},
                {"cycles_completed", "Циклов завершено:"},
                {"avg_cycle_time", "Ср. время цикла:"},
                {"total_detections", "Всего обнаружений:"},
                {"detection_rate", "Скорость обнаружения:"},
                {"memory_used", "Использовано:"},
                {"available", "Доступно:"},
                {"efficiency", "Эффективность:"},

                // Debug
                {"debug_info", "Информация Отладки"},
                {"current_freq", "Частота:"},
                {"threat_level", "Уровень Угрозы:"},
                {"spectrum_active", "Спектр активен:"},
                {"rssi", "RSSI:"},
                {"temperature", "Температура:"},
                {"running", "РАБОТАЕТ"},
                {"stopped", "ОСТАНОВЛЕНО"},

                // Frequency ranges
                {"freq_ranges", "Диапазоны Частот (МГц)"},
                {"min_drone_freq", "Мин. частота дрона:"},
                {"max_drone_freq", "Макс. частота дрона:"},

                // Other strings
                {"select_language", "Выбрать Язык"},
                {"english", "Английский"},
                {"russian", "Русский"}
            }}
        };

        auto lang_it = translations.find(current_language_);
        if (lang_it != translations.end()) {
            auto str_it = lang_it->second.find(key);
            if (str_it != lang_it->second.end()) {
                return str_it->second.c_str();
            }
        }
        return key.c_str(); // Fallback to key if translation not found
    }

private:
    static Language current_language_;
};

Language Translator::current_language_ = Language::ENGLISH;

<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
=======
>>>>>>> a48640829308c7ab7e0768914004ec59c3d45960
>>>>>>> 8ed76d5e76761e9c544152cd14263c2abc260169
>>>>>>> 12abf0fef69304265a361b1845ddcbcb5b32279b
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

// Settings dialog view with tabbed interface
class DroneSettingsView : public View {
public:
    DroneSettingsView(NavigationView& nav, DroneAnalyzerSettings& settings);

    std::string title() const override { return "Drone Analyzer Settings"; };
    void focus() override { tab_view.focus(); }

private:
    NavigationView& nav_;
    DroneAnalyzerSettings& settings_;
    DroneAnalyzerSettings backup_settings_;  // For cancel functionality

    // Main UI components
    TabView tab_view{
        {0, 0}, {screen_width, screen_height - 3 * 16},
        {"Spectrum", "Detection", "Audio", "Display"}
    };

    Button button_save{
        {UI_POS_X_CENTER(6), screen_height - 2 * 16, 6 * 8, 2 * 16},
        "Save"
    };

    Button button_cancel{
        {UI_POS_X_CENTER(6) + 6 * 8 + 4, screen_height - 2 * 16, 6 * 8, 2 * 16},
        "Cancel"
    };

    Button button_reset{
        {UI_POS_X_CENTER(6), screen_height - 16, 6 * 8, 16},
        "Reset"
    };

    // ===== SPECTRUM TAB =====
    Text text_spectrum_title{{8, 2 * 16}, "Spectrum Analysis", Theme::getInstance()->fg_light->foreground};

    Text text_spectrum_mode_label{{8, 3 * 16 + 8}, "Mode:", Theme::getInstance()->fg_light->foreground};
    OptionsField field_spectrum_mode{
        {8 * 5, 3 * 16 + 8}, 10,
        {
            {"ULTRA NARROW", static_cast<int>(SpectrumMode::ULTRA_NARROW)},
            {"NARROW", static_cast<int>(SpectrumMode::NARROW)},
            {"MEDIUM", static_cast<int>(SpectrumMode::MEDIUM)},
            {"WIDE", static_cast<int>(SpectrumMode::WIDE)},
            {"ULTRA WIDE", static_cast<int>(SpectrumMode::ULTRA_WIDE)}
        }
    };

    Text text_scan_interval_label{{8, 4 * 16 + 8}, "Scan Int.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_scan_interval{
        {8 * 9, 4 * 16 + 8}, 4, {200, 5000}, 100, ' '
    };

    Text text_rssi_threshold_label{{8, 5 * 16 + 8}, "RSSI Thresh.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_rssi_threshold{
        {8 * 12, 5 * 16 + 8}, 3, {-120, -20}, 1, ' '
    };

    Text text_rssi_alpha_label{{8, 6 * 16 + 8}, "RSSI Smooth:", Theme::getInstance()->fg_light->foreground};
    NumberField field_rssi_alpha{
        {8 * 12, 6 * 16 + 8}, 2, {1, 9}, 1, ' '  // Values 0.1 to 0.9
    };

    // ===== DETECTION TAB =====
    Text text_detection_title{{8, 2 * 16}, "Signal Detection", Theme::getInstance()->fg_light->foreground};

    Text text_min_detections_label{{8, 3 * 16 + 8}, "Min Detections:", Theme::getInstance()->fg_light->foreground};
    NumberField field_min_detections{
        {8 * 14, 3 * 16 + 8}, 2, {1, 10}, 1, ' '
    };

    Text text_hysteresis_label{{8, 4 * 16 + 8}, "Hysteresis:", Theme::getInstance()->fg_light->foreground};
    NumberField field_hysteresis{
        {8 * 11, 4 * 16 + 8}, 2, {1, 20}, 1, ' '
    };

    Text text_trend_threshold_label{{8, 5 * 16 + 8}, "Trend Thresh.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_trend_threshold{
        {8 * 14, 5 * 16 + 8}, 2, {1, 15}, 1, ' '
    };

    Text text_reset_interval_label{{8, 6 * 16 + 8}, "Reset Int.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_reset_interval{
        {8 * 10, 6 * 16 + 8}, 3, {25, 500}, 25, ' '
    };

    // ===== AUDIO TAB =====
    Text text_audio_title{{8, 2 * 16}, "Audio Alerts", Theme::getInstance()->fg_light->foreground};

    Text text_alert_freq_label{{8, 3 * 16 + 8}, "Alert Freq.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_alert_freq{
        {8 * 11, 3 * 16 + 8}, 4, {400, 3000}, 50, ' '
    };

    Text text_beep_duration_label{{8, 4 * 16 + 8}, "Beep Duration:", Theme::getInstance()->fg_light->foreground};
    NumberField field_beep_duration{
        {8 * 14, 4 * 16 + 8}, 3, {50, 1000}, 50, ' '
    };

    Text text_alert_squelch_label{{8, 5 * 16 + 8}, "Alert Squelch:", Theme::getInstance()->fg_light->foreground};
    NumberField field_alert_squelch{
        {8 * 14, 5 * 16 + 8}, 3, {-100, -20}, 5, ' '
    };

    // ===== DISPLAY TAB =====
    Text text_display_title{{8, 2 * 16}, "Display Settings", Theme::getInstance()->fg_light->foreground};

    Checkbox check_show_graph{
        {8, 3 * 16 + 8},
        16,
        "Show frequency chart"
    };

    Checkbox check_show_trends{
        {8, 4 * 16 + 8},
        16,
        "Show movement trends"
    };

    Text text_max_drones_label{{8, 5 * 16 + 8}, "Max display drones:", Theme::getInstance()->fg_light->foreground};
    NumberField field_max_drones{
        {8 * 17, 5 * 16 + 8}, 1, {1, 5}, 1, ' '
    };

    Text text_color_scheme_label{{8, 6 * 16 + 8}, "Color scheme:", Theme::getInstance()->fg_light->foreground};
    OptionsField field_color_scheme{
        {8 * 13, 6 * 16 + 8}, 15,
        {
            {"Standard", 0},
            {"High contrast", 1}
        }
    };

    // Event handlers
    void setup_ui_elements();
    void load_settings_to_ui();
    void save_ui_to_settings();
    void validate_settings();
    void show_validation_error(const std::string& message);
};

// Forward declaration for settings manager
class ConstantSettingsManager {
public:
    static bool load_settings();
    static bool save_settings();
    static void reset_to_defaults();

    // Static variables holding current working constants
    static rf::Frequency current_min_drone_freq;
    static rf::Frequency current_max_drone_freq;
    static uint8_t current_min_detection_count;
    static int32_t current_hysteresis_margin_db;
    static int16_t current_trend_threshold_db;
    static int32_t current_default_rssi_threshold_db;
    static uint32_t current_scan_interval_ms;
    static uint32_t current_stale_timeout_ms;
    // Additional configurable constants
    static float current_rssi_smoothing_alpha;
    static uint32_t current_detection_reset_interval;
    static size_t current_max_displayed_drones;
    static rf::Frequency current_mini_spectrum_min_freq;
    static rf::Frequency current_mini_spectrum_max_freq;
    static uint16_t current_sos_frequency_hz;
    static uint32_t current_beep_duration_ms;
    static float current_rssi_threat_weight;
    static float current_database_threat_weight;

    // Additional hardware and buffer constants
    static size_t current_detection_table_size;
    static size_t current_max_tracked_drones;
    static size_t current_max_database_entries;
    static size_t current_spectrum_buffer_size;
    static rf::Frequency current_ism_center_freq;
};

class ConstantSettingsView : public View {
public:
    ConstantSettingsView(NavigationView& nav);
    ~ConstantSettingsView() = default;

    void focus() override;
    std::string title() const override { return "Constant Settings"; }

private:
    NavigationView& nav_;

    // FREQUENCY RANGE SETTINGS
    Text text_freq_range_title_{ {5, 10}, "Frequency Ranges (MHz)" };
    Text text_min_drone_freq_{ {5, 25}, "Min Drone Freq:" };
    NumberField field_min_drone_freq_{ {85, 22}, 5, {400}, 6000, 240 };
    Text text_max_drone_freq_{ {5, 42}, "Max Drone Freq:" };
    NumberField field_max_drone_freq_{ {85, 39}, 5, {400}, 6000, 6000 };

    // DETECTION PARAMETERS
    Text text_detection_title_{ {5, 60}, "Detection Parameters" };
    Text text_min_detection_{ {5, 75}, "Min Detection Count:" };
    NumberField field_min_detection_count_{ {110, 72}, 2, {1}, 10, 3 };
    Text text_hysteresis_{ {5, 92}, "Hysteresis Margin (dB):" };
    NumberField field_hysteresis_margin_{ {130, 89}, 3, {-20}, 20, 5 };
    Text text_trend_threshold_{ {5, 109}, "Trend Threshold (dB):" };
    NumberField field_trend_threshold_{ {120, 106}, 3, {1}, 10, 3 };
    Text text_default_rssi_{ {5, 126}, "Default RSSI Threshold:" };
    NumberField field_default_rssi_threshold_{ {135, 123}, 4, {-120}, -10, -90 };

    // SCANNING PARAMETERS
    Text text_scanning_title_{ {5, 140}, "Scanning Parameters" };
    Text text_scan_interval_{ {5, 155}, "Scan Interval (ms):" };
    NumberField field_scan_interval_{ {95, 152}, 4, {500}, 5000, 1000 };
    Text text_stale_timeout_{ {5, 172}, "Stale Timeout (ms):" };
    NumberField field_stale_timeout_{ {95, 169}, 5, {5000}, 60000, 30000 };

    // ACTION BUTTONS
    Button button_defaults_{ {5, 480}, "Load Defaults" };
    Button button_save_{ {80, 480}, "Save Settings" };
    Button button_ok_{ {165, 480}, "OK" };

    // STATUS TEXT
    Text text_status_{ {5, 500}, 230, "" };

    // Helper methods
    void initialize_field_values();
    void update_status_text(const char* text);
    bool validate_all_fields();
    void apply_current_values();

    // Button callback methods
    void on_load_defaults();
    void on_save_settings();
    void on_ok_pressed();
    void on_cancel();

    // Validation callbacks for individual fields
    void validate_min_freq(int32_t value);
    void validate_max_freq(int32_t value);
    void validate_detection_params();

    // SPECTRUM & UI PARAMETERS
    Text text_spectrum_title_{ {5, 190}, "Spectrum & UI Parameters" };
    Text text_rssi_smoothing_{ {5, 205}, "RSSI Smoothing Alpha:" };
    NumberField field_rssi_smoothing_{ {125, 202}, 3, {1}, 99, 30 };  // multiplied by 100 for 0.01 precision
    Text text_detection_reset_{ {5, 222}, "Detection Reset Interval:" };
    NumberField field_detection_reset_{ {150, 219}, 3, {10}, 500, 50 };
    Text text_max_drones_{ {5, 239}, "Max Displayed Drones:" };
    NumberField field_max_drones_{ {125, 236}, 2, {1}, 8, 3 };
    Text text_spectrum_min_{ {5, 256}, "Mini Spectrum Min Freq (GHz):" };
    NumberField field_spectrum_min_{ {180, 253}, 1, {1}, 6, 2400 };  // in MHz
    Text text_spectrum_max_{ {5, 273}, "Mini Spectrum Max Freq (GHz):" };
    NumberField field_spectrum_max_{ {180, 270}, 1, {1}, 6, 2500 };  // in MHz

    // AUDIO PARAMETERS
    Text text_audio_title_{ {5, 290}, "Audio Alert Parameters" };
    Text text_sos_freq_{ {5, 305}, "SOS Frequency (Hz):" };
    NumberField field_sos_freq_{ {115, 302}, 4, {500}, 3000, 1500 };
    Text text_beep_duration_{ {5, 322}, "Beep Duration (ms):" };
    NumberField field_beep_duration_{ {125, 319}, 3, {50}, 1000, 200 };

    // THREAT SCORING PARAMETERS
    Text text_threat_title_{ {5, 335}, "Threat Scoring Weights" };
    Text text_rssi_weight_{ {5, 350}, "RSSI Threat Weight:" };
    NumberField field_rssi_weight_{ {115, 347}, 2, {1}, 99, 40 };  // multiplied by 100 for 0.01 precision
    Text text_db_weight_{ {5, 367}, "Database Threat Weight:" };
    NumberField field_db_weight_{ {140, 364}, 2, {1}, 99, 60 };  // multiplied by 100 for 0.01 precision

    // HARDWARE & BUFFER PARAMETERS
    Text text_hardware_title_{ {5, 380}, "Hardware & Buffer Parameters" };
    Text text_detection_table_{ {5, 395}, "Detection Table Size:" };
    NumberField field_detection_table_{ {120, 392}, 4, {128}, 1024, 512 };
    Text text_max_tracked_{ {5, 412}, "Max Tracked Drones:" };
    NumberField field_max_tracked_{ {120, 409}, 2, {1}, 16, 8 };
    Text text_max_db_{ {5, 429}, "Max DB Entries:" };
    NumberField field_max_db_{ {100, 426}, 3, {32}, 128, 64 };
    Text text_spectrum_buffer_{ {5, 446}, "Spectrum Buffer Size:" };
    NumberField field_spectrum_buffer_{ {130, 443}, 4, {128}, 512, 256 };
    Text text_ism_center_{ {5, 463}, "ISM Center Freq (MHz):" };
    NumberField field_ism_center_{ {130, 460}, 4, {400}, 600, 433 };

    // Prevent copying
    ConstantSettingsView(const ConstantSettingsView&) = delete;
    ConstantSettingsView& operator=(ConstantSettingsView&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

// HEADER-ONLY IMPLEMENTATIONS - From ui_drone_settings.cpp and ui_drone_settings_view.cpp
// Wrapped in anonymous namespace for internal linkage
namespace {

using namespace ui::external_app::enhanced_drone_analyzer;

// DroneAnalyzerSettings implementation
void DroneAnalyzerSettings::reset_to_defaults() {
    spectrum_mode = SpectrumMode::MEDIUM;

    // Spectrum settings
    spectrum.min_scan_interval_ms = 1000u;
    spectrum.stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;
    spectrum.default_rssi_threshold = DEFAULT_RSSI_THRESHOLD_DB;
    spectrum.rssi_smoothing_alpha = 0.6f;

    // Detection settings
    detection.min_detection_count = 5u;
    detection.hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
    detection.trend_threshold_db = 5;
    detection.detection_reset_interval = 100u;

    // Audio settings
    audio.alert_frequency_hz = SOS_FREQUENCY_HZ;
    audio.beep_duration_ms = DETECTION_BEEP_DURATION_MS;
    audio.alert_squelch_db = -80;

    // Display settings
    display.show_rssi_graph = true;
    display.show_trends = true;
    display.max_display_drones = MAX_DISPLAYED_DRONES;
    display.color_scheme = 0;
}

// Serialize settings for SettingsManager storage
std::string DroneAnalyzerSettings::serialize() const {
    std::stringstream ss;

    // Версия формата для будущей совместимости
    ss << "v1:";

    // Spectrum settings
    ss << spectrum_mode << ":";
    ss << spectrum.min_scan_interval_ms << ":";
    ss << spectrum.stale_timeout_ms << ":";
    ss << spectrum.default_rssi_threshold << ":";
    ss << spectrum.rssi_smoothing_alpha << ":";

    // Detection settings
    ss << static_cast<int>(detection.min_detection_count) << ":";
    ss << detection.hysteresis_margin_db << ":";
    ss << detection.trend_threshold_db << ":";
    ss << detection.detection_reset_interval << ":";

    // Audio settings
    ss << audio.alert_frequency_hz << ":";
    ss << audio.beep_duration_ms << ":";
    ss << audio.alert_squelch_db << ":";

    // Display settings
    ss << (display.show_rssi_graph ? "1" : "0") << ":";
    ss << (display.show_trends ? "1" : "0") << ":";
    ss << display.max_display_drones << ":";
    ss << (display.color_scheme ? "1" : "0");

    return ss.str();
}

// Deserialize settings from SettingsManager storage
bool DroneAnalyzerSettings::deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string token;

    // Check version
    if (!std::getline(ss, token, ':')) return false;
    if (token != "v1") return false; // Unsupported version

    auto parse_int = [&](int& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = std::stoi(token);
        return true;
    };

    auto parse_uint = [&](uint32_t& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = static_cast<uint32_t>(std::stoul(token));
        return true;
    };

    auto parse_float = [&](float& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = std::stof(token);
        return true;
    };

    auto parse_bool = [&](bool& value) {
        if (!std::getline(ss, token, ':')) return false;
        value = (token == "1");
        return true;
    };

    try {
        int spectrum_mode_int;
        if (!parse_int(spectrum_mode_int)) return false;
        spectrum_mode = static_cast<SpectrumMode>(spectrum_mode_int);

        // Spectrum
        if (!parse_uint(spectrum.min_scan_interval_ms)) return false;
        if (!parse_uint(spectrum.stale_timeout_ms)) return false;
        if (!parse_int(spectrum.default_rssi_threshold)) return false;
        if (!parse_float(spectrum.rssi_smoothing_alpha)) return false;

        // Detection
        int min_det_count;
        if (!parse_int(min_det_count)) return false;
        detection.min_detection_count = static_cast<uint8_t>(min_det_count);
        if (!parse_int(detection.hysteresis_margin_db)) return false;
        if (!parse_int(detection.trend_threshold_db)) return false;
        if (!parse_uint(detection.detection_reset_interval)) return false;

        // Audio
        if (!parse_uint(audio.alert_frequency_hz)) return false;
        if (!parse_uint(audio.beep_duration_ms)) return false;
        if (!parse_int(audio.alert_squelch_db)) return false;

        // Display
        if (!parse_bool(display.show_rssi_graph)) return false;
        if (!parse_bool(display.show_trends)) return false;
        size_t max_drones;
        if (!parse_uint(max_drones)) return false;
        display.max_display_drones = max_drones;
        if (!parse_bool(display.color_scheme)) return false;

        return true;
    } catch (const std::exception&) {
        return false; // Parse error
    }
}

// Get default settings instance
const DroneAnalyzerSettings& get_default_settings() {
    static DroneAnalyzerSettings defaults;
    static bool initialized = false;

    if (!initialized) {
        defaults.reset_to_defaults();
        initialized = true;
    }

    return defaults;
}

// DroneSettingsView implementation
DroneSettingsView::DroneSettingsView(NavigationView& nav, DroneAnalyzerSettings& settings)
    : nav_(nav), settings_(settings), backup_settings_(settings)
{
    // Add all UI elements to children
    add_children({
        &tab_view,
        &button_save,
        &button_cancel,
        &button_reset,

        // Spectrum tab
        &text_spectrum_title,
        &text_spectrum_mode_label, &field_spectrum_mode,
        &text_scan_interval_label, &field_scan_interval,
        &text_rssi_threshold_label, &field_rssi_threshold,
        &text_rssi_alpha_label, &field_rssi_alpha,

        // Detection tab
        &text_detection_title,
        &text_min_detections_label, &field_min_detections,
        &text_hysteresis_label, &field_hysteresis,
        &text_trend_threshold_label, &field_trend_threshold,
        &text_reset_interval_label, &field_reset_interval,

        // Audio tab
        &text_audio_title,
        &text_alert_freq_label, &field_alert_freq,
        &text_beep_duration_label, &field_beep_duration,
        &text_alert_squelch_label, &field_alert_squelch,

        // Display tab
        &text_display_title,
        &check_show_graph,
        &check_show_trends,
        &text_max_drones_label, &field_max_drones,
        &text_color_scheme_label, &field_color_scheme,
    });

    setup_ui_elements();
    load_settings_to_ui();
}

void DroneSettingsView::setup_ui_elements() {
    // Event handlers for buttons
    button_save.on_select = [this](Button&) {
        if (validate_settings()) {
            save_ui_to_settings();
            nav_.pop();
        }
    };

    button_cancel.on_select = [this](Button&) {
        // Restore from backup (cancel changes)
        settings_ = backup_settings_;
        nav_.pop();
    };

    button_reset.on_select = [this](Button&) {
        settings_.reset_to_defaults();
        load_settings_to_ui();
    };

    // Spectrum mode change handler
    field_spectrum_mode.on_change = [this](size_t, OptionsField::value_t value) {
        settings_.spectrum_mode = static_cast<SpectrumMode>(value);
    };
}

void DroneSettingsView::load_settings_to_ui() {
    // Load spectrum settings
    field_spectrum_mode.set_by_value(static_cast<int>(settings_.spectrum_mode));
    field_scan_interval.set_value(settings_.spectrum.min_scan_interval_ms);
    field_rssi_threshold.set_value(settings_.spectrum.default_rssi_threshold);
    field_rssi_alpha.set_value(static_cast<int>(settings_.spectrum.rssi_smoothing_alpha * 10)); // Convert 0.1-0.9 to 1-9

    // Load detection settings
    field_min_detections.set_value(settings_.detection.min_detection_count);
    field_hysteresis.set_value(settings_.detection.hysteresis_margin_db);
    field_trend_threshold.set_value(settings_.detection.trend_threshold_db);
    field_reset_interval.set_value(settings_.detection.detection_reset_interval);

    // Load audio settings
    field_alert_freq.set_value(settings_.audio.alert_frequency_hz);
    field_beep_duration.set_value(settings_.audio.beep_duration_ms);
    field_alert_squelch.set_value(settings_.audio.alert_squelch_db);

    // Load display settings
    check_show_graph.set_value(settings_.display.show_rssi_graph);
    check_show_trends.set_value(settings_.display.show_trends);
    field_max_drones.set_value(settings_.display.max_display_drones);
    field_color_scheme.set_by_value(settings_.display.color_scheme);
}

void DroneSettingsView::save_ui_to_settings() {
    // Save spectrum settings
    settings_.spectrum.min_scan_interval_ms = field_scan_interval.to_integer();
    settings_.spectrum.default_rssi_threshold = field_rssi_threshold.to_integer();
    settings_.spectrum.rssi_smoothing_alpha = field_rssi_alpha.to_integer() / 10.0f; // Convert back

    // Save detection settings
    settings_.detection.min_detection_count = field_min_detections.to_integer();
    settings_.detection.hysteresis_margin_db = field_hysteresis.to_integer();
    settings_.detection.trend_threshold_db = field_trend_threshold.to_integer();
    settings_.detection.detection_reset_interval = field_reset_interval.to_integer();

    // Save audio settings
    settings_.audio.alert_frequency_hz = field_alert_freq.to_integer();
    settings_.audio.beep_duration_ms = field_beep_duration.to_integer();
    settings_.audio.alert_squelch_db = field_alert_squelch.to_integer();

    // Save display settings
    settings_.display.show_rssi_graph = check_show_graph.value();
    settings_.display.show_trends = check_show_trends.value();
    settings_.display.max_display_drones = field_max_drones.to_integer();
    settings_.display.color_scheme = field_color_scheme.selected_index_value();
}

void DroneSettingsView::validate_settings() {
    // Validate RSSI alpha (1-9 maps to 0.1-0.9)
    float alpha = field_rssi_alpha.to_integer() / 10.0f;
    if (alpha < 0.1f || alpha > 0.9f) {
        show_validation_error("RSSI Smooth must be 0.1-0.9");
        field_rssi_alpha.focus();
        return false;
    }

    // Validate minimum detections
    int min_det = field_min_detections.to_integer();
    if (min_det < 1 || min_det > 10) {
        show_validation_error("Min detections must be 1-10");
        field_min_detections.focus();
        return false;
    }

    // Validate scan interval
    uint32_t scan_int = field_scan_interval.to_integer();
    if (scan_int < 200 || scan_int > 5000) {
        show_validation_error("Scan interval must be 200-5000 ms");
        field_scan_interval.focus();
        return false;
    }

    // Validate alert frequency
    uint32_t alert_freq = field_alert_freq.to_integer();
    if (alert_freq < 400 || alert_freq > 3000) {
        show_validation_error("Alert frequency must be 400-3000 Hz");
        field_alert_freq.focus();
        return false;
    }

    return true;
}

void DroneSettingsView::show_validation_error(const std::string& message) {
    nav_.display_modal("Validation Error", message);
}

// Initialize static variables with default values
rf::Frequency ConstantSettingsManager::current_min_drone_freq = MIN_DRONE_FREQUENCY;
rf::Frequency ConstantSettingsManager::current_max_drone_freq = MAX_DRONE_FREQUENCY;
uint8_t ConstantSettingsManager::current_min_detection_count = MIN_DETECTION_COUNT;
int32_t ConstantSettingsManager::current_hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
int16_t ConstantSettingsManager::current_trend_threshold_db = TREND_THRESHOLD_DB;
int32_t ConstantSettingsManager::current_default_rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
uint32_t ConstantSettingsManager::current_scan_interval_ms = MIN_SCAN_INTERVAL_MS;
uint32_t ConstantSettingsManager::current_stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;
// Additional constants initialized with defaults
float ConstantSettingsManager::current_rssi_smoothing_alpha = RSSI_SMOOTHING_ALPHA;
uint32_t ConstantSettingsManager::current_detection_reset_interval = DETECTION_RESET_INTERVAL;
size_t ConstantSettingsManager::current_max_displayed_drones = MAX_DISPLAYED_DRONES;
rf::Frequency ConstantSettingsManager::current_mini_spectrum_min_freq = MINI_SPECTRUM_MIN_FREQ;
rf::Frequency ConstantSettingsManager::current_mini_spectrum_max_freq = MINI_SPECTRUM_MAX_FREQ;
uint16_t ConstantSettingsManager::current_sos_frequency_hz = SOS_FREQUENCY_HZ;
uint32_t ConstantSettingsManager::current_beep_duration_ms = DETECTION_BEEP_DURATION_MS;
float ConstantSettingsManager::current_rssi_threat_weight = RSSI_THREAT_WEIGHT;
float ConstantSettingsManager::current_database_threat_weight = DATABASE_THREAT_WEIGHT;

// Additional hardware and buffer constants
size_t ConstantSettingsManager::current_detection_table_size = DETECTION_TABLE_SIZE;
size_t ConstantSettingsManager::current_max_tracked_drones = MAX_TRACKED_DRONES;
size_t ConstantSettingsManager::current_max_database_entries = MAX_DATABASE_ENTRIES;
size_t ConstantSettingsManager::current_spectrum_buffer_size = SPECTRUM_BUFFER_SIZE;
rf::Frequency ConstantSettingsManager::current_ism_center_freq = ISM_CENTER_FREQ;

bool ConstantSettingsManager::load_settings() {
    // Load settings from persistent storage
    app_settings::SettingsManager settings_manager{"eda_constants", app_settings::Mode::SETTINGS, {
        {"min_drone_freq"sv, &current_min_drone_freq},
        {"max_drone_freq"sv, &current_max_drone_freq},
        {"min_detection_count"sv, (void*)&current_min_detection_count},
        {"hysteresis_margin_db"sv, &current_hysteresis_margin_db},
        {"trend_threshold_db"sv, (void*)&current_trend_threshold_db},
        {"default_rssi_threshold_db"sv, &current_default_rssi_threshold_db},
        {"scan_interval_ms"sv, &current_scan_interval_ms},
        {"stale_timeout_ms"sv, &current_stale_timeout_ms},
        // Additional configurable constants
        {"rssi_smoothing_alpha"sv, &current_rssi_smoothing_alpha},
        {"detection_reset_interval"sv, &current_detection_reset_interval},
        {"max_displayed_drones"sv, &current_max_displayed_drones},
        {"mini_spectrum_min_freq"sv, &current_mini_spectrum_min_freq},
        {"mini_spectrum_max_freq"sv, &current_mini_spectrum_max_freq},
        {"sos_frequency_hz"sv, &current_sos_frequency_hz},
        {"beep_duration_ms"sv, &current_beep_duration_ms},
        {"rssi_threat_weight"sv, &current_rssi_threat_weight},
        {"database_threat_weight"sv, &current_database_threat_weight},
        // Additional hardware and buffer constants
        {"detection_table_size"sv, (void*)&current_detection_table_size},
        {"max_tracked_drones"sv, (void*)&current_max_tracked_drones},
        {"max_database_entries"sv, (void*)&current_max_database_entries},
        {"spectrum_buffer_size"sv, (void*)&current_spectrum_buffer_size},
        {"ism_center_freq"sv, &current_ism_center_freq}
    }};

    return settings_manager.load();
}

bool ConstantSettingsManager::save_settings() {
    // Save settings to persistent storage
    app_settings::SettingsManager settings_manager{"eda_constants", app_settings::Mode::SETTINGS, {
        {"min_drone_freq"sv, &current_min_drone_freq},
        {"max_drone_freq"sv, &current_max_drone_freq},
        {"min_detection_count"sv, (void*)&current_min_detection_count},
        {"hysteresis_margin_db"sv, &current_hysteresis_margin_db},
        {"trend_threshold_db"sv, (void*)&current_trend_threshold_db},
        {"default_rssi_threshold_db"sv, &current_default_rssi_threshold_db},
        {"scan_interval_ms"sv, &current_scan_interval_ms},
        {"stale_timeout_ms"sv, &current_stale_timeout_ms},
        // Additional configurable constants
        {"rssi_smoothing_alpha"sv, &current_rssi_smoothing_alpha},
        {"detection_reset_interval"sv, &current_detection_reset_interval},
        {"max_displayed_drones"sv, &current_max_displayed_drones},
        {"mini_spectrum_min_freq"sv, &current_mini_spectrum_min_freq},
        {"mini_spectrum_max_freq"sv, &current_mini_spectrum_max_freq},
        {"sos_frequency_hz"sv, &current_sos_frequency_hz},
        {"beep_duration_ms"sv, &current_beep_duration_ms},
        {"rssi_threat_weight"sv, &current_rssi_threat_weight},
        {"database_threat_weight"sv, &current_database_threat_weight},
        // Additional hardware and buffer constants
        {"detection_table_size"sv, (void*)&current_detection_table_size},
        {"max_tracked_drones"sv, (void*)&current_max_tracked_drones},
        {"max_database_entries"sv, (void*)&current_max_database_entries},
        {"spectrum_buffer_size"sv, (void*)&current_spectrum_buffer_size},
        {"ism_center_freq"sv, &current_ism_center_freq}
    }};

    return settings_manager.save();
}

void ConstantSettingsManager::reset_to_defaults() {
    // Reset all constants to defaults from ui_drone_config.hpp
    current_min_drone_freq = MIN_DRONE_FREQUENCY;
    current_max_drone_freq = MAX_DRONE_FREQUENCY;
    current_min_detection_count = MIN_DETECTION_COUNT;
    current_hysteresis_margin_db = HYSTERESIS_MARGIN_DB;
    current_trend_threshold_db = TREND_THRESHOLD_DB;
    current_default_rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
    current_scan_interval_ms = MIN_SCAN_INTERVAL_MS;
    current_stale_timeout_ms = STALE_DRONE_TIMEOUT_MS;
    // Reset additional constants
    current_rssi_smoothing_alpha = RSSI_SMOOTHING_ALPHA;
    current_detection_reset_interval = DETECTION_RESET_INTERVAL;
    current_max_displayed_drones = MAX_DISPLAYED_DRONES;
    current_mini_spectrum_min_freq = MINI_SPECTRUM_MIN_FREQ;
    current_mini_spectrum_max_freq = MINI_SPECTRUM_MAX_FREQ;
    current_sos_frequency_hz = SOS_FREQUENCY_HZ;
    current_beep_duration_ms = DETECTION_BEEP_DURATION_MS;
    current_rssi_threat_weight = RSSI_THREAT_WEIGHT;
    current_database_threat_weight = DATABASE_THREAT_WEIGHT;
    // Reset additional hardware and buffer constants
    current_detection_table_size = DETECTION_TABLE_SIZE;
    current_max_tracked_drones = MAX_TRACKED_DRONES;
    current_max_database_entries = MAX_DATABASE_ENTRIES;
    current_spectrum_buffer_size = SPECTRUM_BUFFER_SIZE;
    current_ism_center_freq = ISM_CENTER_FREQ;
}

ConstantSettingsView::ConstantSettingsView(NavigationView& nav)
    : nav_(nav) {

    // Add all UI components to the view (including new additional fields)
    add_children({
        &text_freq_range_title_,
        &text_min_drone_freq_,
        &field_min_drone_freq_,
        &text_max_drone_freq_,
        &field_max_drone_freq_,
        &text_detection_title_,
        &text_min_detection_,
        &field_min_detection_count_,
        &text_hysteresis_,
        &field_hysteresis_margin_,
        &text_trend_threshold_,
        &field_trend_threshold_,
        &text_default_rssi_,
        &field_default_rssi_threshold_,
        &text_scanning_title_,
        &text_scan_interval_,
        &field_scan_interval_,
        &text_stale_timeout_,
        &field_stale_timeout_,
        &text_spectrum_title_,
        &text_rssi_smoothing_,
        &field_rssi_smoothing_,
        &text_detection_reset_,
        &field_detection_reset_,
        &text_max_drones_,
        &field_max_drones_,
        &text_spectrum_min_,
        &field_spectrum_min_,
        &text_spectrum_max_,
        &field_spectrum_max_,
        &text_audio_title_,
        &text_sos_freq_,
        &field_sos_freq_,
        &text_beep_duration_,
        &field_beep_duration_,
        &text_threat_title_,
        &text_rssi_weight_,
        &field_rssi_weight_,
        &text_db_weight_,
        &field_db_weight_,
        &text_hardware_title_,
        &text_detection_table_,
        &field_detection_table_,
        &text_max_tracked_,
        &field_max_tracked_,
        &text_max_db_,
        &field_max_db_,
        &text_spectrum_buffer_,
        &field_spectrum_buffer_,
        &text_ism_center_,
        &field_ism_center_,
        &button_defaults_,
        &button_save_,
        &button_ok_,
        &text_status_
    });

    // Initialize field values from current settings
    initialize_field_values();

    // Set up button callbacks
    button_defaults_.on_select = [this]() { on_load_defaults(); };
    button_save_.on_select = [this]() { on_save_settings(); };
    button_ok_.on_select = [this]() { on_ok_pressed(); };

    // Set up field validation callbacks (extended for new fields)
    field_min_drone_freq_.on_change = [this](int32_t value) { validate_min_freq(value); };
    field_max_drone_freq_.on_change = [this](int32_t value) { validate_max_freq(value); };
    field_min_detection_count_.on_change = [this](int32_t) { validate_detection_params(); };
    field_hysteresis_margin_.on_change = [this](int32_t) { validate_detection_params(); };
    field_trend_threshold_.on_change = [this](int32_t) { validate_detection_params(); };
    field_default_rssi_threshold_.on_change = [this](int32_t) { validate_detection_params(); };

    update_status_text("Ready - edit values and press OK to apply");
}

void ConstantSettingsView::focus() {
    // Focus on first field for editing
    field_min_drone_freq_.focus();
}

void ConstantSettingsView::initialize_field_values() {
    // Load current values into UI fields (convert Hz to MHz for display)
    field_min_drone_freq_.set_value(ConstantSettingsManager::current_min_drone_freq / 1000000);
    field_max_drone_freq_.set_value(ConstantSettingsManager::current_max_drone_freq / 1000000);
    field_min_detection_count_.set_value(ConstantSettingsManager::current_min_detection_count);
    field_hysteresis_margin_.set_value(ConstantSettingsManager::current_hysteresis_margin_db);
    field_trend_threshold_.set_value(ConstantSettingsManager::current_trend_threshold_db);
    field_default_rssi_threshold_.set_value(ConstantSettingsManager::current_default_rssi_threshold_db);
    field_scan_interval_.set_value(ConstantSettingsManager::current_scan_interval_ms);
    field_stale_timeout_.set_value(ConstantSettingsManager::current_stale_timeout_ms);

    // Initialize additional fields with current values
    field_rssi_smoothing_.set_value(static_cast<int32_t>(ConstantSettingsManager::current_rssi_smoothing_alpha * 100)); // convert to int for display
    field_detection_reset_.set_value(ConstantSettingsManager::current_detection_reset_interval);
    field_max_drones_.set_value(ConstantSettingsManager::current_max_displayed_drones);
    field_spectrum_min_.set_value(ConstantSettingsManager::current_mini_spectrum_min_freq / 1000000000); // GHz in MHz field
    field_spectrum_max_.set_value(ConstantSettingsManager::current_mini_spectrum_max_freq / 1000000000);
    field_sos_freq_.set_value(ConstantSettingsManager::current_sos_frequency_hz);
    field_beep_duration_.set_value(ConstantSettingsManager::current_beep_duration_ms);
    field_rssi_weight_.set_value(static_cast<int32_t>(ConstantSettingsManager::current_rssi_threat_weight * 100));
    field_db_weight_.set_value(static_cast<int32_t>(ConstantSettingsManager::current_database_threat_weight * 100));

    // Initialize new hardware and buffer constants
    field_detection_table_.set_value(ConstantSettingsManager::current_detection_table_size);
    field_max_tracked_.set_value(ConstantSettingsManager::current_max_tracked_drones);
    field_max_db_.set_value(ConstantSettingsManager::current_max_database_entries);
    field_spectrum_buffer_.set_value(ConstantSettingsManager::current_spectrum_buffer_size);
    field_ism_center_.set_value(ConstantSettingsManager::current_ism_center_freq / 1000000);  // Hz to MHz
}

void ConstantSettingsView::update_status_text(const char* text) {
    text_status_.set(text);
}

bool ConstantSettingsView::validate_all_fields() {
    // Validate frequency ranges
    int32_t min_freq = field_min_drone_freq_.get_value();
    int32_t max_freq = field_max_drone_freq_.get_value();

    if (min_freq < 50 || min_freq > 6000) {  // 50MHz to 6000MHz (6GHz)
        update_status_text("Error: Min frequency out of range (50MHz-6000MHz)");
        return false;
    } else if (max_freq < min_freq || max_freq > 6000) {
        update_status_text("Error: Max frequency must be >= Min and <= 6000MHz");
        return false;
    }

    // Validate detection parameters
    int32_t min_det = field_min_detection_count_.get_value();
    int32_t hysteresis = field_hysteresis_margin_.get_value();
    int32_t trend_thresh = field_trend_threshold_.get_value();
    int32_t rssi_thresh = field_default_rssi_threshold_.get_value();

    if (min_det < 1 || min_det > 10) {
        update_status_text("Error: Min detection count must be 1-10");
        return false;
    } else if (hysteresis < -20 || hysteresis > 20) {
        update_status_text("Error: Hysteresis margin must be -20dB to +20dB");
        return false;
    } else if (trend_thresh < 1 || trend_thresh > 10) {
        update_status_text("Error: Trend threshold must be 1-10 dB");
        return false;
    } else if (rssi_thresh < -120 || rssi_thresh > -10) {
        update_status_text("Error: Default RSSI threshold must be -120dB to -10dB");
        return false;
    }

    update_status_text("All values valid - press OK to apply");
    return true;
}

void ConstantSettingsView::apply_current_values() {
    // Apply current field values to the manager
    ConstantSettingsManager::current_min_drone_freq = field_min_drone_freq_.get_value() * 1000000ULL;
    ConstantSettingsManager::current_max_drone_freq = field_max_drone_freq_.get_value() * 1000000ULL;
    ConstantSettingsManager::current_min_detection_count = field_min_detection_count_.get_value();
    ConstantSettingsManager::current_hysteresis_margin_db = field_hysteresis_margin_.get_value();
    ConstantSettingsManager::current_trend_threshold_db = field_trend_threshold_.get_value();
    ConstantSettingsManager::current_default_rssi_threshold_db = field_default_rssi_threshold_.get_value();
    ConstantSettingsManager::current_scan_interval_ms = field_scan_interval_.get_value();
    ConstantSettingsManager::current_stale_timeout_ms = field_stale_timeout_.get_value();

    // Apply additional field values
    ConstantSettingsManager::current_rssi_smoothing_alpha = field_rssi_smoothing_.get_value() / 100.0f; // convert back from int
    ConstantSettingsManager::current_detection_reset_interval = field_detection_reset_.get_value();
    ConstantSettingsManager::current_max_displayed_drones = field_max_drones_.get_value();
    ConstantSettingsManager::current_mini_spectrum_min_freq = field_spectrum_min_.get_value() * 1000000000ULL; // convert GHz back to Hz
    ConstantSettingsManager::current_mini_spectrum_max_freq = field_spectrum_max_.get_value() * 1000000000ULL; // convert GHz back to Hz
    ConstantSettingsManager::current_sos_frequency_hz = field_sos_freq_.get_value();
    ConstantSettingsManager::current_beep_duration_ms = field_beep_duration_.get_value();
    ConstantSettingsManager::current_rssi_threat_weight = field_rssi_weight_.get_value() / 100.0f;
    ConstantSettingsManager::current_database_threat_weight = field_db_weight_.get_value() / 100.0f;

    // Apply new hardware and buffer constants
    ConstantSettingsManager::current_detection_table_size = field_detection_table_.get_value();
    ConstantSettingsManager::current_max_tracked_drones = field_max_tracked_.get_value();
    ConstantSettingsManager::current_max_database_entries = field_max_db_.get_value();
    ConstantSettingsManager::current_spectrum_buffer_size = field_spectrum_buffer_.get_value();
    ConstantSettingsManager::current_ism_center_freq = field_ism_center_.get_value() * 1000000ULL;
}

void ConstantSettingsView::validate_min_freq(int32_t value) {
    // Check if min frequency is within hardware limits
    if (value < 50 || value > 6000) {
        update_status_text("Warning: Frequency outside typical range");
    } else {
        validate_all_fields();
    }
}

void ConstantSettingsView::validate_max_freq(int32_t value) {
    // Check if max frequency is >= min frequency
    int32_t min_freq = field_min_drone_freq_.get_value();
    if (value < min_freq) {
        update_status_text("Error: Max frequency must be >= Min frequency");
    } else {
        validate_all_fields();
    }
}

void ConstantSettingsView::validate_detection_params() {
    validate_all_fields();
}

void ConstantSettingsView::on_load_defaults() {
    // Reset manager to defaults and update UI
    ConstantSettingsManager::reset_to_defaults();
    initialize_field_values();
    update_status_text("Default values loaded - press OK to apply");
}

void ConstantSettingsView::on_save_settings() {
    // Validate all fields first
    if (!validate_all_fields()) {
        return;
    }

    // Apply current values and save to persistent storage
    apply_current_values();

    if (ConstantSettingsManager::save_settings()) {
        update_status_text("Settings saved successfully");
    } else {
        update_status_text("Error: Failed to save settings");
    }
}

void ConstantSettingsView::on_ok_pressed() {
    // Validate all fields first
    if (!validate_all_fields()) {
        return;
    }

    // Apply current values (but don't save automatically)
    apply_current_values();
    update_status_text("Settings applied - remember to save if needed");

    // Close the dialog
    nav_.pop();
}

void ConstantSettingsView::on_cancel() {
    // Close without applying changes
    nav_.pop();
}

} // anonymous namespace

#endif // __UI_DRONE_SETTINGS_COMPLETE_HPP__
