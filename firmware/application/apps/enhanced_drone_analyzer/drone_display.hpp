#ifndef DRONE_DISPLAY_HPP
#define DRONE_DISPLAY_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include "ui_widget.hpp"
#include "drone_types.hpp"
#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief Display component for drone analyzer UI
 * @note Inherits from ui::View, renders spectrum, histogram, drone list, status bar
 * @note Uses static storage for large buffers to reduce stack usage
 * @note Removed features: mini spectrum, RSSI history, frequency ruler, detailed info
 */
class DroneDisplay : public ui::View {
public:
    DroneDisplay(const Rect parent_rect) noexcept;
    ~DroneDisplay() noexcept override;

    DroneDisplay(const DroneDisplay&) = delete;
    DroneDisplay& operator=(const DroneDisplay&) = delete;

    /**
     * @brief Paint method - main rendering entry point
     * @note Uses static storage for large buffers to reduce stack usage
     * @note Stack optimized: large buffers in static storage
     */
    void paint(Painter& painter) override;

    /**
     * @brief Render spectrum display
     * @param painter Painter instance for drawing
     * @param spectrum_data Spectrum data buffer
     * @param spectrum_size Size of spectrum data
     * @param start_x Starting X coordinate
     * @param start_y Starting Y coordinate
     * @param width Display width
     * @param height Display height
     */
    void render_spectrum(
        Painter& painter,
        const uint8_t* spectrum_data,
        size_t spectrum_size,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height
    ) noexcept;

    /**
     * @brief Render histogram display
     * @param painter Painter instance for drawing
     * @param histogram_data Histogram data buffer
     * @param histogram_size Size of histogram data
     * @param start_x Starting X coordinate
     * @param start_y Starting Y coordinate
     * @param width Display width
     * @param height Display height
     */
    void render_histogram(
        Painter& painter,
        const uint16_t* histogram_data,
        size_t histogram_size,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height
    ) noexcept;

    /**
     * @brief Render drone list display
     * @param painter Painter instance for drawing
     * @param drones Array of drone entries to display
     * @param drone_count Number of drones to display
     * @param start_x Starting X coordinate
     * @param start_y Starting Y coordinate
     * @param width Display width
     * @param height Display height
     */
    void render_drone_list(
        Painter& painter,
        const DisplayDroneEntry* drones,
        size_t drone_count,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height
    ) noexcept;

    /**
     * @brief Render status bar
     * @param painter Painter instance for drawing
     * @param status_text Status text to display
     * @param start_x Starting X coordinate
     * @param start_y Starting Y coordinate
     * @param width Display width
     * @param height Display height
     */
    void render_status_bar(
        Painter& painter,
        const char* status_text,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height
    ) noexcept;

    /**
     * @brief Update display data
     * @param display_data New display data
     * @return ErrorCode::SUCCESS if updated, error code otherwise
     */
    ErrorCode update_display_data(const DisplayData& display_data) noexcept;

    /**
     * @brief Get current display data
     * @return Reference to current display data
     */
    [[nodiscard]] const DisplayData& get_display_data() const noexcept;

    /**
     * @brief Clear display
     * @param painter Painter instance for drawing
     */
    void clear_display(Painter& painter) noexcept;

    /**
     * @brief Set spectrum data for rendering
     * @param spectrum_data Spectrum data buffer
     * @param spectrum_size Size of spectrum data
     * @return ErrorCode::SUCCESS if set, error code otherwise
     */
    ErrorCode set_spectrum_data(
        const uint8_t* spectrum_data,
        size_t spectrum_size
    ) noexcept;

    /**
     * @brief Set histogram data for rendering
     * @param histogram_data Histogram data buffer
     * @param histogram_size Size of histogram data
     * @return ErrorCode::SUCCESS if set, error code otherwise
     */
    ErrorCode set_histogram_data(
        const uint16_t* histogram_data,
        size_t histogram_size
    ) noexcept;

    /**
     * @brief Set status text
     * @param status_text Status text to display
     */
    void set_status_text(const char* status_text) noexcept;

