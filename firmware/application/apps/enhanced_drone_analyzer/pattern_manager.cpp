#include "pattern_manager.hpp"
#include <cstring>

namespace drone_analyzer {

namespace fs = std::filesystem;

uint8_t PatternManager::parse_uint8(const char* str, size_t len) noexcept {
    if (str == nullptr || len == 0) return 0;
    uint8_t result = 0;
    for (size_t i = 0; i < len && str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + static_cast<uint8_t>(str[i] - '0');
    }
    return result;
}

uint16_t PatternManager::parse_uint16(const char* str, size_t len) noexcept {
    if (str == nullptr || len == 0) return 0;
    uint16_t result = 0;
    for (size_t i = 0; i < len && str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + static_cast<uint16_t>(str[i] - '0');
    }
    return result;
}

uint32_t PatternManager::parse_uint32(const char* str, size_t len) noexcept {
    if (str == nullptr || len == 0) return 0;
    uint32_t result = 0;
    for (size_t i = 0; i < len && str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + static_cast<uint32_t>(str[i] - '0');
    }
    return result;
}

uint64_t PatternManager::parse_uint64(const char* str, size_t len) noexcept {
    if (str == nullptr || len == 0) return 0;
    uint64_t result = 0;
    for (size_t i = 0; i < len && str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + static_cast<uint64_t>(str[i] - '0');
    }
    return result;
}

PatternManager::PatternManager() noexcept
    : patterns_{}
    , pattern_count_(0)
    , mutex_() {
    chMtxInit(&mutex_);
}

PatternManager::~PatternManager() noexcept = default;

ErrorCode PatternManager::load_patterns() noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    if (loaded_) return ErrorCode::SUCCESS;

    pattern_count_ = 0;

    if (!fs::is_directory(patterns_dir)) {
        loaded_ = true;
        return ErrorCode::SUCCESS;
    }

    for (const auto& entry : fs::directory_iterator(patterns_dir, u"*.TXT")) {
        if (pattern_count_ >= MAX_PATTERNS) break;
        if (fs::is_regular_file(entry.status())) {
            const ErrorCode err = load_pattern_from_line(entry.path());
            (void)err;
        }
    }

    loaded_ = true;
    return ErrorCode::SUCCESS;
}

ErrorCode PatternManager::load_pattern_from_line(
    const fs::path& filepath
) noexcept {
    if (filepath.empty()) return ErrorCode::INVALID_PARAMETER;
    if (pattern_count_ >= MAX_PATTERNS) return ErrorCode::BUFFER_FULL;

    File file;
    const auto open_err = file.open(filepath, true, false);
    if (open_err) return ErrorCode::DATABASE_LOAD_TIMEOUT;

    struct FileGuard {
        File* const file;
        explicit FileGuard(File* f) noexcept : file(f) {}
        ~FileGuard() { if (file) file->close(); }
        FileGuard(const FileGuard&) = delete;
        FileGuard& operator=(const FileGuard&) = delete;
    } file_guard(&file);

    // Read entire file into read_buf_ (pattern files are <150 bytes).
    // Single read — eliminates line assembly buffer (line_buf_ removed).
    const auto read_result = file.read(read_buf_.data(), read_buf_.size());
    if (!read_result.is_ok()) return ErrorCode::DATABASE_LOAD_TIMEOUT;
    const size_t bytes_read = read_result.value();
    if (bytes_read == 0) return ErrorCode::SUCCESS;

    // Find first line (skip leading whitespace, stop at newline/CR)
    size_t line_start = 0;
    while (line_start < bytes_read) {
        const char c = static_cast<char>(read_buf_[line_start]);
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
        ++line_start;
    }
    size_t line_len = line_start;
    while (line_len < bytes_read) {
        const char c = static_cast<char>(read_buf_[line_len]);
        if (c == '\n' || c == '\r') break;
        ++line_len;
    }
    line_len -= line_start;

    if (line_len == 0) return ErrorCode::SUCCESS;

    // Null-terminate in place (read_buf_ is mutable).
    // Guard: ensure null-terminator fits within read_buf_ (256 bytes).
    const size_t term_pos = line_start + line_len;
    if (term_pos >= read_buf_.size()) {
        line_len = read_buf_.size() - line_start - 1;
    }
    read_buf_[line_start + line_len] = 0;
    const ErrorCode parse_err = parse_pattern_csv(
        reinterpret_cast<const char*>(&read_buf_[line_start]), line_len);
    if (parse_err == ErrorCode::SUCCESS) {
        ++pattern_count_;
    }

    return ErrorCode::SUCCESS;
}

