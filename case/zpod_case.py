"""Parametric zPod case (v0.1).

Writes the STLs to case/stl/. Run with:
    case/.venv/bin/python case/zpod_case.py

Coordinate system of the assembled device (mm):
    X = width, Y = height (0 = bottom edge, where USB-C and headphone jack are),
    Z = thickness (0 = front face, T = back face).

Parts:
    front   - tray with the front face and the four walls (prints face down)
    back    - screwed cover with the speaker grille and the TP4056 cradle
    buttons - 1 play + 2 skip + 2 volume

ALL component dimensions below are nominal. Measure the real parts with
calipers and adjust the PARAMETERS section before the final print.
"""
from pathlib import Path
import math

import numpy as np
import trimesh
from manifold3d import CrossSection, Manifold

# ----------------------------------------------------------------- PARAMETERS
# Body
W, H, T = 66.0, 114.0, 23.0   # outer width, height, thickness
R = 10.0                      # corner radius
WALL = 2.0                    # side walls
FRONT = 2.0                   # front face thickness
LID = 2.0                     # back cover thickness
CHAMFER = 1.5                 # chamfer on the front and back edges
FIT = 0.25                    # general fit clearance

# 2.0" ST7789 display (measure!)
DISP_ACTIVE = (30.6, 40.8)    # visible area (width, height)
DISP_PCB = (36.0, 60.0)       # module PCB
DISP_MARGIN_TOP = 6.0         # from the PCB top edge to the visible area
DISP_WINDOW_TOP = 104.0       # Y of the window top on the case
DISP_WIN_EXTRA = 0.5          # window margin around the visible area
DISP_RIB_H = 3.5              # height of the corner ribs locating the PCB

# Buttons (6x6x5 tactile switch on perfboard, 2.54 mm pitch)
PITCH = 2.54
BTN_CENTER = (W / 2, 26.0)
BTN_STEP = 6 * PITCH          # distance from play to the other buttons
BTN_D_PLAY = 12.0
BTN_D_SMALL = 10.5
BTN_FLANGE = 1.25             # flange that keeps the button from falling out the front
BTN_FLANGE_T = 1.2
BTN_PROUD = 1.4               # how far the button sticks out
BTN_GAP = 0.3                 # radial hole clearance
SWITCH_H = 5.0                # total tactile switch height
POST_DX = 7.5 * PITCH         # perfboard posts: half spacing in X
POST_DY = 7 * PITCH           # same in Y
POST_D = 6.5

# Fasteners (M2.5 x 4 heat-set insert, M2.5 x 6 screw)
INSERT_D = 3.2
INSERT_DEPTH = 7.0
SCREW_D = 2.9
SCREW_HEAD_D = 5.2
SCREW_HEAD_H = 1.0
BOSS_D = 7.0
BOSS_INSET = 6.5              # distance from boss centre to the outer walls

# Cover
LIP_T = 1.5
LIP_H = 2.5

# TP4056 USB-C (measure!)
USB_X = 45.0                  # connector centre in X
USB_OPEN = (10.0, 4.0)        # opening (width, height)
TP_PCB = (17.5, 28.0, 1.6)    # PCB width, length, thickness
TP_CONN_H = 3.3               # connector height above the PCB

# PJ-307 headphone jack soldered to the button perfboard (measure!)
JACK_X = 24.0
JACK_D = 7.0
JACK_AXIS = 2.5               # jack axis height above the perfboard

# 28 mm speaker
SPK_CENTER = (19.0, 29.0)
SPK_D = 28.0
SPK_RING_H = 3.0

# TTP223 touch sensor (pocket in the top wall)
TOUCH_POCKET = (16.0, 12.0, 0.8)   # X width, Z height, depth
TOUCH_Z = 14.0

SEG = 64
# ---------------------------------------------------------------------------

Z_LID = T - LID                       # inner plane of the cover
Z_PERF = FRONT + BTN_FLANGE_T + 0.2 + SWITCH_H   # perfboard face pointing to the front
BOSSES = [(BOSS_INSET, BOSS_INSET), (W - BOSS_INSET, BOSS_INSET),
          (BOSS_INSET, H - BOSS_INSET), (W - BOSS_INSET, H - BOSS_INSET)]
POSTS = [(BTN_CENTER[0] + sx * POST_DX, BTN_CENTER[1] + sy * POST_DY)
         for sx in (-1, 1) for sy in (-1, 1)]
BUTTONS = {
    "play": (BTN_CENTER[0], BTN_CENTER[1], BTN_D_PLAY),
    "prev": (BTN_CENTER[0] - BTN_STEP, BTN_CENTER[1], BTN_D_SMALL),
    "next": (BTN_CENTER[0] + BTN_STEP, BTN_CENTER[1], BTN_D_SMALL),
    "vol_up": (BTN_CENTER[0], BTN_CENTER[1] + BTN_STEP, BTN_D_SMALL),
    "vol_down": (BTN_CENTER[0], BTN_CENTER[1] - BTN_STEP, BTN_D_SMALL),
}


