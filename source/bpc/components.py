# components.py
"""Split a mesh into connected components by shared vertex indices."""
import numpy as np
from collections import defaultdict


def split_connected(positions, indices):
    """Return list of (sub_positions, sub_indices) for each connected component.

    sub_indices are remapped into 0..len(sub_positions)-1.
    """
    tris = indices.reshape(-1, 3)

    # Union-find over vertex ids
    parent = list(range(len(positions)))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb

    for a, b, c in tris:
        union(int(a), int(b))
        union(int(b), int(c))

    # Group triangles by root vertex
    groups = defaultdict(list)
    for tri in tris:
        groups[find(int(tri[0]))].append(tri)

    components = []
    for _, tri_list in groups.items():
        tri_arr = np.array(tri_list, dtype=np.uint32)
        vids = np.unique(tri_arr)
        remap = -np.ones(len(positions), dtype=np.int64)
        remap[vids] = np.arange(len(vids))
        sub_pos = positions[vids]
        sub_idx = remap[tri_arr].ravel().astype(np.uint32)
        components.append((sub_pos, sub_idx))
    return components