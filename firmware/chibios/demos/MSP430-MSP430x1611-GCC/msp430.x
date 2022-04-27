/* Default linker script, for normal executables */
OUTPUT_FORMAT("elf32-msp430")
OUTPUT_ARCH("msp430")
INCLUDE memory.x
INCLUDE periph.x
SECTIONS
{
  /* Read-only sections, merged into text segment.  */
  .hash            : { *(.hash)          }
  .dynsym          : { *(.dynsym)        }
  .dynstr          : { *(.dynstr)        }
  .gnu.version     : { *(.gnu.version)   }
  .gnu.version_d   : { *(.gnu.version_d) }
  .gnu.version_r   : { *(.gnu.version_r) }
  .rel.init      : { *(.rel.init)  }
  .rela.init     : { *(.rela.init) }
  .rel.fini      : { *(.rel.fini)  }
  .rela.fini     : { *(.rela.fini) }
  .rel.text      : { *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)        }
  .rela.text     : { *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)     }
  .rel.rodata    : { *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)    }
  .rela.rodata   : { *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*) }
  .rel.data      : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)        }
  .rela.data     : { *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)     }
  .rel.bss       : { *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)          }
  .rela.bss      : { *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)       }
  .rel.ctors     : { *(.rel.ctors)  }
  .rela.ctors    : { *(.rela.ctors) }
  .rel.dtors     : { *(.rel.dtors)  }
  .rela.dtors    : { *(.rela.dtors) }
  .rel.got       : { *(.rel.got)    }
  .rela.got      : { *(.rela.got)   }
  .rel.plt       : { *(.rel.plt)    }
  .rela.plt      : { *(.rela.plt)   }
  .text :
  {
     . = ALIGN(2);
     KEEP(*(.init .init.*))
     KEEP(*(.init0))  /* Start here after reset.               */
     KEEP(*(.init1))  /* User definable.                       */
     KEEP(*(.init2))  /* Initialize stack.                     */
     KEEP(*(.init3))  /* Initialize hardware, user definable.  */
     KEEP(*(.init4))  /* Copy data to .data, clear bss.        */
     KEEP(*(.init5))  /* User definable.                       */
     KEEP(*(.init6))  /* C++ constructors.                     */
     KEEP(*(.init7))  /* User definable.                       */
     KEEP(*(.init8))  /* User definable.                       */
     KEEP(*(.init9))  /* Call main().                          */
     KEEP(*(.fini9))  /* Falls into here after main(). User definable.  */
     KEEP(*(.fini8))  /* User definable.                           */
     KEEP(*(.fini7))  /* User definable.                           */
     KEEP(*(.fini6))  /* C++ destructors.                          */
     KEEP(*(.fini5))  /* User definable.                           */
     KEEP(*(.fini4))  /* User definable.                           */
     KEEP(*(.fini3))  /* User definable.                           */
     KEEP(*(.fini2))  /* User definable.                           */
     KEEP(*(.fini1))  /* User definable.                           */
     KEEP(*(.fini0))  /* Infinite loop after program termination.  */
     KEEP(*(.fini .fini.*))
     . = ALIGN(2);
     __ctors_start = . ;
     KEEP(*(.ctors))
     __ctors_end = . ;
     __dtors_start = . ;
     KEEP(*(.dtors))
     __dtors_end = . ;
     . = ALIGN(2);
    *(.text .text.* .gnu.linkonce.t.*)
     . = ALIGN(2);
  }  > REGION_TEXT
  .rodata   :
  {
     . = ALIGN(2);
    *(.rodata .rodata.* .gnu.linkonce.r.*)
     . = ALIGN(2);
  }  > REGION_TEXT
   _etext = .; /* Past last read-only (loadable) segment */
  .data   :
  {
     . = ALIGN(2);
     PROVIDE (__data_start = .) ;
    *(.data .data.* .gnu.linkonce.d.*)
     . = ALIGN(2);
     _edata = . ;  /* Past last read-write (loadable) segment */
  }  > REGION_DATA AT > REGION_TEXT
   PROVIDE (__data_load_start = LOADADDR(.data) );
   PROVIDE (__data_size = SIZEOF(.data) );
  .bss   :
  {
     PROVIDE (__bss_start = .) ;
    *(.bss .bss.*)
    *(COMMON)
     . = ALIGN(2);
     PROVIDE (__bss_end = .) ;
  }  > REGION_DATA
   PROVIDE (__bss_size = SIZEOF(.bss) );
  .noinit   :
  {
     PROVIDE (__noinit_start = .) ;
    *(.noinit .noinit.*)
     . = ALIGN(2);
     PROVIDE (__noinit_end = .) ;
  }  > REGION_DATA
   . = ALIGN(2);
   _end = . ;   /* Past last write (loadable) segment */
  .infomem   :
  {
    *(.infomem)
     . = ALIGN(2);
    *(.infomem.*)
  }  > infomem
  .infomemnobits   :
  {
    *(.infomemnobits)
     . = ALIGN(2);
    *(.infomemnobits.*)
  }  > infomem
  .infoa   :
  {
    *(.infoa .infoa.*)
  }  > infoa
  .infob   :
  {
    *(.infob .infob.*)
  }  > infob
  .infoc   :
  {
    *(.infoc .infoc.*)
  }  > infoc
  .infod   :
  {
    *(.infod .infod.*)
  }  > infod
  .vectors  :
  {
     PROVIDE (__vectors_start = .) ;
    KEEP(*(.vectors*))
     _vectors_end = . ;
  }  > vectors
  .fartext :
  {
     . = ALIGN(2);
    *(.fartext)
     . = ALIGN(2);
    *(.fartext.*)
     _efartext = .;
  }  > REGION_FAR_ROM
  /* Stabs for profiling information*/
  .profiler 0 : { *(.profiler) }
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) *(.gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* DWARF 3 */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
   PROVIDE (__stack = ORIGIN(ram) + LENGTH(ram));
   PROVIDE (__data_start_rom = _etext);
   PROVIDE (__data_end_rom   = _etext + SIZEOF (.data));
}
