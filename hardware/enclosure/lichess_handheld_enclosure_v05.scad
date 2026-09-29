// ESP32-C5 Lichess Handheld enclosure v0.5
// Units: mm
// Main fixes vs v0.4:
// 1) front/back fastening no longer needs 35 mm screws; rear shell now has deep screwdriver wells,
// 2) short M2.5 x 8 screws sit near the seam and thread directly into front-shell pilot bosses,
// 3) other short screws remain local by design: screen->front, ESP clips->back, battery door->back,
// 4) case well outer diameter is enlarged for FDM strength and collision-checked,
// 5) fit-test includes a real short-screw deep-well sample.

$fn = 72;
part = "assembly"; // front, back, battery_cover, esp_clip, fit_test, assembly

// ---------- Main envelope ----------
case_x = 130.0;
case_y = 86.0;
front_z = 7.5;
back_z = 31.5;
wall = 2.4;
corner_r = 6.0;
bezel_t = 2.2;

// ---------- Display: MSP4021 ----------
// Exact from user/seller + official drawing:
// PCB 108.04 x 61.74, AA 83.52 x 55.68,
// mounting pitch 102.04 x 54.90, 4 x Ø3.20,
// total thickness 5.65 without header / 14.00 with header.
screen_pcb_x = 108.04;
screen_pcb_y = 61.74;
screen_clear_total = 0.90; // design clearance, not source dimension
screen_recess_x = screen_pcb_x + screen_clear_total;
screen_recess_y = screen_pcb_y + screen_clear_total;
screen_x0 = (case_x-screen_pcb_x)/2;
screen_y0 = (case_y-screen_pcb_y)/2;

screen_hole_pitch_x = 102.04;
screen_hole_pitch_y = 54.90;
screen_hole_d = 3.20;
screen_hole_x0 = screen_x0 + (screen_pcb_x-screen_hole_pitch_x)/2;
screen_hole_y0 = screen_y0 + (screen_pcb_y-screen_hole_pitch_y)/2;

screen_aa_x = 83.52;
screen_aa_y = 55.68;
screen_aa_left = 14.36;
screen_aa_top = (screen_pcb_y-screen_aa_y)/2;
window_overlap = 0.28; // bezel mask/design choice
screen_window_x = screen_aa_x - 2*window_overlap;
screen_window_y = screen_aa_y - 2*window_overlap;
screen_window_x0 = screen_x0 + screen_aa_left + window_overlap;
screen_window_y0 = screen_y0 + screen_aa_top + window_overlap;

// Screen screws: M3 through PCB Ø3.20, into printed pilot bosses.
screen_boss_d = 7.2;
screen_pilot_d = 2.55;
// Support surface is chosen to leave the PCB inside the front cavity.
// Fine Z adjustment can be made later with thin washers if needed.
screen_support_z = 5.55;
screen_boss_h = screen_support_z - bezel_t;

// ---------- Battery holder ----------
// Exact from user's product image: 76 x 20 x 21 mm.
batt_x = 76.0;
batt_y = 20.0;
batt_z = 21.0;
// Adhesive/service allowance; design clearance.
batt_space_x = 81.0;
batt_space_y = 25.0;
batt_space_z = 23.5;
batt_x0 = (case_x-batt_x)/2;
batt_y0 = 5.5;

// Rear hatch exposes the cell/holder area but leaves end support zones.
batt_hatch_x = 72.0;
batt_hatch_y = 22.0;
batt_hatch_x0 = (case_x-batt_hatch_x)/2;
batt_hatch_y0 = batt_y0 + batt_y/2 - batt_hatch_y/2;
batt_cover_margin = 4.0;
batt_cover_x = batt_hatch_x + 2*batt_cover_margin;
batt_cover_y = batt_hatch_y + 2*batt_cover_margin;
batt_cover_t = 2.2;
batt_cover_lip_h = 0.8;
batt_cover_lip_clear = 0.35;
batt_cover_x0 = batt_hatch_x0 - batt_cover_margin;
batt_cover_y0 = batt_hatch_y0 - batt_cover_margin;

// Four M2.5 battery-door screws.
batt_screw_clear_d = 2.8;
batt_screw_pilot_d = 2.10;
batt_screw_head_d = 5.6;
batt_boss_d = 6.2;
batt_boss_depth = 7.5;
batt_screw_pts = [
    [batt_cover_x0+4.3, batt_cover_y0+4.3],
    [batt_cover_x0+batt_cover_x-4.3, batt_cover_y0+4.3],
    [batt_cover_x0+4.3, batt_cover_y0+batt_cover_y-4.3],
    [batt_cover_x0+batt_cover_x-4.3, batt_cover_y0+batt_cover_y-4.3]
];

