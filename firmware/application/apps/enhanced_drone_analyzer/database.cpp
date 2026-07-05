#include "database.hpp"
#include "file_path.hpp"
#include <cstring>
#include <ctype.h>

namespace drone_analyzer {

/**
 * @brief Parse drone type from the first word of a description string
 * @param buffer Description text (null-terminated)
 * @param value_len Length of description text
 * @return DroneType or UNKNOWN if no keyword matched
 */
static DroneType parse_drone_type_from_description(
    const char* buffer,
    size_t value_len
) noexcept {
    if (value_len == 0 || buffer == nullptr) {
        return DroneType::UNKNOWN;
    }

    size_t first_word_len = 0;
    for (size_t i = 0; i < value_len && i < 16; i++) {
        const char c = buffer[i];
        if (c == ' ' || c == '\t' || c == ',' || c == '\r' || c == '\n') {
            break;
        }
        first_word_len++;
    }

    if (first_word_len == 0) {
        return DroneType::UNKNOWN;
    }

    auto compare_word = [&](const char* word, size_t word_len) -> bool {
        if (first_word_len != word_len) {
            return false;
        }
        for (size_t i = 0; i < word_len; i++) {
            if (::tolower(static_cast<unsigned char>(buffer[i])) !=
                ::tolower(static_cast<unsigned char>(word[i]))) {
                return false;
            }
        }
        return true;
    };

    if (compare_word("FPV", 3)) return DroneType::FPV;
    if (compare_word("DJI", 3)) return DroneType::DJI;
    if (compare_word("PARROT", 6)) return DroneType::PARROT;
    if (compare_word("YUNEEC", 6)) return DroneType::YUNEEC;
    if (compare_word("HOBBY", 5)) return DroneType::HOBBY;
    if (compare_word("AUTEL", 5)) return DroneType::AUTEL;
    if (compare_word("3DR", 3)) return DroneType::DR_3DR;
    if (compare_word("OTHER", 5)) return DroneType::OTHER;
    if (compare_word("CUSTOM", 6)) return DroneType::CUSTOM;

    return DroneType::UNKNOWN;
}

/**
 * @brief Load frequency database using FreqmanDB framework API
 *
 * Opens the freqman file via FreqmanDB (file-based iterator — no bulk heap
 * allocation for entries). Copies each entry into the fixed entries_[] array,
 * then closes the file. Same pattern as ui_freqman and recon.
 *
 * @return ErrorCode::SUCCESS if at least one entry loaded
 * @pre Mutex must be held (DATABASE_MUTEX)
 */
ErrorCode DatabaseManager::load_from_file_internal() noexcept {
    entry_count_ = 0;
    current_index_ = 0;

    // Build path using framework helper: "/FREQMAN/<stem>"
    const auto path = get_freqman_path(database_file_);

    // Open via FreqmanDB — single heap alloc for FileWrapper (~620 bytes)
    if (!freqman_db_.open(path)) {
        // File doesn't exist — try creating empty
        if (!freqman_db_.open(path, true)) {
            return ErrorCode::DATABASE_NOT_LOADED;
        }
    }

    if (freqman_db_.empty()) {
        freqman_db_.close();
        return ErrorCode::DATABASE_EMPTY;
    }

    // Copy entries from FreqmanDB into fixed array
    // FreqmanDB::operator[] returns freqman_entry by value (read from file)
    const size_t count = freqman_db_.entry_count();
    for (size_t i = 0; i < count && entry_count_ < MAX_DATABASE_ENTRIES; ++i) {
        const auto entry = freqman_db_[i];

        const auto freq = static_cast<FreqHz>(entry.frequency_a);
        if (freq < MIN_FREQUENCY_HZ || freq > MAX_FREQUENCY_HZ) {
            continue;
        }

        // Parse drone type from description
        DroneType type = DroneType::OTHER;
        if (!entry.description.empty()) {
            const DroneType parsed = parse_drone_type_from_description(
                entry.description.c_str(), entry.description.size());
            if (parsed != DroneType::UNKNOWN) {
                type = parsed;
            }
        }

        entries_[entry_count_++] = FrequencyEntry(freq, type);
    }

    freqman_db_.close();

    if (entry_count_ == 0) {
        return ErrorCode::DATABASE_EMPTY;
    }

    return ErrorCode::SUCCESS;
}

ErrorCode DatabaseManager::load_frequency_database() noexcept {
    if (loaded_.test()) {
        return ErrorCode::SUCCESS;
    }

    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    if (loaded_.test()) {
        return ErrorCode::SUCCESS;
    }

    const ErrorCode result = load_from_file_internal();

    if (result == ErrorCode::SUCCESS) {
        loaded_.set();
    }

    return result;
}

ErrorResult<FreqHz> DatabaseManager::get_next_frequency(FreqHz current_freq) noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    if (entry_count_ == 0) {
        return ErrorResult<FreqHz>::failure(ErrorCode::DATABASE_EMPTY);
    }

