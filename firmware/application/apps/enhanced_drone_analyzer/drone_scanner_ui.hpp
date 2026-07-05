#ifndef DRONE_SCANNER_UI_HPP
#define DRONE_SCANNER_UI_HPP

#include <cstdint>
#include <cstddef>
#include <array>

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "message.hpp"
#include "portapack.hpp"
#include "radio_state.hpp"
#include "app_settings.hpp"

#include "drone_types.hpp"
#include "constants.hpp"
#include "scanner.hpp"
#include "scanner_thread.hpp"
#include "drone_display.hpp"
#include "sweep_processor.hpp"

namespace drone_analyzer {

enum class BigDisplayColor : int8_t {
    GREY = -2,
    YELLOW = -1,
    GREEN = 0,
    RED = 1
};

class DroneScanner;
class DatabaseManager;
class HardwareController;
struct ScanConfig;

class DroneScannerUI : public ui::View {
public:
    explicit DroneScannerUI(NavigationView& nav) noexcept;
    ~DroneScannerUI() noexcept override;

    DroneScannerUI(const DroneScannerUI&) = delete;
    DroneScannerUI& operator=(const DroneScannerUI&) = delete;

    std::string title() const override {
        static const std::string t = "EDA";
        return t;
    }
    void paint(Painter& painter) override;
    void focus() override;
    void on_show() override;
    void on_hide() override;

    void show_alert(const char* message, uint32_t duration_ms) noexcept;
    void show_error(ErrorCode error, uint32_t duration_ms) noexcept;

private:

    static constexpr uint16_t BIG_FREQUENCY_X = 4;
    static constexpr uint16_t BIG_FREQUENCY_Y = 1 * 16;
    static constexpr uint16_t BIG_FREQUENCY_WIDTH = 28 * 8;
    static constexpr uint16_t DRONE_TYPE_SPACING = 5;
    static constexpr uint16_t DRONE_TYPE_Y_OFFSET = 20;

    static constexpr uint32_t ERROR_DURATION_MS = 3000;

    void construct_objects() noexcept;
    void destruct_objects() noexcept;

    NavigationView& nav_;

    RxRadioState radio_state_{ReceiverModel::Mode::SpectrumAnalysis};

    ui::BigFrequency big_display_;

    HardwareController* hardware_ptr_{nullptr};
    DatabaseManager* database_ptr_{nullptr};
    DroneScanner* scanner_ptr_{nullptr};
    ScannerThread* scanner_thread_{nullptr};

    ui::Labels labels_{
        {{UI_POS_X(0), UI_POS_Y(0)}, "LNA   VGA   AMP  ", Color::white()},
    };

    ui::LNAGainField field_lna_{{UI_POS_X(4), 0}};
    ui::VGAGainField field_vga_{{UI_POS_X(11), 0}};
    ui::RFAmpField field_rf_amp_{{UI_POS_X(18), 0}};
    ui::AudioVolumeField field_volume_{{UI_POS_X_RIGHT(2), UI_POS_Y(0)}};
    ui::NumberField field_rssi_dec_cyc_{{UI_POS_X_RIGHT(6), UI_POS_Y(0)}, 2, {1, 50}, 1, ' '};
    ui::Labels labels_cyc_{
        {{UI_POS_X_RIGHT(9), UI_POS_Y(0)}, "cy", Color::white()},
    };

    ui::Labels filter_labels_{
        {{UI_POS_X(0), 268}, "FILT:", Color::white()},
    };

    ui::OptionsField field_filter_{
        {UI_POS_X(5), 268},
        4,
        {
            {"OFF ", SPECTRUM_FILTER_OFF},
            {"MID ", SPECTRUM_FILTER_MID},
            {"HIGH", SPECTRUM_FILTER_HIGH},
        }};

    ui::Button button_median_{{UI_POS_X(10), 268, UI_POS_WIDTH(3), 16}, "OFF"};

