#ifndef PATTERN_MANAGER_VIEW_HPP
#define PATTERN_MANAGER_VIEW_HPP

#include <cstdint>
#include <cstddef>

#include "ui_painter.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "event_m0.hpp"

#include "pattern_manager.hpp"
#include "scanner.hpp"
#include "constants.hpp"

namespace drone_analyzer {

class DroneScanner;

/**
 * @brief Pattern capture and management UI for spectrum fingerprint matching.
 * @note Replaces the deleted PatternManagerView (dcc2d456) with fixes:
 *       - Thread-safe get_config() (value return, not reference)
 *       - Stack-safe refresh_list() (64B vs old 704B)
 *       - DBLREG-safe handler lifecycle (register on show, unregister on hide)
 *       - baseband_needs_restore_ flag for proper baseband recovery
 *
 * @note Follows handler lifecycle pattern:
 *       - Registers own handlers (ChannelSpectrumConfig + DisplayFrameSync)
 *       - Main DroneScannerUI unregisters its handlers before push
 *       - On return, DroneScannerUI::on_show() re-registers its handlers
 *
 * SRAM: ~1,676 B (capture_spectrum 256B + spectrum_buffer 256B + members ~1,100B)
 * Stack: max 288 B (capture_and_save with PeakDetector sort_buf)
 * Flash: ~2 KB (code)
 */
class PatternManagerView : public ui::View {
public:
    explicit PatternManagerView(NavigationView& nav) noexcept;
    ~PatternManagerView() noexcept override;

    PatternManagerView(const PatternManagerView&) = delete;
    PatternManagerView& operator=(const PatternManagerView&) = delete;

    void paint(ui::Painter& painter) override;
    void focus() override;
    void on_show() override;
    void on_hide() override;
    bool on_touch(const ui::TouchEvent event) override;

    std::string title() const override {
        static const std::string t = "PTR Pattern";
        return t;
    }

private:
    static constexpr uint16_t SPECTRUM_Y = 40;
    static constexpr uint16_t SPECTRUM_HEIGHT = 100;
    static constexpr uint16_t SPECTRUM_X = 0;
    static constexpr uint16_t SPECTRUM_WIDTH = 240;
    static constexpr uint16_t LIST_Y = 150;
    static constexpr uint16_t BUTTONS2_Y = 290;

    enum class ViewState : uint8_t {
        IDLE,
        CAPTURING,
        LIVE
    };

    NavigationView& nav_;

    PatternManager* pm_{nullptr};
    DroneScanner* scanner_{nullptr};

    // UI widgets
    ui::Labels labels_;
    ui::OptionsField field_patterns_;
    ui::Button button_capture_;
    ui::Button button_save_;
    ui::Button button_delete_;
    ui::Button button_toggle_;
    ui::Button button_back_;
    ui::NumberField field_freq_mhz_;
    ui::Button button_enable_all_;
    ui::Button button_disable_all_;
    ui::Text label_status_;

    // State
    uint8_t selected_index_{0};
    ViewState view_state_{ViewState::IDLE};
    bool capture_active_{false};
    bool capture_completed_{false};
    int16_t selected_bin_{-1};
    FreqHz capture_freq_{0};

    // Sweep state for LIVE mode
    FreqHz sweep_start_{0};
    FreqHz sweep_end_{0};
    FreqHz sweep_step_{0};
    FreqHz current_sweep_freq_{0};

    // Display buffers (BSS, not stack)
    uint8_t capture_spectrum_[FFT_BIN_COUNT]{};
    ChannelSpectrum spectrum_buffer_{};
    ChannelSpectrumFIFO* spectrum_fifo_{nullptr};

    // Baseband config saved from scanner (value copy, thread-safe)
    ScanConfig scanner_config_{};

    // Message handler lifecycle (register on show, unregister on hide)
    struct HandlerStorage {
        MessageHandlerRegistration spectrum_config;
        MessageHandlerRegistration frame_sync;
    };
    static_assert(sizeof(HandlerStorage) <= 128, "HandlerStorage unexpectedly large");
    alignas(alignof(HandlerStorage)) uint8_t handler_storage_[sizeof(HandlerStorage)];
    bool handlers_active_{false};

    void register_handlers() noexcept;
    void unregister_handlers() noexcept;

    void on_channel_spectrum_config(ChannelSpectrumFIFO* fifo) noexcept;
    void on_frame_sync() noexcept;

    void draw_spectrum(ui::Painter& painter, const uint8_t* spectrum, int16_t sel_bin) noexcept;
    void refresh_list() noexcept;
    void capture_and_save() noexcept;
    void delete_selected() noexcept;
    void toggle_enabled() noexcept;
    void start_capture() noexcept;
    void init_sweep_range() noexcept;

    void on_bin_selected(int16_t bin) noexcept;
    FreqHz bin_to_frequency(int16_t bin) const noexcept;
};

}  // namespace drone_analyzer

#endif  // PATTERN_MANAGER_VIEW_HPP