// ---------- ESP32-C5-WIFI6-KIT-N32R8-UM ----------
// Exact overall outline from Waveshare drawing: 61.40 x 25.40 mm.
// Waveshare also shows Ø2.00 mounting holes and 50.16 mm X pitch / 22.86 mm Y pitch,
// but the public image does not fully dimension the absolute hole offsets from the overall
// USB-overhang envelope. So this revision intentionally uses screw-down corner clips rather
// than inventing direct-hole coordinates.
esp_x = 61.40;
esp_y = 25.40;
esp_x0 = 8.0;
esp_y0 = 38.5;
esp_pad = 3.8;
esp_under_clear = 6.0; // design space for solder/header protrusion
esp_board_z = back_z - wall - esp_under_clear;

// Four M2.5 screw-down clips around board corners.
esp_clip_boss_d = 6.0;
esp_clip_pilot_d = 2.10;
esp_clip_gap = 2.7;
esp_clip_pts = [
    [esp_x0-esp_clip_gap, esp_y0-esp_clip_gap],
    [esp_x0+esp_x+esp_clip_gap, esp_y0-esp_clip_gap],
    [esp_x0-esp_clip_gap, esp_y0+esp_y+esp_clip_gap],
    [esp_x0+esp_x+esp_clip_gap, esp_y0+esp_y+esp_clip_gap]
];

// Oversized dual-USB service opening; exact connector Z stack is not published,
// so this is intentionally generous rather than pretending to know it exactly.
usb_open_y = 30.0;
usb_open_z = 16.0;
usb_center_y = esp_y0 + esp_y/2;
usb_center_z = esp_board_z - 2.0;

// ---------- Rocker switch ----------
// Exact from user's table: face 21 x 15, panel cutout 12 x 19 mm.
switch_cut_y = 19.35; // +0.35 total FDM clearance
switch_cut_z = 12.35;
switch_center_y = 18.0;
switch_center_z = 17.0;
// Panel thickness is NOT in the seller table. 1.5 mm is a design assumption
// for common snap tabs and is included in the fit-test coupon.
switch_local_wall = 1.5;

// ---------- Internal antenna ----------
// Exact antenna body from Waveshare: 109 x 10 mm, SMA male.
// No wall hole. The whip is pressed into open saddles from the open rear shell
// before the front is screwed on. Ends are open, so the unknown SMA/pigtail
// coupling length does not need to be dimensioned into the case.
ant_len = 109.0;
ant_d = 10.0;
ant_cavity_d = 10.9; // design clearance
ant_x0 = (case_x-ant_len)/2;
ant_y = 70.5;
ant_center_z = back_z - wall - 3.0;
ant_saddle_len = 7.0;
ant_saddle_y = 13.0;
ant_saddle_z = 6.0;
ant_saddle_xs = [22, 47, 92, 117];

// ---------- Front/back case screws ----------
// Six SHORT M2.5 screws. The screw heads do NOT stay on the outer rear surface.
// A deep access well lets a screwdriver reach almost to the front/back seam,
// so an M2.5 x 8 screw only crosses a 3.0 mm bridge and then threads ~5 mm
// into the front-shell pilot boss. No 35 mm case screw is required.
case_screw_clear_d = 2.8;
case_pilot_d = 2.10;
case_head_access_d = 6.2;   // fits common M2.5 pan/socket heads + tool clearance
case_front_boss_d = 6.4;
case_well_od = 9.6;         // back-shell access-well outer diameter
case_screw_bridge_h = 3.0;  // material between deep well seat and front/back seam
case_screw_recommended_len = 8.0;
case_screw_pts = [
    [5.8,5.8], [case_x-5.8,5.8],
    [5.8,case_y-5.8], [case_x-5.8,case_y-5.8],
    [5.8,24.5], [case_x-5.8,32.0]
];

// ---------- Helpers ----------
module rounded_rect_2d(w,h,r) {
    hull() {
        translate([r,r]) circle(r=r);
        translate([w-r,r]) circle(r=r);
        translate([r,h-r]) circle(r=r);
        translate([w-r,h-r]) circle(r=r);
    }
}

module rounded_box(w,h,z,r) {
    linear_extrude(height=z) rounded_rect_2d(w,h,r);
}

module cyl_x(len,d) {
    rotate([0,90,0]) cylinder(h=len,d=d);
}

