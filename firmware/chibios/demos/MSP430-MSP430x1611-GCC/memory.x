MEMORY {
  sfr              : ORIGIN = 0x0000, LENGTH = 0x0010 /* END=0x0010, size 16 */
  peripheral_8bit  : ORIGIN = 0x0010, LENGTH = 0x00f0 /* END=0x0100, size 240 */
  peripheral_16bit : ORIGIN = 0x0100, LENGTH = 0x0100 /* END=0x0200, size 256 */
  ram_mirror (wx)  : ORIGIN = 0x0200, LENGTH = 0x0800 /* END=0x0a00, size 2K */
  infomem          : ORIGIN = 0x1000, LENGTH = 0x0100 /* END=0x1100, size 256 as 2 128-byte segments */
  infob            : ORIGIN = 0x1000, LENGTH = 0x0080 /* END=0x1080, size 128 */
  infoa            : ORIGIN = 0x1080, LENGTH = 0x0080 /* END=0x1100, size 128 */
  ram (wx)         : ORIGIN = 0x1100, LENGTH = 0x2800 /* END=0x3900, size 10K */
  rom (rx)         : ORIGIN = 0x4000, LENGTH = 0xbfe0 /* END=0xffe0, size 49120 */
  vectors          : ORIGIN = 0xffe0, LENGTH = 0x0020 /* END=0x10000, size 32 as 16 2-byte segments */
  /* Remaining banks are absent */
  bsl              : ORIGIN = 0x0000, LENGTH = 0x0000
  infoc            : ORIGIN = 0x0000, LENGTH = 0x0000
  infod            : ORIGIN = 0x0000, LENGTH = 0x0000
  ram2 (wx)        : ORIGIN = 0x0000, LENGTH = 0x0000
  usbram (wx)      : ORIGIN = 0x0000, LENGTH = 0x0000
  far_rom          : ORIGIN = 0x00000000, LENGTH = 0x00000000
}
REGION_ALIAS("REGION_TEXT", rom);
REGION_ALIAS("REGION_DATA", ram);
REGION_ALIAS("REGION_FAR_ROM", far_rom);
PROVIDE (__info_segment_size = 0x80);
PROVIDE (__infob = 0x1000);
PROVIDE (__infoa = 0x1080);
