# Copyright (C) 2026
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Single source of truth for Meteor LRPT deinterleave stack sources shared
# between Cortex-M4 baseband and Cortex-M0 application (Option A worker).
#
# Do not use one CMake OBJECT library for both: M0 (-Os) and M4 (-O3) need
# different compile flags; listing the same .cpp in each target compiles twice.

set(METEOR_LRPT_DEINT_CPPSRC_BASEBAND
	meteor_lrpt/meteor_deinterleaver.cpp
	meteor_lrpt/meteor_lrpt_deinterleave_dispatch.cpp
	meteor_lrpt/meteor_deinterleaver_reader.cpp
	meteor_lrpt/meteor_soft_correlate.cpp
	meteor_lrpt/external_ring_ram.cpp
)

set(METEOR_LRPT_DEINT_CPPSRC_APPLICATION
	../baseband/meteor_lrpt/meteor_deinterleaver.cpp
	../baseband/meteor_lrpt/meteor_deinterleaver_reader.cpp
	../baseband/meteor_lrpt/meteor_soft_correlate.cpp
)
# M0 links `meteor_lrpt_deinterleave_dispatch_m0.cpp` from application/CMakeLists.txt (not this list)
# so `meteor_lrpt_deinterleave_dispatch.cpp` is not duplicated in application.elf.
