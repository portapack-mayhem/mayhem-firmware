include <pp_h1_parameters.scad>

$fs=0.1;

module pcb_mounting_hole_drill() {
    circle(r=pcb_hole_r);
}

module pcb_mounting_hole_drills() {
    translate([ 64, 104]) pcb_mounting_hole_drill();
    translate([126, 104]) pcb_mounting_hole_drill();
    translate([176, 104]) pcb_mounting_hole_drill();
    translate([ 64, 171]) pcb_mounting_hole_drill();
    translate([131, 144]) pcb_mounting_hole_drill();
    translate([176, 171]) pcb_mounting_hole_drill();
}

////////////////////////////////////////////////////////

module pcb_cutout_antenna_pos() {
    x = 61.5;
    y1 = 154.75;
    y2 = 167.25;
    r = 1.5;
    minkowski() {
        polygon(
            points=[[x+8,y1-4],[x,y1-4],[x,y1],[x+4,y1],[x+4,y2],[x,y2],[x,y2+4],[x+8,y2+4]]
        );
        circle(r=r);
    };
}

module pcb_cutout_antenna_neg_curve() {
    x = 59.5;
    y1 = 157;
    y2 = 165;
    r = 1.5;
    minkowski() {
        polygon(
            points=[[x-2,y1],[x,y1],[x,y2],[x-2,y2]]
        );
        circle(r=r);
    };
}

module pcb_cutout_antenna_neg() {
    x = 60.5;
    y1 = 157;
    y2 = 165;
    w = -4;
    union() {
        polygon(
            points=[[x+w,y1-3],[x,y1-3],[x,y2+3],[x+w,y2+3]]
        );
        pcb_cutout_antenna_neg_curve();
    }
}

module pcb_cutout_antenna() {
    difference() {
        pcb_cutout_antenna_neg();
        pcb_cutout_antenna_pos();
    }
}

////////////////////////////////////////////////////////

module pcb_cutout_clocks_pos() {
    x = 178.5;
    y1 = 138.75;
    y2 = 169.25;
    r = 1.5;
    w = 8;
    w2 = 4;
    minkowski() {
        polygon(
            points=[[x-w,y1-4],[x,y1-4],[x,y1],[x-w2,y1],[x-w2,y2],[x,y2],[x,y2+4],[x-w,y2+4]]
        );
        circle(r=r);
    };
}

module pcb_cutout_clock_neg_curve() {
    x = 180.5;
    y1 = 141;
    y2 = 167;
    r = 1.5;
    w = 2;
    minkowski() {
        polygon(
            points=[[x,y1],[x+w,y1],[x+w,y2],[x,y2]]
        );
        circle(r=r);
    };
}

module pcb_cutout_clocks_neg() {
    x = 179.5;
    y1 = 141;
    y2 = 167;
    w = 6;
    union() {
        polygon(
            points=[[x,y1-3],[x+w,y1-3],[x+w,y2+3],[x,y2+3]]
        );
        pcb_cutout_clock_neg_curve();
    }
}

module pcb_cutout_clocks() {
    difference() {
        pcb_cutout_clocks_neg();
        pcb_cutout_clocks_pos();
    }
}

////////////////////////////////////////////////////////

bulkhead_w = 6.35;
bulkhead_h = 6.35;
bulkhead_thickness = 1.02;

barrel_d = 6.2;
barrel_clip_d = 5.5;
barrel_l = 12.45 - bulkhead_thickness;

barrel_r = barrel_d / 2;
barrel_clip_r = barrel_clip_d / 2;

peg_l = 3.81;
peg_w = 1.02;
peg_bottom_h = 0.76;
peg_top_h = 1.27;
peg_space = 1.78;

module sma_73251_2120_pegs() {
    peg_top_ty = bulkhead_h/2 - peg_bottom_h - peg_space - peg_top_h;
    linear_extrude(height=peg_l)
    {
    translate([-bulkhead_w/2, peg_top_ty])
        square([peg_w, peg_top_h]);
    translate([bulkhead_w/2 - peg_w, peg_top_ty])
        square([peg_w, peg_top_h]);
    translate([-bulkhead_w/2, bulkhead_h/2 - peg_bottom_h])
        square([peg_w, peg_bottom_h]);
    translate([bulkhead_w/2 - peg_w, bulkhead_h/2 - peg_bottom_h])
        square([peg_w, peg_bottom_h]);
    }
}