    void set_spectrum_visible(bool visible) noexcept { spectrum_visible_ = visible; dirty_flags_ = DIRTY_ALL; set_dirty(); }
    void set_histogram_visible(bool visible) noexcept { histogram_visible_ = visible; dirty_flags_ = DIRTY_ALL; set_dirty(); }
    [[nodiscard]] bool get_histogram_visible() const noexcept { return histogram_visible_; }
    void set_drone_list_visible(bool visible) noexcept { drone_list_visible_ = visible; dirty_flags_ = DIRTY_ALL; set_dirty(); }
    void set_status_bar_visible(bool visible) noexcept { status_bar_visible_ = visible; dirty_flags_ = DIRTY_ALL; set_dirty(); }

    void set_spectrum_filter(uint8_t min_power) noexcept { min_color_power_ = min_power; dirty_flags_ |= DIRTY_SPEC; set_dirty(); }
    void set_spectrum_integration(uint8_t factor) noexcept { spectrum_integration_ = factor; dirty_flags_ |= DIRTY_SPEC; set_dirty(); }

    /**
     * @brief Set spectrum shape filter parameters (margin, min_width, max_width)
     * @param margin Minimum peak margin above noise floor (bins below this are suppressed)
     * @param min_width Minimum signal width in bins (isolated spikes removed)
     * @param max_width Maximum signal width (flat noise rejected)
     */
    void set_spectrum_shape_params(uint8_t margin, uint8_t min_width, uint8_t max_width) noexcept {
        spectrum_shape_margin_ = margin;
        spectrum_shape_min_width_ = min_width;
        spectrum_shape_max_width_ = max_width;
    }

    void set_composite_mode(bool enabled) noexcept { composite_mode_ = enabled; dirty_flags_ = DIRTY_ALL; set_dirty(); }
    void set_composite_data(const uint8_t* data, size_t size) noexcept;
    void set_multi_zone_data(const uint8_t buffers[][240], uint8_t zone_count, size_t buffer_size,
                             const FreqHz* freq_starts, const FreqHz* freq_ends) noexcept;
    void set_sweep_range(FreqHz start, FreqHz end) noexcept {
        sweep_freq_start_ = start;
        sweep_freq_end_ = end;
        dirty_flags_ |= DIRTY_SPEC;
        set_dirty();
    }

    // Dual-sweep support
    void set_dual_sweep_mode(bool enabled) noexcept { dual_sweep_mode_ = enabled; dirty_flags_ = DIRTY_ALL; set_dirty(); }
    void set_sweep2_data(const uint8_t* data, size_t size) noexcept;
    void set_sweep2_range(FreqHz start, FreqHz end) noexcept {
        sweep2_freq_start_ = start;
        sweep2_freq_end_ = end;
        dirty_flags_ |= DIRTY_SPEC;
        set_dirty();
    }

    /**
     * @brief Reset EMA persistence buffer and auto-computed noise floor.
     * @note Call from sweep pass boundary (pair_complete / exit_sweep_mode) so
     *       stale peaks from the previous pass do not mask fresh pixels of the
     *       new pass. Without this, the auto-noise-floor stays at the level of
     *       the previous pass's signals and subtraction zeroes new pixels.
     * @pre  Called only from UI thread.
     * @post composite_persist_buf_ = 0, init = false, noise_floor_valid = false.
     *       composite_data_ and composite_data_size_ are PRESERVED — the zeroed
     *       persist buffer renders empty bars until set_composite_data() fills it.
     *       This prevents the layout collapse that caused the sweep-hang bug.
     * @note Stack: 0 B. SRAM: 0 B (in-place zeroing of existing buffer).
     */
    void reset_composite_persistence() noexcept;

    /**
     * @brief Recompute noise floor from EMA persistence buffer using quickselect.
     * @note Called once per sweep pass (pair_complete), NOT per frame.
     *       Eliminates ~1200 integer comparisons per frame that previously ran
     *       in set_composite_data() and set_sweep2_data().
     * @note Uses composite_sort_buf_ / sweep2_sort_buf_ as temporary storage.
     * @note SRAM: 0 B additional (uses existing sort buffers).
     */
    void update_noise_floor() noexcept;

    /**
     * @brief Set real-time scan-head position for the upper and lower composite bands.
     * @param upper Pixel index 0..COMPOSITE_SIZE-1, or -1 to hide on upper band.
     * @param lower Pixel index 0..COMPOSITE_SIZE-1, or -1 to hide on lower band.
     * @note Draws a 1-px white vertical marker so the user sees where the sweep
     *       currently is, independent of pixel-value filtering.
     */
    void set_scan_head(int16_t upper, int16_t lower) noexcept {
        scan_head_position_[0] = upper;
        scan_head_position_[1] = lower;
        dirty_flags_ |= DIRTY_SPEC;
        set_dirty();
    }

