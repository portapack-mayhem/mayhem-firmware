/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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

/*
 * AT91SAM7S256 memory setup.
 */
__und_stack_size__	= 0x0004;
__abt_stack_size__	= 0x0004;
__fiq_stack_size__	= 0x0010;
__irq_stack_size__	= 0x0080;
__svc_stack_size__	= 0x0004;
__sys_stack_size__	= 0x0400;
__stacks_total_size__	= __und_stack_size__ + __abt_stack_size__ + __fiq_stack_size__ + __irq_stack_size__ + __svc_stack_size__ + __sys_stack_size__;

MEMORY
{
    flash : org = 0x100000, len = 256k
    ram : org = 0x200020, len = 64k - 0x20
}

__ram_start__		= ORIGIN(ram);
__ram_size__		= LENGTH(ram);
__ram_end__		= __ram_start__ + __ram_size__;

SECTIONS
{
	. = 0;

	.text : ALIGN(16) SUBALIGN(16)
	{
        _text = .;
        KEEP(*(vectors))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7t)
        *(.glue_7)
        *(.gcc*)
        *(.ctors)
        *(.dtors)
    } > flash

    .ARM.extab : {*(.ARM.extab* .gnu.linkonce.armextab.*)}

    __exidx_start = .;
    .ARM.exidx : {*(.ARM.exidx* .gnu.linkonce.armexidx.*)} > flash
    __exidx_end = .;

    .eh_frame_hdr : {*(.eh_frame_hdr)}

    .eh_frame : ONLY_IF_RO {*(.eh_frame)}

    . = ALIGN(4);
    _etext = .;
    _textdata = _etext;

    .data ALIGN(4) : ALIGN(4)
    {
        . = ALIGN(4);
        PROVIDE(_data = .);
        *(.data)
        *(.data.*)
        *(.ramtext)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > ram AT > flash

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
}

PROVIDE(end = .);
_end = .;

__heap_base__              = _end;
__heap_end__               = __ram_end__ - __stacks_total_size__;
__main_thread_stack_base__ = __ram_end__ - __stacks_total_size__;
