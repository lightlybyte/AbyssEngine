"""
classify.py

Classify each mesh in a tri_map as a primitive candidate (box / slab /
plate / cylinder / sphere / complex) using OBB fitting and fill ratio.

Usage:
    python classify.py                    # reads tri_map.tmb / .json next to script
    python classify.py path/to/tri_map    # explicit path

Pure diagnostic — prints a report. The actual primitive-fitting pipeline
(per-component splitting, per-primitive triangle assignment) is a separate
later stage.
"""

import os
import sys
import json
import numpy as np

from tri_map_loader import load_tri_map


# ------------------------------------------------------------
# Fitting helpers
# ------------------------------------------------------------
MIN_EXTENT = 1e-5


def fit_obb(positions):
    """PCA-based oriented bounding box. Returns center, axes, half_extents."""
    center = positions.mean(axis=0)
    centered = positions - center
    cov = np.cov(centered.T)
    eigvals, eigvecs = np.linalg.eigh(cov)
    order = np.argsort(eigvals)[::-1]
    axes = eigvecs[:, order]
    if np.linalg.det(axes) < 0:
        axes[:, 2] *= -1

    projected = centered @ axes
    pmin = projected.min(axis=0)
    pmax = projected.max(axis=0)
    mid = (pmin + pmax) * 0.5
    half = (pmax - pmin) * 0.5
    center = center + axes @ mid

    order2 = np.argsort(half)[::-1]
    axes = axes[:, order2]
    half = half[order2]
    if np.linalg.det(axes) < 0:
        axes[:, 2] *= -1

    is_degenerate = bool(half[2] < MIN_EXTENT)
    half = np.maximum(half, MIN_EXTENT)

    return {
        "center": center,
        "axes": axes,
        "half_extents": half,
        "is_degenerate": is_degenerate,
    }


def fill_ratio(positions, indices, obb):
    """Convex hull volume / OBB volume, clamped to [0, 5]."""
    if obb["is_degenerate"]:
        return 0.0
    obb_vol = float(np.prod(obb["half_extents"] * 2.0))
    if obb_vol <= 0:
        return 0.0
    try:
        from scipy.spatial import ConvexHull
        hull_vol = ConvexHull(positions).volume
    except Exception:
        idx = indices.reshape(-1, 3)
        v0 = positions[idx[:, 0]]
        v1 = positions[idx[:, 1]]
        v2 = positions[idx[:, 2]]
        hull_vol = abs(float(np.einsum("ij,ij->i", v0, np.cross(v1, v2)).sum()) / 6.0)
    return float(min(hull_vol / obb_vol, 5.0))


def fit_sphere(positions):
    A = np.hstack([2.0 * positions, np.ones((len(positions), 1))])
    b = (positions ** 2).sum(axis=1)
    sol, *_ = np.linalg.lstsq(A, b, rcond=None)
    center = sol[:3]
    r = np.sqrt(max(sol[3] + (center ** 2).sum(), 0.0))
    return {"center": center, "radius": float(r)}


def fit_cylinder(positions, obb):
    """Fit cylinder with the OBB's longest axis (axis 0) as axis."""
    axes = obb["axes"]
    local = positions @ axes
    along = local[:, 0]
    perp = local[:, 1:]

    lo, hi = along.min(), along.max()
    height = hi - lo
    center_along = (hi + lo) * 0.5

    x, y = perp[:, 0], perp[:, 1]
    A = np.column_stack([2 * x, 2 * y, np.ones(len(x))])
    b = x * x + y * y
    try:
        sol, *_ = np.linalg.lstsq(A, b, rcond=None)
        cx, cy = sol[0], sol[1]
        r = np.sqrt(max(sol[2] + cx * cx + cy * cy, 0.0))
    except np.linalg.LinAlgError:
        cx, cy = x.mean(), y.mean()
        r = float(np.sqrt(((perp - [cx, cy]) ** 2).sum(axis=1)).mean())

    center_local = np.array([center_along, cx, cy])
    center_world = obb["center"] + axes @ center_local

    return {
        "center": center_world,
        "radius": float(r),
        "height": float(height),
        "axis": axes[:, 0].copy(),
    }


