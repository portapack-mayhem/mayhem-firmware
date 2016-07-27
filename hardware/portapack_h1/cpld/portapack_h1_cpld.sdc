## Generated SDC file "portapack_hackrf_one_cpld.sdc"

## Copyright (C) 1991-2014 Altera Corporation
## Your use of Altera Corporation's design tools, logic functions 
## and other software and tools, and its AMPP partner logic 
## functions, and any output files from any of the foregoing 
## (including device programming or simulation files), and any 
## associated documentation or information are expressly subject 
## to the terms and conditions of the Altera Program License 
## Subscription Agreement, Altera MegaCore Function License 
## Agreement, or other applicable license agreement, including, 
## without limitation, that your use is for the sole purpose of 
## programming logic devices manufactured by Altera and sold by 
## Altera or its authorized distributors.  Please refer to the 
## applicable agreement for further details.


## VENDOR  "Altera"
## PROGRAM "Quartus II"
## VERSION "Version 13.1.4 Build 182 03/12/2014 SJ Web Edition"

## DATE    "Sat May  3 10:22:18 2014"

##
## DEVICE  "5M40ZE64C5"
##

# RS = 0, D = DB[15:8]
#		wait max(tast = 0 ns, CPLD setup = ?)
# WR = 0, D = DB[7:0]
#		wait max(CPLD )

#**************************************************************
# Time Information
#**************************************************************

set_time_format -unit ns -decimal_places 3

set mcu_clk_period 4.9

set lcd_data_wr_setup 10.0
set lcd_data_wr_hold 10.0

#**************************************************************
# Create Clock
#**************************************************************

create_clock -name {MCU_STROBE} -period 66.000 -waveform { 0.000 33.000 } [get_ports {MCU_STROBE}]
#create_clock -name strobe_virt -period 66.000

#**************************************************************
# Create Generated Clock
#**************************************************************



#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************



#**************************************************************
# Set Input Delay
#**************************************************************

#set_input_delay -clock strobe_virt [get_ports {D[*]}]

#**************************************************************
# Set Output Delay
#**************************************************************



#**************************************************************
# Set Clock Groups
#**************************************************************



#**************************************************************
# Set False Path
#**************************************************************

set_false_path -from [get_clocks {MCU_STROBE}] -to [get_ports {TP_D TP_L TP_R TP_U}]
set_false_path -from [get_ports {SW_D SW_L SW_R SW_ROT_A SW_ROT_B SW_SEL SW_U}] -to [get_ports {MCU_D[*]}]


#**************************************************************
# Set Multicycle Path
#**************************************************************



#**************************************************************
# Set Maximum Delay
#**************************************************************



#**************************************************************
# Set Minimum Delay
#**************************************************************



#**************************************************************
# Set Input Transition
#**************************************************************

