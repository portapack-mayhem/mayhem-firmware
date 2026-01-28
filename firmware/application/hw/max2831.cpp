#include "max2831.hpp"
#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
#include "ch.h"
#include "hal.h"
#include <algorithm>

using namespace hackrf::one;

namespace max2831 {

// CRITICAL: 18-bit Write Logic for MAX2831
// Protocol: 18 bits total = { R/W (1 bit), Address (4 bits), Data (13 bits) }
// R/W bit: 0 = Write, 1 = Read
void MAX2831::write(const address_t reg_num, const reg_t value) {
    // Construct 18-bit packet:
    // Bit 17:    0 (Write)
    // Bits 16-13: Address
    // Bits 12-0:  Data
    uint32_t packet = (0UL << 17) | ((reg_num & 0xF) << 13) | (value & 0x1FFF);
    write_raw(packet);
}

void MAX2831::write_raw(const uint32_t packet) {
    // Split 18-bit packet into two 9-bit words
    // We assume the SPI bus is configured for 9-bit DSS (Data Size Select)
    uint16_t transfers[2];
    
    // MSB first. Upper 9 bits [17:9]
    transfers[0] = (uint16_t)((packet >> 9) & 0x1FF);
    
    // LSB next. Lower 9 bits [8:0]
    transfers[1] = (uint16_t)(packet & 0x1FF);

    // Send via ChibiOS SPI driver
    _target.transfer(transfers, 2);
}

reg_t MAX2831::read(const address_t reg_num) {
    // MAX2831 Read is complex (requires MUXOUT pin configuration). 
    // For now, return cached value or 0 to prevent crashes.
    return _map.w[reg_num]; 
}

void MAX2831::init() {
    set_mode(Mode::Shutdown);

    // Initialize GPIOs
    gpio_max283x_enable.output();
    gpio_max2837_rxenable.output();
    gpio_max2837_txenable.output();

    // CRITICAL: Configure PLL for 40MHz Reference -> 20MHz Comparison
    // Register 5 (PLL_Config):
    // D2 = 1 (Reference Divider = Divide by 2)
    // D5 = 1 (Lock Detect Output Enable)
    uint16_t reg5 = 0;
    reg5 |= (1 << 2); // Ref Div / 2
    reg5 |= (1 << 5); // LD Enable
    
    // Update shadow map (Array index needs toUType)
    _map.w[toUType(Register::PLL_Config)] = reg5;
    
    // Mark Dirty using operator[] (Enum does NOT need toUType)
    _dirty[Register::PLL_Config] = 1;

    // FORCE WRITE NOW so the divider is active before we touch Reg 0
    write(toUType(Register::PLL_Config), reg5);
    _dirty[Register::PLL_Config] = 0; // Clear dirty bit

    // CRITICAL: Enable Fractional-N Mode
    // Register 0 (PLL_Mode):
    // D10 = 1 (Fractional-N Mode Enable)
    // D9:D0 = Recommended defaults (often 0x240 or similar, check datasheet)
    uint16_t reg0 = (1 << 10) | 0x240;
    _map.w[toUType(Register::PLL_Mode)] = reg0;  // FIX: Changed name to PLL_Mode
    _dirty[Register::PLL_Mode] = 1;              // FIX: Changed name to PLL_Mode

    // Configure Lock Detect Pin Drive
    // Register 1 (Lock_Detect_Config):
    // D12 = 1 (CMOS Output - Actively drives High/Low)
    uint16_t reg1 = (1 << 12);
    _map.w[toUType(Register::Lock_Detect_Config)] = reg1; // FIX: Changed name to Lock_Detect_Config
    _dirty[Register::Lock_Detect_Config] = 1;             // FIX: Changed name to Lock_Detect_Config

    // Apply all configurations
    flush();

    // Wake up
    set_mode(Mode::Standby);
}

void MAX2831::set_mode(const Mode mode) {
    _mode = mode;
    
    uint8_t en = 0, rx = 0, tx = 0;

    switch (mode) {
        case Mode::Standby:  en=1; rx=0; tx=0; break;
        case Mode::Receive:  en=1; rx=1; tx=0; break;
        case Mode::Transmit: en=1; rx=0; tx=1; break;
        default:             en=0; rx=0; tx=0; break; // Shutdown
    }

    gpio_max283x_enable.write(en);
    gpio_max2837_rxenable.write(rx);
    gpio_max2837_txenable.write(tx);
}

void MAX2831::flush() {
    if (_dirty) {
        for (size_t n = 0; n < reg_count; n++) {
            if (_dirty[n]) {
                write(n, _map.w[n]);
            }
        }
        _dirty.clear();
    }
}

void MAX2831::flush_one(const Register reg) {
    const auto reg_num = toUType(reg);
    write(reg_num, _map.w[reg_num]);
    _dirty[reg] = 0;
}

void MAX2831::set_tx_vga_gain(const int_fast8_t db) {
    // Clamp to valid range (0-63 dB approx)
    int_fast8_t safe_db = db;
    if (safe_db < 0) safe_db = 0;
    if (safe_db > 63) safe_db = 63;

    const auto reg_idx = toUType(Register::TX_VGA_Config);

    // Update the shadow map
    _map.w[reg_idx] &= ~0x003F;       // 0x003F (6 bits)
    _map.w[reg_idx] |= (safe_db & 0x003F);

    _dirty[Register::TX_VGA_Config] = 1;
    flush();
}

void MAX2831::set_lna_gain(const int_fast8_t db) {
    // Datasheet Table 26: Register 11 (A3:A0 = 1011)
    // Bits D6:D5 control LNA Gain.
    // 11 = High, 10 = Med, 0X = Low

    uint16_t lna_bits = 0;

    if (db > 20)      lna_bits = 0b11; // High
    else if (db > 0)  lna_bits = 0b10; // Medium
    else              lna_bits = 0b00; // Low

    // 1. Read current value from shadow map
    uint16_t reg_val = _map.w[toUType(Register::RX_Gain_Config)];

    // 2. Clear bits 6 and 5 (Mask 0x0060)
    reg_val &= ~0x0060;

    // 3. Set new bits
    reg_val |= (lna_bits << 5);

    // 4. Update map and flush
    _map.w[toUType(Register::RX_Gain_Config)] = reg_val;
    _dirty[Register::RX_Gain_Config] = 1;
    flush();
}

void MAX2831::set_vga_gain(const int_fast8_t db) {
    // Register 11 (RX_Gain_Config): Bits D4:D0 control VGA Gain.
    // Range: 0dB to 62dB in 2dB steps.

    // 1. Clamp and Map dB to 5-bit value (0-31)
    int_fast8_t safe_db = db;
    if (safe_db < 0) safe_db = 0;
    if (safe_db > 62) safe_db = 62;

    // Convert dB to hardware code (2dB per LSB)
    uint16_t vga_code = safe_db / 2;

    // 2. Read current value (Preserves LNA settings in D6:D5)
    uint16_t reg_val = _map.w[toUType(Register::RX_Gain_Config)];

    // 3. Clear VGA bits (D4:D0) - Mask 0x001F
    reg_val &= ~0x001F;

    // 4. Set new VGA bits
    reg_val |= (vga_code & 0x001F);

    // 5. Update map and flush
    _map.w[toUType(Register::RX_Gain_Config)] = reg_val;
    _dirty[Register::RX_Gain_Config] = 1;
    flush();
}

void MAX2831::set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) {
    // Register 8 [1:0] controls Coarse LPF for BOTH Rx and Tx.
    uint16_t coarse_bits = 0;
    if (bandwidth_minimum <= 7500000)       coarse_bits = 0b00;
    else if (bandwidth_minimum <= 8500000)  coarse_bits = 0b01;
    else if (bandwidth_minimum <= 15000000) coarse_bits = 0b10;
    else                                    coarse_bits = 0b11;

    // 1. Update Register 8 (Config_Control)
    _map.w[toUType(Register::Config_Control)] &= ~0x0003; 
    _map.w[toUType(Register::Config_Control)] |= coarse_bits;
    _dirty[Register::Config_Control] = 1;

    // 2. Update Register 7 (Filter_Fine_Tune) - Rx Fine Tuning (D2:D0)
    // We set this to Nominal (100%) which is binary 010.
    _map.w[toUType(Register::Filter_Fine_Tune)] &= ~0x0007; 
    _map.w[toUType(Register::Filter_Fine_Tune)] |= 0b010;   

    _dirty[Register::Filter_Fine_Tune] = 1;
    flush();
}

