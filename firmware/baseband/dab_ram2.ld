/*
 * Added to LPC43xx_M4.ld for the DAB image alone.
 *
 * The top 8 KiB of the 64 KiB m4_code region in Local SRAM bank 2, for the
 * buffers proc_dab.cpp places in .ram2 to keep them off bank 1, which the DMA
 * ring competes for (7,976 bytes of them). NOLOAD: startup neither loads nor
 * clears it. Raised from 8.5 KiB to give the code 512 bytes (DAB-IMPL 6.118).
 */
MEMORY
{
    ram2 (rwx) : org = 0x1008E000, len = 0x2000
}

SECTIONS
{
    .ram2 (NOLOAD) : ALIGN(4)
    {
        *(.ram2)
        *(.ram2.*)
    } > ram2
}

/* 'flash' is bank 2 seen at 0, so the image must end below ram2's offset. */
ASSERT(_textend <= ORIGIN(ram2) - 0x10080000, "DAB image runs into .ram2")
