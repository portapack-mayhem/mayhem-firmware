#include "battery.hpp"
#include "portapack.hpp"
#include "i2cdev_max17055.hpp"
#include "i2cdev_ads1110.hpp"

namespace battery {

bool BatteryManagement::calcOverride = false;

// set if the default percentage calculation should be overrided by voltage based one
void BatteryManagement::set_calc_override(bool override) {
    calcOverride = override;
}

// Helper function to checkif there is ANY battery management ic present.
bool BatteryManagement::isDetected() {
    auto dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_MAX17055);
    if (dev) return true;
    dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_ADS1110);
    if (dev) return true;
    return false;
}

uint8_t BatteryManagement::calc_percent_voltage(uint16_t voltage) {
    // Calculate the remaining battery percentage
    uint8_t batteryPercentage = (float)(voltage - BATTERY_MIN_VOLTAGE) / (float)(BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;
    // Limit the values to the valid range
    batteryPercentage = (batteryPercentage > 100) ? 100 : batteryPercentage;
    return batteryPercentage;
}

// helper function to get data from ANY batt management ic
void BatteryManagement::getBatteryInfo(uint8_t& valid_mask, uint8_t& percent, uint16_t& voltage, int32_t& current) {
    auto dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_MAX17055);
    if (dev) {
        ((i2cdev::I2cDev_MAX17055*)dev)->getBatteryInfo(valid_mask, percent, voltage, current);
        return;
    }
    dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_ADS1110);
    if (dev) {
        voltage = ((i2cdev::I2cDev_ADS1110*)dev)->readVoltage();
        percent = calc_percent_voltage(voltage);
        valid_mask = 1;
        return;
    }

    valid_mask = 0;
}

// helper function to get data from ANY batt management ic
uint16_t BatteryManagement::get_cycles() {
    auto dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_MAX17055);
    if (dev) {
        return ((i2cdev::I2cDev_MAX17055*)dev)->getValue("Cycles");
    }
    return 0;
}

// helper function to get data from ANY batt management ic
float BatteryManagement::get_tte() {
    auto dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_MAX17055);
    if (dev) {
        return ((i2cdev::I2cDev_MAX17055*)dev)->getValue("TTE");
    }
    return 0;
}

// helper function to get data from ANY batt management ic
float BatteryManagement::get_ttf() {
    auto dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEVMDL_MAX17055);
    if (dev) {
        return ((i2cdev::I2cDev_MAX17055*)dev)->getValue("TTF");
    }
    return 0;
}

}  // namespace battery