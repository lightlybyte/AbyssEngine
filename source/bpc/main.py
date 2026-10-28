# main.py
import sys
import json
import numpy as np

from glb_reader import read_glb
from scene_walk import walk_scene
from extract import extract_positions
from classify import classify


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
        for prim_idx, positions, indices, normals in extract_positions(
            gltf, bin_blob, mesh_idx
        ):
            ones = np.ones((len(positions), 1))
            pos_w = (np.hstack([positions, ones]) @ world_M.T)[:, :3]

            nrm_w = None
            if normals is not None:
                nrm_w = normals @ world_M[:3, :3].T
                n = np.linalg.norm(nrm_w, axis=1, keepdims=True)
                nrm_w = nrm_w / np.maximum(n, 1e-12)

            parts = classify(pos_w, indices, name=name, normals=nrm_w)

            for part in parts:
                instructions.append({
                    "source": name,
                    "primitive_index": prim_idx,
                    "part_index": part["component"],
                    "primitive": part["primitive"],
                    "params": _serialize(part["params"]),
                })

    return instructions


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python main.py <file.glb>")
        sys.exit(1)

    result = build_instructions(sys.argv[1])
    print(json.dumps(result, indent=2))