    ui::Button button_start_stop_{{UI_POS_X(0), 284, UI_POS_WIDTH(6), 28}, "Start"};
    ui::Button button_mode_{{UI_POS_X(7), 284, UI_POS_WIDTH(5), 28}, "Mode"};
    ui::Button button_load_{{UI_POS_X(13), 284, UI_POS_WIDTH(4), 28}, "Load"};
    ui::Button button_settings_{{UI_POS_X(18), 284, UI_POS_WIDTH(4), 28}, "Set"};
    ui::Button button_swp_{{UI_POS_X(23), 284, UI_POS_WIDTH(3), 28}, "SWP"};
    ui::Button button_ptr_{{UI_POS_X(27), 284, UI_POS_WIDTH(3), 28}, "PTR"};

    FreqHz current_frequency_{0};
    void set_current_frequency_safe(FreqHz freq) noexcept;
    [[nodiscard]] FreqHz get_current_frequency_safe() const noexcept;
    int32_t current_rssi_{RSSI_NOISE_FLOOR_DBM};
    ScannerState current_scanner_state_{ScannerState::IDLE};

    char displayed_drone_type_[MAX_DRONE_TYPE_DISPLAY + 1]{};
    uint32_t drone_type_display_timer_{0};

    bool scanning_{false};
    ScanningMode scanning_mode_{DEFAULT_SCANNING_MODE};

    bool alert_active_{false};
    char alert_message_[MAX_TEXT_LENGTH]{};
    uint32_t alert_start_time_{0};
    uint32_t alert_duration_ms_{0};

    bool error_active_{false};
    ErrorCode last_error_{ErrorCode::SUCCESS};
    uint32_t error_start_time_{0};
    uint32_t error_duration_ms_{0};

    ChannelSpectrumFIFO* spectrum_fifo_{nullptr};
    bool initialization_failed_{false};
    bool db_loaded_{false};
    size_t db_entry_count_{0};
    FreqHz last_tuned_freq_{0};  // exact freq radio was tuned to when FFT was captured
    bool skip_next_fft_{false};  // skip first FFT after sweep entry (may be stale)

    // Reusable buffer to prevent stack overflow in message handler
    // ChannelSpectrum is 256 bytes - moved from local stack to BSS
    ChannelSpectrum spectrum_buffer_{};

    DroneDisplay drone_display_{{0, 68, DISPLAY_WIDTH, 206}};

    void bigdisplay_update(BigDisplayColor color) noexcept;
    void refresh_ui() noexcept;
    void on_channel_spectrum(const ChannelSpectrum& spectrum) noexcept;
    void on_retune(FreqHz freq, uint32_t range) noexcept;

    // Band sweep — Looking Glass pattern: stop → process → retune → start
    // COMPOSITE_SIZE, SWEEP_SLICE_BW, MAX_SWEEP_WINDOWS defined in constants.hpp
    static constexpr uint8_t DB_SCANS_PER_SWEEP = 50;
    // EACH_BIN_SIZE removed — use SWEEP_BIN_SIZE from constants.hpp instead

    /**
     * @brief Encapsulates all state for a single sweep window (Meyers: replace duplication with data)
     * @note SRAM cost: ~300B per window (240 bytes composite + ~50 bytes metadata)
     * @note Total for MAX_SWEEP_WINDOWS (4) windows: ~1,200B BSS
     * @note Within 128KB SRAM budget but significant for resource-constrained system
     */
    struct SweepWindow {
        uint8_t composite[COMPOSITE_SIZE]{};  // pixel buffer
        FreqHz f_min{0};
        FreqHz f_max{0};
        FreqHz f_center{0};
        FreqHz f_center_ini{0};
        FreqHz pixel_step_hz{0};
        FreqHz step_hz{0};
        FreqHz bins_hz_acc{0};
        FreqHz exceptions[EXCEPTIONS_PER_WINDOW]{};  // exception frequencies (0 = unused)
        FreqHz exception_radius_hz{3000000ULL};       // configurable exclusion radius (Hz)
        uint16_t pixel_index{0};
        uint8_t pixel_max{0};
        uint8_t settle_frames_remaining_{0};  // frames to skip after retune
        bool enabled{false};

