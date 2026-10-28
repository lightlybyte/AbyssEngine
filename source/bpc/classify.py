# classify.py
"""Classify a mesh (or its components) as a primitive.

Strategy:
  1. Fit OBB to whole mesh.
  2. If degenerate (flat sheet) -> "plane".
  3. If fill >= 0.80 and not too eccentric -> "box".
  4. If fill >= 0.70 and eccentric -> "cylinder".
  5. If fill >= 0.45 and isotropic -> "sphere".
  6. If fill >= 0.35 -> "box_nc" (non-convex box; still one primitive).
  7. Otherwise -> split into components, classify each recursively.
"""
import numpy as np
from fit import (
    fit_obb, fit_sphere, fit_cylinder_in_obb_frame, fill_ratio, MIN_EXTENT,
)
from components import split_connected


# Thresholds
BOX_FILL   = 0.80
CYL_FILL   = 0.70
SPH_FILL   = 0.45
SPLIT_FILL = 0.35     # below this, split into components

# Aspect thresholds on half_extents sorted desc
SLAB_RATIO = 0.25     # smallest/largest below this -> slab (not box)


def _aspect(he):
    if he[0] <= 0:
        return 0.0, 0.0
    return float(he[2] / he[0]), float(he[1] / he[0])


def _serialize_params(obb, fill, n_verts, n_tris, extra=None):
    p = {
        "center": obb["center"],
        "axes": obb["axes"],
        "half_extents": obb["half_extents"],
        "fill_ratio": fill,
        "n_verts": int(n_verts),
        "n_tris": int(n_tris),
    }
    if extra:
        p.update(extra)
    return p


def _kind_from_obb(obb, fill):
    """Return (kind, extra_params_dict) given fitted OBB + fill."""
    he = obb["half_extents"]
    sr, mr = _aspect(he)

    if obb["is_degenerate"]:
        return "plane", {}

    if fill >= BOX_FILL:
        if sr < SLAB_RATIO:
            return "slab", {}
        return "box", {}

    if fill >= CYL_FILL and sr < 0.5:
        return "cylinder", {}

    if fill >= SPH_FILL and mr >= 0.75:
        return "sphere", {}

    if fill >= SPLIT_FILL:
        return "box_nc", {}   # non-convex box

    return "complex", {}


def classify_single(positions, indices, normals=None):
    """Classify one connected mesh. Returns (kind, params)."""
    if len(positions) < 4:
        return "degenerate", {"n_verts": int(len(positions))}

    obb = fit_obb(positions)
    fill = fill_ratio(positions, indices, obb)
    kind, _ = _kind_from_obb(obb, fill)

    if kind == "plane":
        extra = {"normal": obb["axes"][:, 2]}
        return kind, _serialize_params(obb, fill, len(positions), len(indices) // 3, extra)

    if kind == "cylinder":
        cyl = fit_cylinder_in_obb_frame(positions, obb)
        extra = {
            "cyl_center": cyl["center"],
            "cyl_radius": cyl["radius"],
            "cyl_height": cyl["height"],
            "cyl_axis": cyl["axis"],
        }
        return kind, _serialize_params(obb, fill, len(positions), len(indices) // 3, extra)

    if kind == "sphere":
        sph = fit_sphere(positions)
        extra = {"sphere_center": sph["center"], "sphere_radius": sph["radius"]}
        return kind, _serialize_params(obb, fill, len(positions), len(indices) // 3, extra)

    return kind, _serialize_params(obb, fill, len(positions), len(indices) // 3)


def classify(positions, indices, name="", normals=None, min_component_verts=8):
    """Return list of {primitive, params, component} dicts."""
    kind, params = classify_single(positions, indices)

    if kind != "complex":
        return [{"primitive": kind, "params": params, "component": 0}]

    # Genuinely decomposed -> split
    components = split_connected(positions, indices)
    parts = []
    for ci, (sub_pos, sub_idx) in enumerate(components):
        if len(sub_pos) < min_component_verts:
            continue
        k, p = classify_single(sub_pos, sub_idx)
        parts.append({"primitive": k, "params": p, "component": ci})

    if not parts:
        # Return the whole thing as a single complex blob
        parts = [{"primitive": "complex", "params": params, "component": 0}]
    return parts