#ifndef PATTERN_MANAGER_HPP
#define PATTERN_MANAGER_HPP

#include <cstdint>
#include <cstddef>

#include "ch.h"
#include "file.hpp"
#include "file_path.hpp"
#include "pattern_types.hpp"
#include "constants.hpp"
#include <array>

#include "locking.hpp"

namespace drone_analyzer {

/**
 * @brief File-backed pattern storage (mutex-protected).
 * @note CSV format: name,wave[16],features[4],threshold,flags,center_freq,range_width
 *                   (25 fields new / 29 fields old, one line per file in /EDA/PATTERNS/).
 * @note Both new (25-field) and old (29-field) CSV formats load correctly.
 * @note Single source of truth for on-disk pattern data.
 *
 * Stack: ~200 bytes (save_pattern write buffer 160B + temps 40B).
 * Flash: 0 (header only).
 * SRAM: MAX_PATTERNS × sizeof(SignalPattern) + 256B read buffer ≈ 10 × 64 + 256 ≈ 896 B.
 */
class PatternManager {
public:
    PatternManager() noexcept;
    ~PatternManager() noexcept;

    PatternManager(const PatternManager&) = delete;
    PatternManager& operator=(const PatternManager&) = delete;
    PatternManager(PatternManager&&) = delete;
    PatternManager& operator=(PatternManager&&) = delete;

    /** @brief Load all patterns from /EDA/PATTERNS/ (any .TXT) into the in-memory array. */
    [[nodiscard]] ErrorCode load_patterns() noexcept;

    /** @brief Save one pattern to /EDA/PATTERNS/<name>.TXT and append to in-memory array. */
    [[nodiscard]] ErrorCode save_pattern(const SignalPattern& pattern) noexcept;

    /** @brief Delete a pattern by index (file + array compaction). */
    [[nodiscard]] ErrorCode delete_pattern(size_t index) noexcept;

    /** @brief Toggle enabled/disabled flag for a pattern by index.
     *  @note Modifies in-memory flags AND rewrites the pattern file on SD.
     *  @return SUCCESS, INVALID_PARAMETER if index out of range,
     *          DATABASE_LOAD_TIMEOUT on file write failure. */
    [[nodiscard]] ErrorCode toggle_pattern(size_t index) noexcept;

    [[nodiscard]] const SignalPattern* get_pattern(size_t index) const noexcept;
    [[nodiscard]] const SignalPattern* get_patterns_array() const noexcept;
    [[nodiscard]] size_t get_pattern_count() const noexcept;

    /** @brief Clear in-memory array (does not delete files). */
    void clear_all_patterns() noexcept;

    /** @brief Re-scan SD card and rebuild in-memory array from scratch. */
    [[nodiscard]] ErrorCode reload_patterns() noexcept;

private:
    std::array<SignalPattern, MAX_PATTERNS> patterns_;
    size_t pattern_count_;
    mutable Mutex mutex_;

    // I/O buffer — single shared buffer for file read and line assembly.
    // SRAM cost: 256 bytes in BSS (constant, not per-call).
    // Removed line_buf_[256] and write_buf_[256] to save 512 bytes.
    // write_pattern uses a local stack buffer (~200 bytes) for CSV serialization.
    std::array<uint8_t, 256> read_buf_{};     // 256B — max CSV line ~146 bytes
    bool loaded_{false};                       // 1B — prevents redundant SD re-reads

    [[nodiscard]] ErrorCode load_pattern_from_line(
        const std::filesystem::path& filepath
    ) noexcept;

    [[nodiscard]] ErrorCode parse_pattern_csv(
        const char* csv_line,
        size_t csv_length
    ) noexcept;

    [[nodiscard]] static uint8_t parse_uint8(
        const char* str,
        size_t len
    ) noexcept;

    [[nodiscard]] static uint16_t parse_uint16(
        const char* str,
        size_t len
    ) noexcept;

    [[nodiscard]] static uint32_t parse_uint32(
        const char* str,
        size_t len
    ) noexcept;

    [[nodiscard]] static uint64_t parse_uint64(
        const char* str,
        size_t len
    ) noexcept;
};

} // namespace drone_analyzer

#endif // PATTERN_MANAGER_HPP
