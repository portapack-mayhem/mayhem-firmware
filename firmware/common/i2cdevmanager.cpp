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
#include <algorithm>
#include "i2cdevmanager.hpp"

/*
    DEAR DEVS.
    Include your devices headers here:
*/

#include "i2cdev_bmx280.hpp"
#include "i2cdev_sht3x.hpp"
#include "i2cdev_max17055.hpp"
#include "i2cdev_ads1110.hpp"
#include "i2cdev_bh1750.hpp"
#include "i2cdev_ppmod.hpp"
#include "i2cdev_sht4x.hpp"

namespace i2cdev {

// statics
uint16_t I2CDevManager::scan_interval = 0;
bool I2CDevManager::force_scan = false;
Thread* I2CDevManager::thread;
std::vector<I2DevListElement> I2CDevManager::devlist;
Mutex I2CDevManager::mutex_list{};
EventDispatcher* I2CDevManager::_eventDispatcher;

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

    /*
        DEAR DEVS
        Put your driver's init code here. ALLWAYS check the !item.dev, if any other driver already took it. Also check the addr if it suits your module. (also need additional checks in the init() code)
    */

    if (!item.dev && (addr == I2CDEV_BMX280_ADDR_1 || addr == I2CDEV_BMX280_ADDR_2)) {  // check if device is already taken, and i can handle the address
        item.dev = std::make_unique<I2cDev_BMX280>();
        if (!item.dev->init(addr)) item.dev = nullptr;  // if not inited, reset it's instance, and let other handlers try
    }

    if (!item.dev && (addr == I2CDEV_SHT3X_ADDR_1 || addr == I2CDEV_SHT3X_ADDR_2)) {
        item.dev = std::make_unique<I2cDev_SHT3x>();
        if (!item.dev->init(addr)) item.dev = nullptr;
    }

    if (!item.dev && (addr == I2CDEV_SHT4X_ADDR_1 || addr == I2CDEV_SHT4X_ADDR_2)) {
        item.dev = std::make_unique<I2cDev_SHT4x>();
        if (!item.dev->init(addr)) item.dev = nullptr;
    }

    if (!item.dev && (addr == I2CDEV_MAX17055_ADDR_1)) {
        item.dev = std::make_unique<I2cDev_MAX17055>();
        if (!item.dev->init(addr)) item.dev = nullptr;
    }

    if (!item.dev && (addr == I2CDEV_ADS1110_ADDR_1)) {
        item.dev = std::make_unique<I2cDev_ADS1110>();
        if (!item.dev->init(addr)) item.dev = nullptr;
    }

    if (!item.dev && (addr == I2CDEV_BH1750_ADDR_1)) {
        item.dev = std::make_unique<I2cDev_BH1750>();
        if (!item.dev->init(addr)) item.dev = nullptr;
    }

    if (!item.dev && (addr == I2CDEV_PPMOD_ADDR_1)) {
        item.dev = std::make_unique<I2cDev_PPmod>();
        if (!item.dev->init(addr)) item.dev = nullptr;
    }

    // if can't find any driver, add it too with empty, so we won't try to init it again and again
    devlist.push_back(std::move(item));
    return true;
}

/*


FROM HERE YOU SHOULDN'T WRITE ANYTHING IF YOU JUST IMPLEMENT A NEW DRIVER
(maybe maximum i2c_rewd / write helpers)



*/

void I2cDev::unlockDevice() {
    mtx = 0;
}

// when device is locked, all not important queries will return 0 or nullopt, so no communication broken.
bool I2cDev::lockDevice() {
    if (mtx == 0) {
        mtx = 1;
        return true;
    }
    return false;
}

void I2cDev::set_update_interval(uint8_t interval) {
    query_interval = interval;
}
uint8_t I2cDev::get_update_interval() {
    return query_interval;
}

void I2cDev::got_error() {
    errcnt++;
    if (errcnt >= 5) need_del = true;  // too many errors. remove dev from list. may be re-discovered and re inited
}

void I2cDev::got_success() {
    errcnt = 0;
}

/*

    I2C read / write functions and helpers.

*/

bool I2cDev::i2c_read(uint8_t* reg, uint8_t reg_size, uint8_t* data, uint8_t bytes) {
    if (bytes == 0) return false;
    bool ret = true;
    if (reg_size > 0 && reg) ret = i2cbus.transmit(addr, reg, reg_size, 150);
    if (!ret) {
        got_error();
        return false;
    }
    ret = i2cbus.receive(addr, data, bytes, 150);
    if (!ret)
        got_error();
    else
        got_success();
    return ret;
}

bool I2cDev::i2c_write(uint8_t* reg, uint8_t reg_size, uint8_t* data, uint8_t bytes) {
    // Check if there's any data to write
    if (bytes == 0) return false;
    // Create a new buffer to hold both reg and data
    uint8_t total_size = reg_size + bytes;
    uint8_t* buffer = new uint8_t[total_size];
    // Copy the register data into the buffer
    if (reg_size > 0 && reg) {
        memcpy(buffer, reg, reg_size);
    }
    // Copy the data into the buffer after the register data
    memcpy(buffer + reg_size, data, bytes);
    // Transmit the combined data
    bool result = i2cbus.transmit(addr, buffer, total_size, 150);
    // Clean up the dynamically allocated buffer
    delete[] buffer;
    if (!result)
        got_error();
    else
        got_success();
    return result;
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
    uint8_t buffer[2];
    i2c_read(&reg, 1, buffer, 2);
    return uint16_t(buffer[0]) << 8 | uint16_t(buffer[1]);
}
uint32_t I2cDev::read24_1(uint8_t reg) {
    uint8_t buffer[3];
    i2c_read(&reg, 1, buffer, 2);
    return uint32_t(buffer[0]) << 16 | uint32_t(buffer[1]) << 8 | uint32_t(buffer[2]);
}