void MAX2831::set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) {
    uint16_t coarse_bits = 0;
    if (bandwidth_minimum <= 8000000)       coarse_bits = 0b00;
    else if (bandwidth_minimum <= 11000000) coarse_bits = 0b01;
    else if (bandwidth_minimum <= 16500000) coarse_bits = 0b10;
    else                                    coarse_bits = 0b11;

    // 1. Update Register 8 (Config_Control)
    _map.w[toUType(Register::Config_Control)] &= ~0x0003; 
    _map.w[toUType(Register::Config_Control)] |= coarse_bits;
    _dirty[Register::Config_Control] = 1;

    // 2. Update Register 7 (Filter_Fine_Tune) - Tx Fine Tuning (D5:D3)
    _map.w[toUType(Register::Filter_Fine_Tune)] &= ~0x0038; 
    _map.w[toUType(Register::Filter_Fine_Tune)] |= (0b010 << 3); 
    
    _dirty[Register::Filter_Fine_Tune] = 1;
    flush();
}

bool MAX2831::set_frequency(const rf::Frequency lo_frequency) {
    // 1. Calculate divider values
    const uint64_t ref_freq = 40000000; // 40MHz Reference
    const uint64_t modulus = 1048576;   // 2^20
    const uint64_t comp_freq = 20000000; // 20MHz Comparison

    uint32_t integer_part = lo_frequency / comp_freq;
    uint64_t remainder = lo_frequency % comp_freq;
    uint32_t fractional_part = (remainder * modulus) / comp_freq;

    // 2. Prepare Register 3: Integer (8 bits) + Lower 6 bits of Fractional
    uint16_t reg3_val = (integer_part & 0xFF) | ((fractional_part & 0x3F) << 8);

    // 3. Prepare Register 4: Upper 14 bits of Fractional
    uint16_t reg4_val = (fractional_part >> 6) & 0x3FFF;

    // 4. Update Shadow Map
    _map.w[toUType(Register::Synthesizer_Integer)] = reg3_val;
    _map.w[toUType(Register::Synthesizer_FracMSB)] = reg4_val;

    // 5. CRITICAL: Manual Write Order
    // Write Reg 4 (Frac MSB) FIRST so the MSBs are buffered in the chip.
    write(toUType(Register::Synthesizer_FracMSB), reg4_val);

    // Write Reg 3 (Integer + Frac LSB) SECOND.
    // Writing Register 3 triggers the synthesizer latch to apply ALL values.
    write(toUType(Register::Synthesizer_Integer), reg3_val);

    // Clear dirty bits
    _dirty[Register::Synthesizer_Integer] = 0;
    _dirty[Register::Synthesizer_FracMSB] = 0;

    flush();
    return true;
}

// Stubs for others
void MAX2831::set_rx_LO_iq_phase_calibration(const size_t v) { (void)v; }
void MAX2831::set_tx_LO_iq_phase_calibration(const size_t v) { (void)v; }
void MAX2831::set_rx_buff_vcm(const size_t v) { (void)v; }
int8_t MAX2831::temp_sense() { return 0; }

} // namespace max2831
