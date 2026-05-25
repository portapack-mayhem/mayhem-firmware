MEMORY
{
    ram : org = 0xADB10000, len = 96k
}

__ram_start__           = ORIGIN(ram);
__ram_size__            = LENGTH(ram);
__ram_end__             = __ram_start__ + __ram_size__;

SECTIONS
{
    . = 0;
    _text = .;
    startup : ALIGN(16) SUBALIGN(16)
    {
        KEEP(*(.standalone_application_information));
    } > ram

    constructors : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);
    } > ram

    .text : ALIGN(4) SUBALIGN(4)
    {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7)
        *(.glue_7t)
    } > ram

    .data : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__data_start__ = .);
        *(.data)
        *(.data.*)
        PROVIDE(__data_end__ = .);
    } > ram

    .bss (NOLOAD) : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__bss_start__ = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        PROVIDE(__bss_end__ = .);
    } > ram

    PROVIDE(end = .);
    PROVIDE(_end = .);
}
