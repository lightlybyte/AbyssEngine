"""
ursina_renderer.py

Render tri_map.json (per-mesh deduped triangle soup) in Ursina.

Usage:
    python ursina_renderer.py                # normal render
    python ursina_renderer.py --no-rotate    # static, no auto-spin
    python ursina_renderer.py --flat         # flat color, no per-mesh tint
    python ursina_renderer.py --wire         # wireframe only (debug)

Requires: pip install ursina
Place tri_map.json next to this script (or edit JSON_PATH).
"""

import json
import os
import sys
import math
from ursina import *


# ------------------------------------------------------------
# Paths — resolve relative to the script, not the cwd
# ------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JSON_PATH = os.path.join(SCRIPT_DIR, "tri_map.json")


# ------------------------------------------------------------
# CLI
# ------------------------------------------------------------
AUTO_ROTATE = "--no-rotate" not in sys.argv
FLAT_COLOR = "--flat" in sys.argv
WIREFRAME = "--wire" in sys.argv
ROTATE_SPEED = 20.0
BACKGROUND = color.dark_gray


# ------------------------------------------------------------
# Load the triangle map
# ------------------------------------------------------------
def load_meshes(path):
    if not os.path.exists(path):
        raise FileNotFoundError(
            f"tri_map.json not found.\n"
            f"Looking for:       {path}\n"
            f"Current directory: {os.getcwd()}\n"
            f"Script directory:  {SCRIPT_DIR}\n"
            f"Files in script dir: "
            f"{[f for f in os.listdir(SCRIPT_DIR) if 'json' in f.lower()]}"
        )
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def validate_mesh(m, idx):
    """Sanity check one mesh dict. Raise a clear error if broken."""
    for key in ("vertices", "indices", "n_verts", "n_tris"):
        if key not in m:
            raise ValueError(f"mesh[{idx}] missing key '{key}'")
    nv = len(m["vertices"])
    ni = len(m["indices"])
    if ni % 3 != 0:
        raise ValueError(f"mesh[{idx}] indices length {ni} not divisible by 3")
    if m["n_verts"] != nv:
        print(f"[warn] mesh[{idx}] n_verts={m['n_verts']} but len(vertices)={nv}")
    if m["n_tris"] * 3 != ni:
        print(f"[warn] mesh[{idx}] n_tris={m['n_tris']} but len(indices)={ni}")
    if nv == 0:
        raise ValueError(f"mesh[{idx}] has 0 vertices")
    # Index bounds — this is the #1 cause of "nothing renders"
    bad = [i for i in m["indices"] if i < 0 or i >= nv]
    if bad:
        raise ValueError(
            f"mesh[{idx}] has {len(bad)} out-of-range indices "
            f"(min={min(bad)}, max={max(bad)}, n_verts={nv})"
        )


meshes_data = load_meshes(JSON_PATH)
for i, m in enumerate(meshes_data):
    validate_mesh(m, i)

print(f"Loaded {len(meshes_data)} meshes from {JSON_PATH}")
total_tris = sum(m["n_tris"] for m in meshes_data)
total_verts = sum(m["n_verts"] for m in meshes_data)
print(f"  total verts: {total_verts}   total tris: {total_tris}")


# ------------------------------------------------------------
# Ursina app
# ------------------------------------------------------------
app = Ursina()
window.color = BACKGROUND
window.title = "Blender Triangle Map — Ursina Renderer"
window.borderless = False
window.fullscreen = False
window.size = (1280, 720)

pivot = Entity()


# ------------------------------------------------------------
# Build one Entity per mesh
# ------------------------------------------------------------
for i, mesh_data in enumerate(meshes_data):
    verts = mesh_data["vertices"]
    tris = mesh_data["indices"]

    u_mesh = Mesh(
        vertices=[Vec3(v[0], v[1], v[2]) for v in verts],
        triangles=tris,
        mode="triangle",
        static=True,
    )

    if FLAT_COLOR:
        col = color.light_gray
    else:
        hue = (i * 47) % 360          # spread colors so parts are distinguishable
        col = color.hsv(hue, 0.35, 0.95)

    Entity(
        parent=pivot,
        model=u_mesh,
        color=col,
        wireframe=WIREFRAME,
        double_sided=not WIREFRAME,
    )

    # Print per-mesh bounds so bad geometry is obvious
    xs = [v[0] for v in verts]
    ys = [v[1] for v in verts]
    zs = [v[2] for v in verts]
    print(
        f"  mesh[{i}] {mesh_data['source']:>10}: "
        f"x[{min(xs):+.3f},{max(xs):+.3f}] "
        f"y[{min(ys):+.3f},{max(ys):+.3f}] "
        f"z[{min(zs):+.3f},{max(zs):+.3f}]"
    )


# ------------------------------------------------------------
# Camera fit to overall bounds
# ------------------------------------------------------------
all_verts = [v for m in meshes_data for v in m["vertices"]]
if all_verts:
    xs = [v[0] for v in all_verts]
    ys = [v[1] for v in all_verts]
    zs = [v[2] for v in all_verts]
    center = Vec3(
        (min(xs) + max(xs)) / 2,
        (min(ys) + max(ys)) / 2,
        (min(zs) + max(zs)) / 2,
    )
    size = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))

    # Shift the whole model to be centered at origin, then place camera
    pivot.position = -center
    camera.position = Vec3(0, 0, -size * 2.2)
    camera.look_at(Vec3(0, 0, 0))

    print(f"Model size: {size:.3f}   center: {center}")
else:
    print("[warn] no vertices at all — nothing to render")


# ------------------------------------------------------------
# Update / input
# ------------------------------------------------------------
_initial_rot = Vec3(0, 0, 0)


def update():
    if AUTO_ROTATE:
        pivot.rotation_y += ROTATE_SPEED * time.dt
    if held_keys["left mouse"]:
        pivot.rotation_y += mouse.velocity[0] * 200
        pivot.rotation_x -= mouse.velocity[1] * 200


def input(key):
    if key == "escape":
        application.quit()
    elif key == "r":
        pivot.rotation = _initial_rot
        camera.position = Vec3(0, 0, -size * 2.2)
        camera.look_at(Vec3(0, 0, 0))


app.run()