int16_t I2cDev::readS16_1(uint8_t reg) {
    return (int16_t)read16_1(reg);
}

uint16_t I2cDev::read16_LE_1(uint8_t reg) {
    uint16_t res = read16_1(reg);
    res = (res >> 8) | (res << 8);
    return res;
}

int16_t I2cDev::readS16_LE_1(uint8_t reg) {
    return (int16_t)read16_LE_1(reg);
}

// END OF i2C communication + helpers

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
    chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr == addr) {
            chMtxUnlock();
            return devlist[i].dev.get();
        }
    }
    chMtxUnlock();
    return nullptr;
}

I2cDev* I2CDevManager::get_dev_by_model(I2C_DEVMDL model) {
    chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].dev && devlist[i].dev->model == model) {
            chMtxUnlock();
            return devlist[i].dev.get();
        }
    }
    chMtxUnlock();
    return nullptr;
}

std::vector<I2C_DEVMDL> I2CDevManager::get_dev_list_by_model() {
    std::vector<I2C_DEVMDL> ret;
    chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].dev) {
            ret.push_back(devlist[i].dev->model);
        }
    }
    chMtxUnlock();
    return ret;
}

std::vector<uint8_t> I2CDevManager::get_gev_list_by_addr() {
    std::vector<uint8_t> ret;
    chMtxLock(&mutex_list);
    for (size_t i = 0; i < devlist.size(); i++) {
        if (devlist[i].addr) {
            ret.push_back(devlist[i].addr);
        }
    }
    chMtxUnlock();
    return ret;
}

bool I2CDevManager::scan() {
    bool changed = false;
    std::vector<uint8_t> currList;
    for (uint8_t i = 1; i < 128; ++i) {
        if (i2cbus.probe(i, 50)) {
            chMtxLock(&mutex_list);
            changed = changed | found(i);
            chMtxUnlock();
            currList.push_back(i);
        }
        chThdSleepMilliseconds(1);
    }
    // remove those not present
    for (size_t i = 0; i < devlist.size(); ++i) {
        if (std::find(currList.begin(), currList.end(), devlist[i].addr) == currList.end()) {
            // found on our list, but now not discovered, so remove it
            devlist[i].addr = 0;  // mark to delete
            changed = true;
        }
    }
    return changed;
}

void I2CDevManager::create_thread() {
    chMtxInit(&mutex_list);
    thread = chThdCreateFromHeap(NULL, 2048, NORMALPRIO, I2CDevManager::timer_fn, nullptr);
}

msg_t I2CDevManager::timer_fn(void* arg) {
    (void)arg;
    uint16_t curr_timer = 0;  // seconds since thread start
    while (1) {
        systime_t start_time = chTimeNow();
        bool changed = false;
        // check if i2c scan needed
        if (force_scan || (scan_interval != 0 && curr_timer % scan_interval == 0)) {
            changed = changed | scan();
            force_scan = false;
        }
        for (size_t i = 0; i < devlist.size(); i++) {
            if (devlist[i].addr != 0 && devlist[i].dev && devlist[i].dev->query_interval != 0) {
                if ((curr_timer % devlist[i].dev->query_interval) == 0) {  // only if it is device's interval
                    devlist[i].dev->update();                              // updates it's data, and broadcasts it. if there is any error it will handle in it, and later we can remove it
                }
            }
        }

        // remove all unneeded items
        chMtxLock(&mutex_list);
        size_t cnt = devlist.size();
        devlist.erase(std::remove_if(devlist.begin(), devlist.end(), [](const I2DevListElement& x) {
                          if (x.addr == 0) return true;
                          if (x.dev && x.dev->need_del == true) return true;  // self destruct on too many errors
                          return false;                                       // won't remove the unidentified ones, so we can list them, and not trying all the time with them
                      }),
                      devlist.end());
        chMtxUnlock();
        if (cnt != devlist.size()) changed = true;

        if (changed) {
            I2CDevListChangedMessage msg{};
            EventDispatcher::send_message(msg);
        }
        systime_t end_time = chTimeNow();
        systime_t delta = (end_time > start_time) ? end_time - start_time : 100;  // wont calculate overflow, just guess.
        if (delta > 950) delta = 950;                                             // ensure minimum 50 milli sleep

        chThdSleepMilliseconds(1000 - delta);  // 1sec timer
        ++curr_timer;
    }
    return 0;
}

};  // namespace i2cdev

extern "C" int oNofityI2cFromShell(uint8_t* buff, size_t len) {
    i2cdev::I2cDev* dev = i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);
    if (!dev) return 0;  // nothing to send to, so /dev/null
    uint16_t reg = 9;    // COMMAND_SHELL_PPTOMOD_DATA;
    if (dev->i2c_write((uint8_t*)&reg, 2, buff, len)) return 0;
    return 0;  // shoud have an error handler
}