ErrorCode PatternManager::parse_pattern_csv(
    const char* csv_line,
    size_t csv_length
) noexcept {
    if (csv_line == nullptr || csv_length == 0) return ErrorCode::INVALID_PARAMETER;
    if (pattern_count_ >= MAX_PATTERNS) return ErrorCode::BUFFER_FULL;

    SignalPattern& pattern = patterns_[pattern_count_];
    pattern = SignalPattern{};

    size_t pos = 0;
    uint8_t field_index = 0;
    constexpr uint8_t OLD_FIELD_COUNT = 29;

    // Temp storage for fields 21-28 — resolves old vs new format after full parse.
    // Stack: 19 bytes (1×uint16_t + 1×uint8_t + 2×uint64_t).
    uint16_t tmp_threshold{0};
    uint8_t tmp_flags{0};
    uint64_t tmp_center_freq{0};
    uint64_t tmp_range_width{0};

    while (pos < csv_length && field_index < OLD_FIELD_COUNT) {
        while (pos < csv_length && (csv_line[pos] == ',' || csv_line[pos] == ' ' || csv_line[pos] == '\t')) {
            ++pos;
        }
        if (pos >= csv_length) break;

        size_t field_start = pos;
        while (pos < csv_length && csv_line[pos] != ',' && csv_line[pos] != '\n' && csv_line[pos] != '\r') {
            ++pos;
        }
        const size_t field_end = pos;
        if (field_start >= field_end) break;
        const size_t field_len = field_end - field_start;

        if (field_index == 0) {
            const size_t copy_len = (field_len < PATTERN_NAME_MAX_LEN) ? field_len : PATTERN_NAME_MAX_LEN - 1;
            for (size_t i = 0; i < copy_len; ++i) pattern.name[i] = csv_line[field_start + i];
            pattern.name[copy_len] = '\0';
        } else if (field_index >= 1 && field_index <= PATTERN_WAVEFORM_SIZE) {
            const uint8_t bin_idx = static_cast<uint8_t>(field_index - 1);
            if (bin_idx < PATTERN_WAVEFORM_SIZE) {
                pattern.waveform[bin_idx] = parse_uint8(&csv_line[field_start], field_len);
            }
        } else if (field_index == 17) {
            pattern.features.peak_position = parse_uint8(&csv_line[field_start], field_len);
        } else if (field_index == 18) {
            pattern.features.peak_value = parse_uint8(&csv_line[field_start], field_len);
        } else if (field_index == 19) {
            pattern.features.noise_floor = parse_uint8(&csv_line[field_start], field_len);
        } else if (field_index == 20) {
            pattern.features.margin = parse_uint8(&csv_line[field_start], field_len);
        } else if (field_index >= 21 && field_index <= 24) {
            // Fields 21-24: NEW format = threshold/flags/center_freq/range_width
            //                 OLD format = width/sharpness/flatness/symmetry (ignored)
            if (field_index == 21) tmp_threshold = parse_uint16(&csv_line[field_start], field_len);
            if (field_index == 22) tmp_flags = parse_uint8(&csv_line[field_start], field_len);
            if (field_index == 23) tmp_center_freq = parse_uint64(&csv_line[field_start], field_len);
            if (field_index == 24) tmp_range_width = parse_uint64(&csv_line[field_start], field_len);
        } else if (field_index >= 25 && field_index <= 28) {
            // OLD format only: threshold(25), flags(26), center_freq(27), range_width(28)
            if (field_index == 25) tmp_threshold = parse_uint16(&csv_line[field_start], field_len);
            if (field_index == 26) tmp_flags = parse_uint8(&csv_line[field_start], field_len);
            if (field_index == 27) tmp_center_freq = parse_uint64(&csv_line[field_start], field_len);
            if (field_index == 28) tmp_range_width = parse_uint64(&csv_line[field_start], field_len);
        }
        ++field_index;
    }

    // Minimum viable pattern: name + 16 waveform bins + 4 features.
    constexpr uint8_t MIN_FIELD_COUNT = 1 + PATTERN_WAVEFORM_SIZE + 4;
    if (field_index < MIN_FIELD_COUNT) {
        return ErrorCode::DATABASE_FORMAT_INVALID;
    }

    // Resolve old vs new format by total field count.
    // OLD (29 fields): fields 21-24 were width/sharpness/flatness/symmetry,
    //                   real threshold/flags/center_freq/range_width are at 25-28.
    // NEW (25 fields):  fields 21-24 are threshold/flags/center_freq/range_width directly.
    if (field_index >= OLD_FIELD_COUNT) {
        // OLD format — fields 25-28 are the real values (already stored in tmp)
        // Fields 21-24 (width/sharpness/flatness/symmetry) are silently discarded.
    }
    // For both formats, tmp_threshold/tmp_flags/tmp_center_freq/tmp_range_width
    // now hold the correct values.

    pattern.match_threshold = tmp_threshold;
    pattern.flags = tmp_flags;
    pattern.center_freq = static_cast<FreqHz>(tmp_center_freq);
    pattern.range_width = static_cast<FreqHz>(tmp_range_width);
    pattern.created_time = chTimeNow();
    return ErrorCode::SUCCESS;
}