    [[nodiscard]] const char* get_status_text() const noexcept;

private:
    /**
     * @brief Draw spectrum line
     * @param painter Painter instance for drawing
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Line color
     */
    void draw_spectrum_line(
        Painter& painter,
        uint16_t x,
        uint16_t y,
        uint32_t color
    ) noexcept;

    /**
     * @brief Draw histogram bar
     * @param painter Painter instance for drawing
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Bar width
     * @param height Bar height
     * @param color Bar color
     */
    void draw_histogram_bar(
        Painter& painter,
        uint16_t x,
        uint16_t y,
        uint16_t width,
        uint16_t height,
        uint32_t color
    ) noexcept;

    /**
     * @brief Draw drone entry
     * @param painter Painter instance for drawing
     * @param drone Drone entry to draw
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Entry width
     * @param height Entry height
     */
    void draw_drone_entry(
        Painter& painter,
        const DisplayDroneEntry& drone,
        uint16_t x,
        uint16_t y,
        uint16_t width,
        uint16_t height
    ) noexcept;

    /**
     * @brief Draw text
     * @param painter Painter instance for drawing
     * @param text Text to draw
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Text color
     */
    void draw_text(
        Painter& painter,
        const char* text,
        uint16_t x,
        uint16_t y,
        uint32_t color
    ) noexcept;

    void draw_text(
        Painter& painter,
        std::string_view text,
        uint16_t x,
        uint16_t y,
        uint32_t color
    ) noexcept;

    /**
     * @brief Draw rectangle
     * @param painter Painter instance for drawing
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Rectangle width
     * @param height Rectangle height
     * @param color Rectangle color
     * @param fill Fill rectangle if true, outline only if false
     */
    void draw_rectangle(
        Painter& painter,
        uint16_t x,
        uint16_t y,
        uint16_t width,
        uint16_t height,
        uint32_t color,
        bool fill = true
    ) noexcept;

    /**
     * @brief Map RSSI value to display height
     * @param rssi RSSI value in dBm
     * @param max_height Maximum display height
     * @return Mapped height in pixels
     */
    [[nodiscard]] uint16_t map_rssi_to_height(
        RssiValue rssi,
        uint16_t max_height
    ) const noexcept;

    /**
     * @brief Get color for threat level
     * @param threat Threat level
     * @return Color value
     */
    [[nodiscard]] uint32_t get_threat_color(ThreatLevel threat) const noexcept;

    /**
     * @brief Format frequency for display
     * @param frequency Frequency in Hz
     * @param buffer Buffer to store formatted string
     * @param buffer_size Buffer size
     */
    void format_frequency(
        FreqHz frequency,
        char* buffer,
        size_t buffer_size
    ) const noexcept;

    /**
     * @brief Format RSSI for display
     * @param rssi RSSI value in dBm
     * @param buffer Buffer to store formatted string
     * @param buffer_size Buffer size
     */
    void format_rssi(
        RssiValue rssi,
        char* buffer,
        size_t buffer_size
    ) const noexcept;

    /**
     * @brief Validate spectrum data
     * @param spectrum_data Spectrum data buffer
     * @param spectrum_size Size of spectrum data
     * @return ErrorCode::SUCCESS if valid, error code otherwise
     */
    [[nodiscard]] ErrorCode validate_spectrum_data(
        const uint8_t* spectrum_data,
        size_t spectrum_size
    ) const noexcept;

    /**
     * @brief Validate histogram data
     * @param histogram_data Histogram data buffer
     * @param histogram_size Size of histogram data
     * @return ErrorCode::SUCCESS if valid, error code otherwise
     */
    [[nodiscard]] ErrorCode validate_histogram_data(
        const uint16_t* histogram_data,
        size_t histogram_size
    ) const noexcept;

