/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "cpld_update.hpp"

#include "hackrf_gpio.hpp"
#include "portapack_hal.hpp"
#include "portapack.hpp"

#include "jtag_target_gpio.hpp"
#include "cpld_max5.hpp"
#include "cpld_xilinx.hpp"
#include "portapack_cpld_data.hpp"
#include "hackrf_cpld_data.hpp"

#include "crc.hpp"

#define REV_20150901_CHECKSUM 0xE0EF80FB
#define REV_20170522_CHECKSUM 0xD1BEB722

namespace portapack {
namespace cpld {

CpldUpdateStatus update_if_necessary(const Config config) {
    jtag::GPIOTarget target{
        portapack::gpio_cpld_tck,
        portapack::gpio_cpld_tms,
        portapack::gpio_cpld_tdi,
        portapack::gpio_cpld_tdo};
    jtag::JTAG jtag{target};
    CPLD cpld{jtag};

    /* Unknown state */
    cpld.reset();
    cpld.run_test_idle();

    /* Run-Test/Idle */
    if (!cpld.idcode_ok()) {
        return CpldUpdateStatus::Idcode_check_failed;
    }

    cpld.sample();
    cpld.bypass();
    cpld.enable();

    /* If silicon ID doesn't match, there's a serious problem. Leave CPLD
     * in passive state.
     */
    if (!cpld.silicon_id_ok()) {
        return CpldUpdateStatus::Silicon_id_check_failed;
    }

    /* Verify CPLD contents against current bitstream. */
    auto ok = cpld.verify(config.block_0, config.block_1);

    /* CPLD verifies incorrectly. Erase and program with current bitstream. */
    if (!ok) {
        ok = cpld.program(config.block_0, config.block_1);
    }

    /* If programming OK, reset CPLD to user mode. Otherwise leave it in
     * passive (ISP) state.
     */
    if (ok) {
        cpld.disable();
        cpld.bypass();

        /* Initiate SRAM reload from flash we just programmed. */
        cpld.sample();
        cpld.clamp();
        cpld.disable();
    }

    return ok ? CpldUpdateStatus::Success : CpldUpdateStatus::Program_failed;
}

static CpldUpdateStatus enter_maintenance_mode(CPLD& cpld) {
    /* Unknown state */
    cpld.reset();
    cpld.run_test_idle();

    /* Run-Test/Idle */
    if (!cpld.idcode_ok()) {
        return CpldUpdateStatus::Idcode_check_failed;
    }

    cpld.sample();
    cpld.bypass();
    cpld.enable();

    /* If silicon ID doesn't match, there's a serious problem. Leave CPLD
     * in passive state.
     */
    if (!cpld.silicon_id_ok()) {
        return CpldUpdateStatus::Silicon_id_check_failed;
    }

    return CpldUpdateStatus::Success;
}

static void exit_maintenance_mode(CPLD& cpld) {
    cpld.disable();
    cpld.bypass();

    /* Initiate SRAM reload from flash we just programmed. */
    cpld.sample();
    cpld.clamp();
    cpld.disable();
}

static uint32_t get_firmware_crc(CPLD& cpld) {
    CRC<32> crc{0x04c11db7, 0xffffffff, 0xffffffff};
    cpld.prepare_read(0x0000);

    for (size_t i = 0; i < 3328; i++) {
        uint16_t data = cpld.read();
        crc.process_byte((data >> 0) & 0xff);
        crc.process_byte((data >> 8) & 0xff);
        crc.process_byte((data >> 16) & 0xff);
        crc.process_byte((data >> 24) & 0xff);
    }

    cpld.prepare_read(0x0001);

    for (size_t i = 0; i < 512; i++) {
        uint16_t data = cpld.read();
        crc.process_byte((data >> 0) & 0xff);
        crc.process_byte((data >> 8) & 0xff);
        crc.process_byte((data >> 16) & 0xff);
        crc.process_byte((data >> 24) & 0xff);
    }

    return crc.checksum();
}

CpldUpdateStatus update_autodetect(const Config config_rev_20150901, const Config config_rev_20170522) {
    jtag::GPIOTarget target{
        portapack::gpio_cpld_tck,
        portapack::gpio_cpld_tms,
        portapack::gpio_cpld_tdi,
        portapack::gpio_cpld_tdo};
    jtag::JTAG jtag{target};
    CPLD cpld{jtag};

    if (portapack::display.read_display_status())
        return CpldUpdateStatus::Success;  // LCD is ready

    CpldUpdateStatus result = enter_maintenance_mode(cpld);
    if (result != CpldUpdateStatus::Success)
        return result;

    uint32_t checksum = get_firmware_crc(cpld);

    if (checksum == REV_20170522_CHECKSUM) {
        // H2 firmware present
        if (!cpld.program(config_rev_20150901.block_0, config_rev_20150901.block_1))
            return CpldUpdateStatus::Program_failed;
    } else if (checksum == REV_20150901_CHECKSUM) {
        // H1 firmware present
        if (!cpld.program(config_rev_20170522.block_0, config_rev_20170522.block_1))
            return CpldUpdateStatus::Program_failed;
    } else {
        // no firmware present
        if (!cpld.program(config_rev_20150901.block_0, config_rev_20150901.block_1))
            return CpldUpdateStatus::Program_failed;
    }

    exit_maintenance_mode(cpld);

    if (portapack::display.read_display_status())
        return CpldUpdateStatus::Success;  // LCD is ready

    if (checksum != REV_20150901_CHECKSUM && checksum != REV_20170522_CHECKSUM) {
        // try the other one
        CpldUpdateStatus result = enter_maintenance_mode(cpld);

        if (result != CpldUpdateStatus::Success)
            return result;

        if (!cpld.program(config_rev_20170522.block_0, config_rev_20170522.block_1))
            return CpldUpdateStatus::Program_failed;

        exit_maintenance_mode(cpld);

        if (portapack::display.read_display_status())
            return CpldUpdateStatus::Success;  // LCD is ready
    }

    return CpldUpdateStatus::Program_failed;
}

} /* namespace cpld */
} /* namespace portapack */

