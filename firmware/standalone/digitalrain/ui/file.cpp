/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "file.hpp"
#include "complex.hpp"

#include <algorithm>
#include <codecvt>
#include <cstring>
#include <locale>

namespace fs = std::filesystem;
static const fs::path c8_ext{u".C8"};
static const fs::path c16_ext{u".C16"};

Optional<File::Error> File::open_fatfs(const std::filesystem::path& filename, BYTE mode) {
    auto result = f_open(&f, reinterpret_cast<const TCHAR*>(filename.c_str()), mode);
    if (result == FR_OK) {
        if (mode & FA_OPEN_ALWAYS) {
            const auto result = f_lseek(&f, f_size(&f));
            if (result != FR_OK) {
                f_close(&f);
            }
        }
    }

    if (result == FR_OK) {
        return {};
    } else {
        return {result};
    }
}

/*
 * @param read_only: open in readonly mode
 * @param create: create if it doesnt exist
 */
Optional<File::Error> File::open(const std::filesystem::path& filename, bool read_only, bool create) {
    BYTE mode = read_only ? FA_READ : FA_READ | FA_WRITE;
    if (create)
        mode |= FA_OPEN_ALWAYS;

    return open_fatfs(filename, mode);
}

Optional<File::Error> File::append(const std::filesystem::path& filename) {
    return open_fatfs(filename, FA_WRITE | FA_OPEN_ALWAYS);
}

Optional<File::Error> File::create(const std::filesystem::path& filename) {
    return open_fatfs(filename, FA_WRITE | FA_CREATE_ALWAYS);
}

File::~File() {
    f_close(&f);
}

void File::close() {
    f_close(&f);
}

File::Result<File::Size> File::read(void* data, Size bytes_to_read) {
    UINT bytes_read = 0;
    const auto result = f_read(&f, data, bytes_to_read, &bytes_read);
    if (result == FR_OK) {
        return {static_cast<size_t>(bytes_read)};
    } else {
        return {static_cast<Error>(result)};
    }
}

File::Result<File::Size> File::write(const void* data, Size bytes_to_write) {
    UINT bytes_written = 0;
    const auto result = f_write(&f, data, bytes_to_write, &bytes_written);
    if (result == FR_OK) {
        if (bytes_to_write == bytes_written) {
            return {static_cast<File::Size>(bytes_written)};
        } else {
            return Error{FR_DISK_FULL};
        }
    } else {
        return {static_cast<Error>(result)};
    }
}

File::Offset File::tell() const {
    return f_tell(&f);
}

File::Result<bool> File::eof() {
    return f_eof(&f);
}

File::Result<File::Offset> File::seek(Offset new_position) {
    /* NOTE: Returns *old* position, not new position */
    const auto old_position = tell();
    const auto result = (old_position == new_position) ? FR_OK : f_lseek(&f, new_position);
    if (result != FR_OK) {
        return {static_cast<Error>(result)};
    }
    if (f_tell(&f) != new_position) {
        return {static_cast<Error>(FR_BAD_SEEK)};
    }
    return {static_cast<File::Offset>(old_position)};
}

File::Result<File::Offset> File::truncate() {
    const auto position = f_tell(&f);
    const auto result = f_truncate(&f);
    if (result != FR_OK) {
        return {static_cast<Error>(result)};
    }
    return {static_cast<File::Offset>(position)};
}

File::Size File::size() const {
    return f_size(&f);
}

Optional<File::Error> File::write_line(const std::string& s) {
    const auto result_s = write(s.c_str(), s.size());
    if (result_s.is_error()) {
        return {result_s.error()};
    }

    const auto result_crlf = write("\r\n", 2);
    if (result_crlf.is_error()) {
        return {result_crlf.error()};
    }

    return {};
}

Optional<File::Error> File::sync() {
    const auto result = f_sync(&f);
    if (result == FR_OK) {
        return {};
    } else {
        return {result};
    }
}

File::Result<std::string> File::read_file(const std::filesystem::path& filename) {
    constexpr size_t buffer_size = 0x80;
    char* buffer[buffer_size];

    File f;
    auto error = f.open(filename);
    if (error)
        return *error;

    std::string content;
    content.resize(f.size());
    auto str = &content[0];
    auto total_read = 0u;

    while (true) {
        auto read = f.read(buffer, buffer_size);
        if (!read)
            return read.error();

        memcpy(str, buffer, *read);
        str += *read;
        total_read += *read;

        if (*read < buffer_size)
            break;
    }

    content.resize(total_read);
    return content;
}

