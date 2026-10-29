"""
trimap.py

Extract every triangle from a .glb into a compact tri-map.

Output per mesh:
  - source         : object name
  - mesh_index     : glTF mesh index
  - primitive_index: glTF primitive index within the mesh
  - material_index : glTF material slot (-1 if none)
  - material_name  : glTF material name
  - world_matrix   : 4x4 world transform applied to vertices
  - n_verts        : number of unique vertices
  - n_tris         : number of triangles
  - index_encoding : "delta" (JSON only)
  - vertices       : [[x, y, z], ...]
  - uvs            : [[u, v], ...] or null
  - normals        : [[nx, ny, nz], ...] or null
  - indices        : delta-encoded flat list (JSON), absolute flat list (binary)

Formats:
  trimap.json  — compact JSON (4-decimal rounding, delta-encoded indices)
  trimap.tmb   — binary (float32 vertices + uint32 indices), 2.7x smaller

Usage:
    python trimap.py <file.glb> [out.json] [--binary]
    python trimap.py example\\test.glb --binary
"""

import os
import sys
import json
import struct
import numpy as np

from glb_reader import read_glb
from scene_walk import walk_scene
from extract import extract_positions


FLOAT_DECIMALS = 4


# ------------------------------------------------------------
# Utilities
# ------------------------------------------------------------
def _round(arr):
    if arr is None:
        return None
    return np.round(arr, FLOAT_DECIMALS).tolist()


def _dedup(pos, uv=None, nrm=None):
    """Dedup by (position, uv). Returns uniq_pos, uniq_uv, uniq_nrm, inverse."""
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


def _delta_encode(indices_flat):
    """Flat [a,b,c,...] -> flat [a,b-a,c-a,...] (int64)."""
    arr = np.asarray(indices_flat, dtype=np.int64).reshape(-1, 3)
    delta = arr.copy()
    delta[:, 1] -= arr[:, 0]
    delta[:, 2] -= arr[:, 0]
    return delta.ravel().tolist()


def _delta_decode(indices_flat):
    """Inverse of _delta_encode. Returns flat numpy int64 array."""
    arr = np.asarray(indices_flat, dtype=np.int64).reshape(-1, 3).copy()
    arr[:, 1] += arr[:, 0]
    arr[:, 2] += arr[:, 0]
    return arr.ravel()


# ------------------------------------------------------------
# Build
# ------------------------------------------------------------
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
            new_indices = inverse[indices].astype(np.int64)

            mat_name = "none"
            if material_idx is not None and 0 <= material_idx < len(materials):
                mat_name = materials[material_idx].get(
                    "name", f"mat_{material_idx}"
                )

            meshes_out.append({
                "source": name,
                "mesh_index": int(mesh_idx),
                "primitive_index": int(prim_idx),
                "material_index": int(material_idx)
                    if material_idx is not None else -1,
                "material_name": mat_name,
                "world_matrix": _round(world_M),
                "n_verts": int(len(uniq_pos)),
                "n_tris": int(len(new_indices) // 3),
                "index_encoding": "delta",
                "vertices": _round(uniq_pos),
                "uvs": _round(uniq_uv),
                "normals": _round(uniq_nrm),
                "indices": _delta_encode(new_indices),
            })

    return meshes_out


# ------------------------------------------------------------
# Writers
# ------------------------------------------------------------
def write_json(meshes_out, path):
    try:
        import orjson
        with open(path, "wb") as f:
            f.write(orjson.dumps(meshes_out))
    except ImportError:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(meshes_out, f, separators=(",", ":"))
    print(f"wrote {path}: {os.path.getsize(path)/1024:.1f} KB")


def write_binary(meshes_out, path):
    header = []
    vertex_blob = bytearray()
    index_blob = bytearray()
    v_off = 0
    i_off = 0

    for m in meshes_out:
        h = {k: v for k, v in m.items() if k not in ("vertices", "indices")}
        h.pop("index_encoding", None)
        h["vertex_offset"] = v_off
        h["index_offset"] = i_off

        idx_list = m["indices"]
        if m.get("index_encoding") == "delta":
            idx_list = _delta_decode(idx_list)

        verts = np.asarray(m["vertices"], dtype=np.float32)
        idxs  = np.asarray(idx_list, dtype=np.uint32)

        vb = verts.tobytes()
        ib = idxs.tobytes()
        vertex_blob.extend(vb)
        index_blob.extend(ib)
        v_off += len(vb)
        i_off += len(ib)

        header.append(h)

    header_json = json.dumps(header, separators=(",", ":")).encode("utf-8")

    with open(path, "wb") as f:
        f.write(b"TMB\x01")
        f.write(struct.pack("<I", len(header_json)))
        f.write(header_json)
        f.write(bytes(vertex_blob))
        f.write(bytes(index_blob))

    total = 4 + 4 + len(header_json) + len(vertex_blob) + len(index_blob)
    print(f"wrote {path}: {total/1024:.1f} KB "
          f"(header {len(header_json)/1024:.1f} KB, "
          f"verts {len(vertex_blob)/1024:.1f} KB, "
          f"idx {len(index_blob)/1024:.1f} KB)")


# ------------------------------------------------------------
# Entry point
# ------------------------------------------------------------
def main(argv):
    if len(argv) < 2:
        print("Usage: python trimap.py <file.glb> [out.json] [--binary]")
        sys.exit(1)

    src = argv[1]
    binary_flag = "--binary" in argv

    # Output name: use explicit non-flag arg, else default
    out = None
    for a in argv[2:]:
        if not a.startswith("--"):
            out = a
            break
    if out is None:
        out = "trimap.json"

    print(f"Reading {src} ...")
    data = build_triangle_map(src)

    total_verts = sum(m["n_verts"] for m in data)
    total_tris = sum(m["n_tris"] for m in data)
    print(f"Extracted {len(data)} meshes, "
          f"{total_verts} verts, {total_tris} tris")

    for i, m in enumerate(data):
        print(f"  [{i}] {m['source']:>10}  "
              f"mat={m['material_index']} ({m['material_name']})  "
              f"v={m['n_verts']} t={m['n_tris']}")

    write_json(data, out)

    if binary_flag:
        bin_path = os.path.splitext(out)[0] + ".tmb"
        write_binary(data, bin_path)


if __name__ == "__main__":
    main(sys.argv)