namespace hackrf {
namespace cpld {

static jtag::GPIOTarget jtag_target_hackrf() {
    return {
        hackrf::one::gpio_cpld_tck,
        hackrf::one::gpio_cpld_tms,
        hackrf::one::gpio_cpld_tdi,
        hackrf::one::gpio_cpld_tdo,
    };
}

bool load_sram() {
    auto jtag_target_hackrf_cpld = jtag_target_hackrf();
    hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};

    hackrf_cpld.write_sram(hackrf::one::cpld::verify_blocks);
    const auto ok = hackrf_cpld.verify_sram(hackrf::one::cpld::verify_blocks);

    return ok;
}

void load_sram_no_verify() {
    // CoolRunner II family has Hybrid memory CPLD architecture (SRAM+NVM)
    // It seems that after using a TX App the CPLD_SRAM part needs to be re_loaded to solve #637 ghost beat.
    // load_sram() it is already called at each boot in portapack.cpp, including verify CPLD part.
    // Here we skipped CPLD verify part, just to be quicker (in case any CPLD problem it will be detected in the boot process).

    auto jtag_target_hackrf_cpld = jtag_target_hackrf();
    hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};

    hackrf_cpld.write_sram(hackrf::one::cpld::verify_blocks);

    return;
}

bool verify_eeprom() {
    auto jtag_target_hackrf_cpld = jtag_target_hackrf();
    hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};

    const auto ok = hackrf_cpld.verify_eeprom(hackrf::one::cpld::verify_blocks);

    return ok;
}

void init_from_eeprom() {
    auto jtag_target_hackrf_cpld = jtag_target_hackrf();
    hackrf::one::cpld::CPLD hackrf_cpld{jtag_target_hackrf_cpld};

    hackrf_cpld.init_from_eeprom();
}

} /* namespace cpld */
} /* namespace hackrf */
