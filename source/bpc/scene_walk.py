import numpy as np


def _node_local_matrix(node):
    if "matrix" in node:
        return np.array(node["matrix"], dtype=np.float64).reshape(4, 4).T
    M = np.eye(4)
    if "translation" in node:
        M[:3, 3] = node["translation"]
    if "rotation" in node:
        x, y, z, w = node["rotation"]
        M[:3, :3] = np.array([
            [1-2*(y*y+z*z), 2*(x*y-z*w),   2*(x*z+y*w)],
            [2*(x*y+z*w),   1-2*(x*x+z*z), 2*(y*z-x*w)],
            [2*(x*z-y*w),   2*(y*z+x*w),   1-2*(x*x+y*y)],
        ])
    if "scale" in node:
        M[:3, :3] *= np.array(node["scale"])
    return M


def walk_scene(gltf):
    scene = gltf["scenes"][gltf.get("scene", 0)]
    nodes = gltf["nodes"]
    stack = [(i, np.eye(4)) for i in reversed(scene["nodes"])]
    while stack:
        idx, parent_M = stack.pop()
        node = nodes[idx]
        world_M = parent_M @ _node_local_matrix(node)
        if "mesh" in node:
            yield node.get("name", f"node{idx}"), node["mesh"], world_M
        for child in node.get("children", []):
            stack.append((child, world_M))