module sma_73251_2120_barrel_outline_circle() {
    circle(r=barrel_r);
}

module sma_73251_2120_barrel_outline() {
    intersection() {
        sma_73251_2120_barrel_outline_circle();
        square([barrel_clip_d, barrel_d + 1], center=true);
    }
}

module sma_73251_2120_barrel() {
    linear_extrude(height=barrel_l) {
        sma_73251_2120_barrel_outline();
    }
}

module sma_73251_2120_bulkhead() {
    linear_extrude(height=bulkhead_thickness) {
        square([bulkhead_w, bulkhead_h], center=true);
    }
}

module sma_73251_2120_union() {
    union() {
        translate([0, 0, -peg_l]) sma_73251_2120_pegs();
        sma_73251_2120_bulkhead();
        translate([0, 0, bulkhead_thickness]) sma_73251_2120_barrel();
    }
}
/*
module sma_73251_2120() {
    ty = bulkhead_h/2 - peg_bottom_h - peg_space/2;
    rotate([90, 0, 0]) translate([0, -ty, 0]) {
        union() {
            translate([0, 0, -peg_l]) sma_73251_2120_pegs();
            sma_73251_2120_bulkhead();
            translate([0, 0, bulkhead_thickness]) sma_73251_2120_barrel();
        }
    }
}
*/
module sma_73251_2120_orient(board_thickness) {
    // Align so that top surface of bottom peg is at z=0 (bottom of PCB).
    t = peg_top_h / 2 + (peg_space - board_thickness) / 2;
    translate([0, 0, -t]) rotate([90, 0, -90]) {
        children();
    }
}

module sma_73251_2120(refdes, board_thickness) {
    sma_73251_2120_orient(board_thickness) {
        sma_73251_2120_union();
    }
}

module sma_73251_2120_drill(tolerance, board_thickness) {
    sma_73251_2120_orient(board_thickness) {
        linear_extrude(height=30) {
            minkowski() {
                sma_73251_2120_barrel_outline_circle();
                circle(r=tolerance);
            }
        }
    }
}

////////////////////////////////////////////////////////

module led(refdes, c) {
    rotate(90) translate([-0.25, -2.15/2, -0.60]) {
        color("gray") linear_extrude(height=0.60) {
            square([0.50, 2.15]);
            translate([0, 2.15/2]) circle(r=0.5);
        }
    }
}

module led_drill() {
    translate([0, -0.25, -0.3]) {
        rotate([90, 0, 0]) {
            cylinder(d=h1_led_hole_diameter, h=10);
        }
    }
}

////////////////////////////////////////////////////////

module sw_outline() {
    circle(d=3.51, center=true);
}

sw_a = 3.25;
sw_l = 5.85;
sw_tz = sw_l - sw_a;

sw_large_hole_spacing = 7.01;
sw_large_hole_diameter = 1.30;
sw_small_hole_spacing = 4.50;
sw_small_hole_diameter = 0.99;
sw_large_small_hole_spacing = 2.49;
sw_pin_length_below_datum = 3.51;

sw_button_z_offset = 4.01;

module sw() {
    rotate([180, 0, 90]) {
        rotate([90, 0, 0]) {
            translate([0, 4.01, sw_tz]) {
                color("gray") translate([-7.11/2, -sw_button_z_offset, -3.68]) linear_extrude(height=3.68) square([7.11, 7.01]);
                color("blue") linear_extrude(height=sw_a) sw_outline();
            }
        }
        
        rotate([180, 0, 180]) linear_extrude(height=sw_pin_length_below_datum) {
            translate([-sw_large_hole_spacing/2, sw_large_small_hole_spacing]) circle(d=sw_large_hole_diameter);
            translate([ sw_large_hole_spacing/2, sw_large_small_hole_spacing]) circle(d=sw_large_hole_diameter);
            translate([-sw_small_hole_spacing/2, 0]) circle(d=sw_small_hole_diameter);
            translate([ sw_small_hole_spacing/2, 0]) circle(d=sw_small_hole_diameter);
        }
    }
}

