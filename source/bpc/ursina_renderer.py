"""
ursina_renderer.py

Render tri_map in Ursina. Loads .tmb (binary) if available, else .json.

Usage:
    python ursina_renderer.py              # auto-rotate + orbit on drag
    python ursina_renderer.py --no-rotate  # static, orbit on drag only
    python ursina_renderer.py --wire       # wireframe (debug)
"""

import os
import sys
from ursina import *

from tri_map_loader import load_tri_map


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JSON_PATH = os.path.join(SCRIPT_DIR, "tri_map.json")
BIN_PATH  = os.path.join(SCRIPT_DIR, "tri_map.tmb")

AUTO_ROTATE = "--no-rotate" not in sys.argv
WIREFRAME = "--wire" in sys.argv
ROTATE_SPEED = 20.0

MATERIAL_COLORS = {
    0: color.rgb(55,  55,  62),    # Black_Metal
    1: color.rgb(42,  42,  42),    # Black_Plastic
    2: color.rgb(95,  95, 100),    # Grey_Metal
    3: color.rgb(78,  78,  78),    # Grey_Plastic
}
DEFAULT_COLOR = color.rgb(80, 80, 80)


# ------------------------------------------------------------
# Load — binary preferred, JSON fallback
# ------------------------------------------------------------
if os.path.exists(BIN_PATH):
    meshes_data = load_tri_map(BIN_PATH)
    print(f"Loaded {len(meshes_data)} meshes from {BIN_PATH}")
elif os.path.exists(JSON_PATH):
    meshes_data = load_tri_map(JSON_PATH)
    print(f"Loaded {len(meshes_data)} meshes from {JSON_PATH}")
else:
    raise FileNotFoundError(
        f"Neither {BIN_PATH} nor {JSON_PATH} exists."
    )

for i, m in enumerate(meshes_data):
    print(f"  [{i}] {m.get('source','?'):>10}  "
          f"mat={m.get('material_index')} ({m.get('material_name')})  "
          f"v={m['n_verts']} t={m['n_tris']}")


# ------------------------------------------------------------
# App
# ------------------------------------------------------------
app = Ursina()
window.title = "Gun Triangle Map"
window.size = (1280, 720)

DirectionalLight(y=3, z=-4, x=-3, rotation=(50, -30, 0), color=color.white)
AmbientLight(color=color.rgba(60, 60, 70, 255))


# ------------------------------------------------------------
# Bounds
# ------------------------------------------------------------
all_verts = [v for m in meshes_data for v in m["vertices"]]
xs = [v[0] for v in all_verts]
ys = [v[1] for v in all_verts]
zs = [v[2] for v in all_verts]
center = Vec3(
    (min(xs) + max(xs)) / 2,
    (min(ys) + max(ys)) / 2,
    (min(zs) + max(zs)) / 2,
)
size = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
print(f"Model size: {size:.3f}  center: {center}")


# ------------------------------------------------------------
# Pivot at origin; vertices centered in local space
# ------------------------------------------------------------
pivot = Entity(position=(0, 0, 0))
pivot.rotation_y = 90


for i, mesh_data in enumerate(meshes_data):
    verts = mesh_data["vertices"]
    tris = mesh_data["indices"]
    mat_idx = mesh_data.get("material_index", -1)
    col = MATERIAL_COLORS.get(mat_idx, DEFAULT_COLOR)

    centered_verts = [
        Vec3(v[0] - center.x, v[1] - center.y, v[2] - center.z)
        for v in verts
    ]

    u_mesh = Mesh(
        vertices=centered_verts,
        triangles=tris,
        mode="triangle",
        static=True,
    )

    Entity(
        parent=pivot,
        model=u_mesh,
        color=col,
        wireframe=WIREFRAME,
        double_sided=True,
    )


# ------------------------------------------------------------
# Camera
# ------------------------------------------------------------
camera.position = Vec3(0, 0, -size * 2.2)
camera.look_at(Vec3(0, 0, 0))


_initial_rot = Vec3(0, 90, 0)


# ------------------------------------------------------------
# Update / input
# ------------------------------------------------------------
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