ErrorCode PatternManager::save_pattern(const SignalPattern& pattern) noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    if (pattern_count_ >= MAX_PATTERNS) return ErrorCode::BUFFER_FULL;

    ensure_directory(patterns_dir);

    const auto filepath = patterns_dir / std::string(pattern.name) + u".TXT";

    File file;
    const auto open_err = file.create(filepath);
    if (open_err.is_valid()) return ErrorCode::DATABASE_LOAD_TIMEOUT;

    struct FileGuard {
        File* const file;
        explicit FileGuard(File* f) noexcept : file(f) {}
        ~FileGuard() { if (file) file->close(); }
        FileGuard(const FileGuard&) = delete;
        FileGuard& operator=(const FileGuard&) = delete;
    } file_guard(&file);

    // Local stack buffer for CSV serialization (write_buf 160B + temps ~40B).
    // Replaces write_buf_[256] to save 256 bytes BSS.
    // Stack: ~200 bytes — acceptable for non-hot SD I/O path.
    static constexpr size_t WRITE_BUF_SIZE = 160;
    uint8_t write_buf[WRITE_BUF_SIZE]{};
    size_t write_pos = 0;

    auto write_char = [&](char c) noexcept -> void {
        if (write_pos < WRITE_BUF_SIZE) write_buf[write_pos++] = static_cast<uint8_t>(c);
    };

    constexpr size_t INT32_STR_BUF_SIZE = 12;
    constexpr size_t UINT64_STR_BUF_SIZE = 24;

    auto write_int = [&](int32_t val) noexcept -> void {
        char tmp[INT32_STR_BUF_SIZE];
        const int len = snprintf(tmp, sizeof(tmp), "%ld", static_cast<long>(val));
        if (len <= 0 || len >= static_cast<int>(sizeof(tmp))) return;
        for (int i = 0; i < len; ++i) write_char(tmp[i]);
    };

    auto write_uint64 = [&](uint64_t val) noexcept -> void {
        char tmp[UINT64_STR_BUF_SIZE];
        const int len = snprintf(tmp, sizeof(tmp), "%llu", static_cast<unsigned long long>(val));
        if (len <= 0 || len >= static_cast<int>(sizeof(tmp))) return;
        for (int i = 0; i < len; ++i) write_char(tmp[i]);
    };

    for (size_t i = 0; i < PATTERN_NAME_MAX_LEN && pattern.name[i] != '\0'; ++i) {
        write_char(pattern.name[i]);
    }
    for (size_t i = 0; i < PATTERN_WAVEFORM_SIZE; ++i) {
        write_char(',');
        write_int(static_cast<int32_t>(pattern.waveform[i]));
    }

    write_char(',');
    write_int(static_cast<int32_t>(pattern.features.peak_position));
    write_char(',');
    write_int(static_cast<int32_t>(pattern.features.peak_value));
    write_char(',');
    write_int(static_cast<int32_t>(pattern.features.noise_floor));
    write_char(',');
    write_int(static_cast<int32_t>(pattern.features.margin));
    write_char(',');
    write_int(static_cast<int32_t>(pattern.match_threshold));
    write_char(',');
    write_int(static_cast<int32_t>(pattern.flags));
    write_char(',');
    write_uint64(static_cast<uint64_t>(pattern.center_freq));
    write_char(',');
    write_uint64(static_cast<uint64_t>(pattern.range_width));
    write_char('\n');

    const File::Result<File::Size> write_result = file.write(write_buf, static_cast<File::Size>(write_pos));
    if (!write_result.is_ok()) return ErrorCode::DATABASE_LOAD_TIMEOUT;

    patterns_[pattern_count_] = pattern;
    ++pattern_count_;
    return ErrorCode::SUCCESS;
}

ErrorCode PatternManager::delete_pattern(size_t index) noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    if (index >= pattern_count_) return ErrorCode::INVALID_PARAMETER;

    const auto filepath = patterns_dir / std::string(patterns_[index].name) + u".TXT";
    const auto del_err = delete_file(filepath);
    if (!del_err.ok()) return ErrorCode::DATABASE_LOAD_TIMEOUT;

    for (size_t i = index; i < pattern_count_ - 1; ++i) {
        patterns_[i] = patterns_[i + 1];
    }
    if (pattern_count_ > 0) {
        patterns_[pattern_count_ - 1] = SignalPattern{};
    }
    --pattern_count_;
    return ErrorCode::SUCCESS;
}

