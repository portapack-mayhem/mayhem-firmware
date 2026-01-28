#ifndef __MAX2831_H__
#define __MAX2831_H__

#include "max283x.hpp"
#include "gpio.hpp"
#include "spi_arbiter.hpp"
#include <cstdint>
#include <array>
#include "dirty_registers.hpp"

namespace max2831 {

using namespace max283x;

// MAX2831 typically has 16 registers (0-15)
constexpr size_t reg_count = 16;

enum class Register : address_t {
    Synthesizer_Integer = 3,    // Integer Div + Frac LSBs
    Synthesizer_FracMSB = 4,    // Frac MSBs
    PLL_Config          = 5,    // Ref Div, Lock Detect
    Tx_Rx_Cal           = 6,    // Calibration modes
    Filter_Fine_Tune    = 7,    // Fine tune for Rx/Tx filters
    Config_Control      = 8,    // Coarse tune, RSSI, SPI Enable
    Tx_Gain_Enable      = 9,    // Tx Gain programming mode
    PA_Bias_Delay       = 10,   // PA Bias and Delay
    RX_Gain_Config      = 11,   // LNA Gain AND Rx VGA Gain
    TX_VGA_Config       = 12,   // Tx VGA Gain
    Ref_Osc_Config      = 14,   // Crystal Tune / Clock Out
    RX_Common_Mode      = 15,   // Rx I/Q Common Mode voltage

    // Registers 0, 1, 2, 13 exist but are mostly static config
    PLL_Mode            = 0,
    Lock_Detect_Config  = 1,
    Synth_Config_2      = 2,
    Reserved_13         = 13
};

struct RegisterMap {
    // Placeholder: Raw storage for 16 registers
    std::array<uint16_t, reg_count> w; 
};

class MAX2831 : public MAX283x {
   public:
    constexpr MAX2831(spi::arbiter::Target& target)
        : _target(target) {
    }

    void init() override;
    void set_mode(const Mode mode) override;

    void set_tx_vga_gain(const int_fast8_t db) override;
    void set_lna_gain(const int_fast8_t db) override;
    void set_vga_gain(const int_fast8_t db) override;
    void set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) override;
    void set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) override;

    bool set_frequency(const rf::Frequency lo_frequency) override;

    // These might not apply to MAX2831 or use different regs
    void set_rx_LO_iq_phase_calibration(const size_t v) override;
    void set_tx_LO_iq_phase_calibration(const size_t v) override;
    void set_rx_buff_vcm(const size_t v) override;

    int8_t temp_sense() override;

    reg_t read(const address_t reg_num) override;
    void write(const address_t reg_num, const reg_t value) override;

   private:
    spi::arbiter::Target& _target;
    Mode _mode{Mode::Standby};

    // Shadow register map
    RegisterMap _map{{0}}; 
    DirtyRegisters<Register, reg_count> _dirty{};

    void flush();
    void flush_one(const Register reg);
    
    // Helper to send 18-bit word as 2x 9-bit
    void write_raw(const uint32_t data);
};

}  // namespace max2831

#endif /*__MAX2831_H__*/