module sw_drill(clearance) {
    translate([0, 0, -sw_button_z_offset]) {
        rotate([0, -90, 0]) {
            linear_extrude(h=10) {
                minkowski() {
                    sw_outline();
                    circle(r=clearance);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////

module header_x2(nx, b) {
    ny = 2;
    
    w = 5.08;
    d = 8.50;
    
    pin_spacing_x = 2.54;
    pin_spacing_y = 2.54;
    pin_d = 1.02;
    pin_length = 3.2;
    
    rotate([180, 0, 0]) {
        color("gray") translate([-b/2, -w/2, 0]) linear_extrude(height=d) square([b, w]);
        
        pin_tx = nx * pin_spacing_x / -2;
        pin_ty = ny * pin_spacing_y / -2;
        translate([pin_tx, pin_ty]) {
            for(y = [1 : ny]) {
                for(x = [1 : nx]) {
                    tx = (x - 0.5) * pin_spacing_x;
                    ty = (y - 0.5) * pin_spacing_y;
                    translate([tx, ty]) {
                        rotate([180, 0]) {
                            linear_extrude(height=pin_length) {
                                circle(d=pin_d);
                            }
                        }
                    }
                }
            }
        }
    }
}

module header_11x2() {
    nx = 11;
    b = 28.44;
    header_x2(nx, b);
}

module header_13x2() {
    nx = 13;
    b = 33.52;
    header_x2(nx, b);
}

////////////////////////////////////////////////////////

module usb_plug_poly() {
    inner_w1 = 6.9;
    inner_h1 = 1.1;
    inner_h = 1.85;
    inner_w2 = 5.4;
    inner_dw = inner_w1 - inner_w2;
    
    translate([-inner_w1/2, 0])
        polygon(points=[
            [0, 0],
            [inner_w1, 0],
            [inner_w1, inner_h1],
            [inner_w1 - inner_dw/2, inner_h],
            [inner_dw/2, inner_h],
            [0, inner_h1]
        ]);
}

module usb_body_outline() {
    body_buffer_r = 0.3;

    translate([0, body_buffer_r]) {
        minkowski() {
            usb_plug_poly();
            circle(r=body_buffer_r);
        }
    }
}

module usb_plug_outline() {
    outer_h = 3;
    outer_ty = (outer_h - 2.45) / 2;
    outer_buffer_r = 0.6;

    translate([0, outer_ty]) {
        minkowski() {
            usb_plug_poly();
            circle(r=outer_buffer_r);
        }
    }
}

usb_body_h = 2.45;
usb_body_depth = 5.0;

//usb_outer_w1 = 8;
usb_outer_depth = 0.63;

module usb_transform() {
    rotate([90, 180, 270]) translate([0, 0, -usb_outer_depth - 2.15 + 1.65]) children();
}

module usb() {    
    color("lightgray") usb_transform() {
        translate([0, 0, usb_outer_depth]) {
            linear_extrude(height=usb_body_depth) {
                usb_body_outline();
            }
        }

        linear_extrude(height=usb_outer_depth) {
            usb_plug_outline();
        }
    }
}

module usb_drill(clearance) {
    usb_transform() {
        translate([0, 0, -usb_outer_depth - 10]) {
            linear_extrude(height=20) {
                minkowski() {
                    usb_plug_outline();
                    circle(r=clearance);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////

module pcb_outline() {
    minkowski() {
        polygon(
            points=[[64,104], [176,104], [176,171], [64,171]]
        );
        circle(r=pcb_corner_r);
    }
}

module pcb_shape() {
    difference() {
        pcb_outline();
        pcb_cutout_antenna();
        pcb_cutout_clocks();
        pcb_mounting_hole_drills();
    }
}

////////////////////////////////////////////////////////

module hackrf_one_components() {
    color("green") linear_extrude(height=h1_pcb_thickness) pcb_shape();

    translate([ 61.00, 161.00]) rotate(  0) sma_73251_2120("p4" , h1_pcb_thickness);
    translate([179.00, 145.00]) rotate(180) sma_73251_2120("p2" , h1_pcb_thickness);
    translate([179.00, 163.00]) rotate(180) sma_73251_2120("p16", h1_pcb_thickness);

    translate([ 61.00, 117.90]) rotate(-90) led("d7", "green");
    translate([ 61.27, 130.55]) rotate(-90) led("d8", "yellow");
    translate([ 61.27, 135.12]) rotate(-90) led("d2", "red");
    translate([ 61.27, 139.69]) rotate(-90) led("d4", "green");
    translate([ 61.27, 144.27]) rotate(-90) led("d5", "yellow");
    translate([ 61.27, 148.84]) rotate(-90) led("d6", "red");

    translate([ 62.70, 111.40]) sw();
    translate([ 62.70, 124.40]) sw();

    translate([171.76, 143.25]) rotate([0, 0,  90]) header_11x2("p20");
    translate([152.71, 164.84]) rotate([0, 0, 180]) header_13x2("p22");
    translate([123.50, 143.25]) rotate([0, 0,  90]) header_11x2("p28");

    translate([180.00, 124.00]) usb();
}

module hackrf_one_transform() {
    rotate([180, 0, 0]) translate([-60, -100 - pcb_w])
        children();
}

module hackrf_one() {
    hackrf_one_transform() hackrf_one_components();
}

////////////////////////////////////////////////////////

module spacer() {
    outer_d = 0.25 * 25.4;
    inner_d = 0.140 * 25.4;
    
    //inner_d = ?
    rotate([0, 180, 0]) {
        color("lightgray") {
            difference() {
                linear_extrude(height=spacer_height) {
                    circle(d=outer_d);
                }
                translate([0, 0, -0.5]) {
                    linear_extrude(height=spacer_height + 1) {
                        circle(d=inner_d);
                    }
                }
            }
        }
    }
}

module screw() {
    wrench_sides = 6;
    wrench_diameter = 2.0 / cos(360 / wrench_sides / 2);
    
    head_height = 2.0;
    head_d = 5.5;
    
    shaft_length = 20.0;
    threaded_d = 3.0;
    
    color("gray") {
        translate([0, 0, -head_height]) difference() {
            linear_extrude(height=head_height)
                circle(d=head_d);
            translate([0, 0, -0.5]) linear_extrude(height=head_height + 1)
                circle(d=wrench_diameter, $fn=wrench_sides);
        }
        linear_extrude(height=shaft_length)
            circle(d=threaded_d);
    }
}

////////////////////////////////////////////////////////

module header_mle_dual(name, nx) {
    w = nx * 2.54;
    h = 5;
    d = 2.54;
    offset = 3.81 - d;
    
    base_h = 7.44;
    
    translate([-w/2, -h/2, offset]) {
        color("gray") linear_extrude(height=d) square([w, h]);
    }
    translate([-w/2, -base_h/2]) {
        color("lightgray") {
            linear_extrude(height=offset) square([w, base_h]);
        }
    }
}

module lcd_kingtech() {
    body_w = 42.72;
    body_h = 60.26;
    body_d = 2.50;
    
    touch_d = 0.7;
    
    tape_d = 0.1;
    
    view_w = 36.72;
    view_h = 48.96;
    view_tx = (body_w - view_w) / 2;
    view_ty = 1.25 + 2.95;
    
    tab_w = 0.7;
    tab_h = 2.5;
    tab_d = 0.9;
    tab_tz = body_d - tab_d;
    tab_bot_ty = body_h - tab_h;
    
    translate([-body_w / 2, -view_ty - view_h/2, -(body_d + touch_d)]) {
        translate([0, 0, touch_d]) {
            color("beige") difference() {
                linear_extrude(height=body_d) {
                    square([body_w, body_h]);
                }
                
                translate([view_tx, view_ty, -1]) {
                    linear_extrude(height=2) {
                        square([view_w, view_h]);
                    }
                }
            }

            color("beige") translate([0, 0, tab_tz]) {
                linear_extrude(height=tab_d) {
                    translate([-tab_w, 0]) square([tab_w, tab_h]);
                    translate([-tab_w, tab_bot_ty]) square([tab_w, tab_h]);
                    translate([body_w, 0]) square([tab_w, tab_h]);
                    translate([body_w, tab_bot_ty]) square([tab_w, tab_h]);
                }
            }

            color("black") translate([view_tx, view_ty]) {
                linear_extrude(height=1) {
                    square([view_w, view_h]);
                }
            }
        }
        
        color("lightgray", alpha=0.5) {
            linear_extrude(height=touch_d) {
                square([body_w, body_h]);
            }
        }
    }
}

module control_wheel() {
    h = 6.0;
    
    top_d = 32.0;
    top_h = 3.0;
    
    ring_d = 34.4;
    ring_h = 0.2;
    
    bot_d = ring_d;
    bot_h = h - ring_h - top_h;
    
    translate([0, 0, -h])
        color("white")
            linear_extrude(height=top_h)
                circle(d=top_d);
    
    translate([0, 0, -(h - top_h)])
        color("white")
            linear_extrude(height=ring_h)
                circle(d=ring_d);
    
    translate([0, 0, -(h - top_h - ring_h)])
        color("black")
            linear_extrude(height=bot_h)
                circle(d=bot_d);
}

module audio_jack_hole() {
    hole_outer_d = 5.00;

    circle(d=hole_outer_d);
}

audio_jack_body_w = 6.00;
audio_jack_body_h = 5.00;
audio_jack_body_depth = 15.5;

audio_jack_hole_inner_d = 3.600;
audio_jack_hole_depth = 1.5;

audio_jack_mounting_offset = 7;

module audio_jack() {
    color("gray") rotate([90, 0, 0]) {
        translate([0, audio_jack_body_h/2, -audio_jack_hole_depth - audio_jack_mounting_offset]) {
            translate([0, 0, audio_jack_hole_depth])
                linear_extrude(height=audio_jack_body_depth)
                    square([audio_jack_body_w, audio_jack_body_h], center=true);
            difference() {
                linear_extrude(height=audio_jack_hole_depth)
                    audio_jack_hole();
                translate([0, 0, -0.5])
                    linear_extrude(height=audio_jack_hole_depth + 1)
                        circle(d=audio_jack_hole_inner_d);
            }
        }
    }
}

module audio_jack_drill(diameter) {
    translate([0, audio_jack_mounting_offset, audio_jack_body_h/2]) {
        rotate([-90, 0, 0]) {
            linear_extrude(height=10) {
                circle(r=diameter / 2.0);
            }
        }
    }
}

micro_sd_body_h = 1.32;
micro_sd_body_w = 13.825;
micro_sd_body_depth = 15.25;

micro_sd_card_w = 11.0;
micro_sd_card_h = 1.0;
micro_sd_card_depth = 15.0;

micro_sd_card_tx = 0.9;
micro_sd_card_ty = (micro_sd_body_h - micro_sd_card_h) / 2;
micro_sd_card_insert_depth = micro_sd_card_depth - 2.3;

micro_sd_card_eject_depth = micro_sd_card_depth - (2.3 + 3.3);

module micro_sd() {
    translate([-micro_sd_body_w/2, -micro_sd_body_depth/2]) {
        rotate([90, 0, 0]) {
            color("lightgray") difference() {
                translate([0, 0, -micro_sd_body_depth])
                    linear_extrude(height=micro_sd_body_depth)
                        square([micro_sd_body_w, micro_sd_body_h]);
                translate([micro_sd_card_tx, micro_sd_card_ty, -micro_sd_card_insert_depth])
                    linear_extrude(height=micro_sd_card_depth)
                        square([micro_sd_card_w, micro_sd_card_h]);
            }

            color("black")
                translate([micro_sd_card_tx, micro_sd_card_ty, -micro_sd_card_eject_depth])
                    linear_extrude(height=micro_sd_card_depth)
                        square([micro_sd_card_w, micro_sd_card_h]);
        }
    }
}

module micro_sd_drill(clearance) {
    extra_width = 2;
    translate([-micro_sd_body_w/2, 0]) {
        rotate([90, 0, 0]) {
            translate([micro_sd_card_tx - clearance - extra_width, micro_sd_card_ty - clearance, micro_sd_body_depth/2]) {
                cube([micro_sd_card_w + 2 * clearance + extra_width, micro_sd_card_h + 2 * clearance, 10]);
            }
        }
    }
}

////////////////////////////////////////////////////////

module portapack_h1_pcb_mounting_hole_drills() {
    translate([ 64, 104]) pcb_mounting_hole_drill();
    translate([176, 104]) pcb_mounting_hole_drill();
    translate([ 64, 171]) pcb_mounting_hole_drill();
    translate([176, 171]) pcb_mounting_hole_drill();
}

module portapack_h1_pcb_shape() {
    difference() {
        pcb_outline();
        portapack_h1_pcb_mounting_hole_drills();
    }
}

module portapack_h1_pcb() {
    color("green") linear_extrude(height=pp_h1_pcb_thickness) {
        portapack_h1_pcb_shape();
    }
}

module portapack_h1_components_top() {
    translate([ 94.83, 137.50]) rotate(90) lcd_kingtech();
    translate([147.50, 137.50]) control_wheel();
}

module portapack_h1_components_bottom() {
    translate([0, 0, pp_h1_pcb_thickness])  {
        translate([171.76, 143.25]) rotate( 90) header_mle_dual("p20", 11);
        translate([152.71, 164.84]) rotate(180) header_mle_dual("p22", 13);
        translate([123.50, 143.25]) rotate( 90) header_mle_dual("p28", 11);
        translate([172.10, 114.80]) rotate(270) audio_jack();
        translate([ 68.40, 114.60]) rotate(270) micro_sd();
    }
}

module portapack_h1_assembly() {
    portapack_h1_pcb();
    portapack_h1_components_top();
    portapack_h1_components_bottom();
}

module portapack_h1_transform() {
    rotate([180, 0, 0]) translate([-60, -100 - pcb_w])
        children();
}

module portapack_h1() {
    portapack_h1_transform() portapack_h1_assembly();
}

////////////////////////////////////////////////////////

module slot() {
    hull() {
        children();
        translate([0, 0, -20]) children();
    }
}

module portapack_h1_drills() {
    micro_sd_clearance = 0.5;
    audio_jack_hole_diameter = 7.0;  // 6.5mm + 0.25mm clearance.

    portapack_h1_transform() {
        translate([172.10, 114.80, pp_h1_pcb_thickness]) rotate(270) audio_jack_drill(audio_jack_hole_diameter);
        slot() translate([ 68.40, 114.60, pp_h1_pcb_thickness]) rotate(270) micro_sd_drill(micro_sd_clearance);
    }
}

module hackrf_one_drills() {
    clearance = 0.5;
    sw_clearance = 0.6;

    hackrf_one_transform() {
        slot() translate([ 61.00, 161.00]) rotate(  0) sma_73251_2120_drill(clearance, h1_pcb_thickness);
        translate([179.00, 145.00]) rotate(180) sma_73251_2120_drill(clearance, h1_pcb_thickness);
        translate([179.00, 163.00]) rotate(180) sma_73251_2120_drill(clearance, h1_pcb_thickness);

        translate([ 61.00, 117.90]) rotate(-90) led_drill();
        translate([ 61.27, 130.55]) rotate(-90) led_drill();
        translate([ 61.27, 135.12]) rotate(-90) led_drill();
        translate([ 61.27, 139.69]) rotate(-90) led_drill();
        translate([ 61.27, 144.27]) rotate(-90) led_drill();
        translate([ 61.27, 148.84]) rotate(-90) led_drill();

        slot() translate([ 62.70, 111.40]) sw_drill(sw_clearance);
        slot() translate([ 62.70, 124.40]) sw_drill(sw_clearance);

        translate([180.00, 124.00]) usb_drill(clearance);
    }
}

module portapack_h1_stack_hackrf_one() {
    hackrf_one();
}

module portapack_h1_stack_spacers() {
    hackrf_one_transform() {
        translate([ 64, 104]) spacer();
        translate([176, 104]) spacer();
        translate([ 64, 171]) spacer();
        translate([176, 171]) spacer();
    }    
}

module portapack_h1_stack_portapack() {
    translate([0, 0, spacer_height + pp_h1_pcb_thickness]) portapack_h1();
}

module portapack_h1_stack_screws() {
    screw_tz = spacer_height + pp_h1_pcb_thickness;
    translate([0, 0, screw_tz]) portapack_h1_transform() {
        translate([ 64, 104]) screw();
        translate([176, 104]) screw();
        translate([ 64, 171]) screw();
        translate([176, 171]) screw();
    }
}

module portapack_h1_stack() {
    portapack_h1_stack_hackrf_one();
    portapack_h1_stack_spacers();
    portapack_h1_stack_portapack();
    portapack_h1_stack_screws();
}

module portapack_h1_stack_drills() {
    hackrf_one_drills();

    translate([0, 0, spacer_height + pp_h1_pcb_thickness]) portapack_h1_drills();
}