// ---------- Geometry sanity checks ----------
function dist2d(a,b) = sqrt(pow(a[0]-b[0],2)+pow(a[1]-b[1],2));
function boss_clearance(a,da,b,db) = dist2d(a,b) - da/2 - db/2;

// These asserts are evaluated when the model is compiled. They prevent a future
// parameter edit from silently recreating overlapping screw towers/holes.
for (cp=case_screw_pts)
    for (ep=esp_clip_pts)
        assert(boss_clearance(cp,case_well_od,ep,esp_clip_boss_d) >= 1.0,
               str("Case/ESP boss clearance too small: ",
                   boss_clearance(cp,case_well_od,ep,esp_clip_boss_d), " mm"));
for (sp=[
        [screen_hole_x0,screen_hole_y0],
        [screen_hole_x0+screen_hole_pitch_x,screen_hole_y0],
        [screen_hole_x0,screen_hole_y0+screen_hole_pitch_y],
        [screen_hole_x0+screen_hole_pitch_x,screen_hole_y0+screen_hole_pitch_y]])
    for (ep=esp_clip_pts)
        assert(boss_clearance(sp,screen_boss_d,ep,esp_clip_boss_d) >= 1.0,
               str("Screen/ESP boss clearance too small: ",
                   boss_clearance(sp,screen_boss_d,ep,esp_clip_boss_d), " mm"));

// ---------- Front shell ----------
module screen_bosses() {
    for (dx=[0,screen_hole_pitch_x])
        for (dy=[0,screen_hole_pitch_y])
            translate([screen_hole_x0+dx,screen_hole_y0+dy,bezel_t])
                difference() {
                    cylinder(h=screen_boss_h,d=screen_boss_d);
                    translate([0,0,-0.1]) cylinder(h=screen_boss_h+0.2,d=screen_pilot_d);
                }
}

module case_bosses_front() {
    for (p=case_screw_pts)
        translate([p[0],p[1],bezel_t])
            difference() {
                cylinder(h=front_z-bezel_t,d=case_front_boss_d);
                translate([0,0,-0.1]) cylinder(h=front_z-bezel_t+0.2,d=case_pilot_d);
            }
}

module front_shell() {
    union() {
        difference() {
            rounded_box(case_x,case_y,front_z,corner_r);

            // LCD active-area window.
            translate([screen_window_x0,screen_window_y0,-0.2])
                cube([screen_window_x,screen_window_y,bezel_t+0.4]);

            // Rear screen PCB/body cavity; open to rear mating face.
            translate([(case_x-screen_recess_x)/2,
                       (case_y-screen_recess_y)/2,
                       bezel_t])
                cube([screen_recess_x,screen_recess_y,front_z-bezel_t+0.3]);
        }
        screen_bosses();
        case_bosses_front();
    }
}

// ---------- Battery door ----------
module battery_cover() {
    difference() {
        union() {
            linear_extrude(height=batt_cover_t)
                offset(r=2.0) square([batt_cover_x-4.0,batt_cover_y-4.0],center=false);
            // Lip is printed upward; when installed it points into the rear hatch.
            translate([(batt_cover_x-(batt_hatch_x-batt_cover_lip_clear))/2,
                       (batt_cover_y-(batt_hatch_y-batt_cover_lip_clear))/2,
                       batt_cover_t])
                cube([batt_hatch_x-batt_cover_lip_clear,
                      batt_hatch_y-batt_cover_lip_clear,
                      batt_cover_lip_h]);
        }
        for (p=batt_screw_pts) {
            lx=p[0]-batt_cover_x0;
            ly=p[1]-batt_cover_y0;
            translate([lx,ly,-0.2])
                cylinder(h=batt_cover_t+batt_cover_lip_h+0.4,d=batt_screw_clear_d);
            translate([lx,ly,-0.2]) cylinder(h=1.2,d=batt_screw_head_d);
        }
    }
}

// ---------- ESP clip ----------
module esp_clip() {
    clip_l=11.0;
    clip_w=6.5;
    clip_t=2.2;
    difference() {
        hull() {
            translate([clip_w/2,clip_w/2,0]) cylinder(h=clip_t,d=clip_w);
            translate([clip_l-clip_w/2,clip_w/2,0]) cylinder(h=clip_t,d=clip_w);
        }
        translate([clip_w/2,clip_w/2,-0.2]) cylinder(h=clip_t+0.4,d=2.8);
    }
}

// ---------- Back shell internals ----------
module back_case_towers() {
    for (p=case_screw_pts)
        translate([p[0],p[1],0])
            cylinder(h=back_z-wall,d=case_well_od);
}

