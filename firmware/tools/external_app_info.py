#!/usr/bin/env python3

#
# Copyright (C) 2024 Mark Thompson
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# External app address ranges must match those in linker file "external.ld".
# The start is fixed. The end (exclusive) depends on the tier's linker script
# (external_tier{0,1,2}.ld), which the build copies to
# <build>/firmware/application/external/external.ld, so it is read from there
# with read_external_apps_address_end() instead of being kept here by hand.
#
# maximum_application_size is the size of one app's link region in
# "external.ld", which is what makes an address belong to an app's own section.
# It is not the run-time budget: an app's M0 code and the M4 baseband image it
# runs share portapack::memory::map::m4_code, whose size is board dependent and
# is passed to "export_external_apps.py" by CMake (M4_CODE_SIZE in
# "rules.cmake").
maximum_application_size = 32*1024
external_apps_address_start = 0xADB00000

import os
import re
import sys

external_ld_relative_path = os.path.join("external", "external.ld")


def read_external_apps_address_end(application_binary_dir):
	"""End (exclusive) of the highest external app region in the build's external.ld.

	application_binary_dir is the build's firmware/application directory.
	"""
	ld_path = os.path.join(application_binary_dir, external_ld_relative_path)
	try:
		with open(ld_path) as f:
			text = f.read()
	except OSError as e:
		sys.exit("cannot read the external app linker script {}: {}".format(ld_path, e))
	regions = re.findall(r"ram_external_app_\w+\s*\(rwx\)\s*:\s*org\s*=\s*(0x[0-9A-Fa-f]+)\s*,\s*len\s*=\s*(\d+)k", text)
	if not regions:
		sys.exit("no external app regions found in " + ld_path)
	return max(int(org, 16) + int(length) * 1024 for org, length in regions)
