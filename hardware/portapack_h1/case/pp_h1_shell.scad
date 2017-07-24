include <pp_h1_parameters.scad>

$fs=0.1;

module attach_corner() {
    circle(attach_foot_r);
    polygon([[-10, -10],[attach_foot_r, -10],[attach_foot_r, 0],[0, attach_foot_r],[-10, attach_foot_r]]);
}

module attach_side() {
    circle(attach_foot_r);
    translate([0, -attach_foot_r]) square([10, attach_foot_r * 2]);
}

module attach_center() {
    circle(attach_foot_r);
}

module pcb_supports() {
    translate(mounting_drills[0]) attach_corner();
    translate(mounting_drills[1]) rotate(90) attach_corner();
    translate(mounting_drills[2]) rotate(270) attach_corner();
    translate(mounting_drills[3]) rotate(180) attach_corner();
}

module pcb_attach_drill_outline() {
    circle(r=attach_drill_r);
}

module pcb_attach_drills_outline() {
    for(p = mounting_drills) {
        translate(p) pcb_attach_drill_outline();
    }
}

module pcb_attach_drills_volume() {
    translate([0, 0, -pcb_attach_drills_depth]) linear_extrude(height=30) {
        pcb_attach_drills_outline();
    }
}

module case_bottom_void_edge() {
    // Edge of PCB, plus case clearance, minus board supports.
    difference() {
        pcb_outline_clearance();
        pcb_supports();
    }
}

module case_bottom_void_tool_path() {
    // Tool path to cut bottom of case.
    offset(r=-case_bottom_tool_r) {
        case_bottom_void_edge();
    }
}

module case_bottom_void_tool_volume_ball() {
    $fs=2;
    $fn=18;
    // Tool cut volume for bottom of case.
    // Z=0 at bottom plane of H1 PCB
    translate([0, 0, -h1_pcb_bottom_clearance + case_bottom_tool_r]) minkowski() {
        linear_extrude(height=50, convexity=10) {
            case_bottom_void_tool_path();
        }
        sphere(r=case_bottom_tool_r);
    }
}

module case_outer_volume_radiused() {
    $fs=2;
    $fn=18;
    tool_r = case_bottom_tool_r + case_thickness;
    tz = h1_pcb_bottom_clearance + case_bottom_thickness - tool_r;
    difference() {
        // Rounded volume
        translate([0, 0, -tz]) {
            minkowski() {
                linear_extrude(height=30, convexity=10) {
                    offset(r=-case_bottom_tool_r) {
                        pcb_outline_clearance();
                    }
                }
                sphere(r=tool_r);
            }
        }
        
        // Cut off the top.
        translate([-10, -10, case_height_above_datum]) cube([200, 200, 200]);
    }
}

module case_bottom_void_tool_volume_end() {
    // Tool cut volume for bottom of case.
    // Z=0 at bottom plane of H1 PCB
    translate([0, 0, -h1_pcb_bottom_clearance]) {
        linear_extrude(height=50) {
            minkowski() {
                case_bottom_void_tool_path();
                circle(r=case_bottom_tool_r);
            }
        }
    }
}

module case_bumper_emboss_outline() {
    circle(r=case_bumper_d / 2 + case_bumper_clearance);
}

module case_bumper_emboss_tool() {
    height = 10;
    translate([0, 0, -height]) linear_extrude(height=height) {
        case_bumper_emboss_outline();
    }
}

module case_bumpers_emboss() {
    tz = case_height_below_datum - case_bumper_emboss_depth;
    translate([0, 0, -tz]) {
        translate([case_bumper_inset_from_pcb_edge, case_bumper_inset_from_pcb_edge, 0]) case_bumper_emboss_tool();
        translate([pcb_l - case_bumper_inset_from_pcb_edge, case_bumper_inset_from_pcb_edge, 0]) case_bumper_emboss_tool();
        translate([case_bumper_inset_from_pcb_edge, pcb_w - case_bumper_inset_from_pcb_edge, 0]) case_bumper_emboss_tool();
        translate([pcb_l - case_bumper_inset_from_pcb_edge, pcb_w - case_bumper_inset_from_pcb_edge, 0]) case_bumper_emboss_tool();
    }
}

module case_pcb_plane_void_tool_edge() {
    offset(r=-pcb_corner_r) {
        pcb_outline_clearance();
    }
}

module case_pcb_plane_void_tool_volume() {
    linear_extrude(height=30, convexity=10) {
        minkowski() {
            case_pcb_plane_void_tool_edge();
            circle(r=pcb_corner_r);
        }
    }
}

module case_outer_volume_squared() {
    t = case_bottom_thickness + h1_pcb_bottom_clearance;
    translate([0, 0, -t]) linear_extrude(height=t + case_height_above_datum) {
        minkowski() {
            pcb_outline();
            circle(r=pcb_case_clearance + case_thickness);
        }
    }
}
