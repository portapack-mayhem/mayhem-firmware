set ALTA_SUPRA true
set sh_continue_on_error false
set sh_echo_on_source  true
set sh_quiet_on_source true
set cc_critical_as_fatal true
set rt_incremental_route true
set ta_report_auto 1
set ta_report_auto_constraints $ta_report_auto

if { ! [info exists RESULT_DIR] } {
  set RESULT_DIR "."
} elseif { ! [info exists alta_work] } {
  set alta_work "${RESULT_DIR}/alta_db"
}
if { ! [info exists DEVICE] } {
  set DEVICE "AG256SL100"
}
if { [info exists DESIGN] && ! [info exists TOP_MODULE] } {
  set TOP_MODULE "$DESIGN"
}
if { ! [info exists DESIGN] } {
  set DESIGN "portapack_h4m_cpld"
}
if { ! [info exists TOP_MODULE] } {
  set TOP_MODULE "top"
}
if { ! [info exists IP_FILES] } {
  set IP_FILES {}
}
if { ! [info exists VE_FILE] } {
  set VE_FILE ""
}
if { ! [info exists TIMING_DERATE] } {
  set TIMING_DERATE {1.000000 1.000000}
}
if { [info exists NO_ROUTE] && $NO_ROUTE } {
  set no_route "-no_route"
} else {
  set no_route ""
}
if { ! [info exists RETRY] } { set RETRY 0 }
if { ! [info exists SEED ] } { set SEED 666 }
set seed_rand ""
if { $SEED == 0 } { set seed_rand "-seed_rand" }
if { [info exists QUARTUS_SDC] } {
  set sdc_remove_quartus_column_name $QUARTUS_SDC
}
if { ! [info exists ORG_PLACE] } { set ORG_PLACE false }
if { ! [info exists MODE] } { set MODE "QUARTUS" }
if { ! [info exists FLOW] } { set FLOW "ALL"    }
if { $FLOW == "PROBE" } {
  if { ! [info exists PROBE_FORCE] } { set PROBE_FORCE false }
  if { ! [info exists PREFIX] } { set PREFIX "probe_" }
}
if { ! [info exists PREFIX] } {
  set RESULT $DESIGN
} else {
  set RESULT $PREFIX$DESIGN
}
if { $FLOW == "GEN" || $FLOW == "PACK" || $FLOW == "LOAD" } { set no_route "-no_route" }
set RUN "run"
if { $FLOW == "CHECK" } {
  set RUN "check"
} elseif { $FLOW == "PROBE" } {
  set RUN "probe"
} elseif { $FLOW == "GEN" } {
  set RUN "gen"
}

if { ! [info exists alta_logs] } {
  set alta_logs "${RESULT_DIR}/alta_logs"
}
file mkdir $alta_logs
alta::begin_log_cmd "$alta_logs/${RUN}.log" "$alta_logs/${RUN}.err"
alta::tcl_whisper "Cmd : [alta::prog_path] [alta::prog_version]([alta::prog_subversion])\n"
alta::tcl_whisper "Args : [string map {\{ \" \} \"} $tcl_cmd_args]\n"

set_seed_rand $SEED
set ar_timing_derate ${TIMING_DERATE}

date_time
if { [file exists "./${DESIGN}.pre.asf"] } {
  alta::tcl_highlight "Using pre-ASF file ${DESIGN}.pre.asf.\n"
  source "./${DESIGN}.pre.asf"
}

set LOAD_DB    false
set LOAD_PLACE false
set LOAD_ROUTE false
if { $FLOW == "LOAD" || $FLOW == "CHECK" || $FLOW == "PROBE" } {
  set LOAD_DB    true
  set LOAD_PLACE true
  set LOAD_ROUTE true
} elseif { $FLOW == "R" || $FLOW == "ROUTE" } {
  set LOAD_DB    true
  set LOAD_PLACE true
}

set ORIGINAL_QSF "A:/Users/jLynx/Documents/Code/C/portapack-mayhem/hardware/portapack_h4m/CPLD/AG256SL100/./portapack_h4m_cpld.qsf"
set ORIGINAL_PIN "A:/Users/jLynx/Documents/Code/C/portapack-mayhem/hardware/portapack_h4m/CPLD/AG256SL100/output_files/portapack_h4m_cpld.pin"

#################################################################################

