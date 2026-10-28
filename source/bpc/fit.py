# fit.py
"""Primitive fitting: OBB, sphere, cylinder, fill-ratio, degenerate detection."""
import numpy as np

# Below this size (meters) an extent is treated as zero-thickness.
MIN_EXTENT = 1e-5


def fit_obb(positions, clamp_extents=True):
    """PCA-based oriented bounding box.

    Returns dict with:
        center (3,), axes (3x3, columns = local X/Y/Z, sorted by extent desc),
        half_extents (3,), is_degenerate (bool: one extent < MIN_EXTENT).
    """
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

    # Sort by extent desc
    order2 = np.argsort(half)[::-1]
    axes = axes[:, order2]
    half = half[order2]
    if np.linalg.det(axes) < 0:
        axes[:, 2] *= -1

    is_degenerate = bool(half[2] < MIN_EXTENT)

    if clamp_extents:
        half = np.maximum(half, MIN_EXTENT)

    return {
        "center": center,
        "axes": axes,
        "half_extents": half,
        "is_degenerate": is_degenerate,
    }


def fit_sphere(positions):
    """Least-squares sphere fit via algebraic (linear) method."""
    A = np.hstack([2.0 * positions, np.ones((len(positions), 1))])
    b = (positions ** 2).sum(axis=1)
    sol, *_ = np.linalg.lstsq(A, b, rcond=None)
    center = sol[:3]
    r = np.sqrt(max(sol[3] + (center ** 2).sum(), 0.0))
    return {"center": center, "radius": float(r)}


def fit_cylinder_in_obb_frame(positions, obb):
    """Fit a cylinder with the OBB's longest axis (axis 0) as cylinder axis.

    Returns dict: center, radius, height, axis (3,).
    """
    axes = obb["axes"]
    local = positions @ axes          # transform into OBB frame
    along = local[:, 0]
    perp = local[:, 1:]

    lo, hi = along.min(), along.max()
    height = hi - lo
    center_along = (hi + lo) * 0.5

    # Circle fit in perpendicular plane
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


def fill_ratio(positions, indices, obb):
    """Convex hull volume / OBB volume.

    If OBB is degenerate (thin plate), returns 0.0 by convention.
    box ≈ 1.0, cylinder ≈ 0.78, sphere ≈ 0.52, complex < 0.5.
    """
    if obb.get("is_degenerate"):
        return 0.0
    he = obb["half_extents"]
    obb_vol = float(np.prod(he * 2.0))
    if obb_vol <= 0:
        return 0.0
    try:
        from scipy.spatial import ConvexHull
        hull_vol = ConvexHull(positions).volume
    except Exception:
        idx = indices.reshape(-1, 3)
        v0, v1, v2 = positions[idx[:, 0]], positions[idx[:, 1]], positions[idx[:, 2]]
        hull_vol = abs(float(np.einsum("ij,ij->i", v0, np.cross(v1, v2)).sum()) / 6.0)
    return float(min(hull_vol / obb_vol, 5.0))   # clamp to sanity range