    /**
     * @brief Clamp value to range
     * @param value Value to clamp
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    [[nodiscard]] uint16_t clamp(
        int32_t value,
        int32_t min,
        int32_t max
    ) const noexcept;

private:
    // Display data (static storage, no heap allocation)
    DisplayData display_data_;

    // Spectrum data buffer (static storage for stack optimization)
    std::array<uint8_t, SPECTRUM_BUFFER_SIZE> spectrum_buffer_;

    // Histogram data buffer (static storage for stack optimization)
    std::array<uint16_t, HISTOGRAM_BUFFER_SIZE> histogram_buffer_;

    // Status text buffer
    char status_text_[MAX_TEXT_LENGTH];

    // Spectrum data size
    size_t spectrum_data_size_;

    // Histogram data size
    size_t histogram_data_size_;

    // Display flags
    bool spectrum_visible_;
    bool histogram_visible_;
    bool drone_list_visible_;
    bool status_bar_visible_;

    // Section-level dirty flags — each bit controls whether a section repaints.
    // Prevents clearing+redrawing unchanged sections, eliminating flicker.
    static constexpr uint8_t DIRTY_SPEC   = 1 << 0;
    static constexpr uint8_t DIRTY_HIST   = 1 << 1;
    static constexpr uint8_t DIRTY_DRONES = 1 << 2;
    static constexpr uint8_t DIRTY_STATUS = 1 << 3;
    static constexpr uint8_t DIRTY_ALL    = 0x0F;
    uint8_t dirty_flags_{DIRTY_ALL};

    // Spectrum filter threshold (0=OFF, 118=MID, 202=HIGH)
    uint8_t min_color_power_{DEFAULT_SPECTRUM_FILTER};

    // Spectrum shape filter params (from Settings: margin, min_width, max_width)
    // Used only for syncing from config when user changes settings via set_spectrum_shape_params()
    uint8_t spectrum_shape_margin_{0};
    uint8_t spectrum_shape_min_width_{DEFAULT_SPECTRUM_MIN_WIDTH};
    uint8_t spectrum_shape_max_width_{DEFAULT_SPECTRUM_MAX_WIDTH};

    // Display-only margin filter (separate from detection margin)
    // Default: 0 = show full spectrum (no noise floor filtering)
    // When > 0, only shows bins above noise_floor + display_margin
    // Updated independently from scanner config to allow different display vs detection behavior
    uint8_t display_margin_{0};

    // Exponential smoothing buffer for LEVEL-V display
    std::array<int32_t, SPECTRUM_BUFFER_SIZE> spectrum_smoothed_{};
    bool spectrum_smoothed_initialized_{false};

    // Spectrum integration factor for smoothing
    uint8_t spectrum_integration_{DEFAULT_SPECTRUM_INTEGRATION};

    // Previous frame bar heights for dirty-check optimization
    std::array<uint8_t, SPECTRUM_BUFFER_SIZE> spectrum_cached_{};
    bool spectrum_cache_valid_{false};

    // Sorting buffer for noise floor calculation (avoid stack allocation)
    std::array<uint8_t, SPECTRUM_BUFFER_SIZE> spectrum_sort_buffer_{};

    // Band sweep composite mode
    bool composite_mode_{false};
    const uint8_t* composite_data_{nullptr};
    size_t composite_data_size_{0};
    FreqHz sweep_freq_start_{0};
    FreqHz sweep_freq_end_{0};

    // Composite persistence buffer (EMA over sweep passes).
    // Updated each pass: buf[i] = max(raw[i], (buf[i] * DECAY) >> 8).
    // Reduces visual noise by smoothing frame-to-frame variance.
    uint8_t composite_persist_buf_[COMPOSITE_SIZE]{};
    bool composite_persist_initialized_{false};

    // Sort buffer for composite noise floor computation (avoids stack allocation).
    uint8_t composite_sort_buf_[COMPOSITE_SIZE]{};

    // Auto-computed noise floor for composite display noise floor subtraction.
    uint8_t composite_noise_floor_{0};
    bool composite_noise_floor_valid_{false};

    // Per-band persistence for sweep band 2 (dual-sweep mode).
    // Band 2 covers a different frequency range than band 1, so it needs
    // independent EMA persistence and noise floor estimation.
    uint8_t sweep2_persist_buf_[COMPOSITE_SIZE]{};
    bool sweep2_persist_initialized_{false};
    uint8_t sweep2_sort_buf_[COMPOSITE_SIZE]{};
    uint8_t sweep2_noise_floor_{0};
    bool sweep2_noise_floor_valid_{false};

    // Multi-zone sweep (4 zones)
    static constexpr uint8_t MAX_ZONES = 4;
    const uint8_t* multi_zone_data_[MAX_ZONES]{};
    FreqHz zone_freq_start_[MAX_ZONES]{};
    FreqHz zone_freq_end_[MAX_ZONES]{};
    uint8_t multi_zone_count_{0};

    /**
     * @brief Envelope state for connecting bar tops with a white trace.
     * @note Eliminates code duplication between render_composite() and render_multi_zone().
     * @note Stack: ~6 bytes when embedded in caller.
     */
    struct EnvelopeState {
        uint16_t prev_y{0};
        bool prev_valid{false};
        uint8_t gap_count{0};
        static constexpr uint8_t MAX_GAP = 5;
    };

