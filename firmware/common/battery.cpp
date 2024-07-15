#include "battery.hpp"
#include "event_m0.hpp"
#include "portapack.hpp"
#include "ads1110.hpp"
#include "max17055.hpp"

// uncomment if you want to emulate batt management system
// #define USE_BATT_EMULATOR

extern I2C portapack::i2c0;

namespace battery {

constexpr uint32_t BATTERY_UPDATE_INTERVAL = 30000;
BatteryManagement::BatteryModules BatteryManagement::detected_ = BatteryManagement::BATT_NONE;

ads1110::ADS1110 battery_ads1110{portapack::i2c0, 0x48};
max17055::MAX17055 battery_max17055{portapack::i2c0, 0x36};

Thread* BatteryManagement::thread = nullptr;

void BatteryManagement::init() {
    // try to detect supported modules
    detected_ = BATT_NONE;
    if (battery_ads1110.detect()) {
        battery_ads1110.init();
        detected_ = BATT_ADS1110;
    } else if (battery_max17055.detect()) {
        battery_max17055.init();
        detected_ = BATT_MAX17055;
    }

    // add new supported module detect + init here

#ifdef USE_BATT_EMULATOR
    if (detected_ == BATT_NONE) {
        detected_ = BATT_EMULATOR;
    }
#endif

    if (detected_ != BATT_NONE) {
        // sets timer to query and broadcats this info
        create_thread();
    }
}

// sets the values, it the currend module supports it.
bool BatteryManagement::getBatteryInfo(uint8_t& batteryPercentage, uint16_t& voltage, int32_t& current) {
    if (detected_ == BATT_NONE) {
        return false;
    } else if (detected_ == BATT_ADS1110) {
        battery_ads1110.getBatteryInfo(batteryPercentage, voltage);
        return true;
    } else if (detected_ == BATT_MAX17055) {
        battery_max17055.getBatteryInfo(batteryPercentage, voltage, current);
        return true;
    }
    // add new module query here

#ifdef USE_BATT_EMULATOR
    if (detected_ == BATT_EMULATOR) {
        batteryPercentage += 5;  // %
        if (batteryPercentage > 100) batteryPercentage = 0;
        voltage = rand() % 1000 + 3000;  // mV
        current = rand() % 150;          // mA
        isCharging = rand() % 2;
        return true;
    }
#endif

    (void)current;
    return false;
}

uint16_t BatteryManagement::read_register(const uint8_t reg) {
    if (detected_ == BATT_MAX17055) {
        return battery_max17055.read_register(reg);
    }
    return 0xFFFF;
}

bool BatteryManagement::write_register(const uint8_t reg, const uint16_t value) {
    if (detected_ == BATT_MAX17055) {
        return battery_max17055.write_register(reg, value);
    }
    return false;
}

uint8_t BatteryManagement::getPercent() {
    if (detected_ == BATT_NONE) return 102;
    uint8_t batteryPercentage = 0;
    uint16_t voltage = 0;
    int32_t current = 0;
    getBatteryInfo(batteryPercentage, voltage, current);
    return batteryPercentage;
}

uint16_t BatteryManagement::getVoltage() {
    if (detected_ == BATT_NONE) return 0;
    if (detected_ == BATT_NONE) return 102;
    uint8_t batteryPercentage = 0;
    uint16_t voltage = 0;
    int32_t current = 0;
    getBatteryInfo(batteryPercentage, voltage, current);
    return voltage;
}

msg_t BatteryManagement::timer_fn(void* arg) {
    (void)arg;
    if (!detected_) return 0;
    uint8_t batteryPercentage = 102;
    uint16_t voltage = 0;
    int32_t current = 0;
    chThdSleepMilliseconds(1000);  // wait ui for fully load
    while (1) {
        if (BatteryManagement::getBatteryInfo(batteryPercentage, voltage, current)) {
            // send local message
            BatteryStateMessage msg{batteryPercentage, current >= 0, voltage};
            EventDispatcher::send_message(msg);
        }
        chThdSleepMilliseconds(BATTERY_UPDATE_INTERVAL);
    }
    return 0;
}

void BatteryManagement::create_thread() {
    thread = chThdCreateFromHeap(NULL, 512, NORMALPRIO, BatteryManagement::timer_fn, nullptr);
}

}  // namespace battery