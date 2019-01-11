/*
    Copyright (C) 2018 Jared Boone, ShareBrained Technology

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"

void peripheral_reset(const peripheral_reset_t* const reset) {
  const size_t register_index = (reset->output_index >> 5) & 1;
  const size_t bit_shift = reset->output_index & 0x1f;
  const uint32_t mask = (1U << bit_shift);
  LPC_RGU->RESET_CTRL[register_index] = mask | (~LPC_RGU->RESET_ACTIVE_STATUS[register_index]);
  while((LPC_RGU->RESET_ACTIVE_STATUS[register_index] & mask) == 0);
}

void base_clock_enable(const base_clock_regs_t* const base) {
  if( base->clk->PD ) {
    base->clk->AUTOBLOCK = 1;
    //base->clk->CLK_SEL = ?;
    base->clk->PD = 0;
  }
}

void base_clock_disable(const base_clock_regs_t* const base) {
  if( !base->clk->PD ) {
    // Are all branch clocks switched off?
    // NOTE: Field stat must be valid memory address.
    // NOTE: Field stat_mask is zero if there's no means to check if a base clock is in use.
    if( (*base->stat & base->stat_mask) == 0 ) {
      base->clk->PD = 1;
      //base->clk->CLK_SEL = IRC?;
    }
  }
}

void branch_clock_enable(const branch_clock_regs_t* const branch) {
  if( !branch->stat->RUN ) {
    branch->cfg->AUTO = 1;
    branch->cfg->RUN = 1;
    while(!branch->stat->RUN);
  }
}

void branch_clock_disable(const branch_clock_regs_t* const branch) {
  if( branch->stat->RUN ) {
    branch->cfg->RUN = 0;
    while(branch->stat->RUN);
  }
}

void interrupt_enable(const interrupt_config_t* const interrupt) {
  nvicEnableVector(interrupt->irq, interrupt->priority_mask);
}

void interrupt_disable(const interrupt_config_t* const interrupt) {
  nvicDisableVector(interrupt->irq);
}