module battery_door_bosses() {
    for (p=batt_screw_pts)
        translate([p[0],p[1],back_z-wall-batt_boss_depth])
            cylinder(h=batt_boss_depth,d=batt_boss_d);
}

module battery_glue_tabs() {
    // Small internal end tabs: enough for adhesive without defining the holder itself.
    // The holder is still user-glued, not clipped or dimension-locked.
    tab_x = 5.0;
    tab_t = 2.0;
    translate([batt_x0,batt_y0,back_z-wall-tab_t]) cube([tab_x,batt_y,tab_t]);
    translate([batt_x0+batt_x-tab_x,batt_y0,back_z-wall-tab_t]) cube([tab_x,batt_y,tab_t]);
}

module esp_mounts() {
    // Four small PCB underside supports with 6 mm clearance to rear wall.
    for (x=[esp_x0+1.8,esp_x0+esp_x-1.8])
        for (y=[esp_y0+1.8,esp_y0+esp_y-1.8])
            translate([x-esp_pad/2,y-esp_pad/2,esp_board_z-esp_pad])
                cube([esp_pad,esp_pad,esp_pad]);

    // Screw bosses for the four separate retaining clips.
    for (p=esp_clip_pts)
        translate([p[0],p[1],esp_board_z-5.0])
            difference() {
                cylinder(h=5.0,d=esp_clip_boss_d);
                translate([0,0,-0.1]) cylinder(h=5.2,d=esp_clip_pilot_d);
            }
}

module antenna_saddle(xc) {
    // Open snap saddle: rod is inserted from the open/front side before case closure.
    // A 10.9 mm round cavity intersects the front edge of a 6 mm-deep support,
    // giving a wide mouth; PETG is ideal, PLA may need a little filing.
    difference() {
        translate([xc-ant_saddle_len/2,
                   ant_y-ant_saddle_y/2,
                   back_z-wall-ant_saddle_z])
            cube([ant_saddle_len,ant_saddle_y,ant_saddle_z]);
        translate([xc-ant_saddle_len/2-0.2, ant_y, ant_center_z])
            cyl_x(ant_saddle_len+0.4,ant_cavity_d);
    }
}

module antenna_saddles() {
    for (x=ant_saddle_xs) antenna_saddle(x);
}

module rear_vent_cuts() {
    // Rear vents over ESP area, not battery area.
    for (i=[0:5])
        translate([15+i*7.2,47,back_z-wall-0.2])
            hull() {
                cylinder(h=wall+0.4,d=1.8);
                translate([0,10,0]) cylinder(h=wall+0.4,d=1.8);
            }
}

module back_shell() {
    difference() {
        union() {
            // Correct orientation: open at z=0 (front mating face), rear wall at z=back_z.
            difference() {
                rounded_box(case_x,case_y,back_z,corner_r);
                translate([wall,wall,-0.2])
                    rounded_box(case_x-2*wall,
                                case_y-2*wall,
                                back_z-wall+0.25,
                                max(1.4,corner_r-wall));
            }
            back_case_towers();
            battery_door_bosses();
            battery_glue_tabs();
            esp_mounts();
            antenna_saddles();
        }

        // Rear battery hatch through the OUTSIDE rear wall.
        translate([batt_hatch_x0,batt_hatch_y0,back_z-wall-0.2])
            cube([batt_hatch_x,batt_hatch_y,wall+0.5]);

        // Battery wire relief toward electronics cavity.
        translate([batt_x0+batt_x-7.0,batt_y0+batt_y-1.0,back_z-wall-5.0])
            cube([6.0,5.0,5.2]);

        // Rocker opening through right side wall.
        translate([case_x-wall-0.3,
                   switch_center_y-switch_cut_y/2,
                   switch_center_z-switch_cut_z/2])
            cube([wall+0.6,switch_cut_y,switch_cut_z]);
        // Locally thin surrounding panel to 1.5 mm (fit-test this assumption).
        translate([case_x-wall-0.1,
                   switch_center_y-(switch_cut_y+4)/2,
                   switch_center_z-(switch_cut_z+4)/2])
            cube([wall-switch_local_wall+0.15,switch_cut_y+4,switch_cut_z+4]);

        // Oversized USB service opening on left wall.
        translate([-0.2,
                   usb_center_y-usb_open_y/2,
                   usb_center_z-usb_open_z/2])
            cube([wall+0.5,usb_open_y,usb_open_z]);

        rear_vent_cuts();

        // Front/back SHORT-screw wells.
        // Small clearance only through the 3 mm bridge at the seam.
        for (p=case_screw_pts) {
            translate([p[0],p[1],-0.2])
                cylinder(h=case_screw_bridge_h+0.4,d=case_screw_clear_d);
            // Large screwdriver/head access from the outside rear face almost to the seam.
            // Its flat bottom is the screw-head seat at z = case_screw_bridge_h.
            translate([p[0],p[1],case_screw_bridge_h])
                cylinder(h=back_z-case_screw_bridge_h+0.4,d=case_head_access_d);
        }

        // Battery-cover pilot holes through wall + boss.
        for (p=batt_screw_pts)
            translate([p[0],p[1],back_z-wall-batt_boss_depth-0.2])
                cylinder(h=batt_boss_depth+wall+0.4,d=batt_screw_pilot_d);
    }
}

