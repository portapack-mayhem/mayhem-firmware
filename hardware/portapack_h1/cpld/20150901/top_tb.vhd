--
-- Copyright (C) 2012 Jared Boone, ShareBrained Technology, Inc.
--
-- This file is part of PortaPack.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2, or (at your option)
-- any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; see the file COPYING.  If not, write to
-- the Free Software Foundation, Inc., 51 Franklin Street,
-- Boston, MA 02110-1301, USA.

library ieee;
use ieee.std_logic_1164.all;

entity top_tb is

end top_tb;

architecture behavior of top_tb is

	component top
	port (
		MCU_D			:	inout	std_logic_vector(7 downto 0);
		MCU_DIR		:	in		std_logic;
		MCU_MODE		:	in		std_logic;
		MCU_STROBE	:	in		std_logic;
		MCU_ADDR		:	in		std_logic;
		
		TP_U			:	out	std_logic;
		TP_D			:	out	std_logic;
		TP_L			:	out	std_logic;
		TP_R			:	out	std_logic;
		
		SW_SEL		:	in		std_logic;
		SW_ROT_A		:	in		std_logic;
		SW_ROT_B		:	in		std_logic;
		SW_U			:	in		std_logic;
		SW_D			:	in		std_logic;
		SW_L			:	in		std_logic;
		SW_R			:	in		std_logic;
		
		LCD_RESETX	:	out	std_logic;
		LCD_RS		:	out	std_logic;
		LCD_WRX		:	out	std_logic;
		LCD_RDX		:	out	std_logic;
		LCD_DB		:	inout	std_logic_vector(17 downto 0);
		LCD_TE		:	in		std_logic
	);
	end component;

	signal mcu_d : std_logic_vector(7 downto 0);
	signal mcu_strobe : std_logic;
	signal mcu_dir : std_logic;
	signal mcu_mode : std_logic;
	signal mcu_addr : std_logic;
  
	signal tp_u : std_logic;
	signal tp_d : std_logic;
	signal tp_l : std_logic;
	signal tp_r : std_logic;
	
	signal sw_sel : std_logic;
	signal sw_rot_a : std_logic;
	signal sw_rot_b : std_logic;
	signal sw_u : std_logic;
	signal sw_d : std_logic;
	signal sw_l : std_logic;
	signal sw_r : std_logic;
  
	signal lcd_resetx : std_logic;
	signal lcd_rs : std_logic;
	signal lcd_wrx : std_logic;
	signal lcd_rdx : std_logic;
	signal lcd_db : std_logic_vector(17 downto 0);
	signal lcd_te : std_logic := '0';
begin

	uut : top
	port map (
	  MCU_D => mcu_d,
	  MCU_STROBE => mcu_strobe,
	  MCU_DIR => mcu_dir,
	  MCU_MODE => mcu_mode,
	  MCU_ADDR => mcu_addr,
	  TP_U => tp_u,
	  TP_D => tp_d,
	  TP_L => tp_l,
	  TP_R => tp_r,
	  SW_SEL => sw_sel,
	  SW_ROT_A => sw_rot_a,
	  SW_ROT_B => sw_rot_b,
	  SW_U => sw_u,
	  SW_D => sw_d,
	  SW_L => sw_l,
	  SW_R => sw_r,
	  LCD_RESETX => lcd_resetx,
	  LCD_RS => lcd_rs,
	  LCD_WRX => lcd_wrx,
	  LCD_RDX => lcd_rdx,
	  LCD_DB => lcd_db,
	  LCD_TE => lcd_te
	);

	stimulus: process is
	begin
		sw_sel <= '0';
		sw_rot_a <= '0';
		sw_rot_b <= '0';
		sw_u <= '0';
		sw_d <= '0';
		sw_l <= '0';
		sw_r <= '0';
		
		mcu_d <= (others => 'Z');
		mcu_mode <= '1';
		mcu_dir <= '1';
		mcu_addr <= '1';
		mcu_strobe <= '1';
		
		wait for 50.0 ns;

		-- Write to resistive touch panel
		mcu_mode <= '0';	-- Target: I/O
		mcu_dir <= '0';	-- Direction: MCU -> CPLD
		mcu_addr <= '0';	-- LCD reset signal
		wait for 19.6 ns;	-- 4 cycles: Wait for CPLD D to reach Hi-Z
		mcu_d <= "11000101";
		wait for 14.7 ns;	-- 3 cycles: Setup time on D before STROBE.
		mcu_strobe <= '0';
		wait for 9.8 ns;	-- 2 cycles
		mcu_strobe <= '1';
		wait for 49.0 ns;

		-- Write to LCD (command, then 16-bit data)
		mcu_mode <= '1';	-- Target: LCD
		mcu_dir <= '0';	-- Direction: MCU -> CPLD
		mcu_addr <= '0';	-- Address: RS = 0 (command)
		wait for 19.6 ns;	-- 4 cycles: Wait for CPLD D to reach Hi-Z
		
		mcu_d <= "10100101";
		wait for 14.7 ns;	-- 3 cycles: Setup time on D before STROBE.
		mcu_strobe <= '0';
		wait for 9.8 ns;	-- 2 cycles
		mcu_d <= "00001111";
		wait for 24.5 ns;	-- 5 cycles: Prop from D to LCD_DB[7:0], WRX# minimum low time.
		mcu_strobe <= '1';
		wait for 9.8 ns;	-- 2 cycles: Part of prop from STROBE to LCD_WRX, delay to keep RS after WRX deassert.
		mcu_addr <= '1';	-- Address: RS = 1 (data)
		wait for 9.8 ns;	-- 2 cycles: Part of prop from STROBE to LCD_WRX.
		
		mcu_d <= "01011010";
		wait for 14.7 ns;	-- 3 cycles: Setup time on D before STROBE.
		mcu_strobe <= '0';
		wait for 9.8 ns;	-- 2 cycles
		mcu_d <= "11110000";
		wait for 24.5 ns;	-- 5 cycles: Prop from D to LCD_DB[7:0], WRX# minimum low time.
		mcu_strobe <= '1';
		wait for 19.6 ns;	-- 4 cycles: Prop from STROBE to LCD_WRX.
		
		mcu_d <= "01010101";
		wait for 14.7 ns;	-- 3 cycles: Setup time on D before STROBE.
		mcu_strobe <= '0';
		wait for 9.8 ns;	-- 2 cycles
		mcu_d <= "10101010";
		wait for 24.5 ns;	-- 5 cycles: Prop from D to LCD_DB[7:0], WRX# minimum low time.
		mcu_strobe <= '1';
		wait for 19.6 ns;	-- 4 cycles: Prop from STROBE to LCD_WRX.
		
		-- Read from switches
		mcu_d <= (others => 'Z');
		mcu_mode <= '0';	-- Target: I/O
		mcu_dir <= '1';	-- Direction: MCU <- CPLD
		wait for 49.0 ns;
		
	end process;
end architecture behavior;
