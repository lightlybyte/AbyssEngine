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
    """Load a GLB and emit classification instructions."""
    try:
        gltf, bin_blob = read_glb(glb_path)
    except FileNotFoundError:
        raise ValueError(f"GLB not found: {glb_path}")
    except ValueError as e:
        raise ValueError(f"Invalid GLB format: {e}")

    instructions = []

    for name, mesh_idx, world_M in walk_scene(gltf):
        results = extract_positions(gltf, bin_blob, mesh_idx)
        for prim_idx, positions, indices, normals, uvs, material_idx in results:
            ones = np.ones((len(positions), 1))
            pos_w = (np.hstack([positions, ones]) @ world_M.T)[:, :3]

            nrm_w = None
            if normals is not None:
                # Inverse-transpose normal matrix for non-uniform scale correctness
                M = world_M[:3, :3]
                try:
                    inv_T = np.linalg.inv(M).T
                except np.linalg.LinAlgError:
                    inv_T = np.eye(3)
                nrm_w = normals @ inv_T
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

    try:
        result = build_instructions(sys.argv[1])
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)

    print(json.dumps(result, indent=2))