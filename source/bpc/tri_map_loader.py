# tri_map_loader.py
"""Load tri_map in either JSON or .tmb binary form."""
import json
import os
import struct
import numpy as np

MAGIC = b"TMB\x01"


def load_tri_map(path):
    """Load a tri_map file. Auto-detects .tmb vs .json by extension.
    Returns a list of mesh dicts with 'vertices' and 'indices' as Python lists.
    """
    if path.endswith(".tmb"):
        return _load_binary(path)
    return _load_json(path)


def _load_json(path):
    try:
        import orjson
        with open(path, "rb") as f:
            data = orjson.loads(f.read())
    except ImportError:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)

    # Decode delta indices if present
    for m in data:
        if m.get("index_encoding") == "delta":
            flat = np.asarray(m["indices"], dtype=np.int64).reshape(-1, 3).copy()
            flat[:, 1] += flat[:, 0]
            flat[:, 2] += flat[:, 0]
            m["indices"] = flat.ravel().tolist()
            del m["index_encoding"]
    return data


def _load_binary(path):
    if not os.path.exists(path):
        raise FileNotFoundError(f"not found: {path}")

    with open(path, "rb") as f:
        magic = f.read(4)
        if magic != MAGIC:
            raise ValueError(f"Bad magic {magic!r} (expected {MAGIC!r})")

        header_len = struct.unpack("<I", f.read(4))[0]
        header = json.loads(f.read(header_len).decode("utf-8"))

        data_start = 4 + 4 + header_len

        # Total vertex bytes = sum of each mesh's (n_verts*3*4)
        total_vbytes = sum(h["n_verts"] * 3 * 4 for h in header)

        meshes = []
        for h in header:
            f.seek(data_start + h["vertex_offset"])
            vbytes = f.read(h["n_verts"] * 3 * 4)
            verts = np.frombuffer(vbytes, dtype=np.float32).reshape(-1, 3)

            f.seek(data_start + total_vbytes + h["index_offset"])
            ibytes = f.read(h["n_tris"] * 3 * 4)
            idxs = np.frombuffer(ibytes, dtype=np.uint32).reshape(-1)

            # Remove offset keys; present the same schema as the JSON loader
            h.pop("vertex_offset", None)
            h.pop("index_offset", None)

            h["vertices"] = verts.tolist()
            h["indices"] = idxs.tolist()
            meshes.append(h)

    return meshes