/* Range used for filename matching.
 * Start and end are inclusive positions of "???" */
struct pattern_range {
    size_t start;
    size_t end;
};

/* Finds the last file matching the specified pattern that
 * can be automatically incremented (digits in pattern).
 * NB: assumes a patten with contiguous '?' like "FOO_???.txt". */
static std::filesystem::path find_last_ordinal_match(
    const std::filesystem::path& folder,
    const std::filesystem::path& pattern,
    pattern_range range) {
    auto last_match = std::filesystem::path();
    auto can_increment = [range](const auto& path) {
        for (auto i = range.start; i <= range.end; ++i)
            if (!isdigit(path.native()[i]))
                return false;

        return true;
    };

    for (const auto& entry : std::filesystem::directory_iterator(folder, pattern)) {
        if (std::filesystem::is_regular_file(entry.status()) && can_increment(entry.path())) {
            const auto& match = entry.path();
            if (match > last_match) {
                last_match = match;
            }
        }
    }

    return last_match;
}

/* Given a file path like "FOO_0001.txt" increment it to "FOO_0002.txt". */
static std::filesystem::path increment_filename_ordinal(
    const std::filesystem::path& path,
    pattern_range range) {
    auto name = path.filename().native();

    for (auto i = range.end; i >= range.start; --i) {
        auto& c = name[i];

        // Not a digit or would overflow the counter.
        if (c < u'0' || c > u'9' || (c == u'9' && i == range.start))
            return {};

        if (c == u'9')
            c = '0';
        else {
            c++;
            break;
        }
    }

    return {name};
}

std::filesystem::path next_filename_matching_pattern(const std::filesystem::path& filename_pattern) {
    auto path = filename_pattern.parent_path();
    auto pattern = filename_pattern.filename();
    auto range = pattern_range{
        pattern.native().find_first_of(u'?'),
        pattern.native().find_last_of(u'?')};

    const auto match = find_last_ordinal_match(path, pattern, range);

    if (match.empty()) {
        auto pattern_str = pattern.native();
        for (auto i = range.start; i <= range.end; ++i)
            pattern_str[i] = u'0';
        return path / pattern_str;
    }

    auto next_name = increment_filename_ordinal(match, range);
    return next_name.empty() ? next_name : path / next_name;
}

std::vector<std::filesystem::path> scan_root_files(const std::filesystem::path& directory,
                                                   const std::filesystem::path& extension) {
    std::vector<std::filesystem::path> file_list{};
    scan_root_files(directory, extension, [&file_list](const std::filesystem::path& p) {
        file_list.push_back(p);
    });

    return file_list;
}

std::vector<std::filesystem::path> scan_root_directories(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> directory_list{};

    for (const auto& entry : std::filesystem::directory_iterator(directory, "*")) {
        if (std::filesystem::is_directory(entry.status())) {
            directory_list.push_back(entry.path());
        }
    }

    return directory_list;
}

std::filesystem::filesystem_error delete_file(const std::filesystem::path& file_path) {
    return {f_unlink(reinterpret_cast<const TCHAR*>(file_path.c_str()))};
}

std::filesystem::filesystem_error rename_file(
    const std::filesystem::path& file_path,
    const std::filesystem::path& new_name) {
    return {f_rename(reinterpret_cast<const TCHAR*>(file_path.c_str()), reinterpret_cast<const TCHAR*>(new_name.c_str()))};
}

std::filesystem::filesystem_error copy_file(
    const std::filesystem::path& file_path,
    const std::filesystem::path& dest_path) {
    constexpr size_t buffer_size = std::filesystem::max_file_block_size;
    uint8_t buffer[buffer_size];
    File src;
    File dst;

    auto error = src.open(file_path);
    if (error) return error.value();

    error = dst.create(dest_path);
    if (error) return error.value();

    while (true) {
        auto result = src.read(buffer, buffer_size);
        if (result.is_error()) return result.error();

        result = dst.write(buffer, *result);
        if (result.is_error()) return result.error();

        if (*result < buffer_size)
            break;
    }

    return {};
}

