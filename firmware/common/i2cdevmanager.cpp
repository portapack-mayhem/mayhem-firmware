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

#include "i2cdevmanager.hpp"
#include "event_m0.hpp"

#define i2cbus portapack::i2c0

namespace i2cdev {

// statics
uint16_t I2CDevManager::scan_interval = 0;
bool I2CDevManager::force_scan = false;
Thread* I2CDevManager::thread;
std::vector<I2DevListElement> I2CDevManager::devlist;
Mutex I2CDevManager::mutex_list;

/*
    DEAR DEVELOPERS!
    IF YOU WANT TO ADD NEW DERIVERS, PUT IT'S I2C ADDRESS AND INIT PART HERE.
    THE INIT MUST RETURN FALSE, WHEN THE DEVICE NOT MATCH OR NOT WORKING.
*/

bool I2CDevManager::found(uint8_t addr) {
    // check if present already
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr == addr) return false;
    }
    // try to find a suitable driver
    // todo htotoo
    // std::unique_ptr<I2cDev> ptr;
    // if (addr == 12 || addr == 44) ptr = std::make_unique<I2cDev>();

    // if can't find any driver, add it too with empty

    devlist.push_back({addr});
    return true;
}

I2cDev::I2cDev() {};
I2cDev::I2cDev(uint8_t addr_) {
    addr = addr_;
};

void I2CDevManager::init() {
    force_scan = true;
    create_thread();
}

void I2CDevManager::manual_scan() {
    force_scan = true;
}

void I2CDevManager::set_autoscan_interval(uint16_t interval) {
    scan_interval = interval;
}
uint16_t I2CDevManager::get_autoscan_interval() {
    return scan_interval;
}

I2cDev* I2CDevManager::get_dev_by_addr(uint8_t addr) {
    // chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr == addr) {
            chMtxUnlock();
            return devlist[i].dev.get();
        }
    }
    // chMtxUnlock();
    return nullptr;
}

I2cDev* I2CDevManager::get_dev_by_model(I2C_DEVS model) {
    // chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].dev && devlist[i].dev->model == model) {
            // chMtxUnlock();
            return devlist[i].dev.get();
        }
    }
    // chMtxUnlock();
    return nullptr;
}

std::vector<I2C_DEVS> I2CDevManager::get_gev_list_by_model() {
    std::vector<I2C_DEVS> ret;
    // chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].dev) {
            ret.push_back(devlist[i].dev->model);
        }
    }
    // chMtxUnlock();
    return ret;
}

std::vector<uint8_t> I2CDevManager::get_gev_list_by_addr() {
    std::vector<uint8_t> ret;
    // chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr) {
            ret.push_back(devlist[i].addr);
        }
    }
    // chMtxUnlock();
    return ret;
}

bool I2CDevManager::scan() {
    bool found_new = false;
    for (uint8_t i = 1; i < 128; ++i) {
        if (i2cbus.probe(i, 50)) {
            // chMtxLock(&mutex_list);
            found_new = found_new | found(i);
            // chMtxUnlock();
        }
    }
    return found_new;
}

void I2CDevManager::create_thread() {
    thread = chThdCreateFromHeap(NULL, 512, NORMALPRIO, I2CDevManager::timer_fn, nullptr);
}

msg_t I2CDevManager::timer_fn(void* arg) {
    (void)arg;
    uint16_t curr_timer = 0;  // seconds since thread start
    while (1) {
        bool changed = false;
        // check if i2c scan needed
        if (force_scan || (scan_interval != 0 && curr_timer % scan_interval == 0)) {
            changed = changed | scan();
            force_scan = false;
        }
        for (size_t i = 0; i < devlist.size(); i++) {
            if (devlist[i].addr != 0 && devlist[i].dev) {
                devlist[i].dev->update();  // updates it's data, and broadcasts it. if there is any error it will handle in it, and later we can remove it
            }
        }

        // remove all unneeded items
        // chMtxLock(&mutex_list);
        size_t cnt = devlist.size();
        devlist.erase(std::remove_if(devlist.begin(), devlist.end(), [](const I2DevListElement& x) {
                          if (x.addr == 0) return true;
                          if (x.dev && x.dev->need_del == true) return true;  // self destruct on too many errors
                          return false;                                       // won't remove the unidentified ones, so we can list them, and not trying all the time with them
                      }),
                      devlist.end());
        // chMtxUnlock();
        if (cnt != devlist.size()) changed = true;

        if (changed) {
            I2CDevListChangedMessage msg{};
            EventDispatcher::send_message(msg);
        }
        chThdSleepMilliseconds(1000);  // 1sec timer //todo htotoo make it really 1 sec, substract the elpased time
        ++curr_timer;
    }
    return 0;
}

};  // namespace i2cdev
