# MSP430 makefile scripts and rules.

# Automatic compiler options
OPT = $(USE_OPT)
COPT = $(USE_COPT)
CPPOPT = $(USE_CPPOPT)
ifeq ($(USE_LINK_GC),yes)
  OPT += -ffunction-sections -fdata-sections
endif

# Source files groups
SRC	     = $(CSRC)$(CPPSRC)

# Object files groups
COBJS   = $(CSRC:.c=.o)
CPPOBJS = $(CPPSRC:.cpp=.o)
ASMOBJS = $(ASMSRC:.s=.o)
OBJS	= $(ASMOBJS) $(COBJS) $(CPPOBJS)

# Paths
IINCDIR = $(patsubst %,-I%,$(INCDIR) $(DINCDIR) $(UINCDIR))
LLIBDIR = $(patsubst %,-L%,$(DLIBDIR) $(ULIBDIR))

# Macros
DEFS    = $(DDEFS) $(UDEFS)
ADEFS   = $(DADEFS) $(UADEFS)

# Libs
LIBS    = $(DLIBS) $(ULIBS)

MCFLAGS = -mmcu=$(MCU)
ODFLAGS	= -x --syms
ASFLAGS = $(MCFLAGS) -Wa,-amhls=$(<:.s=.lst) $(ADEFS)
CPFLAGS = $(MCFLAGS) $(OPT) $(COPT) $(WARN) -Wa,-alms=$(<:.c=.lst) $(DEFS)
ifeq ($(LINK_GC),yes)
  LDFLAGS = $(MCFLAGS) -T$(LDSCRIPT) -Wl,-Map=$(PROJECT).map,--cref,--no-warn-mismatch,--gc-sections $(LLIBDIR)
else
  LDFLAGS = $(MCFLAGS) -T$(LDSCRIPT) -Wl,-Map=$(PROJECT).map,--cref,--no-warn-mismatch $(LLIBDIR)
endif

# Generate dependency information
ASFLAGS += -MD -MP -MF .dep/$(@F).d
CPFLAGS += -MD -MP -MF .dep/$(@F).d

#
# Makefile rules
#
all: $(OBJS) $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin $(PROJECT).dmp MAKE_ALL_RULE_HOOK

MAKE_ALL_RULE_HOOK:

$(CPPOBJS) : %.o : %.cpp
	@echo
	$(CPPC) -c $(CPPFLAGS) -I . $(IINCDIR) $< -o $@

$(COBJS) : %.o : %.c
	@echo
	$(CC) -c $(CPFLAGS) -I . $(IINCDIR) $< -o $@

$(ASMOBJS) : %.o : %.s
	@echo
	$(AS) -c $(ASFLAGS) -I . $(IINCDIR) $< -o $@

%elf: $(OBJS)
	@echo
	$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%hex: %elf
	$(HEX) $< $@

%bin: %elf
	$(BIN) $< $@

%dmp: %elf
	$(OD) $(ODFLAGS) $< > $@

clean:
	-rm -f $(OBJS)
	-rm -f $(CSRC:.c=.lst) $(CPPSRC:.cpp=.lst) $(ASMSRC:.s=.lst)
	-rm -f $(PROJECT).elf $(PROJECT).dmp $(PROJECT).map $(PROJECT).hex $(PROJECT).bin
	-rm -fR .dep

#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
