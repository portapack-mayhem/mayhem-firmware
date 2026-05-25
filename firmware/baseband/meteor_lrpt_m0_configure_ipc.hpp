/*
 * Copyright (C) 2026
 *
 * M0→M4 Meteor LRPT configure handshake (lives in baseband_shared so every M4
 * image, including external PMLR.bin, polls the same way from event_m4.cpp).
 */
#ifndef __METEOR_LRPT_M0_CONFIGURE_IPC_HPP__
#define __METEOR_LRPT_M0_CONFIGURE_IPC_HPP__

#include "baseband_processor.hpp"

void meteor_lrpt_rx_reset_m0_configure_state();
void meteor_lrpt_rx_poll_m0_configure(BasebandProcessor* processor);

#endif /*__METEOR_LRPT_M0_CONFIGURE_IPC_HPP__*/
