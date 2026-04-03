`timescale 1 ps/ 1 ps

module top(
	MCU_D,
	MCU_DIR,
	MCU_IO_STBX,
	MCU_LCD_WRX,
	MCU_ADDR,
	MCU_LCD_TE,
	MCU_P2_8,
	MCU_LCD_RDX,
	TP_U,
	TP_D,
	TP_L,
	TP_R,
	SW_SEL,
	SW_ROT_A,
	SW_ROT_B,
	SW_U,
	SW_D,
	SW_L,
	SW_R,
	LCD_RESETX,
	LCD_RS,
	LCD_WRX,
	LCD_RDX,
	LCD_DB,
	LCD_TE,
	LCD_BACKLIGHT,
	SYSOFF,
	AUDIO_RESETX,
	REF_EN,
	GPS_RESETX,
	GPS_TX_READY,
	GPS_TIMEPULSE,
	DEVICE_RESET,
	DEVICE_RESET_V);
output	[7:0] MCU_D;
input	MCU_DIR;
input	MCU_IO_STBX;
input	MCU_LCD_WRX;
input	MCU_ADDR;
output	MCU_LCD_TE;
input	MCU_P2_8;
input	MCU_LCD_RDX;
output	TP_U;
output	TP_D;
output	TP_L;
output	TP_R;
input	SW_SEL;
input	SW_ROT_A;
input	SW_ROT_B;
input	SW_U;
input	SW_D;
input	SW_L;
input	SW_R;
output	LCD_RESETX;
output	LCD_RS;
output	LCD_WRX;
output	LCD_RDX;
output	[15:0] LCD_DB;
input	LCD_TE;
output	LCD_BACKLIGHT;
output	SYSOFF;
output	AUDIO_RESETX;
output	REF_EN;
output	GPS_RESETX;
input	GPS_TX_READY;
input	GPS_TIMEPULSE;
input	DEVICE_RESET;
input	DEVICE_RESET_V;

//wire	gnd;
//wire	vcc;
wire	AsyncReset_X1_Y15_GND;
wire	AsyncReset_X1_Y19_GND;
wire	AsyncReset_X1_Y20_GND;
wire	AsyncReset_X1_Y21_GND;
wire	AsyncReset_X1_Y24_GND;
wire	AsyncReset_X1_Y26_GND;
wire	\DEVICE_RESET_V~input_o ;
wire	\DEVICE_RESET~input_o ;
wire	\GPS_TIMEPULSE~input_o ;
wire	\GPS_TX_READY~input_o ;
wire	\LCD_DB[0]~input_o ;
wire	\LCD_DB[10]~input_o ;
wire	\LCD_DB[11]~input_o ;
wire	\LCD_DB[12]~input_o ;
wire	\LCD_DB[13]~input_o ;
wire	\LCD_DB[14]~input_o ;
wire	\LCD_DB[15]~input_o ;
wire	\LCD_DB[1]~input_o ;
wire	\LCD_DB[2]~input_o ;
wire	\LCD_DB[3]~input_o ;
wire	\LCD_DB[4]~input_o ;
wire	\LCD_DB[5]~input_o ;
wire	\LCD_DB[6]~input_o ;
wire	\LCD_DB[7]~input_o ;
wire	\LCD_DB[8]~input_o ;
wire	\LCD_DB[9]~input_o ;
wire	\LCD_TE~input_o ;
wire	\MCU_ADDR~input_o ;
wire	\MCU_DIR~input_o ;
wire	\MCU_D[0]~input_o ;
wire	\MCU_D[1]~input_o ;
wire	\MCU_D[2]~input_o ;
wire	\MCU_D[3]~input_o ;
wire	\MCU_D[4]~input_o ;
wire	\MCU_D[5]~input_o ;
wire	\MCU_D[6]~input_o ;
wire	\MCU_D[7]~input_o ;
wire	\MCU_IO_STBX~input_o ;
wire	\MCU_IO_STBX~inputclkctrl_outclk ;
wire	\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ;
wire	\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ;
wire	\MCU_LCD_RDX~input_o ;
wire	\MCU_LCD_RDX~inputclkctrl_outclk ;
wire	\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y19_SIG_VCC ;
wire	\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y21_SIG_VCC ;
wire	\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y24_SIG_VCC ;
wire	\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y26_SIG_VCC ;
wire	\MCU_LCD_WRX~input_o ;
wire	\MCU_LCD_WRX~inputclkctrl_outclk ;
wire	\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ;
wire	\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y26_INV_VCC ;
wire	\MCU_P2_8~input_o ;
wire	\SW_D~input_o ;
wire	\SW_L~input_o ;
wire	\SW_ROT_A~input_o ;
wire	\SW_ROT_B~input_o ;
wire	\SW_R~input_o ;
wire	\SW_SEL~input_o ;
wire	\SW_U~input_o ;
wire	SyncLoad_X1_Y15_VCC;
wire	SyncLoad_X1_Y19_VCC;
wire	SyncLoad_X1_Y20_VCC;
wire	SyncLoad_X1_Y21_VCC;
wire	SyncLoad_X1_Y24_VCC;
wire	SyncLoad_X1_Y26_VCC;
wire	SyncReset_X1_Y15_GND;
wire	SyncReset_X1_Y19_GND;
wire	SyncReset_X1_Y20_GND;
wire	SyncReset_X1_Y21_GND;
wire	SyncReset_X1_Y24_GND;
wire	SyncReset_X1_Y26_GND;
wire	\audio_reset_q~0_combout ;
wire	\audio_reset_q~q ;
tri1	devclrn;
tri1	devoe;
tri1	devpor;
wire	\lcd_backlight_q~feeder_combout ;
wire	\lcd_backlight_q~q ;
wire	[7:0] lcd_data_in_q;
//wire	lcd_data_in_q[0];
//wire	lcd_data_in_q[1];
//wire	lcd_data_in_q[2];
//wire	lcd_data_in_q[3];
//wire	lcd_data_in_q[4];
//wire	lcd_data_in_q[5];
//wire	lcd_data_in_q[6];
//wire	lcd_data_in_q[7];
wire	[7:0] lcd_data_out_q;
//wire	lcd_data_out_q[0];
wire	\lcd_data_out_q[0]~feeder_combout ;
//wire	lcd_data_out_q[1];
wire	\lcd_data_out_q[1]~feeder_combout ;
//wire	lcd_data_out_q[2];
wire	\lcd_data_out_q[2]~feeder_combout ;
//wire	lcd_data_out_q[3];
//wire	lcd_data_out_q[4];
wire	\lcd_data_out_q[4]~feeder_combout ;
//wire	lcd_data_out_q[5];
//wire	lcd_data_out_q[6];
//wire	lcd_data_out_q[7];
wire	\lcd_data_out_q[7]~feeder_combout ;
wire	\lcd_reset_q~0_combout ;
wire	\lcd_reset_q~1_combout ;
wire	\lcd_reset_q~q ;
wire	\mcu_data_out[0]~0_combout ;
wire	\mcu_data_out[0]~1_combout ;
wire	\mcu_data_out[1]~2_combout ;
wire	\mcu_data_out[1]~3_combout ;
wire	\mcu_data_out[2]~4_combout ;
wire	\mcu_data_out[2]~5_combout ;
wire	\mcu_data_out[3]~6_combout ;
wire	\mcu_data_out[3]~7_combout ;
wire	\mcu_data_out[4]~8_combout ;
wire	\mcu_data_out[4]~9_combout ;
wire	\mcu_data_out[5]~10_combout ;
wire	\mcu_data_out[5]~11_combout ;
wire	\mcu_data_out[6]~12_combout ;
wire	\mcu_data_out[6]~13_combout ;
wire	\mcu_data_out[7]~14_combout ;
wire	\mcu_data_out[7]~15_combout ;
wire	\ref_en_q~feeder_combout ;
wire	\ref_en_q~q ;
wire	\sysoff_q~feeder_combout ;
wire	\sysoff_q~q ;
wire	[7:0] tp_q;
//wire	tp_q[0];
//wire	tp_q[1];
//wire	tp_q[2];
wire	\tp_q[2]~feeder_combout ;
//wire	tp_q[3];
wire	\tp_q[3]~0_combout ;
//wire	tp_q[4];
wire	\tp_q[4]~feeder_combout ;
//wire	tp_q[5];
//wire	tp_q[6];
wire	\tp_q[6]~feeder_combout ;
//wire	tp_q[7];
wire	\tp_q[7]~feeder_combout ;
wire	unknown;
wire	\~ALTERA_ASDO_DATA1~~ibuf_o ;
wire	\~ALTERA_ASDO_DATA1~~padout ;
wire	\~ALTERA_DATA0~~ibuf_o ;
wire	\~ALTERA_DATA0~~padout ;
wire	\~ALTERA_FLASH_nCE_nCSO~~ibuf_o ;
wire	\~ALTERA_FLASH_nCE_nCSO~~padout ;

wire vcc;
wire gnd;
assign vcc = 1'b1;
assign gnd = 1'b0;

alta_io \AUDIO_RESETX~output (
	.datain(\audio_reset_q~q ),
	.oe(vcc),
	.padio(AUDIO_RESETX),
	.combout());
defparam \AUDIO_RESETX~output .coord_x = 7;
defparam \AUDIO_RESETX~output .coord_y = 2;
defparam \AUDIO_RESETX~output .coord_z = 4;
defparam \AUDIO_RESETX~output .PRG_DELAYB = 1'b1;
defparam \AUDIO_RESETX~output .RX_SEL = 1'b0;
defparam \AUDIO_RESETX~output .PDCNTL = 2'b01;
defparam \AUDIO_RESETX~output .NDCNTL = 2'b01;
defparam \AUDIO_RESETX~output .PRG_SLR = 1'b1;
defparam \AUDIO_RESETX~output .CFG_KEEP = 2'b00;
defparam \AUDIO_RESETX~output .PU = 4'b0000;

alta_io \DEVICE_RESET_V~input (
	.datain(gnd),
	.oe(gnd),
	.padio(DEVICE_RESET_V),
	.combout(\DEVICE_RESET_V~input_o ));
defparam \DEVICE_RESET_V~input .coord_x = 4;
defparam \DEVICE_RESET_V~input .coord_y = 0;
defparam \DEVICE_RESET_V~input .coord_z = 5;
defparam \DEVICE_RESET_V~input .PRG_DELAYB = 1'b1;
defparam \DEVICE_RESET_V~input .RX_SEL = 1'b0;
defparam \DEVICE_RESET_V~input .PDCNTL = 2'b11;
defparam \DEVICE_RESET_V~input .NDCNTL = 2'b11;
defparam \DEVICE_RESET_V~input .PRG_SLR = 1'b0;
defparam \DEVICE_RESET_V~input .CFG_KEEP = 2'b00;
defparam \DEVICE_RESET_V~input .PU = 4'b0000;

alta_io \DEVICE_RESET~input (
	.datain(gnd),
	.oe(gnd),
	.padio(DEVICE_RESET),
	.combout(\DEVICE_RESET~input_o ));
defparam \DEVICE_RESET~input .coord_x = 6;
defparam \DEVICE_RESET~input .coord_y = 0;
defparam \DEVICE_RESET~input .coord_z = 0;
defparam \DEVICE_RESET~input .PRG_DELAYB = 1'b1;
defparam \DEVICE_RESET~input .RX_SEL = 1'b0;
defparam \DEVICE_RESET~input .PDCNTL = 2'b11;
defparam \DEVICE_RESET~input .NDCNTL = 2'b11;
defparam \DEVICE_RESET~input .PRG_SLR = 1'b0;
defparam \DEVICE_RESET~input .CFG_KEEP = 2'b00;
defparam \DEVICE_RESET~input .PU = 4'b0000;

alta_io \GPS_RESETX~output (
	.datain(vcc),
	.oe(vcc),
	.padio(GPS_RESETX),
	.combout());
defparam \GPS_RESETX~output .coord_x = 7;
defparam \GPS_RESETX~output .coord_y = 3;
defparam \GPS_RESETX~output .coord_z = 0;
defparam \GPS_RESETX~output .PRG_DELAYB = 1'b1;
defparam \GPS_RESETX~output .RX_SEL = 1'b0;
defparam \GPS_RESETX~output .PDCNTL = 2'b01;
defparam \GPS_RESETX~output .NDCNTL = 2'b01;
defparam \GPS_RESETX~output .PRG_SLR = 1'b1;
defparam \GPS_RESETX~output .CFG_KEEP = 2'b00;
defparam \GPS_RESETX~output .PU = 4'b0000;

alta_io \GPS_TIMEPULSE~input (
	.datain(gnd),
	.oe(gnd),
	.padio(GPS_TIMEPULSE),
	.combout(\GPS_TIMEPULSE~input_o ));
defparam \GPS_TIMEPULSE~input .coord_x = 6;
defparam \GPS_TIMEPULSE~input .coord_y = 4;
defparam \GPS_TIMEPULSE~input .coord_z = 5;
defparam \GPS_TIMEPULSE~input .PRG_DELAYB = 1'b1;
defparam \GPS_TIMEPULSE~input .RX_SEL = 1'b0;
defparam \GPS_TIMEPULSE~input .PDCNTL = 2'b01;
defparam \GPS_TIMEPULSE~input .NDCNTL = 2'b01;
defparam \GPS_TIMEPULSE~input .PRG_SLR = 1'b1;
defparam \GPS_TIMEPULSE~input .CFG_KEEP = 2'b00;
defparam \GPS_TIMEPULSE~input .PU = 4'b1000;

alta_io \GPS_TX_READY~input (
	.datain(gnd),
	.oe(gnd),
	.padio(GPS_TX_READY),
	.combout(\GPS_TX_READY~input_o ));
defparam \GPS_TX_READY~input .coord_x = 6;
defparam \GPS_TX_READY~input .coord_y = 4;
defparam \GPS_TX_READY~input .coord_z = 4;
defparam \GPS_TX_READY~input .PRG_DELAYB = 1'b1;
defparam \GPS_TX_READY~input .RX_SEL = 1'b0;
defparam \GPS_TX_READY~input .PDCNTL = 2'b01;
defparam \GPS_TX_READY~input .NDCNTL = 2'b01;
defparam \GPS_TX_READY~input .PRG_SLR = 1'b1;
defparam \GPS_TX_READY~input .CFG_KEEP = 2'b00;
defparam \GPS_TX_READY~input .PU = 4'b1000;

alta_io \LCD_BACKLIGHT~output (
	.datain(\lcd_backlight_q~q ),
	.oe(vcc),
	.padio(LCD_BACKLIGHT),
	.combout());
defparam \LCD_BACKLIGHT~output .coord_x = 5;
defparam \LCD_BACKLIGHT~output .coord_y = 4;
defparam \LCD_BACKLIGHT~output .coord_z = 4;
defparam \LCD_BACKLIGHT~output .PRG_DELAYB = 1'b1;
defparam \LCD_BACKLIGHT~output .RX_SEL = 1'b0;
defparam \LCD_BACKLIGHT~output .PDCNTL = 2'b01;
defparam \LCD_BACKLIGHT~output .NDCNTL = 2'b01;
defparam \LCD_BACKLIGHT~output .PRG_SLR = 1'b1;
defparam \LCD_BACKLIGHT~output .CFG_KEEP = 2'b00;
defparam \LCD_BACKLIGHT~output .PU = 4'b0000;

alta_io \LCD_DB[0]~output (
	.datain(\MCU_D[0]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[0]),
	.combout(\LCD_DB[0]~input_o ));
defparam \LCD_DB[0]~output .coord_x = 2;
defparam \LCD_DB[0]~output .coord_y = 4;
defparam \LCD_DB[0]~output .coord_z = 1;
defparam \LCD_DB[0]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[0]~output .RX_SEL = 1'b0;
defparam \LCD_DB[0]~output .PDCNTL = 2'b01;
defparam \LCD_DB[0]~output .NDCNTL = 2'b01;
defparam \LCD_DB[0]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[0]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[0]~output .PU = 4'b1000;

alta_io \LCD_DB[10]~output (
	.datain(lcd_data_out_q[2]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[10]),
	.combout(\LCD_DB[10]~input_o ));
defparam \LCD_DB[10]~output .coord_x = 4;
defparam \LCD_DB[10]~output .coord_y = 4;
defparam \LCD_DB[10]~output .coord_z = 0;
defparam \LCD_DB[10]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[10]~output .RX_SEL = 1'b0;
defparam \LCD_DB[10]~output .PDCNTL = 2'b01;
defparam \LCD_DB[10]~output .NDCNTL = 2'b01;
defparam \LCD_DB[10]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[10]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[10]~output .PU = 4'b1000;

alta_io \LCD_DB[11]~output (
	.datain(lcd_data_out_q[3]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[11]),
	.combout(\LCD_DB[11]~input_o ));
defparam \LCD_DB[11]~output .coord_x = 4;
defparam \LCD_DB[11]~output .coord_y = 4;
defparam \LCD_DB[11]~output .coord_z = 1;
defparam \LCD_DB[11]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[11]~output .RX_SEL = 1'b0;
defparam \LCD_DB[11]~output .PDCNTL = 2'b01;
defparam \LCD_DB[11]~output .NDCNTL = 2'b01;
defparam \LCD_DB[11]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[11]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[11]~output .PU = 4'b1000;

alta_io \LCD_DB[12]~output (
	.datain(lcd_data_out_q[4]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[12]),
	.combout(\LCD_DB[12]~input_o ));
defparam \LCD_DB[12]~output .coord_x = 4;
defparam \LCD_DB[12]~output .coord_y = 4;
defparam \LCD_DB[12]~output .coord_z = 2;
defparam \LCD_DB[12]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[12]~output .RX_SEL = 1'b0;
defparam \LCD_DB[12]~output .PDCNTL = 2'b01;
defparam \LCD_DB[12]~output .NDCNTL = 2'b01;
defparam \LCD_DB[12]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[12]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[12]~output .PU = 4'b1000;

alta_io \LCD_DB[13]~output (
	.datain(lcd_data_out_q[5]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[13]),
	.combout(\LCD_DB[13]~input_o ));
defparam \LCD_DB[13]~output .coord_x = 4;
defparam \LCD_DB[13]~output .coord_y = 4;
defparam \LCD_DB[13]~output .coord_z = 3;
defparam \LCD_DB[13]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[13]~output .RX_SEL = 1'b0;
defparam \LCD_DB[13]~output .PDCNTL = 2'b01;
defparam \LCD_DB[13]~output .NDCNTL = 2'b01;
defparam \LCD_DB[13]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[13]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[13]~output .PU = 4'b1000;

alta_io \LCD_DB[14]~output (
	.datain(lcd_data_out_q[6]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[14]),
	.combout(\LCD_DB[14]~input_o ));
defparam \LCD_DB[14]~output .coord_x = 4;
defparam \LCD_DB[14]~output .coord_y = 4;
defparam \LCD_DB[14]~output .coord_z = 4;
defparam \LCD_DB[14]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[14]~output .RX_SEL = 1'b0;
defparam \LCD_DB[14]~output .PDCNTL = 2'b01;
defparam \LCD_DB[14]~output .NDCNTL = 2'b01;
defparam \LCD_DB[14]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[14]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[14]~output .PU = 4'b1000;

alta_io \LCD_DB[15]~output (
	.datain(lcd_data_out_q[7]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[15]),
	.combout(\LCD_DB[15]~input_o ));
defparam \LCD_DB[15]~output .coord_x = 5;
defparam \LCD_DB[15]~output .coord_y = 4;
defparam \LCD_DB[15]~output .coord_z = 0;
defparam \LCD_DB[15]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[15]~output .RX_SEL = 1'b0;
defparam \LCD_DB[15]~output .PDCNTL = 2'b01;
defparam \LCD_DB[15]~output .NDCNTL = 2'b01;
defparam \LCD_DB[15]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[15]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[15]~output .PU = 4'b1000;

alta_io \LCD_DB[1]~output (
	.datain(\MCU_D[1]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[1]),
	.combout(\LCD_DB[1]~input_o ));
defparam \LCD_DB[1]~output .coord_x = 2;
defparam \LCD_DB[1]~output .coord_y = 4;
defparam \LCD_DB[1]~output .coord_z = 2;
defparam \LCD_DB[1]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[1]~output .RX_SEL = 1'b0;
defparam \LCD_DB[1]~output .PDCNTL = 2'b01;
defparam \LCD_DB[1]~output .NDCNTL = 2'b01;
defparam \LCD_DB[1]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[1]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[1]~output .PU = 4'b1000;

alta_io \LCD_DB[2]~output (
	.datain(\MCU_D[2]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[2]),
	.combout(\LCD_DB[2]~input_o ));
defparam \LCD_DB[2]~output .coord_x = 2;
defparam \LCD_DB[2]~output .coord_y = 4;
defparam \LCD_DB[2]~output .coord_z = 3;
defparam \LCD_DB[2]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[2]~output .RX_SEL = 1'b0;
defparam \LCD_DB[2]~output .PDCNTL = 2'b01;
defparam \LCD_DB[2]~output .NDCNTL = 2'b01;
defparam \LCD_DB[2]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[2]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[2]~output .PU = 4'b1000;

alta_io \LCD_DB[3]~output (
	.datain(\MCU_D[3]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[3]),
	.combout(\LCD_DB[3]~input_o ));
defparam \LCD_DB[3]~output .coord_x = 2;
defparam \LCD_DB[3]~output .coord_y = 4;
defparam \LCD_DB[3]~output .coord_z = 4;
defparam \LCD_DB[3]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[3]~output .RX_SEL = 1'b0;
defparam \LCD_DB[3]~output .PDCNTL = 2'b01;
defparam \LCD_DB[3]~output .NDCNTL = 2'b01;
defparam \LCD_DB[3]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[3]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[3]~output .PU = 4'b1000;

alta_io \LCD_DB[4]~output (
	.datain(\MCU_D[4]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[4]),
	.combout(\LCD_DB[4]~input_o ));
defparam \LCD_DB[4]~output .coord_x = 2;
defparam \LCD_DB[4]~output .coord_y = 4;
defparam \LCD_DB[4]~output .coord_z = 5;
defparam \LCD_DB[4]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[4]~output .RX_SEL = 1'b0;
defparam \LCD_DB[4]~output .PDCNTL = 2'b01;
defparam \LCD_DB[4]~output .NDCNTL = 2'b01;
defparam \LCD_DB[4]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[4]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[4]~output .PU = 4'b1000;

alta_io \LCD_DB[5]~output (
	.datain(\MCU_D[5]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[5]),
	.combout(\LCD_DB[5]~input_o ));
defparam \LCD_DB[5]~output .coord_x = 3;
defparam \LCD_DB[5]~output .coord_y = 4;
defparam \LCD_DB[5]~output .coord_z = 0;
defparam \LCD_DB[5]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[5]~output .RX_SEL = 1'b0;
defparam \LCD_DB[5]~output .PDCNTL = 2'b01;
defparam \LCD_DB[5]~output .NDCNTL = 2'b01;
defparam \LCD_DB[5]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[5]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[5]~output .PU = 4'b1000;

alta_io \LCD_DB[6]~output (
	.datain(\MCU_D[6]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[6]),
	.combout(\LCD_DB[6]~input_o ));
defparam \LCD_DB[6]~output .coord_x = 3;
defparam \LCD_DB[6]~output .coord_y = 4;
defparam \LCD_DB[6]~output .coord_z = 1;
defparam \LCD_DB[6]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[6]~output .RX_SEL = 1'b0;
defparam \LCD_DB[6]~output .PDCNTL = 2'b01;
defparam \LCD_DB[6]~output .NDCNTL = 2'b01;
defparam \LCD_DB[6]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[6]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[6]~output .PU = 4'b1000;

alta_io \LCD_DB[7]~output (
	.datain(\MCU_D[7]~input_o ),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[7]),
	.combout(\LCD_DB[7]~input_o ));
defparam \LCD_DB[7]~output .coord_x = 3;
defparam \LCD_DB[7]~output .coord_y = 4;
defparam \LCD_DB[7]~output .coord_z = 2;
defparam \LCD_DB[7]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[7]~output .RX_SEL = 1'b0;
defparam \LCD_DB[7]~output .PDCNTL = 2'b01;
defparam \LCD_DB[7]~output .NDCNTL = 2'b01;
defparam \LCD_DB[7]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[7]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[7]~output .PU = 4'b1000;

alta_io \LCD_DB[8]~output (
	.datain(lcd_data_out_q[0]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[8]),
	.combout(\LCD_DB[8]~input_o ));
defparam \LCD_DB[8]~output .coord_x = 3;
defparam \LCD_DB[8]~output .coord_y = 4;
defparam \LCD_DB[8]~output .coord_z = 3;
defparam \LCD_DB[8]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[8]~output .RX_SEL = 1'b0;
defparam \LCD_DB[8]~output .PDCNTL = 2'b01;
defparam \LCD_DB[8]~output .NDCNTL = 2'b01;
defparam \LCD_DB[8]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[8]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[8]~output .PU = 4'b1000;

alta_io \LCD_DB[9]~output (
	.datain(lcd_data_out_q[1]),
	.oe(\MCU_LCD_RDX~input_o ),
	.padio(LCD_DB[9]),
	.combout(\LCD_DB[9]~input_o ));
defparam \LCD_DB[9]~output .coord_x = 3;
defparam \LCD_DB[9]~output .coord_y = 4;
defparam \LCD_DB[9]~output .coord_z = 4;
defparam \LCD_DB[9]~output .PRG_DELAYB = 1'b1;
defparam \LCD_DB[9]~output .RX_SEL = 1'b0;
defparam \LCD_DB[9]~output .PDCNTL = 2'b01;
defparam \LCD_DB[9]~output .NDCNTL = 2'b01;
defparam \LCD_DB[9]~output .PRG_SLR = 1'b1;
defparam \LCD_DB[9]~output .CFG_KEEP = 2'b00;
defparam \LCD_DB[9]~output .PU = 4'b1000;

alta_io \LCD_RDX~output (
	.datain(\MCU_LCD_RDX~input_o ),
	.oe(vcc),
	.padio(LCD_RDX),
	.combout());
defparam \LCD_RDX~output .coord_x = 1;
defparam \LCD_RDX~output .coord_y = 4;
defparam \LCD_RDX~output .coord_z = 1;
defparam \LCD_RDX~output .PRG_DELAYB = 1'b1;
defparam \LCD_RDX~output .RX_SEL = 1'b0;
defparam \LCD_RDX~output .PDCNTL = 2'b01;
defparam \LCD_RDX~output .NDCNTL = 2'b01;
defparam \LCD_RDX~output .PRG_SLR = 1'b1;
defparam \LCD_RDX~output .CFG_KEEP = 2'b00;
defparam \LCD_RDX~output .PU = 4'b0000;

alta_io \LCD_RESETX~output (
	.datain(\lcd_reset_q~q ),
	.oe(vcc),
	.padio(LCD_RESETX),
	.combout());
defparam \LCD_RESETX~output .coord_x = 2;
defparam \LCD_RESETX~output .coord_y = 4;
defparam \LCD_RESETX~output .coord_z = 0;
defparam \LCD_RESETX~output .PRG_DELAYB = 1'b1;
defparam \LCD_RESETX~output .RX_SEL = 1'b0;
defparam \LCD_RESETX~output .PDCNTL = 2'b01;
defparam \LCD_RESETX~output .NDCNTL = 2'b01;
defparam \LCD_RESETX~output .PRG_SLR = 1'b1;
defparam \LCD_RESETX~output .CFG_KEEP = 2'b00;
defparam \LCD_RESETX~output .PU = 4'b0000;

alta_io \LCD_RS~output (
	.datain(\MCU_ADDR~input_o ),
	.oe(vcc),
	.padio(LCD_RS),
	.combout());
defparam \LCD_RS~output .coord_x = 0;
defparam \LCD_RS~output .coord_y = 3;
defparam \LCD_RS~output .coord_z = 0;
defparam \LCD_RS~output .PRG_DELAYB = 1'b1;
defparam \LCD_RS~output .RX_SEL = 1'b0;
defparam \LCD_RS~output .PDCNTL = 2'b01;
defparam \LCD_RS~output .NDCNTL = 2'b01;
defparam \LCD_RS~output .PRG_SLR = 1'b1;
defparam \LCD_RS~output .CFG_KEEP = 2'b00;
defparam \LCD_RS~output .PU = 4'b0000;

alta_io \LCD_TE~input (
	.datain(gnd),
	.oe(gnd),
	.padio(LCD_TE),
	.combout(\LCD_TE~input_o ));
defparam \LCD_TE~input .coord_x = 0;
defparam \LCD_TE~input .coord_y = 3;
defparam \LCD_TE~input .coord_z = 1;
defparam \LCD_TE~input .PRG_DELAYB = 1'b1;
defparam \LCD_TE~input .RX_SEL = 1'b0;
defparam \LCD_TE~input .PDCNTL = 2'b01;
defparam \LCD_TE~input .NDCNTL = 2'b01;
defparam \LCD_TE~input .PRG_SLR = 1'b1;
defparam \LCD_TE~input .CFG_KEEP = 2'b00;
defparam \LCD_TE~input .PU = 4'b0000;

alta_io \LCD_WRX~output (
	.datain(\MCU_LCD_WRX~input_o ),
	.oe(vcc),
	.padio(LCD_WRX),
	.combout());
defparam \LCD_WRX~output .coord_x = 1;
defparam \LCD_WRX~output .coord_y = 4;
defparam \LCD_WRX~output .coord_z = 0;
defparam \LCD_WRX~output .PRG_DELAYB = 1'b1;
defparam \LCD_WRX~output .RX_SEL = 1'b0;
defparam \LCD_WRX~output .PDCNTL = 2'b01;
defparam \LCD_WRX~output .NDCNTL = 2'b01;
defparam \LCD_WRX~output .PRG_SLR = 1'b1;
defparam \LCD_WRX~output .CFG_KEEP = 2'b00;
defparam \LCD_WRX~output .PU = 4'b0000;

alta_io \MCU_ADDR~input (
	.datain(gnd),
	.oe(gnd),
	.padio(MCU_ADDR),
	.combout(\MCU_ADDR~input_o ));
defparam \MCU_ADDR~input .coord_x = 5;
defparam \MCU_ADDR~input .coord_y = 0;
defparam \MCU_ADDR~input .coord_z = 3;
defparam \MCU_ADDR~input .PRG_DELAYB = 1'b1;
defparam \MCU_ADDR~input .RX_SEL = 1'b0;
defparam \MCU_ADDR~input .PDCNTL = 2'b01;
defparam \MCU_ADDR~input .NDCNTL = 2'b01;
defparam \MCU_ADDR~input .PRG_SLR = 1'b1;
defparam \MCU_ADDR~input .CFG_KEEP = 2'b00;
defparam \MCU_ADDR~input .PU = 4'b1000;

alta_io \MCU_DIR~input (
	.datain(gnd),
	.oe(gnd),
	.padio(MCU_DIR),
	.combout(\MCU_DIR~input_o ));
defparam \MCU_DIR~input .coord_x = 7;
defparam \MCU_DIR~input .coord_y = 3;
defparam \MCU_DIR~input .coord_z = 1;
defparam \MCU_DIR~input .PRG_DELAYB = 1'b1;
defparam \MCU_DIR~input .RX_SEL = 1'b0;
defparam \MCU_DIR~input .PDCNTL = 2'b01;
defparam \MCU_DIR~input .NDCNTL = 2'b01;
defparam \MCU_DIR~input .PRG_SLR = 1'b1;
defparam \MCU_DIR~input .CFG_KEEP = 2'b00;
defparam \MCU_DIR~input .PU = 4'b1000;

alta_io \MCU_D[0]~output (
	.datain(\mcu_data_out[0]~1_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[0]),
	.combout(\MCU_D[0]~input_o ));
defparam \MCU_D[0]~output .coord_x = 4;
defparam \MCU_D[0]~output .coord_y = 0;
defparam \MCU_D[0]~output .coord_z = 2;
defparam \MCU_D[0]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[0]~output .RX_SEL = 1'b0;
defparam \MCU_D[0]~output .PDCNTL = 2'b01;
defparam \MCU_D[0]~output .NDCNTL = 2'b01;
defparam \MCU_D[0]~output .PRG_SLR = 1'b1;
defparam \MCU_D[0]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[0]~output .PU = 4'b1000;

alta_io \MCU_D[1]~output (
	.datain(\mcu_data_out[1]~3_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[1]),
	.combout(\MCU_D[1]~input_o ));
defparam \MCU_D[1]~output .coord_x = 4;
defparam \MCU_D[1]~output .coord_y = 0;
defparam \MCU_D[1]~output .coord_z = 3;
defparam \MCU_D[1]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[1]~output .RX_SEL = 1'b0;
defparam \MCU_D[1]~output .PDCNTL = 2'b01;
defparam \MCU_D[1]~output .NDCNTL = 2'b01;
defparam \MCU_D[1]~output .PRG_SLR = 1'b1;
defparam \MCU_D[1]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[1]~output .PU = 4'b1000;

alta_io \MCU_D[2]~output (
	.datain(\mcu_data_out[2]~5_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[2]),
	.combout(\MCU_D[2]~input_o ));
defparam \MCU_D[2]~output .coord_x = 4;
defparam \MCU_D[2]~output .coord_y = 0;
defparam \MCU_D[2]~output .coord_z = 0;
defparam \MCU_D[2]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[2]~output .RX_SEL = 1'b0;
defparam \MCU_D[2]~output .PDCNTL = 2'b01;
defparam \MCU_D[2]~output .NDCNTL = 2'b01;
defparam \MCU_D[2]~output .PRG_SLR = 1'b1;
defparam \MCU_D[2]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[2]~output .PU = 4'b1000;

alta_io \MCU_D[3]~output (
	.datain(\mcu_data_out[3]~7_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[3]),
	.combout(\MCU_D[3]~input_o ));
defparam \MCU_D[3]~output .coord_x = 3;
defparam \MCU_D[3]~output .coord_y = 0;
defparam \MCU_D[3]~output .coord_z = 4;
defparam \MCU_D[3]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[3]~output .RX_SEL = 1'b0;
defparam \MCU_D[3]~output .PDCNTL = 2'b01;
defparam \MCU_D[3]~output .NDCNTL = 2'b01;
defparam \MCU_D[3]~output .PRG_SLR = 1'b1;
defparam \MCU_D[3]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[3]~output .PU = 4'b1000;

alta_io \MCU_D[4]~output (
	.datain(\mcu_data_out[4]~9_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[4]),
	.combout(\MCU_D[4]~input_o ));
defparam \MCU_D[4]~output .coord_x = 3;
defparam \MCU_D[4]~output .coord_y = 0;
defparam \MCU_D[4]~output .coord_z = 2;
defparam \MCU_D[4]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[4]~output .RX_SEL = 1'b0;
defparam \MCU_D[4]~output .PDCNTL = 2'b01;
defparam \MCU_D[4]~output .NDCNTL = 2'b01;
defparam \MCU_D[4]~output .PRG_SLR = 1'b1;
defparam \MCU_D[4]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[4]~output .PU = 4'b1000;

alta_io \MCU_D[5]~output (
	.datain(\mcu_data_out[5]~11_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[5]),
	.combout(\MCU_D[5]~input_o ));
defparam \MCU_D[5]~output .coord_x = 3;
defparam \MCU_D[5]~output .coord_y = 0;
defparam \MCU_D[5]~output .coord_z = 3;
defparam \MCU_D[5]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[5]~output .RX_SEL = 1'b0;
defparam \MCU_D[5]~output .PDCNTL = 2'b01;
defparam \MCU_D[5]~output .NDCNTL = 2'b01;
defparam \MCU_D[5]~output .PRG_SLR = 1'b1;
defparam \MCU_D[5]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[5]~output .PU = 4'b1000;

alta_io \MCU_D[6]~output (
	.datain(\mcu_data_out[6]~13_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[6]),
	.combout(\MCU_D[6]~input_o ));
defparam \MCU_D[6]~output .coord_x = 3;
defparam \MCU_D[6]~output .coord_y = 0;
defparam \MCU_D[6]~output .coord_z = 1;
defparam \MCU_D[6]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[6]~output .RX_SEL = 1'b0;
defparam \MCU_D[6]~output .PDCNTL = 2'b01;
defparam \MCU_D[6]~output .NDCNTL = 2'b01;
defparam \MCU_D[6]~output .PRG_SLR = 1'b1;
defparam \MCU_D[6]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[6]~output .PU = 4'b1000;

alta_io \MCU_D[7]~output (
	.datain(\mcu_data_out[7]~15_combout ),
	.oe(\MCU_DIR~input_o ),
	.padio(MCU_D[7]),
	.combout(\MCU_D[7]~input_o ));
defparam \MCU_D[7]~output .coord_x = 3;
defparam \MCU_D[7]~output .coord_y = 0;
defparam \MCU_D[7]~output .coord_z = 0;
defparam \MCU_D[7]~output .PRG_DELAYB = 1'b1;
defparam \MCU_D[7]~output .RX_SEL = 1'b0;
defparam \MCU_D[7]~output .PDCNTL = 2'b01;
defparam \MCU_D[7]~output .NDCNTL = 2'b01;
defparam \MCU_D[7]~output .PRG_SLR = 1'b1;
defparam \MCU_D[7]~output .CFG_KEEP = 2'b00;
defparam \MCU_D[7]~output .PU = 4'b1000;

alta_io \MCU_IO_STBX~input (
	.datain(gnd),
	.oe(gnd),
	.padio(MCU_IO_STBX),
	.combout(\MCU_IO_STBX~input_o ));
defparam \MCU_IO_STBX~input .coord_x = 5;
defparam \MCU_IO_STBX~input .coord_y = 0;
defparam \MCU_IO_STBX~input .coord_z = 2;
defparam \MCU_IO_STBX~input .PRG_DELAYB = 1'b1;
defparam \MCU_IO_STBX~input .RX_SEL = 1'b0;
defparam \MCU_IO_STBX~input .PDCNTL = 2'b01;
defparam \MCU_IO_STBX~input .NDCNTL = 2'b01;
defparam \MCU_IO_STBX~input .PRG_SLR = 1'b1;
defparam \MCU_IO_STBX~input .CFG_KEEP = 2'b00;
defparam \MCU_IO_STBX~input .PU = 4'b1000;

alta_io_gclk \MCU_IO_STBX~inputclkctrl (
	.inclk(\MCU_IO_STBX~input_o ),
	.outclk(\MCU_IO_STBX~inputclkctrl_outclk ));
defparam \MCU_IO_STBX~inputclkctrl .coord_x = 0;
defparam \MCU_IO_STBX~inputclkctrl .coord_y = 2;
defparam \MCU_IO_STBX~inputclkctrl .coord_z = 1;

alta_io \MCU_LCD_RDX~input (
	.datain(gnd),
	.oe(gnd),
	.padio(MCU_LCD_RDX),
	.combout(\MCU_LCD_RDX~input_o ));
defparam \MCU_LCD_RDX~input .coord_x = 5;
defparam \MCU_LCD_RDX~input .coord_y = 0;
defparam \MCU_LCD_RDX~input .coord_z = 0;
defparam \MCU_LCD_RDX~input .PRG_DELAYB = 1'b1;
defparam \MCU_LCD_RDX~input .RX_SEL = 1'b0;
defparam \MCU_LCD_RDX~input .PDCNTL = 2'b01;
defparam \MCU_LCD_RDX~input .NDCNTL = 2'b01;
defparam \MCU_LCD_RDX~input .PRG_SLR = 1'b1;
defparam \MCU_LCD_RDX~input .CFG_KEEP = 2'b00;
defparam \MCU_LCD_RDX~input .PU = 4'b1000;

alta_io_gclk \MCU_LCD_RDX~inputclkctrl (
	.inclk(\MCU_LCD_RDX~input_o ),
	.outclk(\MCU_LCD_RDX~inputclkctrl_outclk ));
defparam \MCU_LCD_RDX~inputclkctrl .coord_x = 7;
defparam \MCU_LCD_RDX~inputclkctrl .coord_y = 2;
defparam \MCU_LCD_RDX~inputclkctrl .coord_z = 0;

alta_io \MCU_LCD_TE~output (
	.datain(\LCD_TE~input_o ),
	.oe(vcc),
	.padio(MCU_LCD_TE),
	.combout());
defparam \MCU_LCD_TE~output .coord_x = 5;
defparam \MCU_LCD_TE~output .coord_y = 0;
defparam \MCU_LCD_TE~output .coord_z = 1;
defparam \MCU_LCD_TE~output .PRG_DELAYB = 1'b1;
defparam \MCU_LCD_TE~output .RX_SEL = 1'b0;
defparam \MCU_LCD_TE~output .PDCNTL = 2'b01;
defparam \MCU_LCD_TE~output .NDCNTL = 2'b01;
defparam \MCU_LCD_TE~output .PRG_SLR = 1'b1;
defparam \MCU_LCD_TE~output .CFG_KEEP = 2'b00;
defparam \MCU_LCD_TE~output .PU = 4'b0000;

alta_io \MCU_LCD_WRX~input (
	.datain(gnd),
	.oe(gnd),
	.padio(MCU_LCD_WRX),
	.combout(\MCU_LCD_WRX~input_o ));
defparam \MCU_LCD_WRX~input .coord_x = 7;
defparam \MCU_LCD_WRX~input .coord_y = 3;
defparam \MCU_LCD_WRX~input .coord_z = 2;
defparam \MCU_LCD_WRX~input .PRG_DELAYB = 1'b1;
defparam \MCU_LCD_WRX~input .RX_SEL = 1'b0;
defparam \MCU_LCD_WRX~input .PDCNTL = 2'b01;
defparam \MCU_LCD_WRX~input .NDCNTL = 2'b01;
defparam \MCU_LCD_WRX~input .PRG_SLR = 1'b1;
defparam \MCU_LCD_WRX~input .CFG_KEEP = 2'b00;
defparam \MCU_LCD_WRX~input .PU = 4'b0000;

alta_io_gclk \MCU_LCD_WRX~inputclkctrl (
	.inclk(\MCU_LCD_WRX~input_o ),
	.outclk(\MCU_LCD_WRX~inputclkctrl_outclk ));
defparam \MCU_LCD_WRX~inputclkctrl .coord_x = 7;
defparam \MCU_LCD_WRX~inputclkctrl .coord_y = 2;
defparam \MCU_LCD_WRX~inputclkctrl .coord_z = 1;

alta_io \MCU_P2_8~input (
	.datain(gnd),
	.oe(gnd),
	.padio(MCU_P2_8),
	.combout(\MCU_P2_8~input_o ));
defparam \MCU_P2_8~input .coord_x = 5;
defparam \MCU_P2_8~input .coord_y = 0;
defparam \MCU_P2_8~input .coord_z = 4;
defparam \MCU_P2_8~input .PRG_DELAYB = 1'b1;
defparam \MCU_P2_8~input .RX_SEL = 1'b0;
defparam \MCU_P2_8~input .PDCNTL = 2'b01;
defparam \MCU_P2_8~input .NDCNTL = 2'b01;
defparam \MCU_P2_8~input .PRG_SLR = 1'b1;
defparam \MCU_P2_8~input .CFG_KEEP = 2'b00;
defparam \MCU_P2_8~input .PU = 4'b0000;

alta_io \REF_EN~output (
	.datain(\ref_en_q~q ),
	.oe(vcc),
	.padio(REF_EN),
	.combout());
defparam \REF_EN~output .coord_x = 7;
defparam \REF_EN~output .coord_y = 2;
defparam \REF_EN~output .coord_z = 3;
defparam \REF_EN~output .PRG_DELAYB = 1'b1;
defparam \REF_EN~output .RX_SEL = 1'b0;
defparam \REF_EN~output .PDCNTL = 2'b01;
defparam \REF_EN~output .NDCNTL = 2'b01;
defparam \REF_EN~output .PRG_SLR = 1'b1;
defparam \REF_EN~output .CFG_KEEP = 2'b00;
defparam \REF_EN~output .PU = 4'b0000;

alta_io \SW_D~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_D),
	.combout(\SW_D~input_o ));
defparam \SW_D~input .coord_x = 0;
defparam \SW_D~input .coord_y = 2;
defparam \SW_D~input .coord_z = 1;
defparam \SW_D~input .PRG_DELAYB = 1'b1;
defparam \SW_D~input .RX_SEL = 1'b1;
defparam \SW_D~input .PDCNTL = 2'b01;
defparam \SW_D~input .NDCNTL = 2'b01;
defparam \SW_D~input .PRG_SLR = 1'b1;
defparam \SW_D~input .CFG_KEEP = 2'b00;
defparam \SW_D~input .PU = 4'b1000;

alta_io \SW_L~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_L),
	.combout(\SW_L~input_o ));
defparam \SW_L~input .coord_x = 4;
defparam \SW_L~input .coord_y = 0;
defparam \SW_L~input .coord_z = 4;
defparam \SW_L~input .PRG_DELAYB = 1'b1;
defparam \SW_L~input .RX_SEL = 1'b1;
defparam \SW_L~input .PDCNTL = 2'b01;
defparam \SW_L~input .NDCNTL = 2'b01;
defparam \SW_L~input .PRG_SLR = 1'b1;
defparam \SW_L~input .CFG_KEEP = 2'b00;
defparam \SW_L~input .PU = 4'b1000;

alta_io \SW_ROT_A~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_ROT_A),
	.combout(\SW_ROT_A~input_o ));
defparam \SW_ROT_A~input .coord_x = 0;
defparam \SW_ROT_A~input .coord_y = 2;
defparam \SW_ROT_A~input .coord_z = 2;
defparam \SW_ROT_A~input .PRG_DELAYB = 1'b1;
defparam \SW_ROT_A~input .RX_SEL = 1'b1;
defparam \SW_ROT_A~input .PDCNTL = 2'b01;
defparam \SW_ROT_A~input .NDCNTL = 2'b01;
defparam \SW_ROT_A~input .PRG_SLR = 1'b1;
defparam \SW_ROT_A~input .CFG_KEEP = 2'b00;
defparam \SW_ROT_A~input .PU = 4'b1000;

alta_io \SW_ROT_B~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_ROT_B),
	.combout(\SW_ROT_B~input_o ));
defparam \SW_ROT_B~input .coord_x = 0;
defparam \SW_ROT_B~input .coord_y = 2;
defparam \SW_ROT_B~input .coord_z = 3;
defparam \SW_ROT_B~input .PRG_DELAYB = 1'b1;
defparam \SW_ROT_B~input .RX_SEL = 1'b1;
defparam \SW_ROT_B~input .PDCNTL = 2'b01;
defparam \SW_ROT_B~input .NDCNTL = 2'b01;
defparam \SW_ROT_B~input .PRG_SLR = 1'b1;
defparam \SW_ROT_B~input .CFG_KEEP = 2'b00;
defparam \SW_ROT_B~input .PU = 4'b1000;

alta_io \SW_R~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_R),
	.combout(\SW_R~input_o ));
defparam \SW_R~input .coord_x = 0;
defparam \SW_R~input .coord_y = 2;
defparam \SW_R~input .coord_z = 0;
defparam \SW_R~input .PRG_DELAYB = 1'b1;
defparam \SW_R~input .RX_SEL = 1'b1;
defparam \SW_R~input .PDCNTL = 2'b01;
defparam \SW_R~input .NDCNTL = 2'b01;
defparam \SW_R~input .PRG_SLR = 1'b1;
defparam \SW_R~input .CFG_KEEP = 2'b00;
defparam \SW_R~input .PU = 4'b1000;

alta_io \SW_SEL~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_SEL),
	.combout(\SW_SEL~input_o ));
defparam \SW_SEL~input .coord_x = 0;
defparam \SW_SEL~input .coord_y = 2;
defparam \SW_SEL~input .coord_z = 4;
defparam \SW_SEL~input .PRG_DELAYB = 1'b1;
defparam \SW_SEL~input .RX_SEL = 1'b1;
defparam \SW_SEL~input .PDCNTL = 2'b01;
defparam \SW_SEL~input .NDCNTL = 2'b01;
defparam \SW_SEL~input .PRG_SLR = 1'b1;
defparam \SW_SEL~input .CFG_KEEP = 2'b00;
defparam \SW_SEL~input .PU = 4'b1000;

alta_io \SW_U~input (
	.datain(gnd),
	.oe(gnd),
	.padio(SW_U),
	.combout(\SW_U~input_o ));
defparam \SW_U~input .coord_x = 4;
defparam \SW_U~input .coord_y = 0;
defparam \SW_U~input .coord_z = 1;
defparam \SW_U~input .PRG_DELAYB = 1'b1;
defparam \SW_U~input .RX_SEL = 1'b1;
defparam \SW_U~input .PDCNTL = 2'b01;
defparam \SW_U~input .NDCNTL = 2'b01;
defparam \SW_U~input .PRG_SLR = 1'b1;
defparam \SW_U~input .CFG_KEEP = 2'b00;
defparam \SW_U~input .PU = 4'b1000;

alta_io \SYSOFF~output (
	.datain(\sysoff_q~q ),
	.oe(vcc),
	.padio(SYSOFF),
	.combout());
defparam \SYSOFF~output .coord_x = 6;
defparam \SYSOFF~output .coord_y = 0;
defparam \SYSOFF~output .coord_z = 1;
defparam \SYSOFF~output .PRG_DELAYB = 1'b1;
defparam \SYSOFF~output .RX_SEL = 1'b0;
defparam \SYSOFF~output .PDCNTL = 2'b11;
defparam \SYSOFF~output .NDCNTL = 2'b11;
defparam \SYSOFF~output .PRG_SLR = 1'b0;
defparam \SYSOFF~output .CFG_KEEP = 2'b00;
defparam \SYSOFF~output .PU = 4'b0000;

alta_io \TP_D~output (
	.datain(tp_q[2]),
	.oe(tp_q[6]),
	.padio(TP_D),
	.combout());
defparam \TP_D~output .coord_x = 0;
defparam \TP_D~output .coord_y = 3;
defparam \TP_D~output .coord_z = 4;
defparam \TP_D~output .PRG_DELAYB = 1'b1;
defparam \TP_D~output .RX_SEL = 1'b0;
defparam \TP_D~output .PDCNTL = 2'b11;
defparam \TP_D~output .NDCNTL = 2'b11;
defparam \TP_D~output .PRG_SLR = 1'b1;
defparam \TP_D~output .CFG_KEEP = 2'b00;
defparam \TP_D~output .PU = 4'b1000;

alta_io \TP_L~output (
	.datain(tp_q[1]),
	.oe(tp_q[5]),
	.padio(TP_L),
	.combout());
defparam \TP_L~output .coord_x = 0;
defparam \TP_L~output .coord_y = 3;
defparam \TP_L~output .coord_z = 5;
defparam \TP_L~output .PRG_DELAYB = 1'b1;
defparam \TP_L~output .RX_SEL = 1'b0;
defparam \TP_L~output .PDCNTL = 2'b11;
defparam \TP_L~output .NDCNTL = 2'b11;
defparam \TP_L~output .PRG_SLR = 1'b1;
defparam \TP_L~output .CFG_KEEP = 2'b00;
defparam \TP_L~output .PU = 4'b1000;

alta_io \TP_R~output (
	.datain(tp_q[0]),
	.oe(tp_q[4]),
	.padio(TP_R),
	.combout());
defparam \TP_R~output .coord_x = 0;
defparam \TP_R~output .coord_y = 3;
defparam \TP_R~output .coord_z = 2;
defparam \TP_R~output .PRG_DELAYB = 1'b1;
defparam \TP_R~output .RX_SEL = 1'b0;
defparam \TP_R~output .PDCNTL = 2'b11;
defparam \TP_R~output .NDCNTL = 2'b11;
defparam \TP_R~output .PRG_SLR = 1'b1;
defparam \TP_R~output .CFG_KEEP = 2'b00;
defparam \TP_R~output .PU = 4'b1000;

alta_io \TP_U~output (
	.datain(tp_q[3]),
	.oe(tp_q[7]),
	.padio(TP_U),
	.combout());
defparam \TP_U~output .coord_x = 0;
defparam \TP_U~output .coord_y = 3;
defparam \TP_U~output .coord_z = 6;
defparam \TP_U~output .PRG_DELAYB = 1'b1;
defparam \TP_U~output .RX_SEL = 1'b0;
defparam \TP_U~output .PDCNTL = 2'b11;
defparam \TP_U~output .NDCNTL = 2'b11;
defparam \TP_U~output .PRG_SLR = 1'b1;
defparam \TP_U~output .CFG_KEEP = 2'b00;
defparam \TP_U~output .PU = 4'b1000;

alta_asyncctrl asyncreset_ctrl_X1_Y15_N0(
	.Din(),
	.Dout(AsyncReset_X1_Y15_GND));
defparam asyncreset_ctrl_X1_Y15_N0.coord_x = 3;
defparam asyncreset_ctrl_X1_Y15_N0.coord_y = 3;
defparam asyncreset_ctrl_X1_Y15_N0.coord_z = 0;
defparam asyncreset_ctrl_X1_Y15_N0.AsyncCtrlMux = 2'b00;

alta_asyncctrl asyncreset_ctrl_X1_Y19_N0(
	.Din(),
	.Dout(AsyncReset_X1_Y19_GND));
defparam asyncreset_ctrl_X1_Y19_N0.coord_x = 3;
defparam asyncreset_ctrl_X1_Y19_N0.coord_y = 1;
defparam asyncreset_ctrl_X1_Y19_N0.coord_z = 0;
defparam asyncreset_ctrl_X1_Y19_N0.AsyncCtrlMux = 2'b00;

alta_asyncctrl asyncreset_ctrl_X1_Y20_N0(
	.Din(),
	.Dout(AsyncReset_X1_Y20_GND));
defparam asyncreset_ctrl_X1_Y20_N0.coord_x = 4;
defparam asyncreset_ctrl_X1_Y20_N0.coord_y = 3;
defparam asyncreset_ctrl_X1_Y20_N0.coord_z = 0;
defparam asyncreset_ctrl_X1_Y20_N0.AsyncCtrlMux = 2'b00;

alta_asyncctrl asyncreset_ctrl_X1_Y21_N0(
	.Din(),
	.Dout(AsyncReset_X1_Y21_GND));
defparam asyncreset_ctrl_X1_Y21_N0.coord_x = 5;
defparam asyncreset_ctrl_X1_Y21_N0.coord_y = 3;
defparam asyncreset_ctrl_X1_Y21_N0.coord_z = 0;
defparam asyncreset_ctrl_X1_Y21_N0.AsyncCtrlMux = 2'b00;

alta_asyncctrl asyncreset_ctrl_X1_Y24_N0(
	.Din(),
	.Dout(AsyncReset_X1_Y24_GND));
defparam asyncreset_ctrl_X1_Y24_N0.coord_x = 5;
defparam asyncreset_ctrl_X1_Y24_N0.coord_y = 2;
defparam asyncreset_ctrl_X1_Y24_N0.coord_z = 0;
defparam asyncreset_ctrl_X1_Y24_N0.AsyncCtrlMux = 2'b00;

alta_asyncctrl asyncreset_ctrl_X1_Y26_N0(
	.Din(),
	.Dout(AsyncReset_X1_Y26_GND));
defparam asyncreset_ctrl_X1_Y26_N0.coord_x = 3;
defparam asyncreset_ctrl_X1_Y26_N0.coord_y = 2;
defparam asyncreset_ctrl_X1_Y26_N0.coord_z = 0;
defparam asyncreset_ctrl_X1_Y26_N0.AsyncCtrlMux = 2'b00;

alta_slice audio_reset_q(
	.A(\MCU_D[1]~input_o ),
	.B(vcc),
	.C(vcc),
	.D(vcc),
	.Cin(),
	.Qin(\audio_reset_q~q ),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\audio_reset_q~0_combout ),
	.Cout(),
	.Q(\audio_reset_q~q ));
defparam audio_reset_q.coord_x = 3;
defparam audio_reset_q.coord_y = 3;
defparam audio_reset_q.coord_z = 12;
defparam audio_reset_q.mask = 16'h5555;
defparam audio_reset_q.modeMux = 1'b0;
defparam audio_reset_q.FeedbackMux = 1'b0;
defparam audio_reset_q.ShiftMux = 1'b0;
defparam audio_reset_q.BypassEn = 1'b0;
defparam audio_reset_q.CarryEnb = 1'b1;

alta_clkenctrl clken_ctrl_X1_Y15_N0(
	.ClkIn(\MCU_IO_STBX~inputclkctrl_outclk ),
	.ClkEn(\tp_q[3]~0_combout ),
	.ClkOut(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ));
defparam clken_ctrl_X1_Y15_N0.coord_x = 3;
defparam clken_ctrl_X1_Y15_N0.coord_y = 3;
defparam clken_ctrl_X1_Y15_N0.coord_z = 0;
defparam clken_ctrl_X1_Y15_N0.ClkMux = 2'b10;
defparam clken_ctrl_X1_Y15_N0.ClkEnMux = 2'b10;

alta_clkenctrl clken_ctrl_X1_Y15_N1(
	.ClkIn(\MCU_IO_STBX~inputclkctrl_outclk ),
	.ClkEn(\lcd_reset_q~0_combout ),
	.ClkOut(\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ));
defparam clken_ctrl_X1_Y15_N1.coord_x = 3;
defparam clken_ctrl_X1_Y15_N1.coord_y = 3;
defparam clken_ctrl_X1_Y15_N1.coord_z = 1;
defparam clken_ctrl_X1_Y15_N1.ClkMux = 2'b10;
defparam clken_ctrl_X1_Y15_N1.ClkEnMux = 2'b10;

alta_clkenctrl clken_ctrl_X1_Y19_N0(
	.ClkIn(\MCU_LCD_RDX~inputclkctrl_outclk ),
	.ClkEn(),
	.ClkOut(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y19_SIG_VCC ));
defparam clken_ctrl_X1_Y19_N0.coord_x = 3;
defparam clken_ctrl_X1_Y19_N0.coord_y = 1;
defparam clken_ctrl_X1_Y19_N0.coord_z = 0;
defparam clken_ctrl_X1_Y19_N0.ClkMux = 2'b10;
defparam clken_ctrl_X1_Y19_N0.ClkEnMux = 2'b01;

alta_clkenctrl clken_ctrl_X1_Y20_N0(
	.ClkIn(\MCU_LCD_WRX~inputclkctrl_outclk ),
	.ClkEn(),
	.ClkOut(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ));
defparam clken_ctrl_X1_Y20_N0.coord_x = 4;
defparam clken_ctrl_X1_Y20_N0.coord_y = 3;
defparam clken_ctrl_X1_Y20_N0.coord_z = 0;
defparam clken_ctrl_X1_Y20_N0.ClkMux = 2'b11;
defparam clken_ctrl_X1_Y20_N0.ClkEnMux = 2'b01;

alta_clkenctrl clken_ctrl_X1_Y21_N0(
	.ClkIn(\MCU_LCD_RDX~inputclkctrl_outclk ),
	.ClkEn(),
	.ClkOut(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y21_SIG_VCC ));
defparam clken_ctrl_X1_Y21_N0.coord_x = 5;
defparam clken_ctrl_X1_Y21_N0.coord_y = 3;
defparam clken_ctrl_X1_Y21_N0.coord_z = 0;
defparam clken_ctrl_X1_Y21_N0.ClkMux = 2'b10;
defparam clken_ctrl_X1_Y21_N0.ClkEnMux = 2'b01;

alta_clkenctrl clken_ctrl_X1_Y24_N0(
	.ClkIn(\MCU_LCD_RDX~inputclkctrl_outclk ),
	.ClkEn(),
	.ClkOut(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y24_SIG_VCC ));
defparam clken_ctrl_X1_Y24_N0.coord_x = 5;
defparam clken_ctrl_X1_Y24_N0.coord_y = 2;
defparam clken_ctrl_X1_Y24_N0.coord_z = 0;
defparam clken_ctrl_X1_Y24_N0.ClkMux = 2'b10;
defparam clken_ctrl_X1_Y24_N0.ClkEnMux = 2'b01;

alta_clkenctrl clken_ctrl_X1_Y26_N0(
	.ClkIn(\MCU_LCD_RDX~inputclkctrl_outclk ),
	.ClkEn(),
	.ClkOut(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y26_SIG_VCC ));
defparam clken_ctrl_X1_Y26_N0.coord_x = 3;
defparam clken_ctrl_X1_Y26_N0.coord_y = 2;
defparam clken_ctrl_X1_Y26_N0.coord_z = 0;
defparam clken_ctrl_X1_Y26_N0.ClkMux = 2'b10;
defparam clken_ctrl_X1_Y26_N0.ClkEnMux = 2'b01;

alta_clkenctrl clken_ctrl_X1_Y26_N1(
	.ClkIn(\MCU_LCD_WRX~inputclkctrl_outclk ),
	.ClkEn(),
	.ClkOut(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y26_INV_VCC ));
defparam clken_ctrl_X1_Y26_N1.coord_x = 3;
defparam clken_ctrl_X1_Y26_N1.coord_y = 2;
defparam clken_ctrl_X1_Y26_N1.coord_z = 1;
defparam clken_ctrl_X1_Y26_N1.ClkMux = 2'b11;
defparam clken_ctrl_X1_Y26_N1.ClkEnMux = 2'b01;

alta_slice lcd_backlight_q(
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[7]~input_o ),
	.Cin(),
	.Qin(\lcd_backlight_q~q ),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_backlight_q~feeder_combout ),
	.Cout(),
	.Q(\lcd_backlight_q~q ));
defparam lcd_backlight_q.coord_x = 3;
defparam lcd_backlight_q.coord_y = 3;
defparam lcd_backlight_q.coord_z = 8;
defparam lcd_backlight_q.mask = 16'hFF00;
defparam lcd_backlight_q.modeMux = 1'b0;
defparam lcd_backlight_q.FeedbackMux = 1'b0;
defparam lcd_backlight_q.ShiftMux = 1'b0;
defparam lcd_backlight_q.BypassEn = 1'b0;
defparam lcd_backlight_q.CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[0] (
	.A(vcc),
	.B(\LCD_DB[8]~input_o ),
	.C(\LCD_DB[0]~input_o ),
	.D(\MCU_LCD_RDX~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[0]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y19_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y19_GND),
	.SyncReset(SyncReset_X1_Y19_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y19_VCC),
	.LutOut(\mcu_data_out[0]~0_combout ),
	.Cout(),
	.Q(lcd_data_in_q[0]));
defparam \lcd_data_in_q[0] .coord_x = 3;
defparam \lcd_data_in_q[0] .coord_y = 1;
defparam \lcd_data_in_q[0] .coord_z = 15;
defparam \lcd_data_in_q[0] .mask = 16'hF0CC;
defparam \lcd_data_in_q[0] .modeMux = 1'b0;
defparam \lcd_data_in_q[0] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[0] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[0] .BypassEn = 1'b1;
defparam \lcd_data_in_q[0] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[1] (
	.A(vcc),
	.B(\MCU_LCD_RDX~input_o ),
	.C(\LCD_DB[1]~input_o ),
	.D(\LCD_DB[9]~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[1]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y21_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y21_GND),
	.SyncReset(SyncReset_X1_Y21_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y21_VCC),
	.LutOut(\mcu_data_out[1]~2_combout ),
	.Cout(),
	.Q(lcd_data_in_q[1]));
defparam \lcd_data_in_q[1] .coord_x = 5;
defparam \lcd_data_in_q[1] .coord_y = 3;
defparam \lcd_data_in_q[1] .coord_z = 5;
defparam \lcd_data_in_q[1] .mask = 16'hF3C0;
defparam \lcd_data_in_q[1] .modeMux = 1'b0;
defparam \lcd_data_in_q[1] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[1] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[1] .BypassEn = 1'b1;
defparam \lcd_data_in_q[1] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[2] (
	.A(vcc),
	.B(\LCD_DB[10]~input_o ),
	.C(\LCD_DB[2]~input_o ),
	.D(\MCU_LCD_RDX~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[2]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y24_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y24_GND),
	.SyncReset(SyncReset_X1_Y24_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y24_VCC),
	.LutOut(\mcu_data_out[2]~4_combout ),
	.Cout(),
	.Q(lcd_data_in_q[2]));
defparam \lcd_data_in_q[2] .coord_x = 5;
defparam \lcd_data_in_q[2] .coord_y = 2;
defparam \lcd_data_in_q[2] .coord_z = 15;
defparam \lcd_data_in_q[2] .mask = 16'hF0CC;
defparam \lcd_data_in_q[2] .modeMux = 1'b0;
defparam \lcd_data_in_q[2] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[2] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[2] .BypassEn = 1'b1;
defparam \lcd_data_in_q[2] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[3] (
	.A(\LCD_DB[11]~input_o ),
	.B(vcc),
	.C(\LCD_DB[3]~input_o ),
	.D(\MCU_LCD_RDX~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[3]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y26_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y26_GND),
	.SyncReset(SyncReset_X1_Y26_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y26_VCC),
	.LutOut(\mcu_data_out[3]~6_combout ),
	.Cout(),
	.Q(lcd_data_in_q[3]));
defparam \lcd_data_in_q[3] .coord_x = 3;
defparam \lcd_data_in_q[3] .coord_y = 2;
defparam \lcd_data_in_q[3] .coord_z = 3;
defparam \lcd_data_in_q[3] .mask = 16'hF0AA;
defparam \lcd_data_in_q[3] .modeMux = 1'b0;
defparam \lcd_data_in_q[3] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[3] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[3] .BypassEn = 1'b1;
defparam \lcd_data_in_q[3] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[4] (
	.A(vcc),
	.B(\MCU_LCD_RDX~input_o ),
	.C(\LCD_DB[4]~input_o ),
	.D(\LCD_DB[12]~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[4]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y26_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y26_GND),
	.SyncReset(SyncReset_X1_Y26_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y26_VCC),
	.LutOut(\mcu_data_out[4]~8_combout ),
	.Cout(),
	.Q(lcd_data_in_q[4]));
defparam \lcd_data_in_q[4] .coord_x = 3;
defparam \lcd_data_in_q[4] .coord_y = 2;
defparam \lcd_data_in_q[4] .coord_z = 5;
defparam \lcd_data_in_q[4] .mask = 16'hF3C0;
defparam \lcd_data_in_q[4] .modeMux = 1'b0;
defparam \lcd_data_in_q[4] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[4] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[4] .BypassEn = 1'b1;
defparam \lcd_data_in_q[4] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[5] (
	.A(\LCD_DB[13]~input_o ),
	.B(vcc),
	.C(\LCD_DB[5]~input_o ),
	.D(\MCU_LCD_RDX~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[5]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y26_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y26_GND),
	.SyncReset(SyncReset_X1_Y26_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y26_VCC),
	.LutOut(\mcu_data_out[5]~10_combout ),
	.Cout(),
	.Q(lcd_data_in_q[5]));
defparam \lcd_data_in_q[5] .coord_x = 3;
defparam \lcd_data_in_q[5] .coord_y = 2;
defparam \lcd_data_in_q[5] .coord_z = 4;
defparam \lcd_data_in_q[5] .mask = 16'hF0AA;
defparam \lcd_data_in_q[5] .modeMux = 1'b0;
defparam \lcd_data_in_q[5] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[5] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[5] .BypassEn = 1'b1;
defparam \lcd_data_in_q[5] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[6] (
	.A(vcc),
	.B(\LCD_DB[14]~input_o ),
	.C(\LCD_DB[6]~input_o ),
	.D(\MCU_LCD_RDX~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[6]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y19_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y19_GND),
	.SyncReset(SyncReset_X1_Y19_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y19_VCC),
	.LutOut(\mcu_data_out[6]~12_combout ),
	.Cout(),
	.Q(lcd_data_in_q[6]));
defparam \lcd_data_in_q[6] .coord_x = 3;
defparam \lcd_data_in_q[6] .coord_y = 1;
defparam \lcd_data_in_q[6] .coord_z = 2;
defparam \lcd_data_in_q[6] .mask = 16'hF0CC;
defparam \lcd_data_in_q[6] .modeMux = 1'b0;
defparam \lcd_data_in_q[6] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[6] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[6] .BypassEn = 1'b1;
defparam \lcd_data_in_q[6] .CarryEnb = 1'b1;

alta_slice \lcd_data_in_q[7] (
	.A(vcc),
	.B(\LCD_DB[15]~input_o ),
	.C(\LCD_DB[7]~input_o ),
	.D(\MCU_LCD_RDX~input_o ),
	.Cin(),
	.Qin(lcd_data_in_q[7]),
	.Clk(\MCU_LCD_RDX~inputclkctrl_outclk_X1_Y21_SIG_VCC ),
	.AsyncReset(AsyncReset_X1_Y21_GND),
	.SyncReset(SyncReset_X1_Y21_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y21_VCC),
	.LutOut(\mcu_data_out[7]~14_combout ),
	.Cout(),
	.Q(lcd_data_in_q[7]));
defparam \lcd_data_in_q[7] .coord_x = 5;
defparam \lcd_data_in_q[7] .coord_y = 3;
defparam \lcd_data_in_q[7] .coord_z = 9;
defparam \lcd_data_in_q[7] .mask = 16'hF0CC;
defparam \lcd_data_in_q[7] .modeMux = 1'b0;
defparam \lcd_data_in_q[7] .FeedbackMux = 1'b1;
defparam \lcd_data_in_q[7] .ShiftMux = 1'b0;
defparam \lcd_data_in_q[7] .BypassEn = 1'b1;
defparam \lcd_data_in_q[7] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[0] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[0]~input_o ),
	.Cin(),
	.Qin(lcd_data_out_q[0]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y20_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_data_out_q[0]~feeder_combout ),
	.Cout(),
	.Q(lcd_data_out_q[0]));
defparam \lcd_data_out_q[0] .coord_x = 4;
defparam \lcd_data_out_q[0] .coord_y = 3;
defparam \lcd_data_out_q[0] .coord_z = 15;
defparam \lcd_data_out_q[0] .mask = 16'hFF00;
defparam \lcd_data_out_q[0] .modeMux = 1'b0;
defparam \lcd_data_out_q[0] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[0] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[0] .BypassEn = 1'b0;
defparam \lcd_data_out_q[0] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[1] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[1]~input_o ),
	.Cin(),
	.Qin(lcd_data_out_q[1]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y20_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_data_out_q[1]~feeder_combout ),
	.Cout(),
	.Q(lcd_data_out_q[1]));
defparam \lcd_data_out_q[1] .coord_x = 4;
defparam \lcd_data_out_q[1] .coord_y = 3;
defparam \lcd_data_out_q[1] .coord_z = 0;
defparam \lcd_data_out_q[1] .mask = 16'hFF00;
defparam \lcd_data_out_q[1] .modeMux = 1'b0;
defparam \lcd_data_out_q[1] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[1] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[1] .BypassEn = 1'b0;
defparam \lcd_data_out_q[1] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[2] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[2]~input_o ),
	.Cin(),
	.Qin(lcd_data_out_q[2]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y20_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_data_out_q[2]~feeder_combout ),
	.Cout(),
	.Q(lcd_data_out_q[2]));
defparam \lcd_data_out_q[2] .coord_x = 4;
defparam \lcd_data_out_q[2] .coord_y = 3;
defparam \lcd_data_out_q[2] .coord_z = 5;
defparam \lcd_data_out_q[2] .mask = 16'hFF00;
defparam \lcd_data_out_q[2] .modeMux = 1'b0;
defparam \lcd_data_out_q[2] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[2] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[2] .BypassEn = 1'b0;
defparam \lcd_data_out_q[2] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[3] (
	.A(),
	.B(),
	.C(\MCU_D[3]~input_o ),
	.D(),
	.Cin(),
	.Qin(lcd_data_out_q[3]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y26_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y26_GND),
	.SyncReset(SyncReset_X1_Y26_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y26_VCC),
	.LutOut(),
	.Cout(),
	.Q(lcd_data_out_q[3]));
defparam \lcd_data_out_q[3] .coord_x = 3;
defparam \lcd_data_out_q[3] .coord_y = 2;
defparam \lcd_data_out_q[3] .coord_z = 7;
defparam \lcd_data_out_q[3] .mask = 16'hFFFF;
defparam \lcd_data_out_q[3] .modeMux = 1'b1;
defparam \lcd_data_out_q[3] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[3] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[3] .BypassEn = 1'b1;
defparam \lcd_data_out_q[3] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[4] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[4]~input_o ),
	.Cin(),
	.Qin(lcd_data_out_q[4]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y26_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y26_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_data_out_q[4]~feeder_combout ),
	.Cout(),
	.Q(lcd_data_out_q[4]));
defparam \lcd_data_out_q[4] .coord_x = 3;
defparam \lcd_data_out_q[4] .coord_y = 2;
defparam \lcd_data_out_q[4] .coord_z = 6;
defparam \lcd_data_out_q[4] .mask = 16'hFF00;
defparam \lcd_data_out_q[4] .modeMux = 1'b0;
defparam \lcd_data_out_q[4] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[4] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[4] .BypassEn = 1'b0;
defparam \lcd_data_out_q[4] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[5] (
	.A(),
	.B(),
	.C(\MCU_D[5]~input_o ),
	.D(),
	.Cin(),
	.Qin(lcd_data_out_q[5]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y26_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y26_GND),
	.SyncReset(SyncReset_X1_Y26_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y26_VCC),
	.LutOut(),
	.Cout(),
	.Q(lcd_data_out_q[5]));
defparam \lcd_data_out_q[5] .coord_x = 3;
defparam \lcd_data_out_q[5] .coord_y = 2;
defparam \lcd_data_out_q[5] .coord_z = 2;
defparam \lcd_data_out_q[5] .mask = 16'hFFFF;
defparam \lcd_data_out_q[5] .modeMux = 1'b1;
defparam \lcd_data_out_q[5] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[5] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[5] .BypassEn = 1'b1;
defparam \lcd_data_out_q[5] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[6] (
	.A(),
	.B(),
	.C(\MCU_D[6]~input_o ),
	.D(),
	.Cin(),
	.Qin(lcd_data_out_q[6]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y20_GND),
	.SyncReset(SyncReset_X1_Y20_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y20_VCC),
	.LutOut(),
	.Cout(),
	.Q(lcd_data_out_q[6]));
defparam \lcd_data_out_q[6] .coord_x = 4;
defparam \lcd_data_out_q[6] .coord_y = 3;
defparam \lcd_data_out_q[6] .coord_z = 6;
defparam \lcd_data_out_q[6] .mask = 16'hFFFF;
defparam \lcd_data_out_q[6] .modeMux = 1'b1;
defparam \lcd_data_out_q[6] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[6] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[6] .BypassEn = 1'b1;
defparam \lcd_data_out_q[6] .CarryEnb = 1'b1;

alta_slice \lcd_data_out_q[7] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[7]~input_o ),
	.Cin(),
	.Qin(lcd_data_out_q[7]),
	.Clk(\MCU_LCD_WRX~inputclkctrl_outclk_X1_Y20_INV_VCC ),
	.AsyncReset(AsyncReset_X1_Y20_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_data_out_q[7]~feeder_combout ),
	.Cout(),
	.Q(lcd_data_out_q[7]));
defparam \lcd_data_out_q[7] .coord_x = 4;
defparam \lcd_data_out_q[7] .coord_y = 3;
defparam \lcd_data_out_q[7] .coord_z = 10;
defparam \lcd_data_out_q[7] .mask = 16'hFF00;
defparam \lcd_data_out_q[7] .modeMux = 1'b0;
defparam \lcd_data_out_q[7] .FeedbackMux = 1'b0;
defparam \lcd_data_out_q[7] .ShiftMux = 1'b0;
defparam \lcd_data_out_q[7] .BypassEn = 1'b0;
defparam \lcd_data_out_q[7] .CarryEnb = 1'b1;

alta_slice lcd_reset_q(
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[0]~input_o ),
	.Cin(),
	.Qin(\lcd_reset_q~q ),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_reset_q~1_combout ),
	.Cout(),
	.Q(\lcd_reset_q~q ));
defparam lcd_reset_q.coord_x = 3;
defparam lcd_reset_q.coord_y = 3;
defparam lcd_reset_q.coord_z = 1;
defparam lcd_reset_q.mask = 16'h00FF;
defparam lcd_reset_q.modeMux = 1'b0;
defparam lcd_reset_q.FeedbackMux = 1'b0;
defparam lcd_reset_q.ShiftMux = 1'b0;
defparam lcd_reset_q.BypassEn = 1'b0;
defparam lcd_reset_q.CarryEnb = 1'b1;

alta_slice \lcd_reset_q~0 (
	.A(vcc),
	.B(vcc),
	.C(\MCU_ADDR~input_o ),
	.D(\MCU_DIR~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\lcd_reset_q~0_combout ),
	.Cout(),
	.Q());
defparam \lcd_reset_q~0 .coord_x = 3;
defparam \lcd_reset_q~0 .coord_y = 3;
defparam \lcd_reset_q~0 .coord_z = 7;
defparam \lcd_reset_q~0 .mask = 16'h00F0;
defparam \lcd_reset_q~0 .modeMux = 1'b0;
defparam \lcd_reset_q~0 .FeedbackMux = 1'b0;
defparam \lcd_reset_q~0 .ShiftMux = 1'b0;
defparam \lcd_reset_q~0 .BypassEn = 1'b0;
defparam \lcd_reset_q~0 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[0]~1 (
	.A(\mcu_data_out[0]~0_combout ),
	.B(\SW_R~input_o ),
	.C(\MCU_DIR~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[0]~1_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[0]~1 .coord_x = 4;
defparam \mcu_data_out[0]~1 .coord_y = 1;
defparam \mcu_data_out[0]~1 .coord_z = 15;
defparam \mcu_data_out[0]~1 .mask = 16'hAA3A;
defparam \mcu_data_out[0]~1 .modeMux = 1'b0;
defparam \mcu_data_out[0]~1 .FeedbackMux = 1'b0;
defparam \mcu_data_out[0]~1 .ShiftMux = 1'b0;
defparam \mcu_data_out[0]~1 .BypassEn = 1'b0;
defparam \mcu_data_out[0]~1 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[1]~3 (
	.A(\SW_L~input_o ),
	.B(\mcu_data_out[1]~2_combout ),
	.C(\MCU_DIR~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[1]~3_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[1]~3 .coord_x = 4;
defparam \mcu_data_out[1]~3 .coord_y = 1;
defparam \mcu_data_out[1]~3 .coord_z = 14;
defparam \mcu_data_out[1]~3 .mask = 16'hCC5C;
defparam \mcu_data_out[1]~3 .modeMux = 1'b0;
defparam \mcu_data_out[1]~3 .FeedbackMux = 1'b0;
defparam \mcu_data_out[1]~3 .ShiftMux = 1'b0;
defparam \mcu_data_out[1]~3 .BypassEn = 1'b0;
defparam \mcu_data_out[1]~3 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[2]~5 (
	.A(\MCU_DIR~input_o ),
	.B(\SW_D~input_o ),
	.C(\MCU_IO_STBX~input_o ),
	.D(\mcu_data_out[2]~4_combout ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[2]~5_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[2]~5 .coord_x = 4;
defparam \mcu_data_out[2]~5 .coord_y = 2;
defparam \mcu_data_out[2]~5 .coord_z = 7;
defparam \mcu_data_out[2]~5 .mask = 16'hF702;
defparam \mcu_data_out[2]~5 .modeMux = 1'b0;
defparam \mcu_data_out[2]~5 .FeedbackMux = 1'b0;
defparam \mcu_data_out[2]~5 .ShiftMux = 1'b0;
defparam \mcu_data_out[2]~5 .BypassEn = 1'b0;
defparam \mcu_data_out[2]~5 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[3]~7 (
	.A(\SW_U~input_o ),
	.B(\mcu_data_out[3]~6_combout ),
	.C(\MCU_DIR~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[3]~7_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[3]~7 .coord_x = 4;
defparam \mcu_data_out[3]~7 .coord_y = 2;
defparam \mcu_data_out[3]~7 .coord_z = 6;
defparam \mcu_data_out[3]~7 .mask = 16'hCC5C;
defparam \mcu_data_out[3]~7 .modeMux = 1'b0;
defparam \mcu_data_out[3]~7 .FeedbackMux = 1'b0;
defparam \mcu_data_out[3]~7 .ShiftMux = 1'b0;
defparam \mcu_data_out[3]~7 .BypassEn = 1'b0;
defparam \mcu_data_out[3]~7 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[4]~9 (
	.A(\MCU_DIR~input_o ),
	.B(\mcu_data_out[4]~8_combout ),
	.C(\SW_SEL~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[4]~9_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[4]~9 .coord_x = 4;
defparam \mcu_data_out[4]~9 .coord_y = 2;
defparam \mcu_data_out[4]~9 .coord_z = 14;
defparam \mcu_data_out[4]~9 .mask = 16'hCC4E;
defparam \mcu_data_out[4]~9 .modeMux = 1'b0;
defparam \mcu_data_out[4]~9 .FeedbackMux = 1'b0;
defparam \mcu_data_out[4]~9 .ShiftMux = 1'b0;
defparam \mcu_data_out[4]~9 .BypassEn = 1'b0;
defparam \mcu_data_out[4]~9 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[5]~11 (
	.A(\mcu_data_out[5]~10_combout ),
	.B(\SW_ROT_A~input_o ),
	.C(\MCU_DIR~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[5]~11_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[5]~11 .coord_x = 4;
defparam \mcu_data_out[5]~11 .coord_y = 2;
defparam \mcu_data_out[5]~11 .coord_z = 15;
defparam \mcu_data_out[5]~11 .mask = 16'hAA3A;
defparam \mcu_data_out[5]~11 .modeMux = 1'b0;
defparam \mcu_data_out[5]~11 .FeedbackMux = 1'b0;
defparam \mcu_data_out[5]~11 .ShiftMux = 1'b0;
defparam \mcu_data_out[5]~11 .BypassEn = 1'b0;
defparam \mcu_data_out[5]~11 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[6]~13 (
	.A(\mcu_data_out[6]~12_combout ),
	.B(\SW_ROT_B~input_o ),
	.C(\MCU_DIR~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[6]~13_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[6]~13 .coord_x = 4;
defparam \mcu_data_out[6]~13 .coord_y = 1;
defparam \mcu_data_out[6]~13 .coord_z = 8;
defparam \mcu_data_out[6]~13 .mask = 16'hAA3A;
defparam \mcu_data_out[6]~13 .modeMux = 1'b0;
defparam \mcu_data_out[6]~13 .FeedbackMux = 1'b0;
defparam \mcu_data_out[6]~13 .ShiftMux = 1'b0;
defparam \mcu_data_out[6]~13 .BypassEn = 1'b0;
defparam \mcu_data_out[6]~13 .CarryEnb = 1'b1;

alta_slice \mcu_data_out[7]~15 (
	.A(\mcu_data_out[7]~14_combout ),
	.B(\LCD_TE~input_o ),
	.C(\MCU_DIR~input_o ),
	.D(\MCU_IO_STBX~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\mcu_data_out[7]~15_combout ),
	.Cout(),
	.Q());
defparam \mcu_data_out[7]~15 .coord_x = 4;
defparam \mcu_data_out[7]~15 .coord_y = 1;
defparam \mcu_data_out[7]~15 .coord_z = 7;
defparam \mcu_data_out[7]~15 .mask = 16'hAACA;
defparam \mcu_data_out[7]~15 .modeMux = 1'b0;
defparam \mcu_data_out[7]~15 .FeedbackMux = 1'b0;
defparam \mcu_data_out[7]~15 .ShiftMux = 1'b0;
defparam \mcu_data_out[7]~15 .BypassEn = 1'b0;
defparam \mcu_data_out[7]~15 .CarryEnb = 1'b1;

alta_slice ref_en_q(
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[6]~input_o ),
	.Cin(),
	.Qin(\ref_en_q~q ),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\ref_en_q~feeder_combout ),
	.Cout(),
	.Q(\ref_en_q~q ));
defparam ref_en_q.coord_x = 3;
defparam ref_en_q.coord_y = 3;
defparam ref_en_q.coord_z = 13;
defparam ref_en_q.mask = 16'hFF00;
defparam ref_en_q.modeMux = 1'b0;
defparam ref_en_q.FeedbackMux = 1'b0;
defparam ref_en_q.ShiftMux = 1'b0;
defparam ref_en_q.BypassEn = 1'b0;
defparam ref_en_q.CarryEnb = 1'b1;

alta_syncctrl syncload_ctrl_X1_Y15(
	.Din(),
	.Dout(SyncLoad_X1_Y15_VCC));
defparam syncload_ctrl_X1_Y15.coord_x = 3;
defparam syncload_ctrl_X1_Y15.coord_y = 3;
defparam syncload_ctrl_X1_Y15.coord_z = 1;
defparam syncload_ctrl_X1_Y15.SyncCtrlMux = 2'b01;

alta_syncctrl syncload_ctrl_X1_Y19(
	.Din(),
	.Dout(SyncLoad_X1_Y19_VCC));
defparam syncload_ctrl_X1_Y19.coord_x = 3;
defparam syncload_ctrl_X1_Y19.coord_y = 1;
defparam syncload_ctrl_X1_Y19.coord_z = 1;
defparam syncload_ctrl_X1_Y19.SyncCtrlMux = 2'b01;

alta_syncctrl syncload_ctrl_X1_Y20(
	.Din(),
	.Dout(SyncLoad_X1_Y20_VCC));
defparam syncload_ctrl_X1_Y20.coord_x = 4;
defparam syncload_ctrl_X1_Y20.coord_y = 3;
defparam syncload_ctrl_X1_Y20.coord_z = 1;
defparam syncload_ctrl_X1_Y20.SyncCtrlMux = 2'b01;

alta_syncctrl syncload_ctrl_X1_Y21(
	.Din(),
	.Dout(SyncLoad_X1_Y21_VCC));
defparam syncload_ctrl_X1_Y21.coord_x = 5;
defparam syncload_ctrl_X1_Y21.coord_y = 3;
defparam syncload_ctrl_X1_Y21.coord_z = 1;
defparam syncload_ctrl_X1_Y21.SyncCtrlMux = 2'b01;

alta_syncctrl syncload_ctrl_X1_Y24(
	.Din(),
	.Dout(SyncLoad_X1_Y24_VCC));
defparam syncload_ctrl_X1_Y24.coord_x = 5;
defparam syncload_ctrl_X1_Y24.coord_y = 2;
defparam syncload_ctrl_X1_Y24.coord_z = 1;
defparam syncload_ctrl_X1_Y24.SyncCtrlMux = 2'b01;

alta_syncctrl syncload_ctrl_X1_Y26(
	.Din(),
	.Dout(SyncLoad_X1_Y26_VCC));
defparam syncload_ctrl_X1_Y26.coord_x = 3;
defparam syncload_ctrl_X1_Y26.coord_y = 2;
defparam syncload_ctrl_X1_Y26.coord_z = 1;
defparam syncload_ctrl_X1_Y26.SyncCtrlMux = 2'b01;

alta_syncctrl syncreset_ctrl_X1_Y15(
	.Din(),
	.Dout(SyncReset_X1_Y15_GND));
defparam syncreset_ctrl_X1_Y15.coord_x = 3;
defparam syncreset_ctrl_X1_Y15.coord_y = 3;
defparam syncreset_ctrl_X1_Y15.coord_z = 0;
defparam syncreset_ctrl_X1_Y15.SyncCtrlMux = 2'b00;

alta_syncctrl syncreset_ctrl_X1_Y19(
	.Din(),
	.Dout(SyncReset_X1_Y19_GND));
defparam syncreset_ctrl_X1_Y19.coord_x = 3;
defparam syncreset_ctrl_X1_Y19.coord_y = 1;
defparam syncreset_ctrl_X1_Y19.coord_z = 0;
defparam syncreset_ctrl_X1_Y19.SyncCtrlMux = 2'b00;

alta_syncctrl syncreset_ctrl_X1_Y20(
	.Din(),
	.Dout(SyncReset_X1_Y20_GND));
defparam syncreset_ctrl_X1_Y20.coord_x = 4;
defparam syncreset_ctrl_X1_Y20.coord_y = 3;
defparam syncreset_ctrl_X1_Y20.coord_z = 0;
defparam syncreset_ctrl_X1_Y20.SyncCtrlMux = 2'b00;

alta_syncctrl syncreset_ctrl_X1_Y21(
	.Din(),
	.Dout(SyncReset_X1_Y21_GND));
defparam syncreset_ctrl_X1_Y21.coord_x = 5;
defparam syncreset_ctrl_X1_Y21.coord_y = 3;
defparam syncreset_ctrl_X1_Y21.coord_z = 0;
defparam syncreset_ctrl_X1_Y21.SyncCtrlMux = 2'b00;

alta_syncctrl syncreset_ctrl_X1_Y24(
	.Din(),
	.Dout(SyncReset_X1_Y24_GND));
defparam syncreset_ctrl_X1_Y24.coord_x = 5;
defparam syncreset_ctrl_X1_Y24.coord_y = 2;
defparam syncreset_ctrl_X1_Y24.coord_z = 0;
defparam syncreset_ctrl_X1_Y24.SyncCtrlMux = 2'b00;

alta_syncctrl syncreset_ctrl_X1_Y26(
	.Din(),
	.Dout(SyncReset_X1_Y26_GND));
defparam syncreset_ctrl_X1_Y26.coord_x = 3;
defparam syncreset_ctrl_X1_Y26.coord_y = 2;
defparam syncreset_ctrl_X1_Y26.coord_z = 0;
defparam syncreset_ctrl_X1_Y26.SyncCtrlMux = 2'b00;

alta_slice sysoff_q(
	.A(\MCU_D[2]~input_o ),
	.B(vcc),
	.C(vcc),
	.D(vcc),
	.Cin(),
	.Qin(\sysoff_q~q ),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__lcd_reset_q~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\sysoff_q~feeder_combout ),
	.Cout(),
	.Q(\sysoff_q~q ));
defparam sysoff_q.coord_x = 3;
defparam sysoff_q.coord_y = 3;
defparam sysoff_q.coord_z = 2;
defparam sysoff_q.mask = 16'hAAAA;
defparam sysoff_q.modeMux = 1'b0;
defparam sysoff_q.FeedbackMux = 1'b0;
defparam sysoff_q.ShiftMux = 1'b0;
defparam sysoff_q.BypassEn = 1'b0;
defparam sysoff_q.CarryEnb = 1'b1;

alta_slice \tp_q[0] (
	.A(),
	.B(),
	.C(\MCU_D[0]~input_o ),
	.D(),
	.Cin(),
	.Qin(tp_q[0]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(SyncReset_X1_Y15_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y15_VCC),
	.LutOut(),
	.Cout(),
	.Q(tp_q[0]));
defparam \tp_q[0] .coord_x = 3;
defparam \tp_q[0] .coord_y = 3;
defparam \tp_q[0] .coord_z = 9;
defparam \tp_q[0] .mask = 16'hFFFF;
defparam \tp_q[0] .modeMux = 1'b1;
defparam \tp_q[0] .FeedbackMux = 1'b0;
defparam \tp_q[0] .ShiftMux = 1'b0;
defparam \tp_q[0] .BypassEn = 1'b1;
defparam \tp_q[0] .CarryEnb = 1'b1;

alta_slice \tp_q[1] (
	.A(),
	.B(),
	.C(\MCU_D[1]~input_o ),
	.D(),
	.Cin(),
	.Qin(tp_q[1]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(SyncReset_X1_Y15_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y15_VCC),
	.LutOut(),
	.Cout(),
	.Q(tp_q[1]));
defparam \tp_q[1] .coord_x = 3;
defparam \tp_q[1] .coord_y = 3;
defparam \tp_q[1] .coord_z = 10;
defparam \tp_q[1] .mask = 16'hFFFF;
defparam \tp_q[1] .modeMux = 1'b1;
defparam \tp_q[1] .FeedbackMux = 1'b0;
defparam \tp_q[1] .ShiftMux = 1'b0;
defparam \tp_q[1] .BypassEn = 1'b1;
defparam \tp_q[1] .CarryEnb = 1'b1;

alta_slice \tp_q[2] (
	.A(\MCU_D[2]~input_o ),
	.B(vcc),
	.C(vcc),
	.D(vcc),
	.Cin(),
	.Qin(tp_q[2]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\tp_q[2]~feeder_combout ),
	.Cout(),
	.Q(tp_q[2]));
defparam \tp_q[2] .coord_x = 3;
defparam \tp_q[2] .coord_y = 3;
defparam \tp_q[2] .coord_z = 4;
defparam \tp_q[2] .mask = 16'hAAAA;
defparam \tp_q[2] .modeMux = 1'b0;
defparam \tp_q[2] .FeedbackMux = 1'b0;
defparam \tp_q[2] .ShiftMux = 1'b0;
defparam \tp_q[2] .BypassEn = 1'b0;
defparam \tp_q[2] .CarryEnb = 1'b1;

alta_slice \tp_q[3] (
	.A(),
	.B(),
	.C(\MCU_D[3]~input_o ),
	.D(),
	.Cin(),
	.Qin(tp_q[3]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(SyncReset_X1_Y15_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y15_VCC),
	.LutOut(),
	.Cout(),
	.Q(tp_q[3]));
defparam \tp_q[3] .coord_x = 3;
defparam \tp_q[3] .coord_y = 3;
defparam \tp_q[3] .coord_z = 14;
defparam \tp_q[3] .mask = 16'hFFFF;
defparam \tp_q[3] .modeMux = 1'b1;
defparam \tp_q[3] .FeedbackMux = 1'b0;
defparam \tp_q[3] .ShiftMux = 1'b0;
defparam \tp_q[3] .BypassEn = 1'b1;
defparam \tp_q[3] .CarryEnb = 1'b1;

alta_slice \tp_q[3]~0 (
	.A(vcc),
	.B(vcc),
	.C(\MCU_ADDR~input_o ),
	.D(\MCU_DIR~input_o ),
	.Cin(),
	.Qin(),
	.Clk(),
	.AsyncReset(),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\tp_q[3]~0_combout ),
	.Cout(),
	.Q());
defparam \tp_q[3]~0 .coord_x = 3;
defparam \tp_q[3]~0 .coord_y = 3;
defparam \tp_q[3]~0 .coord_z = 3;
defparam \tp_q[3]~0 .mask = 16'h000F;
defparam \tp_q[3]~0 .modeMux = 1'b0;
defparam \tp_q[3]~0 .FeedbackMux = 1'b0;
defparam \tp_q[3]~0 .ShiftMux = 1'b0;
defparam \tp_q[3]~0 .BypassEn = 1'b0;
defparam \tp_q[3]~0 .CarryEnb = 1'b1;

alta_slice \tp_q[4] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[4]~input_o ),
	.Cin(),
	.Qin(tp_q[4]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\tp_q[4]~feeder_combout ),
	.Cout(),
	.Q(tp_q[4]));
defparam \tp_q[4] .coord_x = 3;
defparam \tp_q[4] .coord_y = 3;
defparam \tp_q[4] .coord_z = 5;
defparam \tp_q[4] .mask = 16'hFF00;
defparam \tp_q[4] .modeMux = 1'b0;
defparam \tp_q[4] .FeedbackMux = 1'b0;
defparam \tp_q[4] .ShiftMux = 1'b0;
defparam \tp_q[4] .BypassEn = 1'b0;
defparam \tp_q[4] .CarryEnb = 1'b1;

alta_slice \tp_q[5] (
	.A(),
	.B(),
	.C(\MCU_D[5]~input_o ),
	.D(),
	.Cin(),
	.Qin(tp_q[5]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(SyncReset_X1_Y15_GND),
	.ShiftData(),
	.SyncLoad(SyncLoad_X1_Y15_VCC),
	.LutOut(),
	.Cout(),
	.Q(tp_q[5]));
defparam \tp_q[5] .coord_x = 3;
defparam \tp_q[5] .coord_y = 3;
defparam \tp_q[5] .coord_z = 6;
defparam \tp_q[5] .mask = 16'hFFFF;
defparam \tp_q[5] .modeMux = 1'b1;
defparam \tp_q[5] .FeedbackMux = 1'b0;
defparam \tp_q[5] .ShiftMux = 1'b0;
defparam \tp_q[5] .BypassEn = 1'b1;
defparam \tp_q[5] .CarryEnb = 1'b1;

alta_slice \tp_q[6] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[6]~input_o ),
	.Cin(),
	.Qin(tp_q[6]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\tp_q[6]~feeder_combout ),
	.Cout(),
	.Q(tp_q[6]));
defparam \tp_q[6] .coord_x = 3;
defparam \tp_q[6] .coord_y = 3;
defparam \tp_q[6] .coord_z = 15;
defparam \tp_q[6] .mask = 16'hFF00;
defparam \tp_q[6] .modeMux = 1'b0;
defparam \tp_q[6] .FeedbackMux = 1'b0;
defparam \tp_q[6] .ShiftMux = 1'b0;
defparam \tp_q[6] .BypassEn = 1'b0;
defparam \tp_q[6] .CarryEnb = 1'b1;

alta_slice \tp_q[7] (
	.A(vcc),
	.B(vcc),
	.C(vcc),
	.D(\MCU_D[7]~input_o ),
	.Cin(),
	.Qin(tp_q[7]),
	.Clk(\MCU_IO_STBX~inputclkctrl_outclk__tp_q[3]~0_combout_X1_Y15_SIG_SIG ),
	.AsyncReset(AsyncReset_X1_Y15_GND),
	.SyncReset(),
	.ShiftData(),
	.SyncLoad(),
	.LutOut(\tp_q[7]~feeder_combout ),
	.Cout(),
	.Q(tp_q[7]));
defparam \tp_q[7] .coord_x = 3;
defparam \tp_q[7] .coord_y = 3;
defparam \tp_q[7] .coord_z = 11;
defparam \tp_q[7] .mask = 16'hFF00;
defparam \tp_q[7] .modeMux = 1'b0;
defparam \tp_q[7] .FeedbackMux = 1'b0;
defparam \tp_q[7] .ShiftMux = 1'b0;
defparam \tp_q[7] .BypassEn = 1'b0;
defparam \tp_q[7] .CarryEnb = 1'b1;

endmodule
