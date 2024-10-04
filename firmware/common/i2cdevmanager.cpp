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

/*
    DEAR DEVS.
    Include your devices headers here:
*/

#include "i2cdev_bmx280.hpp"

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
    I2DevListElement item;
    item.addr = addr;
    if (!item.dev && (addr == 0x76 || addr == 0x77)) {  // check if device is already taken, and i can handle the address
        item.dev = std::make_unique<I2cDev_BMX280>();
        if (!item.dev->init(addr)) item.dev = nullptr;  // if not inited, reset it's instance, and let other handlers try
    }
    /*
        DEAR DEVS
        Put your driver's init code here. ALLWAYS check the !item.dev, if any other driver already took it. Also check the addr if it suits your module. (also need additional checks in the init() code)
    */

    // if can't find any driver, add it too with empty, so we won't try to init it again and again
    devlist.push_back(std::move(item));
    return true;
}

bool I2cDev::i2c_read(uint8_t* reg, uint8_t reg_size, uint8_t* data, uint8_t bytes) {
    if (bytes == 0) return false;
    if (reg_size > 0 && reg) i2cbus.transmit(addr, reg, reg_size);
    return i2cbus.receive(addr, data, bytes);
}

bool I2cDev::i2c_write(uint8_t* reg, uint8_t reg_size, uint8_t* data, uint8_t bytes) {
    if (bytes == 0) return false;
    if (reg_size > 0 && reg) i2cbus.transmit(addr, reg, reg_size);
    return i2cbus.transmit(addr, data, bytes);
}

bool I2cDev::write8_1(uint8_t reg, uint8_t data) {
    return i2c_write(&reg, 1, &data, 1);
}

uint8_t I2cDev::read8_1(uint8_t reg) {
    uint8_t res = 0;
    i2c_read(&reg, 1, &res, 1);
    return res;
}

uint16_t I2cDev::read16_1(uint8_t reg) {
    uint16_t res = 0;
    i2c_read(&reg, 1, (uint8_t*)&res, 2);
    return res;
}

uint16_t I2cDev::read16_LE_1(uint8_t reg) {
    uint16_t res = read16_1(reg);
    res = (res >> 8) | (res << 8);
    return res;
}
int16_t I2cDev::readS16_LE_1(uint8_t reg) {
    int16_t res = (int16_t)read16_1(reg);
    res = (res >> 8) | (res << 8);
    return res;
}

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
    // todo htotoo remove unplugged items
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
                if ((curr_timer % devlist[i].dev->query_interval) == 0) {  // only if it is device's interval
                    devlist[i].dev->update();                              // updates it's data, and broadcasts it. if there is any error it will handle in it, and later we can remove it
                }
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
        chThdSleepMilliseconds(1000);  // 1sec timer //todo htotoo make it really 1 sec, substract the elpased time, but minimayl wait of 100 must be met
        ++curr_timer;
    }
    return 0;
}

};  // namespace i2cdev