# ------------------------------------------------------------
# Classification
# ------------------------------------------------------------
BOX_FILL  = 0.80
CYL_FILL  = 0.70
SPH_FILL  = 0.45
SLAB_RATIO = 0.25


def _aspect(he):
    if he[0] <= 0:
        return 0.0, 0.0
    return float(he[2] / he[0]), float(he[1] / he[0])


def classify_mesh(positions, indices):
    """Return (kind, params_dict)."""
    if len(positions) < 4:
        return "degenerate", {"n_verts": int(len(positions))}

    obb = fit_obb(positions)
    fill = fill_ratio(positions, indices, obb)
    he = obb["half_extents"]
    sr, mr = _aspect(he)

    common = {
        "center": obb["center"],
        "axes": obb["axes"],
        "half_extents": he,
        "fill_ratio": fill,
        "n_verts": int(len(positions)),
        "n_tris": int(len(indices) // 3),
    }

    if obb["is_degenerate"]:
        return "plane", {**common, "normal": obb["axes"][:, 2]}

    if fill >= BOX_FILL:
        kind = "plate" if sr < 0.05 else ("slab" if sr < SLAB_RATIO else "box")
        return kind, common

    if fill >= CYL_FILL and sr < 0.5:
        cyl = fit_cylinder(positions, obb)
        return "cylinder", {
            **common,
            "cyl_center": cyl["center"],
            "cyl_radius": cyl["radius"],
            "cyl_height": cyl["height"],
            "cyl_axis": cyl["axis"],
        }

    if fill >= SPH_FILL and mr >= 0.75:
        sph = fit_sphere(positions)
        return "sphere", {
            **common,
            "sphere_center": sph["center"],
            "sphere_radius": sph["radius"],
        }

    return "complex", common


# ------------------------------------------------------------
# Report
# ------------------------------------------------------------
def _fmt(v, decimals=3):
    if isinstance(v, np.ndarray):
        return np.array2string(v, precision=decimals, suppress_small=True)
    if isinstance(v, (list, tuple)):
        return "[" + ", ".join(f"{x:.{decimals}f}" for x in v) + "]"
    if isinstance(v, float):
        return f"{v:.{decimals}f}"
    return str(v)


def report(meshes_data, label=""):
    print(f"\n{'='*70}")
    print(f"Classification report  {label}")
    print(f"{'='*70}")

    counts = {}
    for i, m in enumerate(meshes_data):
        verts = np.asarray(m["vertices"], dtype=np.float64)
        idx = np.asarray(m["indices"], dtype=np.int64)

        kind, params = classify_mesh(verts, idx)
        counts[kind] = counts.get(kind, 0) + 1

        print(f"\n[{i}] {m.get('source','?')}  "
              f"(mat {m.get('material_index')}, "
              f"{len(verts)} v, {len(idx)//3} t)")
        print(f"    -> {kind}")
        print(f"    center:       {_fmt(params.get('center'))}")
        print(f"    half_extents: {_fmt(params.get('half_extents'))}")
        if "fill_ratio" in params:
            print(f"    fill_ratio:   {params['fill_ratio']:.3f}")
        if "cyl_radius" in params:
            print(f"    cyl_radius:   {params['cyl_radius']:.4f}")
            print(f"    cyl_height:   {params['cyl_height']:.4f}")
        if "sphere_radius" in params:
            print(f"    sph_radius:   {params['sphere_radius']:.4f}")

    print(f"\n{'='*70}")
    print("Totals:")
    for k, v in sorted(counts.items(), key=lambda x: -x[1]):
        print(f"  {k:>10}: {v}")
    print(f"{'='*70}\n")


# ------------------------------------------------------------
# Entry point
# ------------------------------------------------------------
if __name__ == "__main__":
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

    if len(sys.argv) >= 2:
        path = sys.argv[1]
    else:
        # Prefer binary if present, else JSON
        tmb = os.path.join(SCRIPT_DIR, "tri_map.tmb")
        jsn = os.path.join(SCRIPT_DIR, "tri_map.json")
        path = tmb if os.path.exists(tmb) else jsn

    if not os.path.exists(path):
        print(f"File not found: {path}")
        sys.exit(1)

    meshes = load_tri_map(path)
    print(f"Loaded {len(meshes)} meshes from {path}")
    report(meshes, label=os.path.basename(path))