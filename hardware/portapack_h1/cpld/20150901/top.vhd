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

entity top is
	port (
		MCU_D				:	inout	std_logic_vector(7 downto 0);
		MCU_DIR			:	in		std_logic;
		MCU_IO_STBX		:	in		std_logic;
		MCU_LCD_WRX		:	in		std_logic;
		MCU_ADDR			:	in		std_logic;
		MCU_LCD_TE		:	out	std_logic;
		MCU_P2_8			:	in		std_logic;
		MCU_LCD_RDX		:	in		std_logic;
		
		TP_U				:	out	std_logic;
		TP_D				:	out	std_logic;
		TP_L				:	out	std_logic;
		TP_R				:	out	std_logic;
		
		SW_SEL			:	in		std_logic;
		SW_ROT_A			:	in		std_logic;
		SW_ROT_B			:	in		std_logic;
		SW_U				:	in		std_logic;
		SW_D				:	in		std_logic;
		SW_L				:	in		std_logic;
		SW_R				:	in		std_logic;
		
		LCD_RESETX		:	out	std_logic;
		LCD_RS			:	out	std_logic;
		LCD_WRX			:	out	std_logic;
		LCD_RDX			:	out	std_logic;
		LCD_DB			:	inout	std_logic_vector(15 downto 0);
		LCD_TE			:	in		std_logic;
		LCD_BACKLIGHT	:	out	std_logic
	);
end top;

architecture rtl of top is

	signal	switches : std_logic_vector(7 downto 0);
	
	type data_direction_t is (from_mcu, to_mcu);
	signal	data_dir	: data_direction_t;
	
	signal	mcu_data_out_lcd : std_logic_vector(7 downto 0);
	signal	mcu_data_out_io : std_logic_vector(7 downto 0);
	signal	mcu_data_out : std_logic_vector(7 downto 0);
	signal	mcu_data_in	: std_logic_vector(7 downto 0);
	
	signal	lcd_data_in : std_logic_vector(15 downto 0);
	signal  lcd_data_in_mux : std_logic_vector(7 downto 0);
	signal	lcd_data_out : std_logic_vector(15 downto 0);
	
	signal  lcd_data_in_q : std_logic_vector(7 downto 0) := (others => '0');
	signal  lcd_data_out_q : std_logic_vector(7 downto 0) := (others => '0');
	
	signal	tp_q : std_logic_vector(7 downto 0) := (others => '0');
	
	signal	lcd_reset_q : std_logic := '1';
	signal	lcd_backlight_q : std_logic := '0';
	
	signal	dir_read : boolean;
	signal	dir_write : boolean;
	
	signal	lcd_read_strobe : boolean;
	signal	lcd_write_strobe : boolean;
	signal	lcd_write : boolean;

	signal	io_strobe : boolean;
	signal	io_read_strobe : boolean;
	signal	io_write_strobe : boolean;
	
begin

	-- I/O data
	switches <= LCD_TE & not SW_ROT_B & not SW_ROT_A & not SW_SEL & not SW_U & not SW_D & not SW_L & not SW_R;

	TP_U <= tp_q(3) when tp_q(7) = '1' else 'Z';
	TP_D <= tp_q(2) when tp_q(6) = '1' else 'Z';
	TP_L <= tp_q(1) when tp_q(5) = '1' else 'Z';
	TP_R <= tp_q(0) when tp_q(4) = '1' else 'Z';
	
	LCD_BACKLIGHT <= lcd_backlight_q;
	
	MCU_LCD_TE <= LCD_TE;
	
	-- State management
	data_dir <= to_mcu when MCU_DIR = '1' else from_mcu;
	dir_read <= (data_dir = to_mcu);
	dir_write <= (data_dir = from_mcu);

	io_strobe <= (MCU_IO_STBX = '0');
	io_read_strobe <= io_strobe and dir_read;

	lcd_read_strobe <= (MCU_LCD_RDX = '0');
	lcd_write <= not lcd_read_strobe;

	-- LCD interface
	LCD_RS <= MCU_ADDR;
	LCD_RDX <= MCU_LCD_RDX;
	LCD_WRX <= MCU_LCD_WRX;

	lcd_data_out <= lcd_data_out_q & mcu_data_in;
	lcd_data_in <= LCD_DB;
	LCD_DB <= lcd_data_out when lcd_write else (others => 'Z');
	
	LCD_RESETX <= not lcd_reset_q;
	
	-- MCU interface
	mcu_data_out_lcd <= lcd_data_in(15 downto 8) when lcd_read_strobe else lcd_data_in_q;
	mcu_data_out_io <= switches;
	mcu_data_out <= mcu_data_out_io when io_read_strobe else mcu_data_out_lcd;

	mcu_data_in <= MCU_D;
	MCU_D <= mcu_data_out when dir_read else (others => 'Z');
	
	-- Synchronous behaviors:
	-- LCD write: Capture LCD high byte on LCD_WRX falling edge.
	process(MCU_LCD_WRX, mcu_data_in)
	begin
		if falling_edge(MCU_LCD_WRX) then
			lcd_data_out_q <= mcu_data_in;
		end if;
	end process;
	
	-- LCD read: Capture LCD low byte on LCD_RD falling edge.
	process(MCU_LCD_RDX, lcd_data_in)
	begin
		if rising_edge(MCU_LCD_RDX) then
			lcd_data_in_q <= lcd_data_in(7 downto 0);
		end if;
	end process;
	
	-- I/O write (to resistive touch panel): Capture data from
	-- MCU and hold on TP pins until further notice.
	process(MCU_IO_STBX, dir_write, mcu_data_in, MCU_ADDR)
	begin
		if rising_edge(MCU_IO_STBX) and dir_write then
			if MCU_ADDR = '0' then
				tp_q <= mcu_data_in;
			else
				lcd_reset_q <= mcu_data_in(0);
				lcd_backlight_q <= mcu_data_in(7);
			end if;
		end if;
	end process;
end rtl;
