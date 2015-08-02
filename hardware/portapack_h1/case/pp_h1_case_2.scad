use <pp_h1_shell.scad>
//use <pp_h1_holes.scad>
use <pp_h1_stack.scad>
include <pp_h1_parameters.scad>

module case() {
    difference() {
        if( case_radiused ) {
            case_outer_volume_radiused();
        } else {
            case_outer_volume_squared();
        }
        
        union() {
            if( case_radiused ) {
                case_bottom_void_tool_volume_ball();
            } else {
                case_bottom_void_tool_volume_end();
            }
            
            case_pcb_plane_void_tool_volume();
            pcb_attach_drills_volume();
            case_bumpers_emboss();
            translate([0, 0, h1_pcb_thickness]) portapack_h1_stack_drills();
        }
    }
}

/* Cross section */
module case_cross_section() {
    difference() {
        case();
        translate([70, -10, -10]) cube([100, 100, 100]);
    }
}

module case_and_h1() {
    case();
    translate([0, 0, h1_pcb_thickness]) hackrf_one();
}

module case_and_stack() {
    case();
    translate([0, 0, h1_pcb_thickness]) portapack_h1_stack();
}

module stack_case_interference() {
    intersection() {
        case();
        translate([0, 0, h1_pcb_thickness]) portapack_h1_stack();
    }
}

module stack_stack_interference() {
    // Ensure stack and spacers do not interfere.
    intersection() {
        union() {
            portapack_h1_stack_hackrf_one();
            portapack_h1_stack_portapack();
        }
        portapack_h1_stack_spacers();
    }
    
    // Ensure screws do not interfere with stack.
    intersection() {
        union() {
            portapack_h1_stack_hackrf_one();
            portapack_h1_stack_spacers();
            portapack_h1_stack_portapack();
        }
        portapack_h1_stack_screws();
    }
}

case();
//case_and_h1();
//case_and_stack();
//case_cross_section();
//stack_case_interference();
//stack_stack_interference();
