#ifndef DRONE_SETTINGS_HPP
#define DRONE_SETTINGS_HPP

#include <cstdint>
#include <cstddef>
#include "drone_types.hpp"
#include "constants.hpp"
#include "scanner.hpp"
#include "settings_manager.hpp"

#include "ui_painter.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"

namespace drone_analyzer {

/**
 * @brief Settings UI component for drone analyzer
 * @note Inherits from ui::View
 * @note Uses SettingsFileManager for all SD card I/O (no duplicated parser)
 */
class DroneScanner;
class DroneDisplay;

class DroneSettingsView : public ui::View {
public:
    explicit DroneSettingsView(NavigationView& nav, const ScanConfig& config, DroneScanner* scanner_ptr, DroneDisplay* display = nullptr) noexcept;

    ~DroneSettingsView() noexcept override;

    DroneSettingsView(const DroneSettingsView&) = delete;
    DroneSettingsView& operator=(const DroneSettingsView&) = delete;

    void paint(ui::Painter& painter) override;

    void focus() override;

    std::string title() const override { return "EDA Settings"; }

private:
    ui::Labels labels_;
    ui::NumberField field_scan_interval_;
    ui::NumberField field_rssi_threshold_;
    ui::NumberField field_volume_;
    ui::NumberField field_rssi_dec_cyc_;
    ui::Checkbox check_mahalanobis_;
    ui::NumberField field_mahalanobis_threshold_;
    ui::Checkbox check_audio_alerts_;
    ui::Checkbox check_spectrum_visible_;
    ui::Checkbox check_histogram_visible_;

    // Detection features
    ui::Checkbox check_dwell_enabled_;
    ui::Checkbox check_confirm_count_;
    ui::NumberField field_confirm_count_;
    ui::Checkbox check_spectrum_detection_;

    // Neighbor margin
    ui::NumberField field_neighbor_margin_;
    ui::Checkbox check_neighbor_margin_;

    ui::Checkbox check_noise_blacklist_;
    ui::Checkbox check_rssi_variance_;

    // Spectrum detection params
    ui::NumberField field_spectrum_margin_;
    ui::NumberField field_spectrum_min_width_;
    ui::NumberField field_spectrum_max_width_;
    ui::NumberField field_spectrum_peak_sharpness_;
    ui::NumberField field_spectrum_peak_ratio_;
    ui::NumberField field_spectrum_valley_depth_;
    ui::NumberField field_spectrum_flatness_;
    ui::NumberField field_spectrum_symmetry_;

    ui::Button button_defaults_;
    ui::Button button_about_;
    ui::Button button_save_;

    // CFAR detection controls
    ui::OptionsField field_cfar_mode_;
    ui::NumberField field_cfar_ref_cells_;
    ui::NumberField field_cfar_guard_cells_;
    ui::NumberField field_cfar_threshold_;

    NavigationView& nav_;
    DroneScanner* scanner_ptr_;
    DroneDisplay* display_ptr_;

    ScanConfig original_config_;
    SettingsStruct settings_;
    bool settings_dirty_;

    void apply_settings_to_ui() noexcept;
    void save_settings_to_sd() noexcept;
};

} // namespace drone_analyzer

#endif // DRONE_SETTINGS_HPP
