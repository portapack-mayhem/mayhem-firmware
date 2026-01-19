/*
 * Copyright (C) 2024
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

#include "tpms_database.hpp"
#include "file_path.hpp"
#include <algorithm>

namespace tpms {

TPMSDatabase::TPMSDatabase() {
}

TPMSDatabase::~TPMSDatabase() {
    save_to_file();
}

bool TPMSDatabase::initialize() {
    make_new_directory(u"TPMS");
    return load_from_file();
}

bool TPMSDatabase::add_or_update_sensor(const Reading& reading, uint32_t timestamp, uint32_t frequency) {
    if (reading.type() == Reading::Type::None) {
        return false;
    }

    uint32_t sensor_id = reading.id().value();
    auto it = cache_.find(sensor_id);

    if (it == cache_.end()) {
        // New sensor
        SensorRecord record;
        record.sensor_id = sensor_id;
        record.type = static_cast<uint8_t>(reading.type());
        record.first_seen = timestamp;
        record.last_seen = timestamp;
        record.packet_count = 1;
        record.frequency = frequency;

        if (reading.pressure().is_valid()) {
            record.last_pressure = static_cast<int32_t>(reading.pressure().value().kilopascal() * 100);
        }
        if (reading.temperature().is_valid()) {
            record.last_temperature = static_cast<int32_t>(reading.temperature().value().celsius() * 100);
        }
        if (reading.flags().is_valid()) {
            record.flags = reading.flags().value();
        }

        cache_[sensor_id] = record;
    } else {
        // Update existing sensor
        SensorRecord& record = it->second;
        record.last_seen = timestamp;
        record.packet_count++;
        record.frequency = frequency;

        if (reading.pressure().is_valid()) {
            record.last_pressure = static_cast<int32_t>(reading.pressure().value().kilopascal() * 100);
        }
        if (reading.temperature().is_valid()) {
            record.last_temperature = static_cast<int32_t>(reading.temperature().value().celsius() * 100);
        }
        if (reading.flags().is_valid()) {
            record.flags = reading.flags().value();
        }
    }

    // Save periodically (every 10 updates)
    if (++save_counter_ >= 10) {
        save_counter_ = 0;
        save_to_file();
    }

    return true;
}

bool TPMSDatabase::get_sensor(uint32_t sensor_id, SensorRecord& record) {
    auto it = cache_.find(sensor_id);
    if (it != cache_.end()) {
        record = it->second;
        return true;
    }
    return false;
}

std::vector<SensorRecord> TPMSDatabase::get_all_sensors() {
    std::vector<SensorRecord> sensors;
    sensors.reserve(cache_.size());

    for (const auto& pair : cache_) {
        sensors.push_back(pair.second);
    }

    // Sort by last seen (most recent first)
    std::sort(sensors.begin(), sensors.end(),
              [](const SensorRecord& a, const SensorRecord& b) {
                  return a.last_seen > b.last_seen;
              });

    return sensors;
}

bool TPMSDatabase::delete_sensor(uint32_t sensor_id) {
    auto it = cache_.find(sensor_id);
    if (it != cache_.end()) {
        cache_.erase(it);
        save_to_file();
        return true;
    }
    return false;
}

bool TPMSDatabase::clear_all() {
    cache_.clear();
    delete_file(db_path);
    return true;
}

size_t TPMSDatabase::get_sensor_count() const {
    return cache_.size();
}

std::vector<SensorRecord> TPMSDatabase::get_recent_sensors(uint32_t seconds) {
    // Simplified version - just return all sensors
    (void)seconds;  // Suppress unused warning
    return get_all_sensors();
}

bool TPMSDatabase::sensor_exists(uint32_t sensor_id) const {
    return cache_.find(sensor_id) != cache_.end();
}

bool TPMSDatabase::load_from_file() {
    File file;
    auto open_result = file.open(db_path);
    if (open_result) {
        // Optional contains error - file doesn't exist or can't open
        return true;  // New database, not an error
    }

    cache_.clear();

    SensorRecord record;
    while (true) {
        auto read_result = file.read(&record, sizeof(SensorRecord));
        if (read_result.is_error() || read_result.value() != sizeof(SensorRecord)) {
            break;
        }
        if (record.sensor_id != 0) {  // Validate record
            cache_[record.sensor_id] = record;
        }
    }

    return true;
}

bool TPMSDatabase::save_to_file() {
    if (cache_.empty()) {
        return true;
    }

    // Write to temporary file first
    auto temp_path = u"TPMS/sensors.tmp";

    File file;
    auto create_result = file.create(temp_path);
    if (create_result) {
        // Optional contains error
        return false;
    }

    for (const auto& pair : cache_) {
        auto write_result = file.write(&pair.second, sizeof(SensorRecord));
        if (write_result.is_error()) {
            return false;
        }
    }

    file.sync();
    file.close();

    // Atomic replace
    delete_file(db_path);
    rename_file(temp_path, db_path);

    return true;
}

}  // namespace tpms
