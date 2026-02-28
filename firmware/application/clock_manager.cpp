/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "clock_manager.hpp"

#include "portapack_persistent_memory.hpp"
#include "portapack_io.hpp"
#include "portapack.hpp"

#include "hackrf_hal.hpp"
using namespace hackrf::one;

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#ifdef PRALINE
extern "C" {
#include "fpga_bridge.h"
}
// Need access to ssp1_arbiter from radio namespace
#include "radio.hpp"
#endif

constexpr uint32_t si5351_vco_f = 800000000;

constexpr si5351::Inputs si5351_inputs{
    .f_xtal = si5351_xtal_f,
    .f_clkin = si5351_clkin_f,
    .clkin_div = 1,
};

static_assert(si5351_inputs.f_xtal == si5351_xtal_f, "XTAL output frequency wrong");
static_assert(si5351_inputs.f_clkin_out() == si5351_clkin_f, "CLKIN output frequency wrong");

constexpr si5351::PLLInputSource::Type si5351c_pll_input_sources{
    si5351::PLLInputSource::PLLA_Source_XTAL | si5351::PLLInputSource::PLLB_Source_CLKIN | si5351::PLLInputSource::CLKIN_Div1};

constexpr si5351::PLLInputSource::Type si5351a_pll_input_sources{
    si5351::PLLInputSource::PLLA_Source_XTAL | si5351::PLLInputSource::PLLB_Source_XTAL | si5351::PLLInputSource::CLKIN_Div1};

constexpr si5351::PLL si5351_pll_xtal_25m{
    .f_in = si5351_inputs.f_xtal,
    .a = 32,
    .b = 0,
    .c = 1,
};
constexpr auto si5351_pll_a_xtal_reg = si5351_pll_xtal_25m.reg(0);
#ifdef PRALINE
static_assert(si5351_pll_xtal_25m.f_vco() == si5351_vco_f, "PLL XTAL frequency wrong");
static_assert(si5351_pll_xtal_25m.p1() == 3584, "PLL XTAL P1 wrong");
static_assert(si5351_pll_xtal_25m.p2() == 0, "PLL XTAL P2 wrong");
static_assert(si5351_pll_xtal_25m.p3() == 1, "PLL XTAL P3 wrong");
#endif

constexpr si5351::PLL si5351_pll_clkin_10m{
    .f_in = si5351_inputs.f_clkin_out(),
    .a = 80,
    .b = 0,
    .c = 1,
};
constexpr auto si5351c_pll_b_clkin_reg = si5351_pll_clkin_10m.reg(1);
constexpr auto si5351a_pll_a_clkin_reg = si5351_pll_clkin_10m.reg(0);

#ifndef PRALINE
static_assert(si5351_pll_xtal_25m.f_vco() == si5351_vco_f, "PLL XTAL frequency wrong");
static_assert(si5351_pll_xtal_25m.p1() == 3584, "PLL XTAL P1 wrong");
static_assert(si5351_pll_xtal_25m.p2() == 0, "PLL XTAL P2 wrong");
static_assert(si5351_pll_xtal_25m.p3() == 1, "PLL XTAL P3 wrong");
#endif

static_assert(si5351_pll_clkin_10m.f_vco() == si5351_vco_f, "PLL CLKIN frequency wrong");
static_assert(si5351_pll_clkin_10m.p1() == 9728, "PLL CLKIN P1 wrong");
static_assert(si5351_pll_clkin_10m.p2() == 0, "PLL CLKIN P2 wrong");
static_assert(si5351_pll_clkin_10m.p3() == 1, "PLL CLKIN P3 wrong");
/*
constexpr si5351::MultisynthFractional si5351_ms_18m432 {
        .f_src = si5351_vco_f,
        .a = 43,
        .b = 29,
        .c = 72,
        .r_div = 1,
};
*/
/*
constexpr si5351::MultisynthFractional si5351_ms_0_20m {
        .f_src = si5351_vco_f,
        .a = 20,
        .b = 0,
        .c = 1,
        .r_div = 1,
};
constexpr auto si5351_ms_0_20m_reg = si5351_ms_0_20m.reg(0);
*/

constexpr si5351::MultisynthFractional si5351_ms_0_4m{
    .f_src = si5351_vco_f,  // 800,000,000 Hz
    .a = 100,               // Integer divider 100
    .b = 0,
    .c = 1,
    .r_div = 1  // Final R-divider: 2^1 = 2
};

constexpr si5351::MultisynthFractional si5351_ms_0_8m{
    .f_src = si5351_vco_f,
    .a = 50,
    .b = 0,
    .c = 1,
    .r_div = 1,
};
constexpr auto si5351c_ms_0_8m_reg = si5351_ms_0_8m.reg(clock_generator_output_og_codec);

#ifdef PRALINE
// Verify compile-time values for 8 MHz config
static_assert(si5351_ms_0_8m.p1() == 5888, "MS0 8MHz P1 should be 5888 (0x1700)");
static_assert(si5351_ms_0_8m.p2() == 0, "MS0 8MHz P2 should be 0");
static_assert(si5351_ms_0_8m.p3() == 1, "MS0 8MHz P3 should be 1");
static_assert(si5351_ms_0_8m.f_out() == 8000000, "MS0 should output 8 MHz");

