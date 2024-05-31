#ifndef __MAX17055_H__
#define __MAX17055_H__

#include <cstdint>
#include <array>
#include <string>

#include "i2c_pp.hpp"

namespace battery {
namespace max17055 {

using address_t = uint8_t;

class MAX17055 {
   public:
    constexpr MAX17055(I2C& bus, const I2C::address_t bus_address)
        : bus(bus), bus_address(bus_address), detected_(false) {}

    void init();
    bool detect();
    bool isDetected() const { return detected_; }

    uint16_t readVoltage();
    void getBatteryInfo(uint8_t& batteryPercentage, uint16_t& voltage);

   private:
    I2C& bus;
    const I2C::address_t bus_address;
    bool detected_;

    bool write(const uint8_t reg, const uint8_t value);
    uint16_t read(const uint8_t reg);
};

} /* namespace max17055 */
}  // namespace battery
#endif /* __MAX17055_H__ */