def rrect(inset=0.0):
    """Rounded device outline, shrunk by `inset`."""
    r = max(R - inset, 0.5)
    c = CrossSection.circle(r, SEG)
    pts = [(inset + r, inset + r), (W - inset - r, inset + r),
           (inset + r, H - inset - r), (W - inset - r, H - inset - r)]
    return CrossSection.batch_hull([c.translate(p) for p in pts])


def slab(inset, z0, z1):
    return Manifold.extrude(rrect(inset), z1 - z0).translate([0, 0, z0])


def box(x0, y0, z0, x1, y1, z1):
    return Manifold.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])


def cyl(x, y, z0, z1, d):
    return Manifold.cylinder(z1 - z0, d / 2, d / 2, SEG).translate([x, y, z0])


def union(parts):
    out = parts[0]
    for p in parts[1:]:
        out = out + p
    return out


def disp_rects():
    ww, wh = DISP_ACTIVE[0] + 2 * DISP_WIN_EXTRA, DISP_ACTIVE[1] + 2 * DISP_WIN_EXTRA
    win = (W / 2 - ww / 2, DISP_WINDOW_TOP - wh, W / 2 + ww / 2, DISP_WINDOW_TOP)
    pcb_top = DISP_WINDOW_TOP - DISP_WIN_EXTRA + DISP_MARGIN_TOP
    pcb = (W / 2 - DISP_PCB[0] / 2, pcb_top - DISP_PCB[1], W / 2 + DISP_PCB[0] / 2, pcb_top)
    return win, pcb


def make_front():
    # outer shell with a chamfer on the front edge
    outer = Manifold.batch_hull([slab(CHAMFER, 0, 0.01), slab(0, CHAMFER, Z_LID)])
    body = outer - slab(WALL, FRONT, Z_LID + 1)

    add = []
    # cover screw bosses
    for x, y in BOSSES:
        add.append(cyl(x, y, FRONT - 0.1, Z_LID, BOSS_D))
    # button perfboard posts
    for x, y in POSTS:
        add.append(cyl(x, y, FRONT - 0.1, Z_PERF, POST_D))
    # display corner ribs
    _, (px0, py0, px1, py1) = disp_rects()
    t, L, z0, z1 = 1.2, 7.0, FRONT - 0.1, FRONT + DISP_RIB_H
    for cx, sx in ((px0 - FIT, -1), (px1 + FIT, 1)):
        for cy, sy in ((py0 - FIT, -1), (py1 + FIT, 1)):
            xa, xb = sorted((cx, cx + sx * t))
            ya, yb = sorted((cy + sy * t, cy - sy * L))
            add.append(box(xa, ya, z0, xb, yb, z1))
            xa, xb = sorted((cx + sx * t, cx - sx * L))
            ya, yb = sorted((cy, cy + sy * t))
            add.append(box(xa, ya, z0, xb, yb, z1))
    body = body + union(add)
    body = body ^ outer            # nothing pokes outside the shell

    cut = []
    # display window
    (wx0, wy0, wx1, wy1), _ = disp_rects()
    cut.append(box(wx0, wy0, -1, wx1, wy1, FRONT + 0.01))
    # button holes
    for x, y, d in BUTTONS.values():
        cut.append(cyl(x, y, -1, FRONT + 0.01, d + 2 * BTN_GAP))
    # insert holes
    for x, y in BOSSES:
        cut.append(cyl(x, y, Z_LID - INSERT_DEPTH, Z_LID + 1, INSERT_D))
    for x, y in POSTS:
        cut.append(cyl(x, y, FRONT + 1.2, Z_PERF + 1, INSERT_D))
    # USB-C: slot in the bottom wall, open toward the cover
    usb_z0 = Z_LID - TP_PCB[2] - TP_CONN_H - 0.4
    cut.append(box(USB_X - USB_OPEN[0] / 2, -1, usb_z0, USB_X + USB_OPEN[0] / 2, WALL + 0.01, Z_LID + 1))
    # headphone jack: round hole in the bottom wall
    jack_z = Z_PERF + 1.6 + JACK_AXIS
    cut.append(Manifold.cylinder(WALL + 2, JACK_D / 2, JACK_D / 2, SEG)
               .rotate([-90, 0, 0]).translate([JACK_X, -1, jack_z]))
    # touch pocket in the top wall
    pw, ph, pd = TOUCH_POCKET
    cut.append(box(W / 2 - pw / 2, H - WALL - 0.01, TOUCH_Z - ph / 2,
                   W / 2 + pw / 2, H - WALL + pd, TOUCH_Z + ph / 2))
    return body - union(cut)