// Verify register array encoding
static_assert(si5351c_ms_0_8m_reg[0] == 42, "MS0 base register should be 42");
static_assert(si5351c_ms_0_8m_reg[1] == 0x00, "MS0 reg43 P3[15:8] should be 0x00");
static_assert(si5351c_ms_0_8m_reg[2] == 0x01, "MS0 reg44 P3[7:0] should be 0x01");
static_assert(si5351c_ms_0_8m_reg[3] == 0x10, "MS0 reg45 R_DIV should be 0x10");
static_assert(si5351c_ms_0_8m_reg[4] == 0x17, "MS0 reg46 P1[15:8] should be 0x17");
static_assert(si5351c_ms_0_8m_reg[5] == 0x00, "MS0 reg47 P1[7:0] should be 0x00");
#endif

constexpr si5351::MultisynthFractional si5351_ms_group{
    .f_src = si5351_vco_f,
    .a = 80, /* Don't care */
    .b = 0,
    .c = 1,
    .r_div = 0,
};
constexpr auto si5351c_ms_1_group_reg = si5351_ms_group.reg(clock_generator_output_og_cpld);
constexpr auto si5351c_ms_2_group_reg = si5351_ms_group.reg(clock_generator_output_og_sgpio);

constexpr si5351::MultisynthFractional si5351_ms_16m{
    .f_src = si5351_vco_f,
    .a = 50,
    .b = 0,
    .c = 1,
    .r_div = 0,
};
constexpr auto si5351a_ms_1_sgpio_16m_reg = si5351_ms_16m.reg(clock_generator_output_r9_sgpio);

constexpr si5351::MultisynthFractional si5351_ms_10m{
    .f_src = si5351_vco_f,
    .a = 80,
    .b = 0,
    .c = 1,
    .r_div = 0,
};
constexpr auto si5351c_ms_3_10m_reg = si5351_ms_10m.reg(3);
constexpr auto si5351a_ms_2_mcu_10m_reg = si5351_ms_10m.reg(clock_generator_output_r9_mcu_clkin);

constexpr si5351::MultisynthFractional si5351_ms_40m{
    .f_src = si5351_vco_f,
    .a = 20,
    .b = 0,
    .c = 1,
    .r_div = 0,
};

constexpr auto si5351_ms_rffc5072 = si5351_ms_40m;
constexpr auto si5351_ms_max283x = si5351_ms_40m;

constexpr auto si5351c_ms_4_reg = si5351_ms_rffc5072.reg(clock_generator_output_og_first_if);
constexpr auto si5351c_ms_5_reg = si5351_ms_max283x.reg(clock_generator_output_og_second_if);
constexpr auto si5351a_ms_0_if_40m_reg = si5351_ms_40m.reg(clock_generator_output_r9_if);

static_assert(si5351_ms_10m.f_out() == 10000000, "MS 10MHz f_out wrong");
static_assert(si5351_ms_10m.p1() == 9728, "MS 10MHz p1 wrong");
static_assert(si5351_ms_10m.p2() == 0, "MS 10MHz p2 wrong");
static_assert(si5351_ms_10m.p3() == 1, "MS 10MHz p3 wrong");

static_assert(si5351_ms_rffc5072.f_out() == rffc5072_reference_f, "RFFC5072 reference f_out wrong");

static_assert(si5351_ms_max283x.f_out() == max283x_reference_f, "MAX283x reference f_out wrong");

constexpr si5351::MultisynthInteger si5351_ms_int_off{
    .f_src = si5351_vco_f,
    .a = 255,
    .r_div = 0,
};

constexpr si5351::MultisynthInteger si5351_ms_int_40m{
    .f_src = si5351_vco_f,
    .a = 20,
    .r_div = 0,
};

constexpr si5351::MultisynthInteger si5351_ms_int_10m{
    .f_src = si5351_vco_f,
    .a = 80,
    .r_div = 0,
};

constexpr auto si5351c_ms_int_mcu_clkin = si5351_ms_int_40m;
constexpr auto si5351a_ms_int_mcu_clkin = si5351_ms_int_10m;

constexpr auto si5351c_ms6_7_off_mcu_clkin_reg = si5351::ms6_7_reg(si5351_ms_int_off, si5351c_ms_int_mcu_clkin);
constexpr auto si5351a_ms6_7_off_reg = si5351::ms6_7_reg(si5351_ms_int_off, si5351_ms_int_off);

static_assert(si5351_ms_int_off.f_out() == 3137254, "MS int off f_out wrong");
static_assert(si5351_ms_int_off.p1() == 255, "MS int off P1 wrong");

static_assert(si5351c_ms_int_mcu_clkin.f_out() == mcu_clkin_og_f, "MS int MCU CLKIN OG f_out wrong");
static_assert(si5351a_ms_int_mcu_clkin.f_out() == mcu_clkin_r9_f, "MS int MCU CLKIN r9 f_out wrong");

using namespace si5351;

static constexpr ClockControl::MultiSynthSource get_si5351c_reference_clock_generator_pll(const ClockManager::ReferenceSource reference_source) {
    return (reference_source == ClockManager::ReferenceSource::Xtal)
               ? ClockControl::MultiSynthSource::PLLA
               : ClockControl::MultiSynthSource::PLLB;
}