// ---------- Fit-test coupon ----------
module fit_test() {
    // A) rocker cutout in 1.5 mm panel.
    translate([0,0,0])
        difference() {
            cube([31,25,switch_local_wall]);
            translate([(31-switch_cut_y)/2,(25-switch_cut_z)/2,-0.2])
                cube([switch_cut_y,switch_cut_z,switch_local_wall+0.4]);
        }

    // B) antenna saddle sample.
    translate([38,-ant_y+8,-(back_z-wall-ant_saddle_z)])
        antenna_saddle(4.0);

    // C) screen M3 pilot boss sample (2.55 mm CAD pilot).
    translate([64,10,0])
        difference() {
            cylinder(h=6,d=screen_boss_d);
            translate([0,0,-0.2]) cylinder(h=6.4,d=screen_pilot_d);
        }

    // D) M2.5 pilot bosses, left-to-right: 2.00 / 2.10 / 2.20 mm CAD.
    for (i=[0:2]) {
        d=[2.00,2.10,2.20][i];
        translate([82+i*11,10,0])
            difference() {
                cylinder(h=8,d=case_front_boss_d);
                translate([0,0,-0.2]) cylinder(h=8.4,d=d);
            }
    }

    // E) M2.5 through-hole plate, left-to-right: 2.70 / 2.80 / 2.90 mm.
    translate([118,1,0])
        difference() {
            cube([30,18,3]);
            for (i=[0:2])
                translate([6+i*9,9,-0.2]) cylinder(h=3.4,d=[2.70,2.80,2.90][i]);
        }

    // F) Short case-screw well sample. Insert M2.5x8 from the wide end.
    // The head should seat at z=3 mm and the tip should bite into the 2.10 mm pilot below.
    translate([158,10,0])
        difference() {
            cylinder(h=14,d=case_well_od);
            translate([0,0,case_screw_bridge_h])
                cylinder(h=11.2,d=case_head_access_d);
            translate([0,0,-0.2])
                cylinder(h=case_screw_bridge_h+0.4,d=case_screw_clear_d);
        }
    // Separate front-pilot block placed directly under the well in a real assembly.
    translate([173,10,0])
        difference() {
            cylinder(h=5.3,d=case_front_boss_d);
            translate([0,0,-0.2]) cylinder(h=5.7,d=case_pilot_d);
        }
}

// ---------- Assembly preview ----------
module assembly() {
    color([0.20,0.22,0.24,0.48]) front_shell();
    translate([0,0,front_z]) color([0.11,0.12,0.13,0.38]) back_shell();

    // Screen PCB and active-area reference.
    color([0.75,0.12,0.10,0.45])
        translate([screen_x0,screen_y0,screen_support_z])
            cube([screen_pcb_x,screen_pcb_y,1.6]);
    color([0.08,0.12,0.15,0.85])
        translate([screen_x0+screen_aa_left,screen_y0+screen_aa_top,bezel_t-0.4])
            cube([screen_aa_x,screen_aa_y,0.35]);

    // Battery holder envelope: rear-mounted, glued by user.
    color([0.12,0.12,0.12,0.70])
        translate([batt_x0,batt_y0,front_z + back_z-wall-batt_z])
            cube([batt_x,batt_y,batt_z]);

    // ESP board envelope.
    color([0.05,0.30,0.17,0.82])
        translate([esp_x0,esp_y0,front_z + esp_board_z-1.6])
            cube([esp_x,esp_y,1.6]);

    // Internal antenna body reference.
    color([0.06,0.06,0.06,0.90])
        translate([ant_x0,ant_y,front_z+ant_center_z])
            cyl_x(ant_len,ant_d);
}

if (part=="front") front_shell();
else if (part=="back") back_shell();
else if (part=="battery_cover") battery_cover();
else if (part=="esp_clip") esp_clip();
else if (part=="fit_test") fit_test();
else assembly();
