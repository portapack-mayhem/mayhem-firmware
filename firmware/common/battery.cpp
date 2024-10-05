#include "battery.hpp"
#include "portapack.hpp"
#include "i2cdevmanager.hpp"
namespace battery {

// ads1110::ADS1110 battery_ads1110{portapack::i2c0, 0x48};
// max17055::MAX17055 battery_max17055{portapack::i2c0, 0x36};

bool BatteryManagement::calcOverride = false;

// set if the default percentage calculation should be overrided by voltage based one
void BatteryManagement::set_calc_override(bool override) {
    calcOverride = override;
}

bool BatteryManagement::isDetected() {
    auto dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEV_MAX17055);
    if (dev) return true;
    dev = i2cdev::I2CDevManager::get_dev_by_model(I2CDEV_ADS1110);
    if (dev) return true;
    return false;
}

// return (uint16_t)battery_max17055.getValue("Cycles");
// return battery_max17055.getValue("TTE");
// return battery_max17055.getValue("TTF");
// return battery_max17055.read_register(reg);

uint8_t BatteryManagement::calc_percent_voltage(uint16_t voltage) {
    // Calculate the remaining battery percentage
    uint8_t batteryPercentage = (float)(voltage - BATTERY_MIN_VOLTAGE) / (float)(BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;
    // Limit the values to the valid range
    batteryPercentage = (batteryPercentage > 100) ? 100 : batteryPercentage;
    return batteryPercentage;
}

}  // namespace battery