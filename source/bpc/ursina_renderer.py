import json
import os
import sys
import numpy as np
from ursina import *


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JSON_PATH = os.path.join(SCRIPT_DIR, "tri_map.json")

AUTO_ROTATE = "--no-rotate" not in sys.argv
WIREFRAME = "--wire" in sys.argv
ROTATE_SPEED = 20.0

MATERIAL_COLORS = {
    0: color.rgb(55,  55,  62),
    1: color.rgb(42,  42,  42),
    2: color.rgb(95,  95, 100),
    3: color.rgb(78,  78,  78),
}
DEFAULT_COLOR = color.rgb(80, 80, 80)


def load_meshes(path):
    """Load tri_map.json; decode delta indices if present."""
    if not os.path.exists(path):
        raise FileNotFoundError(f"not found: {path}")

    try:
        import orjson
        with open(path, "rb") as f:
            data = orjson.loads(f.read())
    except ImportError:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)

    # Decode delta-encoded indices
    for m in data:
        if m.get("index_encoding") == "delta":
            flat = np.asarray(m["indices"], dtype=np.int64)
            tri = flat.reshape(-1, 3).copy()
            tri[:, 1] += tri[:, 0]
            tri[:, 2] += tri[:, 0]
            m["indices"] = tri.ravel().tolist()
            del m["index_encoding"]
        # Else: already absolute, leave as-is.

    return data


meshes_data = load_meshes(JSON_PATH)
print(f"Loaded {len(meshes_data)} meshes")
for i, m in enumerate(meshes_data):
    print(f"  [{i}] {m.get('source','?'):>10}  "
          f"mat={m.get('material_index')} ({m.get('material_name')})  "
          f"v={m['n_verts']} t={m['n_tris']}")


app = Ursina()
window.title = "Gun Triangle Map"
window.size = (1280, 720)

DirectionalLight(y=3, z=-4, x=-3, rotation=(50, -30, 0), color=color.white)
AmbientLight(color=color.rgba(60, 60, 70, 255))


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


camera.position = Vec3(0, 0, -size * 2.2)
camera.look_at(Vec3(0, 0, 0))


_initial_rot = Vec3(0, 90, 0)


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