set AGM_SUPRA true
set DESIGN "portapack_h4m_cpld"
set IPLIST {alta_bram alta_bram9k alta_sram alta_wram alta_pll alta_pllx alta_pllv alta_pllve alta_boot alta_osc alta_mult alta_multm alta_ufm alta_ufms alta_ufml alta_i2c alta_spi alta_irda alta_mcu alta_mcu_m3 alta_saradc alta_adc alta_dac alta_cmp }
lappend IPLIST alta_rv32

proc set_alta_partition {inst tag} {
  set full_name [get_name_info -observable_type pre_synthesis -info full_path $inst]
  set inst_name [get_name_info -observable_type pre_synthesis -info short_full_path $inst]
  set base_name [get_name_info -observable_type pre_synthesis -info instance_name $inst]
  set section_id [string map { [ _ ] _ . _ | _} $inst_name]
  eval "set_global_assignment -name PARTITION_COLOR 52377 -section_id $section_id -tag $tag"
  eval "set_global_assignment -name PARTITION_NETLIST_TYPE SOURCE -section_id $section_id -tag $tag"
  eval "set_global_assignment -name PARTITION_FITTER_PRESERVATION_LEVEL PLACEMENT_AND_ROUTING -section_id $section_id -tag $tag"
  eval "set_instance_assignment -name PARTITION_HIERARCHY $section_id -to $full_name -section_id $section_id -tag $tag"
}

load_package flow
if { $DESIGN == "" } {
  set DESIGN $::quartus(args)
}
project_open $DESIGN

set tag alta_auto
if { [llength $IPLIST] > 0 } {
  # A Quartus bug saves PARTITION_HIERARCHY assignments without tag. Use section_id to remove them.
  set asgn_col [get_all_global_assignments -name PARTITION_NETLIST_TYPE -tag $tag]
  foreach_in_collection part $asgn_col {
    set section_id [lindex $part 0]
    eval "remove_all_instance_assignments -name PARTITION_HIERARCHY -section_id $section_id"
  }
  eval "remove_all_global_assignments -name PARTITION_COLOR -tag $tag"
  eval "remove_all_global_assignments -name PARTITION_NETLIST_TYPE -tag $tag"
  eval "remove_all_global_assignments -name PARTITION_FITTER_PRESERVATION_LEVEL -tag $tag"
  catch { execute_module -tool map }

  foreach ip $IPLIST {
    foreach_in_collection inst [get_names -node_type hierarchy -observable_type pre_synthesis -filter "$ip:*"] {
      set_alta_partition $inst $tag
    }
    foreach_in_collection inst [get_names -node_type hierarchy -observable_type pre_synthesis -filter "*|$ip:*"] {
      set_alta_partition $inst $tag
    }
  }
}
eval "set_global_assignment -name EDA_MAINTAIN_DESIGN_HIERARCHY PARTITION_ONLY -section_id eda_simulation"

project_close

exit

