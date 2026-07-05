#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include "ch.h"
#include "drone_types.hpp"
#include "locking.hpp"
#include "constants.hpp"
#include "freqman_db.hpp"

namespace drone_analyzer {

struct FrequencyEntry {
    FreqHz frequency;
    DroneType drone_type;

    FrequencyEntry() noexcept
        : frequency(0)
        , drone_type(DroneType::UNKNOWN) {}

    FrequencyEntry(FreqHz freq, DroneType type) noexcept
        : frequency(freq)
        , drone_type(type) {}

    [[nodiscard]] bool is_valid() const noexcept {
        return (frequency >= MIN_FREQUENCY_HZ) &&
               (frequency <= MAX_FREQUENCY_HZ) &&
               (drone_type != DroneType::UNKNOWN);
    }
};

class DatabaseManager {
public:
    DatabaseManager() noexcept;
    ~DatabaseManager() noexcept;

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

    [[nodiscard]] ErrorCode load_frequency_database() noexcept;
    [[nodiscard]] ErrorResult<FreqHz> get_next_frequency(FreqHz current_freq) noexcept;
    [[nodiscard]] ErrorResult<FrequencyEntry> find_entry(FreqHz frequency) const noexcept;
    [[nodiscard]] size_t get_database_size() const noexcept;
    [[nodiscard]] bool is_loaded() const noexcept;
    void set_database_file(const char* filename) noexcept;
    
    /**
     * @brief Get current database index (for state save/restore)
     * @return Current index in database entries array
     * @note Thread-safe: acquires DATABASE_MUTEX
     */
    [[nodiscard]] size_t get_current_index() const noexcept;
    
    /**
     * @brief Set current database index (for state restore after sweep)
     * @param index Index to restore (must be < entry_count_)
     * @note Thread-safe: acquires DATABASE_MUTEX
     * @note Clamps index to valid range if out of bounds
     */
    void set_current_index(size_t index) noexcept;
    
private:
    
    [[nodiscard]] ErrorResult<size_t> find_entry_index_internal(FreqHz frequency) const noexcept;
    [[nodiscard]] ErrorCode load_from_file_internal() noexcept;

    std::array<FrequencyEntry, MAX_DATABASE_ENTRIES> entries_;
    
    size_t current_index_{0};
    size_t entry_count_{0};
    AtomicFlag loaded_;
    mutable Mutex mutex_;
    char database_file_[32]{'D', 'R', 'O', 'N', 'E', 'S', '\0'};

    FreqmanDB freqman_db_;
};

} // namespace drone_analyzer

#endif // DATABASE_HPP