FATTimestamp file_created_date(const std::filesystem::path& file_path) {
    FILINFO filinfo;

    f_stat(reinterpret_cast<const TCHAR*>(file_path.c_str()), &filinfo);

    return {filinfo.fdate, filinfo.ftime};
}

std::filesystem::filesystem_error file_update_date(const std::filesystem::path& file_path, FATTimestamp timestamp) {
    FILINFO filinfo{};

    filinfo.fdate = timestamp.FAT_date;
    filinfo.ftime = timestamp.FAT_time;
    return f_utime(reinterpret_cast<const TCHAR*>(file_path.c_str()), &filinfo);
}

std::filesystem::filesystem_error make_new_file(
    const std::filesystem::path& file_path) {
    File f;
    auto error = f.create(file_path);
    if (error)
        return *error;

    return {};
}

std::filesystem::filesystem_error make_new_directory(
    const std::filesystem::path& dir_path) {
    return {f_mkdir(reinterpret_cast<const TCHAR*>(dir_path.c_str()))};
}

std::filesystem::filesystem_error ensure_directory(
    const std::filesystem::path& dir_path) {
    if (dir_path.empty() || std::filesystem::file_exists(dir_path))
        return {};

    auto result = ensure_directory(dir_path.parent_path());
    if (result.code())
        return result;

    return make_new_directory(dir_path);
}

