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
#include "event_m0.hpp"

#define i2cbus portapack::i2c0

extern I2C portapack::i2c0;

namespace i2cdev {

// The device class. You'll derive your from this. Override init() and update();
class I2cDev {
   public:
    virtual ~I2cDev(){};
    virtual bool init(uint8_t addr) = 0;  // returns true if it is that that device we are looking for.
    virtual void update() = 0;            // override this, and you'll be able to query your device and broadcast the result to the system

    void set_update_interval(uint8_t interval);  // sets the device's update interval in sec. if you change it, don't forget to change back to it's original value after you finished!
    uint8_t get_update_interval();               // gets the device's update interval in sec

    bool i2c_read(uint8_t* reg, uint8_t reg_size, uint8_t* data, uint8_t bytes);   // if want to read without register addr, just set reg_size to 0. this way can read 8, or 16 or 32 bit registers too. reg_size in bytes! returns true on succes. handles the errcnt automatically!
    bool i2c_write(uint8_t* reg, uint8_t reg_size, uint8_t* data, uint8_t bytes);  // if want to write without register addr, just set reg_size to 0. this way can read 8, or 16 or 32 bit registers too. reg_size in bytes! returns true on succes. handles the errcnt automatically!

    // helpers for easier i2c communication
    uint8_t read8_1(uint8_t reg);
    bool write8_1(uint8_t reg, uint8_t data);
    uint16_t read16_1(uint8_t reg);
    int16_t readS16_1(uint8_t reg);
    uint16_t read16_LE_1(uint8_t reg);
    int16_t readS16_LE_1(uint8_t reg);
    uint32_t read24_1(uint8_t reg);

    bool lockDevice();    // locks the device's "mutex". returns true on success. guard important communication with this
    void unlockDevice();  // unlocks the device's "mutex"
    inline bool isLocked() { return mtx != 0; }

    bool need_del = false;                // device can self destruct, and re-init when new scan discovers it
    I2C_DEVMDL model = I2CDEVMDL_NOTSET;  // overwrite it in the init()!!!
    uint8_t query_interval = 5;           // in seconds. can be overriden in init() if necessary
   protected:
    void got_error();    // i2c communication will call this when communication was not ok. you can call it from any part of your code too.
    void got_success();  // i2c communication will call this when the communication was ok. you can call it from any part of your code too.

    uint8_t addr = 0;    // some devices can have different addresses, so we store what was it wound with
    uint8_t errcnt = 0;  // error count during communication. if it reaches a threshold set need_del to remove itself from the device list
    uint8_t mtx = 0;     // internal lock, so important client code can be skipped when lock is on. check PPMod for an example
};

// store for the devices. may not have a driver if not supported
class I2DevListElement {
   public:
    uint8_t addr = 0;                       // i2c addr of the device
    std::unique_ptr<I2cDev> dev = nullptr;  // device driver if any
};

class I2CDevManager {
   public:
    static void init();                                    // creates the thread, and sets an one time full scan
    static void manual_scan();                             // it'll init a forced device scan in the thread's next cycle. (1sec max)
    static void set_autoscan_interval(uint16_t interval);  // 0 no auto scan, other values: seconds
    static uint16_t get_autoscan_interval();
    static I2cDev* get_dev_by_addr(uint8_t addr);            // caller function needs to cast to the specific device!
    static I2cDev* get_dev_by_model(I2C_DEVMDL model);       // caller function needs to cast to the specific device!
    static std::vector<I2C_DEVMDL> get_dev_list_by_model();  // returns the currently discovered
    static std::vector<uint8_t> get_gev_list_by_addr();      // returns the currently discovered
    static void setEventDispatcher(EventDispatcher* ed) { _eventDispatcher = ed; }
    static EventDispatcher* get_event_dispatcher() { return _eventDispatcher; }

   private:
    static uint16_t scan_interval;
    static bool force_scan;  // if set to true, on hte next run it'll do an i2c scan, ONCE
    static std::vector<I2DevListElement> devlist;

    static bool found(uint8_t addr);  // returns true on any new device. also initializes the driver if there is a suitable one
    static bool scan();               // return true on any change (delete or found new)
    static void create_thread();
    static msg_t timer_fn(void* arg);
    static Thread* thread;
    static Mutex mutex_list;
    static EventDispatcher* _eventDispatcher;
};
};  // namespace i2cdev

#endif