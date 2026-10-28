import sys
import os
import json
import numpy as np

from glb_reader import read_glb
from scene_walk import walk_scene
from extract import extract_positions

FLOAT_DECIMALS = 6


def _round(arr):
    if arr is None:
        return None
    return np.round(arr, FLOAT_DECIMALS).tolist()


def _dedup(pos, uv=None, nrm=None):
    pos_r = np.round(pos, FLOAT_DECIMALS)
    if uv is not None:
        combined = np.hstack([pos_r, np.round(uv, FLOAT_DECIMALS)])
    else:
        combined = pos_r

    uniq_combined, inverse = np.unique(combined, axis=0, return_inverse=True)

    uniq_pos = uniq_combined[:, 0:3]
    uniq_uv = uniq_combined[:, 3:5] if uv is not None else None

    uniq_nrm = None
    if nrm is not None:
        n_uniq = len(uniq_combined)
        acc = np.zeros((n_uniq, 3), dtype=np.float64)
        counts = np.zeros(n_uniq, dtype=np.int64)
        np.add.at(acc, inverse, nrm)
        np.add.at(counts, inverse, 1)
        acc /= np.maximum(counts[:, None], 1)
        norms = np.linalg.norm(acc, axis=1, keepdims=True)
        norms[norms == 0] = 1
        uniq_nrm = acc / norms

    return uniq_pos, uniq_uv, uniq_nrm, inverse


def build_triangle_map(glb_path):
    gltf, bin_blob = read_glb(glb_path)
    materials = gltf.get("materials", [])
    meshes_out = []

    for name, mesh_idx, world_M in walk_scene(gltf):
        for prim in extract_positions(gltf, bin_blob, mesh_idx):
            prim_idx, positions, indices, normals, uvs, material_idx = prim

            ones = np.ones((len(positions), 1))
            pos_w = (np.hstack([positions, ones]) @ world_M.T)[:, :3]

            nrm_w = None
            if normals is not None:
                nrm_w = normals @ world_M[:3, :3].T
                n = np.linalg.norm(nrm_w, axis=1, keepdims=True)
                nrm_w = nrm_w / np.maximum(n, 1e-12)

            uniq_pos, uniq_uv, uniq_nrm, inverse = _dedup(pos_w, uvs, nrm_w)
            new_indices = inverse[indices].astype(np.uint32)

            mat_name = "none"
            if material_idx is not None and 0 <= material_idx < len(materials):
                mat_name = materials[material_idx].get("name", f"mat_{material_idx}")

            meshes_out.append({
                "source": name,
                "mesh_index": int(mesh_idx),
                "primitive_index": int(prim_idx),
                "material_index": int(material_idx) if material_idx is not None else -1,
                "material_name": mat_name,
                "world_matrix": _round(world_M),
                "n_verts": int(len(uniq_pos)),
                "n_tris": int(len(new_indices) // 3),
                "vertices": _round(uniq_pos),
                "uvs": _round(uniq_uv),
                "normals": _round(uniq_nrm),
                "indices": new_indices.tolist(),
            })

    return meshes_out


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python tri_map.py <file.glb> [out.json]")
        sys.exit(1)
    src = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else "tri_map.json"

    data = build_triangle_map(src)
    with open(out, "w", encoding="utf-8") as f:
        json.dump(data, f, separators=(",", ":"))

    print(f"wrote {out}: {len(data)} meshes, "
          f"{sum(m['n_verts'] for m in data)} verts, "
          f"{sum(m['n_tris'] for m in data)} tris, "
          f"{os.path.getsize(out)/1024/1024:.2f} MB")
    for i, m in enumerate(data):
        print(f"  [{i}] {m['source']:>10}  "
              f"mat={m['material_index']} ({m['material_name']})  "
              f"v={m['n_verts']} t={m['n_tris']}")