constexpr ClockControls si5351c_clock_control_common{{
    {ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Fractional, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Group, ClockControl::ClockInvert::Invert, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Group, ClockControl::ClockInvert::Normal, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_6mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Invert, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_4mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Fractional, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, get_si5351c_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
}};

constexpr ClockControls si5351a_clock_control_common{{
#ifdef PRALINE
    // CLK0: MAX5864 (ADC)
    {ClockControl::ClockCurrentDrive::_4mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK1: SCT_CLK - iCE40 FPGA timing clock
    {ClockControl::ClockCurrentDrive::_6mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK2: LPC43xx MCU
    {ClockControl::ClockCurrentDrive::_4mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK3: CLKOUT (optional) SMA Port P1
    {ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK4: PRALINE MAX2831 reference (40 MHz) - INVERTED per hackrf_usb, 4mA, Integer mode
    {ClockControl::ClockCurrentDrive::_4mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Invert, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK5: PRALINE RFFC5072 reference (40 MHz) - INVERTED, 6mA, Integer mode
    // This matches HackRF One OG configuration for RFFC5072
    {ClockControl::ClockCurrentDrive::_6mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Invert, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK6: SMA Port P2
    {ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
#else
    {ClockControl::ClockCurrentDrive::_6mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_4mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Fractional, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK4: HackRF r9 - not inverted
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK5: HackRF r9 - not used
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
    // CLK6: Not used
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},
#endif

    // CLK7: Not used
    {ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self, ClockControl::ClockInvert::Normal, ClockControl::MultiSynthSource::PLLA, ClockControl::MultiSynthMode::Integer, ClockControl::ClockPowerDown::Power_Off},

}};

ClockManager::Reference ClockManager::get_reference() const {
    return reference;
}

std::string ClockManager::get_source() {
    std::string source_name("---");
    switch (reference.source) {
        case ClockManager::ReferenceSource::Xtal:
            source_name = "HackRF";
            break;
        case ClockManager::ReferenceSource::PortaPack:
            source_name = "PortaPack";
            break;
        case ClockManager::ReferenceSource::External:
            source_name = "External";
            break;
    }
    return source_name;
}

std::string ClockManager::get_freq() {
    return to_string_dec_uint(reference.frequency / 1000000, 2) + "." +
           to_string_dec_uint((reference.frequency % 1000000) / 100, 4, '0') + " MHz";
}

static void portapack_tcxo_enable() {
    portapack::io.reference_oscillator(true);

    /* Delay >10ms at 96MHz clock speed for reference oscillator to start. */
    /* Delay an additional 1ms (arbitrary) for the clock generator to detect a signal. */
    volatile uint32_t delay = 240000 + 24000;
    while (delay--);
}

static void portapack_tcxo_disable() {
    portapack::io.reference_oscillator(false);
}

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

void ClockManager::init_clock_generator() {
#ifdef PRALINE
    // PRALINE: Configure clock input mux GPIO
    // GPIO0_15 (clkin_ctrl) selects GP_CLKIN source:
    //   0 = P1 connector (external)
    //   1 = P22 (internal Si5351 CLK2)
    constexpr GPIO gpio_clkin_ctrl = gpio[GPIO0_15];
    gpio_clkin_ctrl.output();
    gpio_clkin_ctrl.write(1);  // CLKIN_SIGNAL_P22 = 1 = internal Si5351 CLK2

    // Also enable MCU clock gate (GPIO0_8)
    gpio_r9_mcu_clk_en.output();
    gpio_r9_mcu_clk_en.write(1);
#else
    // HackRF One r9: GPIO0_8 (mcu_clk_en) gates Si5351 CLK2/CLK7 to GP_CLKIN
    if (hackrf_r9) {
        gpio_r9_mcu_clk_en.output();
        gpio_r9_mcu_clk_en.write(1);
    }
#endif

    clock_generator.reset();
    clock_generator.set_crystal_internal_load_capacitance(CrystalInternalLoadCapacitance::XTAL_CL_8pF);
    clock_generator.enable_fanout();

#ifdef PRALINE
    /* PRALINE has Si5351A (NOT Si5351C like HackRF One OG).
     * Must use Si5351A configuration: PLLA only, no CLKIN support.
     *
     * IMPORTANT: Follow HackRF reference sequence:
     * 1. Set PLL input sources
     * 2. Configure PLL and multisynths
     * 3. Set clock control registers (AFTER multisynths!)
     * 4. Reset PLLs
     * 5. Enable outputs
     */
    clock_generator.set_pll_input_sources(si5351a_pll_input_sources);

    /* Skip MCU CLKIN setup and reference detection for PRALINE - not applicable */
    reference = Reference{ReferenceSource::Xtal, 0};

    /* Clock control will be set AFTER multisynth configuration - see below */
#else
    clock_generator.set_pll_input_sources(hackrf_r9
                                              ? si5351a_pll_input_sources
                                              : si5351c_pll_input_sources);

    auto si5351_clock_control_common = hackrf_r9
                                           ? si5351a_clock_control_common
                                           : si5351c_clock_control_common;

    auto clock_generator_output_mcu_clkin = hackrf_r9
                                                ? clock_generator_output_r9_mcu_clkin
                                                : clock_generator_output_og_mcu_clkin;

    clock_generator.set_clock_control(
        clock_generator_output_mcu_clkin,
        si5351_clock_control_common[clock_generator_output_mcu_clkin]
            .clk_src(hackrf_r9
                         ? ClockControl::ClockSource::Xtal
                         : ClockControl::ClockSource::CLKIN)
            .clk_pdn(ClockControl::ClockPowerDown::Power_On));
    clock_generator.enable_output(clock_generator_output_mcu_clkin);

    reference = choose_reference();

    clock_generator.disable_output(clock_generator_output_mcu_clkin);

    const auto ref_pll = hackrf_r9
                             ? ClockControl::MultiSynthSource::PLLA
                             : get_si5351c_reference_clock_generator_pll(reference.source);

    const ClockControls si5351_clock_control = ClockControls{{
        si5351_clock_control_common[0].ms_src(ref_pll),
        si5351_clock_control_common[1].ms_src(ref_pll),
        si5351_clock_control_common[2].ms_src(ref_pll),
        si5351_clock_control_common[3].ms_src(ref_pll),
        si5351_clock_control_common[4].ms_src(ref_pll),
        si5351_clock_control_common[5].ms_src(ref_pll),
        si5351_clock_control_common[6].ms_src(ref_pll),
        si5351_clock_control_common[7].ms_src(ref_pll),
    }};
    clock_generator.set_clock_control(si5351_clock_control);
#endif

#ifdef PRALINE

    /* * Praline HackRF Pro Clock Assignments (800 MHz VCO Configuration)
     * VCO Frequency: 800,000,000 Hz (Master Reference)
     * * CLK0: AFE_CLK (MAX5864 Codec & FPGA ADC Interface)
     *  - Initialized to: 4,000,000 Hz (si5351_ms_4m)
     *  - Divider: 100 (Integer), R_DIV: 2 (r_div=1)
     *  - Note: Defines hardware sample rate. Essential for WFM purity.
     * * CLK1: SCT_CLK (iCE40 FPGA System/Timing Clock)
     *  - Initialized to: 10,000,000 Hz (si5351_ms_10m)
     *  - Divider: 80 (Integer), R_DIV: 1 (r_div=0)
     *  - Note: Timing for SGPIO data bus; scales to 2x SR in wideband modes.
     * * CLK2: MCU_CLKIN (LPC43xx MCU External Clock Input)
     *  - Initialized to: 10,000,000 Hz (si5351_ms_10m)
     *  - Divider: 80 (Integer), R_DIV: 1 (r_div=0)
     *  - Note: Synchronizes MCU processing to the RF clock tree.
     * * CLK3: SG_CLK (Switching Regulator/Internal Logic Sync)
     *  - Initialized to: 10,000,000 Hz (si5351_ms_10m)
     *  - Divider: 80 (Integer), R_DIV: 1 (r_div=0)
     *  - Note: Used for internal FPGA logic/gateware synchronization.
     * * CLK4: P_CLK (Peripheral/Expansion Clock)
     *  - Configured via: si5351c_ms_4_reg
     *  - Note: Routed to expansion headers for external hardware sync.
     * * CLK5: AUX_CLK (Auxiliary reference for secondary logic)
     *  - Configured via: si5351c_ms_5_reg
     *  - Note: Provides additional timing flexibility for the iCE40 FPGA.
     * * CLK6/7: Unused / Power-Down
     *  - State: Disabled (si5351a_ms6_7_off_reg)
     *  - Note: Kept OFF to reduce EMI/RFI near the RF front-end.
     * * CLKOUT: Optional external clock output on the header.
     */

    /* Step 1: Write PLL A configuration (800 MHz VCO from 25 MHz XTAL) */
    /* Use single-byte writes to debug I2C issues */
    {
        const auto& pll_regs = si5351_pll_a_xtal_reg;
        const uint8_t base_reg = pll_regs[0];
        for (size_t i = 1; i < pll_regs.size(); i++) {
            clock_generator.write_register(base_reg + i - 1, pll_regs[i]);
        }
    }

    /* Step 2: Write multisynth configurations using single-byte writes */
    // These cover all active channels on the Praline board
    clock_generator.write_ms_single_byte(0, si5351_ms_0_4m);  // CLK0: Codec (4 MHz)
    clock_generator.write_ms_single_byte(1, si5351_ms_10m);   // CLK1: FPGA Timing (10 MHz)
    clock_generator.write_ms_single_byte(2, si5351_ms_10m);   // CLK2: MCU Input (10 MHz)
    clock_generator.write_ms_single_byte(3, si5351_ms_10m);   // CLK3: Logic Sync (10 MHz)
    clock_generator.write_ms_single_byte(4, si5351_ms_40m);   // CLK4: First IF (40 MHz)
    clock_generator.write_ms_single_byte(5, si5351_ms_40m);   // CLK5: Second IF (40 MHz)

    /* Step 3: NOW set clock control registers (AFTER multisynths per HackRF reference) */
    const auto ref_pll = ClockControl::MultiSynthSource::PLLA;
    const ClockControls si5351_clock_control = ClockControls{{
        si5351a_clock_control_common[0].ms_src(ref_pll),
        si5351a_clock_control_common[1].ms_src(ref_pll),
        si5351a_clock_control_common[2].ms_src(ref_pll),
        si5351a_clock_control_common[3].ms_src(ref_pll),
        si5351a_clock_control_common[4].ms_src(ref_pll),
        si5351a_clock_control_common[5].ms_src(ref_pll),
        si5351a_clock_control_common[6].ms_src(ref_pll),
        si5351a_clock_control_common[7].ms_src(ref_pll),
    }};
    clock_generator.set_clock_control(si5351_clock_control);
#else
    if (hackrf_r9) {
        const PLLReg pll_reg = (reference.source == ReferenceSource::Xtal)
                                   ? si5351_pll_a_xtal_reg
                                   : si5351a_pll_a_clkin_reg;
        clock_generator.write(pll_reg);
        clock_generator.write(si5351a_ms_0_if_40m_reg);
        clock_generator.write(si5351a_ms_1_sgpio_16m_reg);
        clock_generator.write(si5351a_ms_2_mcu_10m_reg);
        clock_generator.write(si5351a_ms6_7_off_reg);
    } else {
        clock_generator.write(si5351_pll_a_xtal_reg);
        clock_generator.write(si5351c_pll_b_clkin_reg);
        clock_generator.write(si5351c_ms_0_8m_reg);
        clock_generator.write(si5351c_ms_1_group_reg);
        clock_generator.write(si5351c_ms_2_group_reg);
        clock_generator.write(si5351c_ms_3_10m_reg);
        clock_generator.write(si5351c_ms_4_reg);
        clock_generator.write(si5351c_ms_5_reg);
        clock_generator.write(si5351c_ms6_7_off_mcu_clkin_reg);
    }
#endif

    clock_generator.reset_plls();

    // Wait for PLL(s) to lock.
#ifdef PRALINE
    // PRALINE: Wait for PLLA to lock (0x20 = LOL_A bit)
    uint8_t device_status_mask = 0x20;
    uint32_t pll_timeout = 100000;
    while ((clock_generator.device_status() & device_status_mask) != 0 && pll_timeout > 0) {
        pll_timeout--;
    }
    // Store PLL lock status for debugging
    static volatile uint32_t pll_lock_timeout = pll_timeout;
    (void)pll_lock_timeout;

    // CRITICAL: Add delay to ensure Si5351 writes complete before I2C bus stops
    chThdSleepMilliseconds(100);
#else
    // Wait for PLL(s) to lock - with timeout to prevent hang
    uint8_t device_status_mask = hackrf_r9
                                     ? 0x20
                                 : (ref_pll == ClockControl::MultiSynthSource::PLLB)
                                     ? 0x40
                                     : 0x20;

    while ((clock_generator.device_status() & device_status_mask) != 0);

    clock_generator.set_clock_control(
        clock_generator_output_mcu_clkin,
        si5351_clock_control_common[clock_generator_output_mcu_clkin].ms_src(ref_pll).clk_pdn(ClockControl::ClockPowerDown::Power_On));
    clock_generator.enable_output(clock_generator_output_mcu_clkin);
#endif
}

uint32_t ClockManager::measure_gp_clkin_frequency() {
    // Measure Si5351B CLKIN frequency against LPC43xx IRC oscillator
    start_frequency_monitor_measurement(cgu::CLK_SEL::GP_CLKIN);
    wait_For_frequency_monitor_measurement_done();
    return get_frequency_monitor_measurement_in_hertz();
}

bool ClockManager::loss_of_signal() {
    if (hackrf_r9) {
        const auto frequency = measure_gp_clkin_frequency();
        return (frequency < 9850000) || (frequency > 10150000);
    } else {
        return clock_generator.clkin_loss_of_signal();
    }
}

ClockManager::ReferenceSource ClockManager::detect_reference_source() {
    if (portapack::persistent_memory::config_disable_external_tcxo())
        return ReferenceSource::Xtal;

    if (loss_of_signal()) {
        // No external reference. Turn on PortaPack reference (if present).
        portapack_tcxo_enable();

        if (loss_of_signal()) {
            // No PortaPack reference was detected. Choose the HackRF crystal as the reference.
            return ReferenceSource::Xtal;
        } else {
            return ReferenceSource::PortaPack;
        }
    } else {
        return ReferenceSource::External;
    }
}

ClockManager::Reference ClockManager::choose_reference() {
    if (hackrf_r9) {
        gpio_r9_clkin_en.write(1);
        volatile uint32_t delay = 240000 + 24000;
        while (delay--);
    }
    const auto detected_reference = detect_reference_source();

    if ((detected_reference == ReferenceSource::External) ||
        (detected_reference == ReferenceSource::PortaPack)) {
        const auto frequency = measure_gp_clkin_frequency();
        if ((frequency >= 9850000) && (frequency <= 10150000)) {
            return {detected_reference, 10000000};
        }
    }

    if (hackrf_r9) {
        gpio_r9_clkin_en.write(0);
    }

    portapack_tcxo_disable();
    return {ReferenceSource::Xtal, 25000000};
}

void ClockManager::shutdown() {
    clock_generator.reset();
}

void ClockManager::enable_codec_clocks() {
#ifdef PRALINE
    /* PRALINE: CLK0 (AFE_CLK) for codec/FPGA, CLK1 (SCT_CLK) for FPGA timing.
     * Reference hackrf_core.c shows PRALINE needs both CLK0 and CLK1. */
    clock_generator.enable_clock(clock_generator_output_og_codec); /* CLK0 MAX5864*/
    clock_generator.enable_clock(clock_generator_output_og_cpld);  /* CLK1 iCE40 FPGA*/
    clock_generator.enable_clock(clock_generator_output_og_sgpio); /* CLK2 LPC43xx*/
    clock_generator.enable_output_mask(
        (1U << clock_generator_output_og_codec) | (1U << clock_generator_output_og_cpld) | (1U << clock_generator_output_og_sgpio));
#else
    if (hackrf_r9) {
        clock_generator.enable_clock(clock_generator_output_r9_sgpio);
    } else {
        clock_generator.enable_clock(clock_generator_output_og_codec);
        clock_generator.enable_clock(clock_generator_output_og_cpld);
        clock_generator.enable_clock(clock_generator_output_og_sgpio);
    }
    /* Turn on all outputs at the same time. This probably doesn't ensure
     * their phase relationships. For example, clocks that output frequencies
     * in a 2:1 relationship may start with the slower clock high or low?
     */
    if (hackrf_r9) {
        clock_generator.enable_output_mask(1U << clock_generator_output_r9_sgpio);
    } else {
        clock_generator.enable_output_mask(
            (1U << clock_generator_output_og_codec) | (1U << clock_generator_output_og_cpld) | (1U << clock_generator_output_og_sgpio));
    }
#endif
}

void ClockManager::disable_codec_clocks() {
    /* Turn off outputs before disabling clocks. It seems the clock needs to
     * be enabled for the output to come to rest at the state specified by
     * CLKx_DISABLE_STATE.
     */
#ifdef PRALINE
    /* PRALINE: CLK0 (AFE_CLK), CLK1 (SCT_CLK), and CLK2 MCU used for codec/FPGA */
    clock_generator.disable_output_mask(
        (1U << clock_generator_output_og_codec) | (1U << clock_generator_output_og_cpld) | (1U << clock_generator_output_og_sgpio));
    clock_generator.disable_clock(clock_generator_output_og_codec);
    clock_generator.disable_clock(clock_generator_output_og_cpld);
    clock_generator.disable_clock(clock_generator_output_og_sgpio);
#else
    if (hackrf_r9) {
        clock_generator.disable_output_mask(1U << clock_generator_output_r9_sgpio);
        clock_generator.disable_clock(clock_generator_output_r9_sgpio);
    } else {
        clock_generator.disable_output_mask(
            (1U << clock_generator_output_og_codec) | (1U << clock_generator_output_og_cpld) | (1U << clock_generator_output_og_sgpio));
        clock_generator.disable_clock(clock_generator_output_og_codec);
        clock_generator.disable_clock(clock_generator_output_og_cpld);
        clock_generator.disable_clock(clock_generator_output_og_sgpio);
    }
#endif
}

void ClockManager::enable_if_clocks() {
#ifdef PRALINE
    /* PRALINE uses CLK5 (first IF) and CLK4 (second IF) */
    clock_generator.enable_clock(clock_generator_output_og_first_if);
    clock_generator.enable_output_mask(1U << clock_generator_output_og_first_if);
    clock_generator.enable_clock(clock_generator_output_og_second_if);
    clock_generator.enable_output_mask(1U << clock_generator_output_og_second_if);
#else
    if (hackrf_r9) {
        clock_generator.enable_clock(clock_generator_output_r9_if);
        clock_generator.enable_output_mask(1U << clock_generator_output_r9_if);
    } else {
        clock_generator.enable_clock(clock_generator_output_og_first_if);
        clock_generator.enable_output_mask(1U << clock_generator_output_og_first_if);
        clock_generator.enable_clock(clock_generator_output_og_second_if);
        clock_generator.enable_output_mask(1U << clock_generator_output_og_second_if);
    }
#endif
}

void ClockManager::disable_if_clocks() {
#ifdef PRALINE
    clock_generator.disable_output_mask(1U << clock_generator_output_og_first_if);
    clock_generator.disable_clock(clock_generator_output_og_first_if);
    clock_generator.disable_output_mask(1U << clock_generator_output_og_second_if);
    clock_generator.disable_clock(clock_generator_output_og_second_if);
#else
    if (hackrf_r9) {
        clock_generator.disable_output_mask(1U << clock_generator_output_r9_if);
        clock_generator.disable_clock(clock_generator_output_r9_if);
    } else {
        clock_generator.disable_output_mask(1U << clock_generator_output_og_first_if);
        clock_generator.disable_clock(clock_generator_output_og_first_if);
        clock_generator.disable_output_mask(1U << clock_generator_output_og_second_if);
        clock_generator.disable_clock(clock_generator_output_og_second_if);
    }
#endif
}

void ClockManager::set_sampling_frequency(const uint32_t frequency) {
#ifdef PRALINE
    /* PRALINE: CLK0=AFE_CLK runs at sample rate (VCO/divider/2)
     *          CLK1=SCT_CLK runs at 2x sample rate (VCO/divider/1)
     * Reference: hackrf_core.c sample_rate_frac_set() lines 580-582
     */

    /* PRALINE: Match HackRF USB  sample_rate_frac_set()
     * Reference: hackrf_usb radio.c lines 29-91, hackrf_core.c lines 501-685 */

    _base_band_frequency = frequency;  // Store frequency for StatusViews

    /*
     * PRALINE sample rate strategy from GSG hackrf_usb radio.c:
     *
     * 1. Run ADC at the highest rate possible (up to 40 MHz)
     * 2. Use FPGA decimation to achieve desired output rate
     * 3. This makes the analog LPF effective at rejecting aliases
     * 4. Re-apply frequency after to reconfigure LPF bandwidth
     */

    // 20 MHz, since GSG reference of 40MHz caused shifts at certain values.
    constexpr uint32_t MAX_AFE_RATE = 20000000;
    constexpr uint8_t MAX_N = 5;  // Max decimation = 2^5 = 32

    // Calculate optimal decimation factor for RX
    // Start with n=1 (minimum decimation of 2) per reference
    uint8_t n = 1;
    uint32_t afe_rate_x2 = 2 * frequency;

    while ((afe_rate_x2 <= MAX_AFE_RATE) && (n < MAX_N)) {
        afe_rate_x2 <<= 1;
        n++;
    }

    // Store decimation factor for potential use elsewhere
    _resampling_n = n;

    // The actual AFE rate = frequency * 2^n
    uint32_t afe_rate = frequency << n;

    // Set FPGA RX decimation register
    fpga_debug_register_write(2, n);
    radio::invalidate_spi_config();

    // Configure Si5351 clocks
    clock_generator.set_ms_frequency(0, afe_rate * 2, si5351_vco_f, 1);  // CLK0: AFE_CLK
    clock_generator.set_ms_frequency(1, afe_rate * 2, si5351_vco_f, 0);  // CLK1: SCT_CLK

#else
    /* Codec clock is at sampling frequency, CPLD and SGPIO clocks are at
     * twice the frequency, and derived from the MS0 synth. So it's only
     * necessary to change the MS0 synth frequency, and ensure the output
     * is divided by two.
     */

    if (hackrf_r9) {
        clock_generator.set_ms_frequency(clock_generator_output_r9_sgpio, frequency * 2, si5351_vco_f, 0);
    } else {
        clock_generator.set_ms_frequency(clock_generator_output_og_codec, frequency * 2, si5351_vco_f, 1);
    }
#endif
}

void ClockManager::set_reference_ppb(const int32_t ppb) {
    /* NOTE: This adjustment only affects PLLA when it is derived from the 25MHz crystal.
     * It is assumed an external clock coming in to CLKIN/PLLB is sufficiently accurate as to not need adjustment.
     * TODO: Revisit the above policy. It may be good to allow adjustment of the external reference too.
     */
    if (hackrf_r9 && reference.source != ReferenceSource::Xtal) {
        return;
    }
    constexpr uint32_t pll_multiplier = si5351_pll_xtal_25m.a;
    constexpr uint32_t denominator = 1000000 / pll_multiplier;
    const uint32_t new_a = (ppb >= 0) ? pll_multiplier : (pll_multiplier - 1);
    const uint32_t new_b = (ppb >= 0) ? (ppb / 1000) : (denominator + (ppb / 1000));
    const uint32_t new_c = (ppb == 0) ? 1 : denominator;

    const si5351::PLL pll{
        .f_in = si5351_inputs.f_xtal,
        .a = new_a,
        .b = new_b,
        .c = new_c,
    };
    const auto pll_a_reg = pll.reg(0);
    clock_generator.write(pll_a_reg);
}

void ClockManager::start_frequency_monitor_measurement(const cgu::CLK_SEL clk_sel) {
    // Measure a clock input for 480 cycles of the LPC43xx IRC.
    LPC_CGU->FREQ_MON = LPC_CGU_FREQ_MON_Type{
        .RCNT = 480,
        .FCNT = 0,
        .MEAS = 0,
        .CLK_SEL = toUType(clk_sel),
        .RESERVED0 = 0};
    LPC_CGU->FREQ_MON.MEAS = 1;
}

void ClockManager::wait_For_frequency_monitor_measurement_done() {
    // FREQ_MON mechanism fails to finish if there's no clock present on selected input?!
#ifndef PRALINE
    while (LPC_CGU->FREQ_MON.MEAS == 1);
#else
    // PRALINE FIX: Add timeout to prevent infinite hang
    uint32_t timeout = 100000;
    while (LPC_CGU->FREQ_MON.MEAS == 1 && timeout > 0) {
        timeout--;
    }
#endif
}

uint32_t ClockManager::get_frequency_monitor_measurement_in_hertz() {
    // Measurement is only as accurate as the LPC43xx IRC oscillator,
    // which is +/- 1.5%. Measurement is for 480 IRC clcocks. Scale
    // the cycle count to get a value in Hertz.
    return LPC_CGU->FREQ_MON.FCNT * 25000;
}

void ClockManager::start_audio_pll() {
#ifdef PRALINE
    /* PRALINE: Use 12MHz XTAL for audio PLL
     * For 12MHz XTAL input, 48kHz audio rate, 256Fs MCLK:
     *   Fout=12.288MHz, Fcco=491.52MHz
     *   12MHz * 1024 / 25 = 491.52MHz
     *   MSEL=1024, NSEL=25, PSEL=20
     */

    cgu::pll0audio::mdiv({
        .mdec = 22625UL,  // Encoded value for MSEL=1024
    });
    cgu::pll0audio::np_div({
        .pdec = 31,  // Encoded value for PSEL=20
        .ndec = 69,  // Encoded value for NSEL=25
    });

    cgu::pll0audio::frac({
        .pllfract_ctrl = 0,
    });

    cgu::pll0audio::power_up();

    // Praline Fix: Wait for lock with a safety timeout
    {
        uint32_t timeout = 100000;
        while (!cgu::pll0audio::is_locked() && timeout > 0) {
            timeout--;
        }
    }

    cgu::pll0audio::clock_enable();

    /* Route the 12.288MHz PLL to the Base Audio Clock */
    // PD = 0 enables the clock; AUTOBLOCK = 1 prevents glitches during clock switching
    LPC_CGU->BASE_AUDIO_CLK.PD = 0;
    LPC_CGU->BASE_AUDIO_CLK.AUTOBLOCK = 1;
    LPC_CGU->BASE_AUDIO_CLK.CLK_SEL = toUType(cgu::CLK_SEL::PLL0AUDIO);

#else
    cgu::pll0audio::ctrl({
        .pd = 1,
        .bypass = 0,
        .directi = 0,
        .directo = 0,
        .clken = 0,
        .frm = 0,
        .autoblock = 1,
        .pllfract_req = 0,
        .sel_ext = 1,
        .mod_pd = 1,
        .clk_sel = cgu::CLK_SEL::GP_CLKIN,
    });

    /* For 40MHz clock source, 48kHz audio rate, 256Fs MCLK:
     * 		Fout=12.288MHz, Fcco=491.52MHz
     *	OG:	PSEL=20, NSEL=125, MSEL=768
     *		PDEC=31, NDEC=45, MDEC=30542
     *	r9:	PSEL=20, NSEL=125, MSEL=3072
     *		PDEC=31, NDEC=45, MDEC=8308
     */
    cgu::pll0audio::mdiv({
        .mdec = hackrf_r9 ? 8308UL : 30542UL,
    });
    cgu::pll0audio::np_div({
        .pdec = 31,
        .ndec = 45,
    });
#endif

    cgu::pll0audio::frac({
        .pllfract_ctrl = 0,
    });

    cgu::pll0audio::power_up();
#ifndef PRALINE
    while (!cgu::pll0audio::is_locked());
#else
    // PRALINE FIX: Add timeout to prevent infinite hang if GP_CLKIN not present
    {
        uint32_t timeout = 100000;
        while (!cgu::pll0audio::is_locked() && timeout > 0) {
            timeout--;
        }
    }
#endif
    cgu::pll0audio::clock_enable();

    set_base_audio_clock_divider(1);

    LPC_CGU->BASE_AUDIO_CLK.AUTOBLOCK = 1;
    LPC_CGU->BASE_AUDIO_CLK.CLK_SEL = toUType(cgu::CLK_SEL::IDIVD);
}

void ClockManager::set_base_audio_clock_divider(const size_t divisor) {
    LPC_CGU->IDIVD_CTRL.word =
        (0 << 0) | ((divisor - 1) << 2) | (1 << 11) | (toUType(cgu::CLK_SEL::PLL0AUDIO) << 24);
}

void ClockManager::stop_audio_pll() {
    cgu::pll0audio::clock_disable();
    cgu::pll0audio::power_down();
    while (cgu::pll0audio::is_locked());
}

void ClockManager::enable_clock_output(bool enable) {
    if (hackrf_r9) {
        gpio_r9_clkout_en.output();
        gpio_r9_clkout_en.write(enable);

        // NOTE: RETURNING HERE IF HACKRF_R9 TO PREVENT CLK2 FROM BEING DISABLED OR FREQ MODIFIED SINCE CLK2 ON R9 IS
        // USED FOR BOTH CLKOUT AND FOR THE MCU_CLOCK (== GP_CLKIN) WHICH OTHER LP43XX CLOCKS CURRENTLY RELY ON.
        // FUTURE TBD: REMOVE OTHER LP43XX CLOCK DEPENDENCIES ON GP_CLKIN, THEN DELETE THE return LINE BELOW TO ALLOW
        // CLKOUT FREQ CHANGES ON R9 BOARDS.
        return;
    }

    auto clkout_select = hackrf_r9 ? clock_generator_output_r9_clkout : clock_generator_output_og_clkout;

    if (enable) {
        clock_generator.enable_output(clkout_select);
        if (portapack::persistent_memory::clkout_freq() < 1000) {
            clock_generator.set_ms_frequency(clkout_select, portapack::persistent_memory::clkout_freq() * 128000, si5351_vco_f, 7);
        } else {
            clock_generator.set_ms_frequency(clkout_select, portapack::persistent_memory::clkout_freq() * 1000, si5351_vco_f, 0);
        }

        auto si5351_clock_control_common = hackrf_r9 ? si5351a_clock_control_common : si5351c_clock_control_common;
        const auto ref_pll = hackrf_r9 ? ClockControl::MultiSynthSource::PLLA : get_si5351c_reference_clock_generator_pll(reference.source);
        clock_generator.set_clock_control(clkout_select, si5351_clock_control_common[clkout_select].ms_src(ref_pll).clk_pdn(ClockControl::ClockPowerDown::Power_On));
    } else {
        clock_generator.disable_output(clkout_select);
        clock_generator.set_clock_control(clkout_select, ClockControl::power_off());
    }
}
