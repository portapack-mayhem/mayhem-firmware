#ifndef PEAK_DETECTOR_HPP
#define PEAK_DETECTOR_HPP

#include <cstdint>
#include <cstddef>
#include <optional>

#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief Single source of truth for FFT peak detection.
 * @note Replaces duplicated peak-detection paths in
 *       DroneScannerUI save and
 *       inline code in scanner.cpp:process_spectrum_sweep().
 * @note Pure integer arithmetic, no FP, no heap, no exceptions.
 * @note Caller owns sort_buf (must be >= FFT_USABLE_BINS bytes).
 *
 * Stack: ~32 bytes (PeakInfo) + 0 (no recursion).
 * Flash: 0 (header only).
 * SRAM: 0 (no static state).
 */
class PeakDetector {
public:
    /**
     * @brief Search range for peak detection.
     */
    enum class Range : uint8_t {
        Full,        // 0..255, skips DC spike + edges
        LowerOnly,   // FFT_EDGE_SKIP_NARROW..FFT_DC_SPIKE_START
        UpperOnly    // FFT_DC_SPIKE_END..(FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW)
    };

    /**
     * @brief Edge skip policy.
     */
    enum class EdgePolicy : uint8_t {
        Wide,        // FFT_EDGE_SKIP = 10 (pattern capture, full sweep)
        Narrow       // FFT_EDGE_SKIP_NARROW = 6 (fast V-shape scan)
    };

    /**
     * @brief Detected peak description.
     */
    struct PeakInfo {
        size_t index{0};          // bin in 256-FFT
        uint8_t value{0};         // peak amplitude (0-255)
        uint8_t noise_floor{0};   // median of usable bins
        uint8_t margin{0};        // peak - noise_floor
        size_t width{0};          // bins above (noise_floor + margin/2)
        size_t left{0};           // left edge of signal band
        size_t right{0};          // right edge of signal band
        size_t usable_count{0};   // number of bins that contributed to noise floor

        [[nodiscard]] bool valid() const noexcept { return margin > 0; }
    };

    /**
     * @brief Find the strongest peak in a 256-bin FFT.
     * @param spectrum  Raw FFT magnitudes (256 bins, 0-255)
     * @param sort_buf  Caller-provided scratch buffer (>= 236 bytes)
     * @param range     Search range (default: Full)
     * @param edge      Edge-skip policy (default: Narrow for live scanning)
     * @param noise_percentile  Percentile for noise floor estimation (0-100, default: 50 = median)
     *                          Use 25 for dense signal environments (WiFi-dense 2.4/5.8 GHz)
     *                          to avoid median bias when signal occupies >50% of bins.
     * @return PeakInfo with all derived quantities, or zeroed struct on empty input
     * @note Skips DC spike (FFT_DC_SPIKE_START..FFT_DC_SPIKE_END).
     * @note Cost: O(n) for quickselect percentile + O(n) for peak scan + O(n) for width.
     */
    [[nodiscard]] static PeakInfo find(
        const uint8_t* spectrum,
        uint8_t* sort_buf,
        Range range = Range::Full,
        EdgePolicy edge = EdgePolicy::Narrow,
        uint8_t noise_percentile = 50
    ) noexcept;

private:
    [[nodiscard]] static uint8_t quickselect_percentile(
        uint8_t* buf,
        size_t count,
        uint8_t percentile
    ) noexcept;
};

} // namespace drone_analyzer

#endif // PEAK_DETECTOR_HPP