while (1) {
if { [info exists CORNER] } { set_mode -corner $CORNER; }

eval "load_architect ${no_route} -type ${DEVICE} 1 1 1000 1000"
foreach ip_file $IP_FILES { read_ip $ip_file; }


if { $FLOW == "GEN" } {
  if { ! [info exists CONFIG_BITS] } {
    set CONFIG_BITS "${RESULT_DIR}/${DESIGN}.bin"
  }
  if { [llength $CONFIG_BITS] > 1 } {
    if { ! [info exists BOOT_BINARY] } {
      set BOOT_BINARY "${RESULT_DIR}/${DESIGN}_boot.bin"
    }
    if { ! [info exists CONFIG_ADDRESSES] } {
      set CONFIG_ADDRESSES ""
    }
    generate_binary -master $BOOT_BINARY -inputs $CONFIG_BITS -address $CONFIG_ADDRESSES
  } else {
    set CONFIG_ROOT   [file rootname [lindex $CONFIG_BITS 0]]
    set SLAVE_RBF     "${CONFIG_ROOT}_slave.rbf"
    set MASTER_BINARY "${CONFIG_ROOT}_master.bin"
    if { [file exists [lindex $CONFIG_BITS 0]] } {
      generate_binary -slave  $SLAVE_RBF     -inputs [lindex $CONFIG_BITS 0] -reverse
      generate_binary -master $MASTER_BINARY -inputs [lindex $CONFIG_BITS 0]
    }
    if { ! [info exists BOOT_BINARY] } {
      set BOOT_BINARY $MASTER_BINARY
    }
  }
  set PRG_FILE [file rootname $BOOT_BINARY].prg
  set AS_FILE  [file rootname $BOOT_BINARY]_as.prg
  generate_programming_file $BOOT_BINARY -erase $ERASE \
                            -program $PROGRAM -verify $VERIFY -offset $OFFSET \
                            -prg $PRG_FILE -as $AS_FILE
  break
}

if { $LOAD_DB } {
  load_db -top ${TOP_MODULE}
  set sdc "./${DESIGN}.adc"
  if { ! [file exists $sdc] } { set sdc "./${DESIGN}.sdc"; }
  if { [file exists $sdc] } { read_sdc $sdc; }

} elseif { $MODE == "QUARTUS" } {
  set verilog ${DESIGN}.vo
  set is_migrated false
  if { ! [file exists $verilog] } {
    set verilog "./simulation/modelsim/${DESIGN}.vo"
    set is_migrated true
  }
  if { ! [file exists $verilog] } {
    error "Can not find design verilog file $verilog"
  }
  alta::tcl_highlight "Using design verilog file $verilog.\n"
  set ret [read_design -top ${TOP_MODULE} -ve $VE_FILE -qsf $ORIGINAL_QSF $verilog -hierachy 1]
  if { !$ret } { exit -1; }

  set sdc "./${DESIGN}.adc"
  if { ! [file exists $sdc] } { set sdc "./${DESIGN}.sdc"; }
  if { ! [file exists $sdc] } {
    alta::tcl_warn "Can not find design SDC file $sdc"
  } else {
    alta::tcl_highlight "Using design SDC file $sdc.\n"
    read_sdc $sdc
  }

} elseif { $MODE == "SYNPLICITY" || $MODE == "NATIVE" } {
  set db_gclk_assignment_level 2
  set verilog ${DESIGN}.vqm
  set is_migrated false
  if { ! [file exists $verilog] } {
    error "Can not find design verilog file $verilog"
  }

  set sdc "./${DESIGN}.adc"
  if { ! [file exists $sdc] } { set sdc "./${DESIGN}.sdc"; }
  alta::tcl_highlight "Using design verilog file $verilog.\n"
  if { ! [file exists $sdc] } {
    alta::tcl_warn "Can not find design SDC file $sdc"
    set ret [read_design_and_pack -sdc $sdc  -top ${TOP_MODULE} $verilog]
  } else {
    alta::tcl_highlight "Using design SDC file $sdc.\n"
    set ret [read_design_and_pack -top ${TOP_MODULE} $verilog]
  }
  if { !$ret } { exit -1; }

} else {
  error "Unsupported mode $MODE"
}

if { $FLOW == "PACK" } { break }

if { [info exists FITTING] } {
  if { $FITTING == "Auto" } { set FITTING auto; }
  set_mode -fitting $FITTING
}
if { [info exists FITTER] } {
  if { $FITTER == "Auto" } {
    if { $MODE == "QUARTUS" } { set FITTER hybrid; } else { set FITTER full; }
  }
  if { $MODE == "SYNPLICITY" || $MODE == "NATIVE" } { set FITTER full; }
  set_mode -fitter $FITTER
}
if { [info exists EFFORT] } { set_mode -effort $EFFORT; }
if { [info exists SKEW  ] } { set_mode -skew   $SKEW  ; }
if { [info exists SKOPE ] } { set_mode -skope  $SKOPE ; }
if { [info exists HOLDX ] } { set_mode -holdx  $HOLDX; }
if { [info exists TUNING] } { set_mode -tuning $TUNING; }
if { [info exists TARGET] } { set_mode -target $TARGET; }
if { [info exists PRESET] } { set_mode -preset $PRESET; }
if { [info exists ADJUST] } { set pl_criticality_wadjust $ADJUST; }

set alta_aqf $::alta_work/alta.aqf
if { $LOAD_DB } {
  # Empty
} elseif { true } {
  if { [file exists $VE_FILE] } {
    set ORIGINAL_PIN ""
  } elseif { ! [file exists $ORIGINAL_PIN] } {
    if { $is_migrated } {
      error "Can not find design PIN file $ORIGINAL_PIN, please compile design first"
    }
    set ORIGINAL_PIN ""
  }
  if { [file exists $ORIGINAL_QSF] } {
    alta::convert_quartus_settings_cmd $ORIGINAL_QSF $ORIGINAL_PIN $alta_aqf
  } elseif { $is_migrated } {
    error "Can not find design exported QSF file $ORIGINAL_QSF, please export assigments first"
  }
}
if { [file exists "$alta_aqf"] } {
  alta::tcl_highlight "Using AQF file $alta_aqf.\n"
  source "$alta_aqf"
}
if { [file exists "./${DESIGN}.asf"] } {
  alta::tcl_highlight "Using ASF file ${DESIGN}.asf.\n"
  source "./${DESIGN}.asf"
}

if { $FLOW == "PROBE" } {
  set ret [place_pseudo -user_io -place_io -place_pll -place_gclk]
  if { !$ret } { exit -1 }

  set force ""
  if { [info exists PROBE_FORCE] && $PROBE_FORCE } { set force "-force" }
  eval "probe_design -froms {${PROBE_FROMS}} -tos {${PROBE_TOS}} ${force}"

} elseif { $FLOW == "CHECK" } {
  set ret [place_pseudo -user_io -place_io -place_pll -place_gclk]
  if { !$ret } { exit -1 }

  if { [file exists "./${DESIGN}.chk"] } {
    alta::tcl_highlight "Using CHK file ${DESIGN}.chk.\n"
    source "./${DESIGN}.chk"
    place_design -dry
    check_design -rule led_guide
  } else {
    error "Can not find design CHECK file ${DESIGN}.chk"
  }

} else {
  set ret [place_pseudo -user_io -place_io -place_pll -place_gclk -warn_io]
  if { !$ret } { exit -1 }

  set org_place ""
  set load_place ""
  set load_route ""
  set quiet ""
  if {  $ORG_PLACE } { set  org_place "-org_place" ; }
  if { $LOAD_PLACE } { set load_place "-load_place"; }
  if { $LOAD_ROUTE } { set load_route "-load_route"; }
  eval "place_and_route_design $org_place $load_place $load_route \
                               -retry $RETRY $seed_rand $quiet"
}

date_time
if { $FLOW != "CHECK" } {
if { $FLOW != "PROBE" } {
#report_timing -verbose 1 -file $::alta_work/timing.rpt.gz
report_timing -verbose 2 -setup -file $::alta_work/setup.rpt.gz
report_timing -verbose 2 -setup -brief -file $::alta_work/setup_summary.rpt.gz
report_timing -verbose 2 -hold -file $::alta_work/hold.rpt.gz
report_timing -verbose 2 -hold -brief -file $::alta_work/hold_summary.rpt.gz

set ta_report_auto_constraints 0
report_timing -fmax -file $::alta_work/fmax.rpt
report_timing -xfer -file $::alta_work/xfer.rpt
set ta_report_auto_constraints $ta_report_auto

#set ta_coverage_limit "0.95 0.90"
set ta_dump_uncovered 1
report_timing -verbose 1 -coverage >! $::alta_work/coverage.rpt.gz
#unset ta_coverage_limit
unset ta_dump_uncovered


if { ! [info exists rt_report_timing_fast] } {
  set rt_report_timing_fast false
}
if { $rt_report_timing_fast } {
  set_timing_corner fast
  route_delay -quiet
  report_timing -verbose 2 -setup -file $::alta_work/setup_fast.rpt.gz
  report_timing -verbose 2 -setup -brief -file $::alta_work/setup_fast_summary.rpt.gz
  report_timing -verbose 2 -hold -file $::alta_work/hold_fast.rpt.gz
  report_timing -verbose 2 -hold -brief -file $::alta_work/hold_fast_summary.rpt.gz
  set ta_report_auto_constraints 0
  report_timing -fmax -file $::alta_work/fmax_fast.rpt
  report_timing -xfer -file $::alta_work/xfer_fast.rpt
  set ta_report_auto_constraints $ta_report_auto
}

write_routed_design "${RESULT_DIR}/${RESULT}_routed.v"
}

bitgen normal -prg "${RESULT_DIR}/${RESULT}.prg" -bin "${RESULT_DIR}/${RESULT}.bin"
bitgen sram -prg "${RESULT_DIR}/${RESULT}_sram.prg"
bitgen download -bin "${RESULT_DIR}/${RESULT}.bin" -svf "${RESULT_DIR}/${RESULT}_download.svf"
generate_binary -slave "${RESULT_DIR}/${RESULT}_slave.rbf" \
                -inputs "${RESULT_DIR}/${RESULT}.bin" -reverse
generate_binary -master "${RESULT_DIR}/${RESULT}_master.bin" \
                -inputs "${RESULT_DIR}/${RESULT}.bin"
generate_programming_file "${RESULT_DIR}/${RESULT}_master.bin" -prg "${RESULT_DIR}/${RESULT}_master.prg" \
  -as "${RESULT_DIR}/${RESULT}_master_as.prg" -hybrid "${RESULT_DIR}/${RESULT}_hybrid.prg"
}
break
}

if { [file exists "./${DESIGN}.post.asf"] } {
  alta::tcl_highlight "Using post-ASF file ${DESIGN}.post.asf.\n"
  source "./${DESIGN}.post.asf"
}
date_time
exit

