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

#ifndef __TPMS_DATABASE_H__
#define __TPMS_DATABASE_H__

#include "tpms_packet.hpp"
#include "file.hpp"
#include <map>
#include <vector>

namespace tpms {

struct SensorRecord {
    uint32_t sensor_id;
    uint8_t type;  // Reading::Type
    uint32_t first_seen;
    uint32_t last_seen;
    int32_t last_pressure;     // kPa * 100
    int32_t last_temperature;  // Celsius * 100
    uint8_t flags;
    uint32_t packet_count;
    uint32_t frequency;  // Last received frequency

    SensorRecord()
        : sensor_id(0),
          type(0),
          first_seen(0),
          last_seen(0),
          last_pressure(0),
          last_temperature(0),
          flags(0),
          packet_count(0),
          frequency(0) {}
};

class TPMSDatabase {
   public:
    TPMSDatabase();
    ~TPMSDatabase();

    bool initialize();
    bool add_or_update_sensor(const Reading& reading, uint32_t timestamp, uint32_t frequency);
    bool get_sensor(uint32_t sensor_id, SensorRecord& record);
    std::vector<SensorRecord> get_all_sensors();
    bool delete_sensor(uint32_t sensor_id);
    bool clear_all();

    size_t get_sensor_count() const;
    std::vector<SensorRecord> get_recent_sensors(uint32_t seconds = 3600);
    bool sensor_exists(uint32_t sensor_id) const;

   private:
    static constexpr auto db_path = u"TPMS/sensors.db";
    std::map<uint32_t, SensorRecord> cache_;
    uint32_t save_counter_{0};

    bool load_from_file();
    bool save_to_file();
};

}  // namespace tpms

#endif
