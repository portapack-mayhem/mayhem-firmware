#
# Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
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

# Makefile based on Altera Quartus documentation example, topic
# "About Using Quartus II from the Command Line"

###################################################################
# Project Configuration: 
# 
# Specify the name of the design (project) and Quartus II Settings 
# File (.qsf) and the list of source files used.
###################################################################

PROJECT=portapack_h1_cpld
SOURCE_FILES=top.vhd
ASSIGNMENT_FILES=$(PROJECT).qpf $(PROJECT).qsf $(PROJECT).sdc
OUTPUT_DIR=output_files

###################################################################
# Main Targets
#
# all: build everything
# clean: remove output files and database
###################################################################

all: smart.log $(OUTPUT_DIR)/$(PROJECT).asm.rpt $(OUTPUT_DIR)/$(PROJECT).sta.rpt

clean:
	rm -rf *.chg *.qws smart.log db/ incremental_db/ $(OUTPUT_DIR)/

map: smart.log $(OUTPUT_DIR)/$(PROJECT).map.rpt

fit: smart.log $(OUTPUT_DIR)/$(PROJECT).fit.rpt

asm: smart.log $(OUTPUT_DIR)/$(PROJECT).asm.rpt

sta: smart.log $(OUTPUT_DIR)/$(PROJECT).sta.rpt

smart: smart.log

###################################################################
# Executable Configuration
###################################################################

MAP_ARGS=
FIT_ARGS=
ASM_ARGS=
STA_ARGS=

###################################################################
# Target implementations
###################################################################

STAMP = echo done >

$(OUTPUT_DIR)/$(PROJECT).map.rpt: $(SOURCE_FILES)
	quartus_map $(MAP_ARGS) $(PROJECT)
	$(STAMP) fit.chg

$(OUTPUT_DIR)/$(PROJECT).fit.rpt: fit.chg $(OUTPUT_DIR)/$(PROJECT).map.rpt
	quartus_fit $(FIT_ARGS) $(PROJECT)
	$(STAMP) asm.chg
	$(STAMP) sta.chg

$(OUTPUT_DIR)/$(PROJECT).asm.rpt: asm.chg $(OUTPUT_DIR)/$(PROJECT).fit.rpt
	quartus_asm $(ASM_ARGS) $(PROJECT)

$(OUTPUT_DIR)/$(PROJECT).sta.rpt: sta.chg $(OUTPUT_DIR)/$(PROJECT).fit.rpt
	quartus_sta $(STA_ARGS) $(PROJECT)

smart.log: $(ASSIGNMENT_FILES) $(OUTPUT_DIR)
	quartus_sh --determine_smart_action $(PROJECT) > smart.log

###################################################################
# Project initialization
###################################################################

$(OUTPUT_DIR):
	mkdir $(OUTPUT_DIR)

$(ASSIGNMENT_FILES): $(OUTPUT_DIR)
	quartus_sh --prepare $(PROJECT)

fit.chg:
	$(STAMP) fit.chg

sta.chg:
	$(STAMP) sta.chg

asm.chg:
	$(STAMP) asm.chg