        void init(FreqHz start, FreqHz end, FreqHz step = 0) noexcept;
        void reset() noexcept;
        void process_bins(const ChannelSpectrum& spectrum) noexcept;

        /**
         * @brief Check if a frequency falls within ±EXCEPTION_RADIUS_HZ of any exception
         * @param hz Frequency to check
         * @return true if frequency should be suppressed
         */
        [[nodiscard]] bool is_exception(FreqHz hz) const noexcept;
    };

    SweepWindow sweep_[MAX_SWEEP_WINDOWS]{};

    bool composite_active_{false};
    bool sweep_auto_mode_{false};
    uint8_t active_sweep_idx_{0};         // 0-3, round-robin index
    uint8_t current_pair_{0};             // Current displayed pair index (0 or 2)
    uint8_t db_scan_count_{0};
    AtomicFlag sweep_transition_guard_;   // Prevents concurrent enter/exit
    AtomicFlag button_debounce_guard_;     // Debounces button_mode_/start_stop rapid taps
    FreqHz last_db_frequency_{0};         // Last DB frequency before sweep
    size_t last_db_index_{0};             // Last DB index before sweep (for exact restore)

    void enter_sweep_mode() noexcept;
    void exit_sweep_mode(bool suppress_auto_restart = false) noexcept;
    void on_sweep_spectrum(const ChannelSpectrum& spectrum) noexcept;
    void retune_sweep_window(SweepWindow& win, const char* prefix = nullptr) noexcept;
    void update_sweep_pair_display() noexcept;
    [[nodiscard]] uint8_t pair_first(uint8_t idx) const noexcept;

    // Spectrum filter threshold (OFF/MID/HIGH)
    uint8_t min_color_power_{DEFAULT_SPECTRUM_FILTER};

    // Median filter toggle state
    bool median_enabled_{true};

    // Latest ChannelStatistics.max_db from baseband (full-bandwidth RSSI)
    int32_t latest_max_db_{RSSI_NOISE_FLOOR_DBM};

    // Reusable buffers for refresh_ui() (class members instead of static locals)
    DisplayData refresh_display_data_{};
    TrackedDrone refresh_drones_[MAX_DISPLAYED_DRONES]{};
    char refresh_status_buf_[MAX_TEXT_LENGTH]{};
    uint16_t refresh_hist_data_[HISTOGRAM_BUFFER_SIZE]{};

    // Per-frame Looking Glass reordered buffer for sweep mode shape analysis.
    // Created by SweepProcessor::reorder_frame() and passed to scanner's
    // process_spectrum_sweep(lg_buffer) overload. Eliminates the DC gap
    // corruption by operating on the same continuous 240-pixel line the
    // user sees on screen. 240 bytes BSS, shared across all sweep windows.
    // Total sweep-related SRAM: sweep_[4] (~1,200B) + lg_frame_buf_ (240B) = ~1,440B
    uint8_t lg_frame_buf_[COMPOSITE_SIZE]{};

    /**
     * @brief Storage layout for message handlers.
     * @note No constructor — members are placement-new'd manually.
     */
    struct HandlerStorage {
        MessageHandlerRegistration spectrum_config;
        MessageHandlerRegistration frame_sync;
        MessageHandlerRegistration retune;
        MessageHandlerRegistration channel_stats;
    };
    // handler_storage_ is explicitly sized to sizeof(HandlerStorage) — guaranteed safe.
    // Prevents DBLREG hard fault via placement new at runtime.
    static_assert(sizeof(HandlerStorage) <= 256, "HandlerStorage unexpectedly large (>256 bytes)");
    alignas(alignof(HandlerStorage)) uint8_t handler_storage_[sizeof(HandlerStorage)];
    bool handlers_active_{false};

    void register_handlers() noexcept;
    void unregister_handlers() noexcept;

    bool on_touch(const ui::TouchEvent event) override;
    bool baseband_needs_restore_{false};
    bool scanning_needs_restore_{false};
};

DroneScanner& get_scanner_instance() noexcept;
DroneScanner* get_scanner_ptr() noexcept;

} // namespace drone_analyzer

#endif // DRONE_SCANNER_UI_HPP
