/*
    Copyright (C) 2024 Bernd Herzog

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

MEMORY
{
    ram : org = 0xADB10000, len = 64k   /* DO NOT CHANGE the address. We make the image relocateable on load. It needs to be 0xADB10000 */
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

    destructors : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__fini_array_start = .);
        KEEP(*(.fini_array))
        KEEP(*(SORT(.fini_array.*)))
        PROVIDE(__fini_array_end = .);
    } > ram

    .text : ALIGN(16) SUBALIGN(16)
    {
        *(.text.startup.*)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7t)
        *(.glue_7)
        *(.gcc*)
    } > ram

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ram

    .ARM.exidx : {
        PROVIDE(__exidx_start = .);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
     } > ram

    .eh_frame_hdr :
    {
        *(.eh_frame_hdr)
    } > ram

    .eh_frame : ONLY_IF_RO
    {
        *(.eh_frame)
    } > ram
    
    .textalign : ONLY_IF_RO
    {
        . = ALIGN(8);
    } > ram

    .bss ALIGN(4) : ALIGN(4)
    {
        . = ALIGN(4);
        PROVIDE(_bss_start = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        PROVIDE(_bss_end = .);
    } > ram

    . = ALIGN(4);
    _etext = .;
    _textdata = _etext;

    .data ALIGN(4) : AT (_textdata)
    {
        . = ALIGN(4);
        PROVIDE(_data = .);
        *(.data)
        *(.data.*)
        *(.ramtext)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > ram
}

PROVIDE(end = .);
_end = .;