namespace std {
namespace filesystem {

std::string filesystem_error::what() const {
    switch (err) {
        case FR_OK:
            return "ok";
        case FR_DISK_ERR:
            return "disk error";
        case FR_INT_ERR:
            return "insanity detected";
        case FR_NOT_READY:
            return "SD card not ready";
        case FR_NO_FILE:
            return "no file";
        case FR_NO_PATH:
            return "no path";
        case FR_INVALID_NAME:
            return "invalid name";
        case FR_DENIED:
            return "denied";
        case FR_EXIST:
            return "exists";
        case FR_INVALID_OBJECT:
            return "invalid object";
        case FR_WRITE_PROTECTED:
            return "write protected";
        case FR_INVALID_DRIVE:
            return "invalid drive";
        case FR_NOT_ENABLED:
            return "not enabled";
        case FR_NO_FILESYSTEM:
            return "no filesystem";
        case FR_MKFS_ABORTED:
            return "mkfs aborted";
        case FR_TIMEOUT:
            return "timeout";
        case FR_LOCKED:
            return "locked";
        case FR_NOT_ENOUGH_CORE:
            return "not enough core";
        case FR_TOO_MANY_OPEN_FILES:
            return "too many open files";
        case FR_INVALID_PARAMETER:
            return "invalid parameter";
        case FR_EOF:
            return "end of file";
        case FR_DISK_FULL:
            return "disk full";
        case FR_BAD_SEEK:
            return "bad seek";
        case FR_UNEXPECTED:
            return "unexpected";
        default:
            return "unknown";
    }
}

path path::parent_path() const {
    const auto index = _s.find_last_of(preferred_separator);
    if (index == _s.npos) {
        return {};  // NB: Deviation from STL.
    } else {
        return _s.substr(0, index);
    }
}

path path::extension() const {
    const auto t = filename().native();
    const auto index = t.find_last_of(u'.');
    if (index == t.npos) {
        return {};
    } else {
        return t.substr(index);
    }
}

path path::filename() const {
    const auto index = _s.find_last_of(preferred_separator);
    if (index == _s.npos) {
        return _s;
    } else {
        return _s.substr(index + 1);
    }
}

path path::stem() const {
    const auto t = filename().native();
    const auto index = t.find_last_of(u'.');
    if (index == t.npos) {
        return t;
    } else {
        return t.substr(0, index);
    }
}

std::string path::string() const {
    std::wstring_convert<std::codecvt_utf8_utf16<path::value_type>, path::value_type> conv;
    return conv.to_bytes(native());
}

// appends a string to the end of filename, but leaves the extension asd.txt + "fg" -> asdfg.txt
path& path::append_filename(const string_type& str) {
    const auto t = extension().native();
    _s.erase(_s.size() - t.size());  // remove extension
    _s += str;                       // append string
    _s += t;                         // add back extension
    return *this;
}

path& path::replace_extension(const path& replacement) {
    const auto t = extension().native();
    _s.erase(_s.size() - t.size());
    if (!replacement._s.empty()) {
        if (replacement._s.front() != u'.') {
            _s += u'.';
        }
        _s += replacement._s;
    }
    return *this;
}

bool operator==(const path& lhs, const path& rhs) {
    return lhs.native() == rhs.native();
}

bool operator!=(const path& lhs, const path& rhs) {
    return !(lhs == rhs);
}

bool operator<(const path& lhs, const path& rhs) {
    return lhs.native() < rhs.native();
}

bool operator>(const path& lhs, const path& rhs) {
    return lhs.native() > rhs.native();
}

path operator+(const path& lhs, const path& rhs) {
    path result = lhs;
    result += rhs;
    return result;
}

path operator/(const path& lhs, const path& rhs) {
    path result = lhs;
    result /= rhs;
    return result;
}

bool path_iequal(
    const path& lhs,
    const path& rhs) {
    const auto& lhs_str = lhs.native();
    const auto& rhs_str = rhs.native();

    // NB: Not correct for Unicode/locales.
    if (lhs_str.length() == rhs_str.length()) {
        for (size_t i = 0; i < lhs_str.length(); ++i)
            if (towupper(lhs_str[i]) != towupper(rhs_str[i]))
                return false;

        return true;
    }

    return false;
}

bool is_cxx_capture_file(const path& filename) {
    auto ext = filename.extension();
    return path_iequal(c8_ext, ext) || path_iequal(c16_ext, ext);
}

uint8_t capture_file_sample_size(const path& filename) {
    if (path_iequal(filename.extension(), c8_ext))
        return sizeof(complex8_t);
    if (path_iequal(filename.extension(), c16_ext))
        return sizeof(complex16_t);
    return 0;
}

directory_iterator::directory_iterator(
    const std::filesystem::path& path,
    const std::filesystem::path& wild)
    : path_{path}, wild_{wild} {
    impl = std::make_shared<Impl>();
    auto result = f_findfirst(&impl->dir, &impl->filinfo,
                              path_.tchar(), wild_.tchar());
    if (result != FR_OK || impl->filinfo.fname[0] == (TCHAR)'\0') {
        impl.reset();
        // TODO: Throw exception if/when I enable exceptions...
    }
}

directory_iterator& directory_iterator::operator++() {
    const auto result = f_findnext(&impl->dir, &impl->filinfo);
    if ((result != FR_OK) || (impl->filinfo.fname[0] == 0)) {
        impl.reset();
    }
    return *this;
}

bool is_directory(const file_status s) {
    return (s & AM_DIR);
}

bool is_regular_file(const file_status s) {
    return !(s & AM_DIR);
}

bool file_exists(const path& file_path) {
    FILINFO filinfo;
    auto fr = f_stat(reinterpret_cast<const TCHAR*>(file_path.c_str()), &filinfo);

    return fr == FR_OK;
}

bool is_directory(const path& file_path) {
    FILINFO filinfo;
    auto fr = f_stat(reinterpret_cast<const TCHAR*>(file_path.c_str()), &filinfo);

    return fr == FR_OK && is_directory(static_cast<file_status>(filinfo.fattrib));
}

bool is_empty_directory(const path& file_path) {
    DIR dir;
    FILINFO filinfo;

    if (!is_directory(file_path))
        return false;

    auto result = f_findfirst(&dir, &filinfo, reinterpret_cast<const TCHAR*>(file_path.c_str()), (const TCHAR*)u"*");
    return !((result == FR_OK) && (filinfo.fname[0] != (TCHAR)'\0'));
}

int file_count(const path& directory) {
    int count{0};

    for (auto& entry : std::filesystem::directory_iterator(directory, (const TCHAR*)u"*")) {
        (void)entry;  // avoid unused warning
        ++count;
    }

    return count;
}

space_info space(const path& p) {
    DWORD free_clusters{0};
    FATFS* fs;
    if (f_getfree(reinterpret_cast<const TCHAR*>(p.c_str()), &free_clusters, &fs) == FR_OK) {
#if _MAX_SS != _MIN_SS
        static_assert(false, "FatFs not configured for fixed sector size");
#else
        const std::uintmax_t cluster_bytes = fs->csize * _MIN_SS;
        return {
            (fs->n_fatent - 2) * cluster_bytes,
            free_clusters * cluster_bytes,
            free_clusters * cluster_bytes,
        };
#endif
    } else {
        return {0, 0, 0};
    }
}

} /* namespace filesystem */
} /* namespace std */
