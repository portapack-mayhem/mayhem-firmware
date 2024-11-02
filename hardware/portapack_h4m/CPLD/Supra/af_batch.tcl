if {![info exist MODE ]} {set MODE QUARTUS}
if {![info exists QUARTUS_SDC]} {set QUARTUS_SDC true}
if {![info exists CORNER]} {set CORNER ""}
if {![info exist COUNT]} {set COUNT 6}
if {![info exist JOBS ]} {set JOBS  1}

if {![info exist SEEDS   ]} {set SEEDS    {0        0        0        0          666        888    }}
if {![info exist EFFORTS ]} {set EFFORTS  {highest  highest  highest  highest    high       high   }}
if {![info exist FITTERS ]} {set FITTERS  {hybrid   hybrid   hybrid   hybrid     hybrid     hybrid }}
if {![info exist FITTINGS]} {set FITTINGS {timing_more timing_more timing_more timing timing basic }}
if {![info exist SKEWS   ]} {set SKEWS    {advanced advanced advanced advanced   aggressive basic  }}
if {![info exist HOLDXS  ]} {set HOLDXS   {default  default  default  default    default    default}}

set bc_config "./bc_config.txt"
if { [file exists $bc_config] } {
  alta::tcl_highlight "Using MC config $bc_config.\n"
  source "$bc_config"
}

#######################################################################

proc get_rand_value { values } {
  if {[llength $values] == 0} { return {} }
  return [lindex $values [expr {int(rand()*10000)%[llength $values]}]]
}

set results "bc_results"
set summary "bc_summary.txt"
file delete -force $results; file mkdir $results
file delete $summary; print -nonewline "" >! $summary

set is_parallel [expr $JOBS > 1]
set is_color ""; set is_gui ""; set is_quiet ""
if { $is_parallel } {
  set is_gui "--quiet"
} else {
  if { [alta::tcl_is_color] } { set is_color "--color" }
  if { [alta::tcl_is_gui  ] } { set is_gui   "--gui"   }
}

#######################################################################

set progs {}
set titles {}
for {set id 1} {$id <= $COUNT} {incr id} {
set result_dir "$results/$id"
file mkdir $result_dir

set seed    [get_rand_value $SEEDS   ]
set effort  [get_rand_value $EFFORTS ]
set skew    [get_rand_value $SKEWS   ]
set fitter  [get_rand_value $FITTERS ]
set fitting [get_rand_value $FITTINGS]
set holdx   [get_rand_value $HOLDXS  ]

set prog [list [info nameofexec] $is_quiet $is_color $is_gui -B --batch --mode $MODE]
alta::lconcat prog [list -X "set QUARTUS_SDC $QUARTUS_SDC"]
if { $CORNER != "" } {
  alta::lconcat prog [list -X "set CORNER $CORNER"]
}
alta::lconcat prog [list -X "set RESULT_DIR $result_dir"]
if { $seed != "" } {
  alta::lconcat prog [list -X "set SEED $seed"]
}
if { $effort != "" } {
  alta::lconcat prog [list -X "set EFFORT $effort"]
}
if { $fitter != "" } {
  alta::lconcat prog [list -X "set FITTER $fitter"]
}
if { $fitting != "" } {
  alta::lconcat prog [list -X "set FITTING $fitting"]
}
if { $skew != "" } {
  alta::lconcat prog [list -X "set SKEW $skew"]
}
if { $holdx != "" } {
  alta::lconcat prog [list -X "set HOLDX $holdx"]
}
#alta::lconcat prog [list -F af_run.tcl]
lappend progs $prog
lappend titles "#$id $result_dir"
}

#######################################################################

if { $is_parallel } {
  set bg_progs {}
  foreach bg_prog $progs {
    lappend bg_progs [lappend bg_prog $is_quiet]
  }
  bg_exec_queue $titles $bg_progs $JOBS
}

#######################################################################

for {set id 1} {$id <= $COUNT} {incr id} {
set result_dir "$results/$id"
set prog  [lindex $progs  [expr $id-1]]
set title [lindex $titles [expr $id-1]]
if { ! $is_parallel } {
  puts $title
  puts $prog
  eval exec -ignorestderr $prog >&@ stdout
}

print "***************************************************************************\n" >> $summary
print "$title\n" >> $summary
cat "$result_dir/alta_db/fmax.rpt" >> $summary
cat "$result_dir/alta_db/xfer.rpt" >> $summary
print "" >> $summary
}

alta::tcl_highlight "Check $summary for result.\n"
