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

#define i2cbus portapack::i2c0

namespace i2cdev {

uint16_t I2CDevManager::scan_interval = 0;
bool I2CDevManager::force_scan = false;

void I2CDevManager::init() {
    create_thread();
    force_scan = true;
}

void I2CDevManager::manual_scan() {
}

void I2CDevManager::set_autoscan_interval(uint16_t interval) {
    scan_interval = interval;
}

I2cDev* I2CDevManager::get_dev_by_addr(uint8_t addr) {
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr == addr) {
            return devlist[i].dev.get();
        }
    }
    return nullptr;
}

void I2CDevManager::found(uint8_t addr) {
    // check if present already
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr == addr) return;
    }
    // try to find a suitable driver
    // todo
    // std::unique_ptr<I2cDev> ptr;
    // if (addr == 12 || addr == 44) ptr = std::make_unique<I2cDev>();
}

void I2CDevManager::scan() {
    for (uint8_t i = 1; i < 128; ++i) {
        if (i2cbus.probe(i, 50)) {
            found(i);
        }
    }
}

void I2CDevManager::create_thread() {
    thread = chThdCreateFromHeap(NULL, 512, NORMALPRIO, I2CDevManager::timer_fn, nullptr);
}

msg_t I2CDevManager::timer_fn(void* arg) {
    (void)arg;
    uint16_t curr_timer = 0;  // seconds since thread start
    while (1) {
        // check if i2c scan needed
        if (force_scan || (scan_interval != 0 && curr_timer % scan_interval == 0)) {
            scan();
            force_scan = false;
        }
        for (size_t i = 0; i < devlist.size(); i++) {
            if (devlist[i].addr != 0 && devlist[i].dev) {
                devlist[i].dev->update();  // updates it's data, and broadcasts it. if there is any error it will handle in it, and later we can remove it
            }
        }

        // remove all unneeded items
        devlist.erase(std::remove_if(devlist.begin(), devlist.end(), [](const I2DevListElement& x) {
                          if (x.addr == true) return true;
                          if (x.dev) return true;
                          if (x.dev->need_del == true) return true;
                          return false;
                      }),
                      devlist.end());
        chThdSleepMilliseconds(1000);  // 1sec timer //todo make it really 1 sec, substract the elpased time
        ++curr_timer;
    }
    return 0;
}

};  // namespace i2cdev