    // If current_freq is 0, return first frequency (initial case)
    if (current_freq == 0) {
        current_index_ = 0;
        return ErrorResult<FreqHz>::success(entries_[current_index_].frequency);
    }

    // Find current frequency in database (linear search from start)
    bool found = false;
    size_t found_index = 0;
    for (size_t i = 0; i < entry_count_; ++i) {
        if (entries_[i].frequency == current_freq) {
            found_index = i;
            found = true;
            break;
        }
    }

    if (found) {
        // Skip ALL entries with the same frequency (handle duplicate channels)
        current_index_ = found_index + 1;
        while (current_index_ < entry_count_ &&
               entries_[current_index_].frequency == current_freq) {
            ++current_index_;
        }
        // Wrap to start if we've exhausted all entries
        if (current_index_ >= entry_count_) {
            current_index_ = 0;
        }
    } else {
        // Frequency not in DB — resume from current_index_
        // Preserves position after sweep restore (last_db_index_)

        // FIX: Add explicit bounds check to prevent out-of-bounds array access.
        // If current_index_ >= entry_count_, reset to 0 to ensure valid index.
        if (current_index_ >= entry_count_ || entry_count_ == 0) {
            current_index_ = 0;
        }
        // Otherwise, preserve current_index_ as-is (it should be valid)
    }

    return ErrorResult<FreqHz>::success(entries_[current_index_].frequency);
}

ErrorResult<FrequencyEntry> DatabaseManager::find_entry(FreqHz frequency) const noexcept {
    if (!loaded_.test()) {
        return ErrorResult<FrequencyEntry>::failure(ErrorCode::DATABASE_NOT_LOADED);
    }

    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    for (size_t i = 0; i < entry_count_; ++i) {
        if (entries_[i].frequency == frequency) {
            return ErrorResult<FrequencyEntry>::success(entries_[i]);
        }
    }

    return ErrorResult<FrequencyEntry>::failure(ErrorCode::INVALID_PARAMETER);
}

size_t DatabaseManager::get_database_size() const noexcept {
    return entry_count_;
}

bool DatabaseManager::is_loaded() const noexcept {
    return loaded_.test();
}

ErrorResult<size_t> DatabaseManager::find_entry_index_internal(FreqHz frequency) const noexcept {
    for (size_t i = 0; i < entry_count_; ++i) {
        if (entries_[i].frequency == frequency) {
            return ErrorResult<size_t>::success(i);
        }
    }

    return ErrorResult<size_t>::failure(ErrorCode::INVALID_PARAMETER);
}

DatabaseManager::DatabaseManager() noexcept
    : entries_()
    , current_index_(0)
    , entry_count_(0)
    , loaded_()
    , mutex_()
    , freqman_db_() {
    chMtxInit(&mutex_);
}

DatabaseManager::~DatabaseManager() noexcept {
}

size_t DatabaseManager::get_current_index() const noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    return current_index_;
}

void DatabaseManager::set_current_index(size_t index) noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    if (entry_count_ == 0) {
        current_index_ = 0;
        return;
    }
    current_index_ = (index < entry_count_) ? index : 0;
}

void DatabaseManager::set_database_file(const char* filename) noexcept {
    if (filename == nullptr) {
        return;
    }

    size_t i = 0;
    while (i < 31 && filename[i] != '\0') {
        database_file_[i] = filename[i];
        ++i;
    }
    database_file_[i] = '\0';

    // Reset state so database will be fully reloaded
    loaded_.clear();
    current_index_ = 0;
    entry_count_ = 0;
}

} // namespace drone_analyzer