ErrorCode PatternManager::toggle_pattern(size_t index) noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);

    if (index >= pattern_count_) return ErrorCode::INVALID_PARAMETER;

    SignalPattern& p = patterns_[index];
    p.set_enabled(!p.is_enabled());

    // Rewrite the file with the toggled flag.
    // Follows the same file path and CSV format as save_pattern().
    ensure_directory(patterns_dir);

    const auto filepath = patterns_dir / std::string(p.name) + u".TXT";

    File file;
    const auto open_err = file.create(filepath);
    if (open_err.is_valid()) return ErrorCode::DATABASE_LOAD_TIMEOUT;

    struct FileGuard {
        File* const file;
        explicit FileGuard(File* f) noexcept : file(f) {}
        ~FileGuard() { if (file) file->close(); }
        FileGuard(const FileGuard&) = delete;
        FileGuard& operator=(const FileGuard&) = delete;
    } file_guard(&file);

    // Local stack buffer for CSV serialization (~160 bytes).
    // Matches save_pattern() format: 25 fields.
    static constexpr size_t WRITE_BUF_SIZE = 160;
    uint8_t write_buf[WRITE_BUF_SIZE]{};
    size_t write_pos = 0;

    auto write_char = [&](char c) noexcept -> void {
        if (write_pos < WRITE_BUF_SIZE) write_buf[write_pos++] = static_cast<uint8_t>(c);
    };

    constexpr size_t INT32_STR_BUF_SIZE = 12;
    constexpr size_t UINT64_STR_BUF_SIZE = 24;

    auto write_int = [&](int32_t val) noexcept -> void {
        char tmp[INT32_STR_BUF_SIZE];
        const int len = snprintf(tmp, sizeof(tmp), "%ld", static_cast<long>(val));
        if (len <= 0 || len >= static_cast<int>(sizeof(tmp))) return;
        for (int i = 0; i < len; ++i) write_char(tmp[i]);
    };

    auto write_uint64 = [&](uint64_t val) noexcept -> void {
        char tmp[UINT64_STR_BUF_SIZE];
        const int len = snprintf(tmp, sizeof(tmp), "%llu", static_cast<unsigned long long>(val));
        if (len <= 0 || len >= static_cast<int>(sizeof(tmp))) return;
        for (int i = 0; i < len; ++i) write_char(tmp[i]);
    };

    // Field 0: name
    for (size_t i = 0; i < PATTERN_NAME_MAX_LEN && p.name[i] != '\0'; ++i) {
        write_char(p.name[i]);
    }
    // Fields 1-16: waveform[16]
    for (size_t i = 0; i < PATTERN_WAVEFORM_SIZE; ++i) {
        write_char(',');
        write_int(static_cast<int32_t>(p.waveform[i]));
    }
    // Fields 17-20: features
    write_char(',');
    write_int(static_cast<int32_t>(p.features.peak_position));
    write_char(',');
    write_int(static_cast<int32_t>(p.features.peak_value));
    write_char(',');
    write_int(static_cast<int32_t>(p.features.noise_floor));
    write_char(',');
    write_int(static_cast<int32_t>(p.features.margin));
    // Fields 21-24: threshold, flags, center_freq, range_width
    write_char(',');
    write_int(static_cast<int32_t>(p.match_threshold));
    write_char(',');
    write_int(static_cast<int32_t>(p.flags));
    write_char(',');
    write_uint64(static_cast<uint64_t>(p.center_freq));
    write_char(',');
    write_uint64(static_cast<uint64_t>(p.range_width));
    write_char('\n');

    const File::Result<File::Size> write_result = file.write(write_buf, static_cast<File::Size>(write_pos));
    if (!write_result.is_ok()) return ErrorCode::DATABASE_LOAD_TIMEOUT;

    return ErrorCode::SUCCESS;
}

const SignalPattern* PatternManager::get_pattern(size_t index) const noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    if (index >= pattern_count_) return nullptr;
    return &patterns_[index];
}

const SignalPattern* PatternManager::get_patterns_array() const noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    return patterns_.data();
}

size_t PatternManager::get_pattern_count() const noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    return pattern_count_;
}

void PatternManager::clear_all_patterns() noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    pattern_count_ = 0;
    for (auto& pattern : patterns_) pattern = SignalPattern{};
}

ErrorCode PatternManager::reload_patterns() noexcept {
    MutexLock<LockOrder::DATABASE_MUTEX> lock(mutex_);
    loaded_ = false;
    pattern_count_ = 0;

    if (!fs::is_directory(patterns_dir)) return ErrorCode::SUCCESS;

    for (const auto& entry : fs::directory_iterator(patterns_dir, u"*.TXT")) {
        if (pattern_count_ >= MAX_PATTERNS) break;
        if (fs::is_regular_file(entry.status())) {
            const ErrorCode err = load_pattern_from_line(entry.path());
            (void)err;
        }
    }

    loaded_ = true;
    return ErrorCode::SUCCESS;
}

} // namespace drone_analyzer
