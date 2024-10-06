#include "battery.hpp"
#include "portapack.hpp"
#include "i2cdevmanager.hpp"
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

}  // namespace battery