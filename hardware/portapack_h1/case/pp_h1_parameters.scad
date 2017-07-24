pcb_l = 120;
pcb_w = 75;
pcb_corner_r = 4;
pcb_hole_r = 3.2 / 2;
pcb_hole_pad_r = 5.6 / 2;

h1_pcb_thickness = 1.64;
pp_h1_pcb_thickness = 1.56;

spacer_height = 12.0;

bolt_drill_d = 3.0;
pcb_attach_drills_depth = 4.0;

pcb_case_clearance = 0.5;
case_thickness = 1.5;
case_bottom_thickness = case_thickness * 2;
h1_pcb_bottom_clearance = 4.0;
case_bottom_tool_r = 3.0;

h1_led_hole_diameter = 2;
h1_led_diffuser_thickness = 0.85;

case_pcb_n_clearance = h1_led_diffuser_thickness + 0.15;
case_pcb_w_clearance = pcb_case_clearance;
case_pcb_e_clearance = pcb_case_clearance;
case_pcb_s_clearance = pcb_case_clearance;

lcd_thickness = 3.8;
case_lid_thickness = 3.0 / 16.0 * 25.4;

case_height_above_datum = h1_pcb_thickness + spacer_height + pp_h1_pcb_thickness + case_lid_thickness;
case_height_below_datum = case_bottom_thickness + h1_pcb_bottom_clearance;
case_height = case_height_below_datum + case_height_above_datum;

attach_foot_r = pcb_hole_pad_r;
attach_drill_r = bolt_drill_d / 2.0;

case_bumper_d = 0.5 * 25.4;
case_bumper_clearance = 0.0;
case_bumper_emboss_depth = 1.0;

case_radiused = true;

case_bumper_inset_from_pcb_edge = case_radiused ? 14.0 : 12.0;

mounting_drills = [
    [4, 4],
    [116, 4],
    [4, pcb_w - 4],
    [116, pcb_w - 4]
];

module pcb_extents() {
    square([pcb_l, pcb_w]);
}

module pcb_outline() {
    minkowski() {
        offset(r=-pcb_corner_r) {
            pcb_extents();
        }
        circle(r=pcb_corner_r);
    }
}

module pcb_outline_clearance() {
    minkowski() {
        offset(r=-pcb_corner_r) {
            polygon([
                [0 - case_pcb_n_clearance, 0 - case_pcb_w_clearance],
                [0 - case_pcb_n_clearance, pcb_w + case_pcb_e_clearance],
                [pcb_l + case_pcb_s_clearance, pcb_w + case_pcb_e_clearance],
                [pcb_l + case_pcb_s_clearance, 0 - case_pcb_w_clearance]
            ]);
        }
        circle(r=pcb_corner_r);
    }
}
