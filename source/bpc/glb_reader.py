import json
import struct
import numpy as np

GLB_MAGIC = 0x46546C67
CHUNK_JSON = 0x4E4F534A
CHUNK_BIN  = 0x004E4942

_COMPONENT_DTYPE = {
    5120: np.int8,   5121: np.uint8,
    5122: np.int16,  5123: np.uint16,
    5125: np.uint32, 5126: np.float32,
}
_TYPE_COUNT = {
    "SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4,
    "MAT2": 4, "MAT3": 9, "MAT4": 16,
}


def read_glb(path):
    with open(path, "rb") as f:
        magic, version, length = struct.unpack("<III", f.read(12))
        if magic != GLB_MAGIC:
            raise ValueError(f"Not a .glb file (magic={magic:#x})")
        if version != 2:
            raise ValueError(f"Unsupported glTF version {version}")
        gltf, bin_blob = None, b""
        while f.tell() < length:
            clen, ctype = struct.unpack("<II", f.read(8))
            data = f.read(clen)
            if ctype == CHUNK_JSON:
                gltf = json.loads(data.decode("utf-8"))
            elif ctype == CHUNK_BIN:
                bin_blob = data
        if gltf is None:
            raise ValueError("No JSON chunk found")
        return gltf, bin_blob


def read_accessor(gltf, bin_blob, accessor_idx):
    acc = gltf["accessors"][accessor_idx]
    n_comp = _TYPE_COUNT[acc["type"]]
    dtype = np.dtype(_COMPONENT_DTYPE[acc["componentType"]])
    count = acc["count"]

    if "bufferView" not in acc:
        return np.zeros((count, n_comp), dtype=dtype)

    bv = gltf["bufferViews"][acc["bufferView"]]
    start = bv.get("byteOffset", 0) + acc.get("byteOffset", 0)
    stride = bv.get("byteStride") or (n_comp * dtype.itemsize)

    if stride == n_comp * dtype.itemsize:
        flat = np.frombuffer(bin_blob, dtype=dtype, count=count * n_comp, offset=start)
        return flat.reshape(count, n_comp).copy()

    out = np.empty((count, n_comp), dtype=dtype)
    for i in range(count):
        off = start + i * stride
        out[i] = np.frombuffer(bin_blob, dtype=dtype, count=n_comp, offset=off)
    return out