/*
 * Copyright (C) 2024 HTotoo.
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

#ifndef __I2CDEVMANAGER_H__
#define __I2CDEVMANAGER_H__

#include <cstdint>
#include <vector>
#include <memory>
#include "ch.h"
#include "portapack.hpp"
#include "i2c_pp.hpp"
#include "i2cdevlist.hpp"

extern I2C portapack::i2c0;

namespace i2cdev {

class I2cDev {
   public:
    virtual ~I2cDev() {};
    virtual bool init(uint8_t addr);  // returns true if it is that that device we are looking for.
    virtual void update() = 0;        // override this, and you'll be able to query your device and broadcast the result to the system

    bool need_del = false;           // device can self destruct, and re-init when new scan discovers it
    I2C_DEVS model = I2CDEV_NOTSET;  // overwrite it in the init()!!!
    uint8_t query_interval = 5;      // in seconds. can be overriden in init() if necessary
   protected:
    void got_error();    // update will call this when communication was not ok
    void got_success();  // update will call this when the communication was ok

    uint8_t addr = 0;    // some devices can have different addresses, so we store what was it wound with
    uint8_t errcnt = 0;  // error count during communication. if it reaches a threshold set need_del to remove itself from the device list
};

class I2DevListElement {
   public:
    uint8_t addr = 0;
    std::unique_ptr<I2cDev> dev = nullptr;
};

class I2CDevManager {
   public:
    static void init();                                    // creates the thread, and sets an one time full scan
    static void manual_scan();                             // it'll init a forced device scan in the thread's next cycle. (1sec max)
    static void set_autoscan_interval(uint16_t interval);  // 0 no auto scan, other values: seconds
    static uint16_t get_autoscan_interval();
    static I2cDev* get_dev_by_addr(uint8_t addr);          // caller function needs to cast to the specific device!
    static I2cDev* get_dev_by_model(I2C_DEVS model);       // caller function needs to cast to the specific device!
    static std::vector<I2C_DEVS> get_gev_list_by_model();  // returns the currently discovered
    static std::vector<uint8_t> get_gev_list_by_addr();    // returns the currently discovered
   private:
    static uint16_t scan_interval;
    static bool force_scan;  // if set to true, on hte next run it'll do an i2c scan, ONCE
    static std::vector<I2DevListElement> devlist;

    static bool found(uint8_t addr);  // returns true on any new device. also initializes the driver if there is a suitable one
    static bool scan();
    static void create_thread();
    static msg_t timer_fn(void* arg);
    static Thread* thread;
    static Mutex mutex_list;  // todo fix mutex crash
};
};  // namespace i2cdev
#endif