def make_back():
    outer = Manifold.batch_hull([slab(0, Z_LID, T - CHAMFER), slab(CHAMFER, T - 0.01, T)])
    add = []
    # lip that fits inside the walls
    lip = slab(WALL + FIT, Z_LID - LIP_H, Z_LID + 0.1) - slab(WALL + FIT + LIP_T, Z_LID - LIP_H - 1, Z_LID + 1)
    for x, y in BOSSES:
        lip = lip - cyl(x, y, 0, T, BOSS_D + 2 * FIT + 0.4)
    tw, tl, _ = TP_PCB
    lip = lip - box(USB_X - tw / 2 - 2.5, 0, 0, USB_X + tw / 2 + 2.5, WALL + FIT + LIP_T + 1, T)
    add.append(lip)
    # speaker ring
    sx, sy = SPK_CENTER
    ring = cyl(sx, sy, Z_LID - SPK_RING_H, Z_LID + 0.1, SPK_D + 2 * FIT + 2.4) \
        - cyl(sx, sy, 0, T, SPK_D + 2 * FIT)
    ring = ring - box(sx - 3, sy, 0, sx + 3, sy + SPK_D, T)   # wire passage
    add.append(ring)
    # TP4056 cradle: two rails and an end stop
    y0 = WALL
    for s in (-1, 1):
        xa = USB_X + s * (tw / 2 + FIT)
        xa, xb = sorted((xa, xa + s * 1.2))
        add.append(box(xa, y0 + 2, Z_LID - LIP_H, xb, y0 + tl + FIT + 1.5, Z_LID + 0.1))
    add.append(box(USB_X - tw / 2 - FIT - 1.2, y0 + tl + FIT, Z_LID - LIP_H,
                   USB_X + tw / 2 + FIT + 1.2, y0 + tl + FIT + 1.5, Z_LID + 0.1))
    body = outer + union(add)

    cut = []
    for x, y in BOSSES:
        cut.append(cyl(x, y, Z_LID - 1, T + 1, SCREW_D))
        cut.append(cyl(x, y, T - SCREW_HEAD_H, T + 1, SCREW_HEAD_D))
    # speaker grille: holes in concentric rings
    cut.append(cyl(sx, sy, Z_LID - 1, T + 1, 2.0))
    for radius, n in ((3.6, 6), (7.2, 12), (10.8, 18)):
        for i in range(n):
            a = 2 * math.pi * i / n
            cut.append(cyl(sx + radius * math.cos(a), sy + radius * math.sin(a), Z_LID - 1, T + 1, 2.0))
    return body - union(cut)


def icon(name, s):
    """Raised symbol fitting in a circle of radius s."""
    def tri(x0, x1):   # triangle pointing from x0 to x1
        h = abs(x1 - x0) * 0.6
        pts = [(x0, -h), (x1, 0), (x0, h)]
        if x1 < x0:
            pts.reverse()
        return CrossSection([pts])
    if name == "play":
        return tri(-0.45 * s, 0.6 * s)
    if name == "next":
        return tri(-0.7 * s, 0.0) + tri(0.0, 0.7 * s)
    if name == "prev":
        return tri(0.7 * s, 0.0) + tri(0.0, -0.7 * s)
    bar = CrossSection.square([1.3 * s, 0.34 * s], True)
    if name == "vol_down":
        return bar
    return bar + bar.rotate(90)


def make_button(name):
    """Button in print orientation: flange on the bed, symbol up."""
    _, _, d = BUTTONS[name]
    h = BTN_FLANGE_T + FRONT + BTN_PROUD
    b = cyl(0, 0, 0, BTN_FLANGE_T, d + 2 * BTN_FLANGE)
    top = Manifold.batch_hull([cyl(0, 0, 0, h - 0.6, d), cyl(0, 0, h - 0.01, h, d - 1.2)])
    b = b + top
    return b + Manifold.extrude(icon(name, d / 2 * 0.62), 0.6).translate([0, 0, h - 0.01])


def to_trimesh(m):
    mesh = m.to_mesh()
    return trimesh.Trimesh(vertices=np.asarray(mesh.vert_properties)[:, :3],
                           faces=np.asarray(mesh.tri_verts), process=True)


def main():
    out = Path(__file__).parent / "stl"
    out.mkdir(exist_ok=True)
    front = make_front()
    back = make_back()
    parts = {
        "zpod_front": front,                                   # already face down
        "zpod_back": back.rotate([180, 0, 0]).translate([0, H, T]),  # outer face on the bed
    }
    for name in BUTTONS:
        parts[f"zpod_button_{name}"] = make_button(name)
    for name, m in parts.items():
        tm = to_trimesh(m)
        tm.export(out / f"{name}.stl")
        size = tm.bounds[1] - tm.bounds[0]
        print(f"{name:24s} {size[0]:6.1f} x {size[1]:6.1f} x {size[2]:5.1f} mm  "
              f"vol {tm.volume / 1000:5.1f} cm3  watertight={tm.is_watertight}  bodies={tm.body_count}")
    # check: assembled front and back must not intersect
    inter = (front ^ back).volume()
    print(f"front/back interference: {inter:.2f} mm3")
    print(f"button perfboard: front face at Z={Z_PERF:.1f} mm; "
          f"posts at {[(round(x, 2), round(y, 2)) for x, y in POSTS]}")
    return parts, front, back


if __name__ == "__main__":
    main()