    /**
     * @brief Draw a single bar column and update the envelope trace.
     * @note Envelope only draws on above-threshold bars, eliminating the
     *       sub-threshold white dot line bug that existed in render_composite().
     * @param x Screen X coordinate
     * @param y Bar top Y coordinate (chart_start_y + chart_height - bar_height)
     * @param bar_height Bar height in pixels
     * @param color Bar color (RGB), or COLOR_BACKGROUND if below threshold
     * @param state Envelope state (updated in-place)
     * @note Stack: ~0 bytes (all state in EnvelopeState ref). Flash: ~200 bytes.
     */
    void draw_bar_with_envelope(
        Painter& painter,
        uint16_t x,
        uint16_t y,
        uint16_t bar_height,
        uint32_t color,
        EnvelopeState& state
    ) noexcept;

    void render_composite(
        Painter& painter,
        const uint8_t* composite_data,
        size_t composite_size,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height,
        int16_t scan_head = -1,
        uint8_t noise_floor = 0
    ) noexcept;

    void render_multi_zone(
        Painter& painter,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height
    ) noexcept;

    void render_dual_composite(
        Painter& painter,
        uint16_t start_x,
        uint16_t start_y,
        uint16_t width,
        uint16_t height
    ) noexcept;

public:
    void set_matched_pattern_bin(int16_t bin) noexcept { matched_pattern_bin_ = bin; dirty_flags_ |= DIRTY_SPEC; set_dirty(); }

    /**
     * @brief Layout metrics computed once per paint/hit_test cycle.
     * @note SINGLE SOURCE OF TRUTH for all coordinate calculations.
     *       Eliminates the desync bug where paint() and hit_test() computed
     *       section heights independently with potentially different logic.
     */
    struct LayoutMetrics {
        uint16_t spec_h;         // Spectrum/composite section height (0 if hidden)
        uint16_t hist_h;         // Histogram section height (0 if hidden)
        uint16_t status_h;       // Status bar height (0 if hidden)
        uint16_t drone_h;        // Drone list section height (remaining space)
        uint16_t list_start_y;   // Y offset where drone list begins (relative to parent)
    };

    /**
     * @brief Calculate layout metrics from current display state.
     * @return LayoutMetrics struct with all computed heights.
     * @note Used by both paint() and hit_test() — single source of truth.
     * @note Stack: ~0 bytes (all data returned by value, small struct).
     */
    [[nodiscard]] LayoutMetrics calculate_layout() const noexcept;

    /**
     * @brief Hit-test: determine drone index at touch coordinates.
     * @param x Local X within DroneDisplay.
     * @param y Local Y within DroneDisplay.
     * @return Drone index (0-based) or -1 if miss.
     * @note Must mirror paint() layout — update here when paint() changes.
     */
    [[nodiscard]] int16_t hit_test(uint16_t x, uint16_t y) const noexcept;

private:
    // Dual-sweep rendering (two horizontal bands)
    bool dual_sweep_mode_{false};
    const uint8_t* sweep2_data_{nullptr};
    size_t sweep2_data_size_{0};
    FreqHz sweep2_freq_start_{0};
    FreqHz sweep2_freq_end_{0};

    // Pattern match highlight (red frame in sweep)
    int16_t matched_pattern_bin_{-1};

    // Real-time scan-head position [0]=upper band, [1]=lower band (dual mode).
    // -1 means "no marker" for that band.
    std::array<int16_t, 2> scan_head_position_{ {-1, -1} };
};

} // namespace drone_analyzer

#endif // DRONE_DISPLAY_HPP
