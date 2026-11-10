# main.py
import sys
import json
import numpy as np

from glb_reader import read_glb
from scene_walk import walk_scene
from extract import extract_positions
from classify import classify_mesh


def _serialize(v):
    if isinstance(v, np.ndarray):
        return v.tolist()
    if isinstance(v, (np.floating, np.integer)):
        return v.item()
    if isinstance(v, dict):
        return {k: _serialize(x) for k, x in v.items()}
    if isinstance(v, (list, tuple)):
        return [_serialize(x) for x in v]
    return v


def build_instructions(glb_path):
    gltf, bin_blob = read_glb(glb_path)
    instructions = []

    for name, mesh_idx, world_M in walk_scene(gltf):
        results = extract_positions(gltf, bin_blob, mesh_idx)
        for result in results:
            prim_idx, positions, indices, normals, uvs, material_idx = result
            ones = np.ones((len(positions), 1))
            pos_w = (np.hstack([positions, ones]) @ world_M.T)[:, :3]

            nrm_w = None
            if normals is not None:
                nrm_w = normals @ world_M[:3, :3].T
                n = np.linalg.norm(nrm_w, axis=1, keepdims=True)
                nrm_w = nrm_w / np.maximum(n, 1e-12)

            kind, params = classify_mesh(pos_w, indices)
            params = dict(params)  # make a mutable copy
            params["name"] = name
            if nrm_w is not None:
                params["normals"] = nrm_w

            instructions.append({
                "source": name,
                "primitive_index": prim_idx,
                "component": 0,
                "primitive": kind,
                "params": _serialize(params),
                "material_index": material_idx,
            })

    return instructions


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python main.py <file.glb>")
        sys.exit(1)

    result = build_instructions(sys.argv[1])
    print(json.dumps(result, indent=2))