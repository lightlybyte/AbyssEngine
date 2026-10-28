import numpy as np
from glb_reader import read_accessor


def extract_positions(gltf, bin_blob, mesh_index):
    """Yield (prim_idx, positions, indices, normals, uvs, material_idx)."""
    mesh = gltf["meshes"][mesh_index]
    out = []
    for pi, prim in enumerate(mesh["primitives"]):
        if prim.get("mode", 4) != 4:
            continue

        pos = read_accessor(gltf, bin_blob, prim["attributes"]["POSITION"])

        if "indices" in prim:
            idx = read_accessor(gltf, bin_blob, prim["indices"]).reshape(-1)
        else:
            idx = np.arange(len(pos), dtype=np.uint32)

        nrm = None
        if "NORMAL" in prim["attributes"]:
            nrm = read_accessor(gltf, bin_blob, prim["attributes"]["NORMAL"])

        uv = None
        if "TEXCOORD_0" in prim["attributes"]:
            uv = read_accessor(gltf, bin_blob, prim["attributes"]["TEXCOORD_0"])

        material_idx = prim.get("material", None)

        out.append((pi, pos, idx, nrm, uv, material_idx))
    return out