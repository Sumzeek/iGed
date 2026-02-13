from __future__ import annotations

import sys
import csv
import logging
import os
from dataclasses import dataclass
from typing import Optional, Tuple, List
from concurrent.futures import ProcessPoolExecutor, as_completed
import multiprocessing as mp

import numpy as np
import torch
import torch.nn as nn
import pymeshlab


@dataclass
class NTFConfig:
    """Configuration for NTF"""

    fflevels: int = 2
    hidden_dim: int = 32


class NTFMLP(nn.Module):
    """MLP: [positional encoding of quad + epsilon] -> 2 continuous rates (inner, outter).

    We regress 2 real-valued rates: inner_rate and outter_rate.
    The consumer can later round or clamp as needed.
    """

    def __init__(self, in_dim: int, hidden_dim: int = 64) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(in_dim, hidden_dim),
            nn.LeakyReLU(),
            nn.Linear(hidden_dim, 2),  # 2 rates: inner_rate, outter_rate
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


class PositionalEncoder(nn.Module):
    """1D positional encoding utility similar to the `positional_encoding` in `ngf.py`.

    Here we treat (quad geometry + epsilon) as the base vector and apply Fourier features.
    """

    def __init__(self, in_dim: int, levels: int) -> None:
        super().__init__()
        self.in_dim = in_dim
        self.levels = levels

    @property
    def out_dim(self) -> int:
        # base + 2 * levels * base
        return self.in_dim * (2 * self.levels)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """x: (..., D) -> (..., D * (2 * L))."""
        outs = []
        for i in range(self.levels):
            k = 2.0 ** i
            outs.append(torch.sin(k * x))
            outs.append(torch.cos(k * x))
        return torch.cat(outs, dim=-1)


class NTF(nn.Module):
    """Neural Tessellation Field for quad geometry + epsilon.

    Input: raw features [p0(3), p1(3), p2(3), p3(3), eps] (D_raw=13).
    We apply Fourier positional encoding, then an MLP that regresses 2
    continuous rates: inner_rate and outter_rate.
    """

    def __init__(self, in_dim_raw: int, config: Optional[NTFConfig] = None) -> None:
        super().__init__()
        self.config = config or NTFConfig()

        # Positional encoder works on the flattened raw feature vector
        self.encoder = PositionalEncoder(in_dim=in_dim_raw, levels=self.config.fflevels)
        self.mlp = NTFMLP(
            in_dim=self.encoder.out_dim,
            hidden_dim=self.config.hidden_dim,
        )

        logging.info("Instantiated NTF with:")
        logging.info(f"  in_dim_raw   = {in_dim_raw}")
        logging.info(f"  enc_out_dim  = {self.encoder.out_dim}")
        logging.info(f"  fflevels     = {self.config.fflevels}")
        logging.info(f"  hidden_dim   = {self.config.hidden_dim}")

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """Forward pass.

        Args:
            x: tensor of shape (B, D_raw), where each row is
               [p0(3), p1(3), p2(3), p3(3), eps].
        Returns:
            rates: tensor of shape (B, 2) with continuous rates
                    (inner_rate, outter_rate).
        """
        enc = self.encoder(x)
        rates = self.mlp(enc)
        return rates

    # Convenience wrapper for prediction, mirroring `Trainer.predict` logic.
    def predict_rate(self, x: torch.Tensor) -> torch.Tensor:
        """Return predicted 2 continuous rates from raw feature tensor x.

        Returns (inner_rate, outter_rate).
        The calling code can clamp/round these to discrete tessellation rates
        if desired.
        """
        self.eval()
        with torch.no_grad():
            rates = self.forward(x)
        return rates


# ---------- Training Data Generation ----------
from OptixBaker import import_optixbaker
from Preprocessor import parse_quad_mesh
from Tessellator import DisplacementSampler, QuadTessellator, QuadTessellatorFast, QuadTessParams, Vertex


# ========== 多进程共享数据结构 ==========
@dataclass
class QuadProcessingContext:
    """Context data shared across all worker processes."""
    quads_baked: list
    verts_baked: list
    norms_baked: list
    uvs_baked: list
    orig_vertices: np.ndarray
    orig_indices: np.ndarray
    sample_us: np.ndarray
    sample_vs: np.ndarray
    samples_per_dim: int
    disp_exr_path: str
    resolution: int


# 全局变量，用于在子进程中存储上下文
_worker_context: Optional[QuadProcessingContext] = None
_worker_disp_sampler: Optional[DisplacementSampler] = None
_worker_optix = None
_worker_orig_gas_cached: bool = False  # 标记原始模型 GAS 是否已缓存


def _worker_initializer(context: QuadProcessingContext) -> None:
    """Initialize worker process with shared context."""
    global _worker_context, _worker_disp_sampler, _worker_optix, _worker_orig_gas_cached
    _worker_context = context
    # 每个子进程独立加载 DisplacementSampler 和 optix
    _worker_disp_sampler = DisplacementSampler(context.disp_exr_path, context.resolution)
    _worker_optix = import_optixbaker()

    # 在 worker 初始化时缓存原始模型的 GAS（只执行一次）
    _worker_optix.build_cached_gas(context.orig_vertices, context.orig_indices)
    _worker_orig_gas_cached = True
    logging.info(
        f"[Worker] Cached original mesh GAS: {context.orig_vertices.shape[0]} vertices, {context.orig_indices.shape[0]} triangles")


def sample_grid_with_disk_jitter(
        samples_per_dim: int,
        delta_ratio: float = 0.45
) -> Tuple[np.ndarray, np.ndarray]:
    """Generate a 2D sample grid with disk-based jittering.
    
    This implements the same jittering strategy as used in NGF training:
    - Generate uniform grid points
    - Apply random disk-shaped jitter to interior points only
    - Boundary points (edges) remain fixed to ensure coverage
    
    Args:
        samples_per_dim: Number of samples per dimension (S)
        delta_ratio: Jitter radius as ratio of grid spacing (default 0.45)
    
    Returns:
        Tuple of (sample_us, sample_vs), each shape (S,)
        Note: These are 1D arrays; use meshgrid to get 2D grid
    
    Jitter method (disk sampling):
        - θ = random angle in [0, 2π)
        - r = sqrt(random) for uniform distribution in disk
        - offset = (δ·r·cos(θ), δ·r·sin(θ))
        - Only interior points are jittered (boundary stays fixed)
    """
    S = samples_per_dim

    # Base uniform grid on [0, 1]
    base_u = np.linspace(0.0, 1.0, S, dtype=np.float32)
    base_v = np.linspace(0.0, 1.0, S, dtype=np.float32)

    if S <= 2:
        # No interior points to jitter
        return base_u, base_v

    # Create 2D grid for jittering
    U, V = np.meshgrid(base_u, base_v)  # Shape: (S, S)

    # Compute interior mask: points not on boundary
    # A point is interior if u ∈ (0,1) AND v ∈ (0,1)
    U_interior = (U > 0) & (U < 1)
    V_interior = (V > 0) & (V < 1)
    interior_mask = U_interior & V_interior  # Shape: (S, S)

    # Jitter radius (δ)
    grid_spacing = 1.0 / (S - 1)
    delta = delta_ratio * grid_spacing

    # Disk sampling for jitter
    # θ ~ Uniform[0, 2π)
    theta = 2 * np.pi * np.random.rand(S, S).astype(np.float32)
    # r ~ sqrt(Uniform[0,1]) for uniform distribution in disk
    r = np.sqrt(np.random.rand(S, S)).astype(np.float32)

    # Compute jitter offsets
    jitter_u = delta * r * np.cos(theta) * interior_mask
    jitter_v = delta * r * np.sin(theta) * interior_mask

    # Apply jitter
    U_jittered = U + jitter_u
    V_jittered = V + jitter_v

    # Safety clamp to [0, 1] (should rarely be needed with delta_ratio < 0.5)
    U_jittered = np.clip(U_jittered, 0.0, 1.0)
    V_jittered = np.clip(V_jittered, 0.0, 1.0)

    # Return as flattened 1D arrays for compatibility with existing code
    # The caller will use meshgrid again, but we return the 1D slices
    # Actually, we need to return the full 2D jittered coordinates
    # Let's return the unique values along each axis

    # For compatibility with the existing code that uses meshgrid,
    # we return the 1D base arrays, and the jittering is applied per-quad
    # But this loses the 2D correlation...

    # Better approach: return 2D arrays and let caller flatten them
    return U_jittered.astype(np.float32), V_jittered.astype(np.float32)


def disk_jitter_2d(
        U: np.ndarray,
        V: np.ndarray,
        delta: float
) -> Tuple[np.ndarray, np.ndarray]:
    """Apply disk-based jitter to a 2D grid of (U, V) coordinates.
    
    Only interior points (not on boundary) are jittered.
    
    Args:
        U: 2D array of U coordinates, shape (H, W)
        V: 2D array of V coordinates, shape (H, W)
        delta: Maximum jitter radius
    
    Returns:
        Tuple of (U_jittered, V_jittered)
    """
    # Interior mask: not on any edge (u=0, u=1, v=0, v=1)
    eps = 1e-6
    interior_mask = (
            (U > eps) & (U < 1 - eps) &
            (V > eps) & (V < 1 - eps)
    ).astype(np.float32)

    # Disk sampling
    theta = 2 * np.pi * np.random.rand(*U.shape).astype(np.float32)
    r = np.sqrt(np.random.rand(*U.shape)).astype(np.float32)

    # Apply jitter only to interior points
    U_jittered = U + delta * r * np.cos(theta) * interior_mask
    V_jittered = V + delta * r * np.sin(theta) * interior_mask

    # Clamp to [0, 1]
    U_jittered = np.clip(U_jittered, 0.0, 1.0)
    V_jittered = np.clip(V_jittered, 0.0, 1.0)

    return U_jittered, V_jittered


def _bilinear(p: List[np.ndarray], u: float, v: float) -> np.ndarray:
    """Bilinear interpolation of four points."""
    w00 = (1 - u) * (1 - v)
    w10 = u * (1 - v)
    w01 = u * v
    w11 = (1 - u) * v
    return w00 * p[0] + w10 * p[1] + w01 * p[2] + w11 * p[3]


def _bilinear_batch(p: List[np.ndarray], us: np.ndarray, vs: np.ndarray) -> np.ndarray:
    """Vectorized bilinear interpolation for multiple (u, v) pairs.

    Args:
        p: List of 4 corner points, each shape (3,)
        us: array of u values, shape (N,)
        vs: array of v values, shape (N,)

    Returns:
        Interpolated points, shape (N, 3)
    """
    w00 = ((1 - us) * (1 - vs))[:, None]
    w10 = (us * (1 - vs))[:, None]
    w01 = (us * vs)[:, None]
    w11 = ((1 - us) * vs)[:, None]
    return w00 * p[0] + w10 * p[1] + w01 * p[2] + w11 * p[3]


def _intersect_rays_with_cached_mesh(origins, dirs, vertices, indices):
    """Ray/mesh intersection using cached GAS (for original mesh) - vectorized t calculation."""
    global _worker_optix
    hits = _worker_optix.intersect_cached(origins, dirs)
    hits = np.asarray(hits, dtype=np.int32)

    num_rays = origins.shape[0]
    t_out = np.full(num_rays, -1.0, dtype=np.float32)

    # Find valid hits
    valid_mask = hits >= 0
    valid_indices = np.where(valid_mask)[0]

    if len(valid_indices) == 0:
        return t_out

    # Get triangle data for all valid hits at once
    valid_tri_ids = hits[valid_indices]
    tri_v_indices = indices[valid_tri_ids]  # shape: (N_valid, 3)

    # Gather triangle vertices
    v0 = vertices[tri_v_indices[:, 0]]  # shape: (N_valid, 3)
    v1 = vertices[tri_v_indices[:, 1]]
    v2 = vertices[tri_v_indices[:, 2]]

    # Gather ray data
    ori = origins[valid_indices]
    d = dirs[valid_indices]

    # Vectorized Möller–Trumbore intersection
    e1 = v1 - v0
    e2 = v2 - v0
    pvec = np.cross(d, e2)
    det = np.einsum('ij,ij->i', e1, pvec)

    # Handle near-zero determinants
    valid_det_mask = np.abs(det) > 1e-8

    # Only compute for valid determinants
    sub_indices = valid_indices[valid_det_mask]
    if len(sub_indices) == 0:
        return t_out

    inv_det = 1.0 / det[valid_det_mask]
    tvec = ori[valid_det_mask] - v0[valid_det_mask]
    qvec = np.cross(tvec, e1[valid_det_mask])
    t = np.einsum('ij,ij->i', e2[valid_det_mask], qvec) * inv_det

    t_out[sub_indices] = np.abs(t)
    return t_out


def _intersect_rays_with_mesh(origins, dirs, vertices, indices):
    """Ray/mesh intersection using Optix (non-cached) - vectorized t calculation."""
    global _worker_optix
    hits = _worker_optix.intersect(origins, dirs, vertices, indices)
    hits = np.asarray(hits, dtype=np.int32)

    num_rays = origins.shape[0]
    t_out = np.full(num_rays, -1.0, dtype=np.float32)

    # Find valid hits
    valid_mask = hits >= 0
    valid_indices = np.where(valid_mask)[0]

    if len(valid_indices) == 0:
        return t_out

    # Get triangle data for all valid hits at once
    valid_tri_ids = hits[valid_indices]
    tri_v_indices = indices[valid_tri_ids]  # shape: (N_valid, 3)

    # Gather triangle vertices
    v0 = vertices[tri_v_indices[:, 0]]  # shape: (N_valid, 3)
    v1 = vertices[tri_v_indices[:, 1]]
    v2 = vertices[tri_v_indices[:, 2]]

    # Gather ray data
    ori = origins[valid_indices]
    d = dirs[valid_indices]

    # Vectorized Möller–Trumbore intersection
    e1 = v1 - v0
    e2 = v2 - v0
    pvec = np.cross(d, e2)
    det = np.einsum('ij,ij->i', e1, pvec)

    # Handle near-zero determinants
    valid_det_mask = np.abs(det) > 1e-8

    # Only compute for valid determinants
    sub_indices = valid_indices[valid_det_mask]
    if len(sub_indices) == 0:
        return t_out

    inv_det = 1.0 / det[valid_det_mask]
    tvec = ori[valid_det_mask] - v0[valid_det_mask]
    qvec = np.cross(tvec, e1[valid_det_mask])
    t = np.einsum('ij,ij->i', e2[valid_det_mask], qvec) * inv_det

    t_out[sub_indices] = np.abs(t)
    return t_out


def _rate_score(inner_rate: int, outter_rate: int) -> Tuple[float, float]:
    """Return (sum, variance) score; smaller is better."""
    rates = np.array([inner_rate, outter_rate], dtype=np.float32)
    return float(rates.sum()), float(rates.var())


def process_single_quad(args: Tuple[int, int]):
    """Process a single quad - designed to run in a worker process.

    Args:
        args: Tuple of (qid, max_rate) where max_rate is the maximum rate value (e.g., 5)

    Optimized version with:
    - Vectorized ray generation
    - Pre-created quad vertices (reused across all rate combinations)
    - Efficient numpy operations
    """
    global _worker_context, _worker_disp_sampler
    ctx = _worker_context
    qid, max_rate = args

    face = ctx.quads_baked[qid]
    p = [np.asarray(ctx.verts_baked[i], dtype=np.float32) for i in face.verts]
    n = [np.asarray(ctx.norms_baked[i], dtype=np.float32) for i in face.norms]
    uv_px = [np.asarray(ctx.uvs_baked[i][:2], dtype=np.float32) for i in face.uvs]

    S = ctx.samples_per_dim
    num_rays = S * S

    # Vectorized ray generation - sample_us and sample_vs are already 2D jittered grids
    # Flatten them directly (no need for meshgrid since they're pre-computed as 2D)
    us_flat = ctx.sample_us.ravel().astype(np.float32)
    vs_flat = ctx.sample_vs.ravel().astype(np.float32)

    # Batch bilinear interpolation
    origins = _bilinear_batch(p, us_flat, vs_flat)
    dirs = _bilinear_batch(n, us_flat, vs_flat)

    # Normalize directions
    norms = np.linalg.norm(dirs, axis=1, keepdims=True)
    dirs = dirs / (norms + 1e-8)

    # Offset origins slightly along normals
    origins = origins - 1e-6 * dirs

    # Reference distances on original mesh (using cached GAS for performance)
    t_ref = _intersect_rays_with_cached_mesh(origins, dirs, ctx.orig_vertices, ctx.orig_indices)

    # Pre-compute valid mask for reference
    t_ref_valid = t_ref > 0

    # Pre-create quad vertices once (they don't change across rate combinations)
    quad_verts = [
        Vertex(position=tuple(p[0].tolist()),
               normal=tuple(n[0].tolist()),
               uv=tuple(uv_px[0].tolist())),
        Vertex(position=tuple(p[1].tolist()),
               normal=tuple(n[1].tolist()),
               uv=tuple(uv_px[1].tolist())),
        Vertex(position=tuple(p[2].tolist()),
               normal=tuple(n[2].tolist()),
               uv=tuple(uv_px[2].tolist())),
        Vertex(position=tuple(p[3].tolist()),
               normal=tuple(n[3].tolist()),
               uv=tuple(uv_px[3].tolist())),
    ]

    # All full rows (one per rate combination) for this quad
    full_rows: List[Tuple[int, int, int, int, int, int, int, float]] = []

    v0, v1, v2, v3 = face.verts

    # Generate all rate combinations: inner_rate and outter_rate in [1, max_rate]
    from itertools import product
    rate_values = list(range(1, max_rate + 1))

    for inner_rate, outter_rate in product(rate_values, repeat=2):
        # outter_rate controls all 4 edges, inner_rate controls inner subdivision
        params = QuadTessParams(
            edge=(outter_rate, outter_rate, outter_rate, outter_rate),
            inner=(inner_rate, inner_rate),
            disp_sampler=_worker_disp_sampler,
        )

        # Use fast tessellator with direct numpy array output
        tessellator = QuadTessellatorFast(params)
        mesh_vertices, mesh_indices = tessellator.tessellate_arrays(quad_verts)

        t_tess = _intersect_rays_with_mesh(origins, dirs, mesh_vertices, mesh_indices)

        # Compute epsilon efficiently
        mask = t_ref_valid & (t_tess > 0)
        if not np.any(mask):
            epsilon = -1.0
        else:
            epsilon = float(np.max(np.abs(t_ref[mask] - t_tess[mask])))

        full_rows.append((qid, v0, v1, v2, v3,
                          inner_rate, outter_rate, epsilon))

    # Sort and build Pareto frontier
    full_rows_sorted = sorted(full_rows, key=lambda r: r[-1])

    pareto_rows: List[Tuple[int, int, int, int, int, float, int, int]] = []
    best_score: Optional[Tuple[float, float]] = None
    best_rates: Optional[Tuple[int, int]] = None

    for (qid_, v0_, v1_, v2_, v3_, inner_r, outter_r, eps) in full_rows_sorted:
        if not np.isfinite(eps) or eps < 0.0:
            continue
        cur_score = _rate_score(inner_r, outter_r)
        if best_score is None or cur_score < best_score:
            best_score = cur_score
            best_rates = (inner_r, outter_r)

        pareto_rows.append((qid_, v0_, v1_, v2_, v3_, eps,
                            best_rates[0], best_rates[1]))

    return full_rows, pareto_rows


def generate_quad_training_csv(
        orig_mesh: str,
        baked_mesh: str,
        baked_disp_exr: str,
        resolution: int,
        samples_per_dim: int,
        max_rate: int = 5,
) -> None:
    """Generate CSV files for NTF training.

    Args:
        orig_mesh: Path to the original high-poly mesh.
        baked_mesh: Path to the baked low-poly quad mesh.
        baked_disp_exr: Path to the displacement EXR file.
        resolution: Resolution of the displacement map.
        samples_per_dim: Number of samples per dimension for ray casting.
        max_rate: Maximum rate value for inner/outter rates (default 5).
                  Rates will be in range [1, max_rate].

    1) <base>_points.csv:
       Columns: point_id,x,y,z
       Data: all vertices from the baked mesh.

    2) <base>_quads.csv:
       Full combination data: for each quad and each
          (inner_rate, outter_rate) in [1, max_rate] x [1, max_rate]
       store epsilon.

    3) <base>_quads_pareto.csv:
       For each quad_id, traverse the full-combination data in 2) sorted
       by epsilon ascending, maintain a "current best" rate combination
       (first prefer smaller sum of the 2 rates, then smaller variance for
       more uniform distribution). Whenever a new combination is strictly
       better than the current best, emit a record with:
         epsilon_target = the epsilon of this row,
         rate = the updated best rate combination.
    """
    logging.info(f"[NTF] Generating quad training CSVs (points + quads + pareto), max_rate={max_rate}")

    # Derive base output filenames from baked mesh path
    base, _ = os.path.splitext(baked_mesh)
    points_csv = f"{base}_points.csv"
    quads_csv = f"{base}_quads.csv"
    pareto_csv = f"{base}_quads_pareto.csv"

    # 1. Reference surface (used for epsilon computation)
    ms = pymeshlab.MeshSet()
    if not os.path.isfile(orig_mesh):
        logging.error(f"[NTF] Original mesh not found: {orig_mesh}")
        return
    ms.load_new_mesh(orig_mesh)
    m = ms.current_mesh()
    orig_vertices = np.array(m.vertex_matrix(), dtype=np.float32)
    orig_indices = np.array(m.face_matrix(), dtype=np.uint32)
    logging.info(
        f"[NTF] Loaded original mesh {orig_mesh}, verts={orig_vertices.shape[0]}, faces={orig_indices.shape[0]}"
    )

    # 2. Parse baked mesh quads (geometry in baked space)
    verts_baked, norms_baked, uvs_baked, quads_baked = parse_quad_mesh(baked_mesh)
    num_quads = len(quads_baked)
    logging.info(f"[NTF] Parsed baked mesh {baked_mesh}, quads={num_quads}")

    # 3. Check displacement EXR exists
    if not os.path.isfile(baked_disp_exr):
        logging.error(f"[NTF] Displacement EXR not found: {baked_disp_exr}")
        return

    # 4. Precompute sample grid on [0,1]^2 with disk jittering
    S = samples_per_dim
    sample_us, sample_vs = sample_grid_with_disk_jitter(S)
    logging.info(f"[NTF] Generated {S}x{S} sample grid with disk jittering")

    # 5. Write points.csv from baked mesh vertices
    logging.info(f"[NTF] Writing points CSV to {points_csv}")
    with open(points_csv, "w", newline="", encoding="utf-8") as f_pts:
        writer = csv.writer(f_pts)
        writer.writerow(["point_id", "x", "y", "z"])
        for pid, (x, y, z) in enumerate(verts_baked):
            writer.writerow([pid, float(x), float(y), float(z)])

    # 6. Create context for worker processes
    context = QuadProcessingContext(
        quads_baked=quads_baked,
        verts_baked=verts_baked,
        norms_baked=norms_baked,
        uvs_baked=uvs_baked,
        orig_vertices=orig_vertices,
        orig_indices=orig_indices,
        sample_us=sample_us,
        sample_vs=sample_vs,
        samples_per_dim=S,
        disp_exr_path=baked_disp_exr,
        resolution=resolution,
    )

    num_rate_combinations = max_rate * max_rate
    logging.info(f"[NTF] Writing quad CSV to {quads_csv}, rate_combinations_per_quad={num_rate_combinations}")

    max_workers = min(60, int(os.cpu_count() * 0.5))
    logging.info(f"[NTF] Using ProcessPoolExecutor with max_workers={max_workers}")

    with open(quads_csv, "w", newline="", encoding="utf-8") as f_q, \
            open(pareto_csv, "w", newline="", encoding="utf-8") as f_p:
        writer_full = csv.writer(f_q)
        writer_pareto = csv.writer(f_p)

        writer_full.writerow([
            "quad_id", "v0", "v1", "v2", "v3",
            "inner_rate", "outter_rate", "epsilon",
        ])
        writer_pareto.writerow([
            "quad_id", "v0", "v1", "v2", "v3",
            "epsilon_target",
            "inner_rate", "outter_rate",
        ])

        with ProcessPoolExecutor(
                max_workers=max_workers,
                initializer=_worker_initializer,
                initargs=(context,)
        ) as executor:
            # Pass (qid, max_rate) tuple to each worker
            futures = {executor.submit(process_single_quad, (qid, max_rate)): qid for qid in range(num_quads)}
            completed = 0
            for future in as_completed(futures):
                qid = futures[future]
                try:
                    full_rows, pareto_rows = future.result()
                except Exception as e:
                    logging.exception(f"[NTF] Exception while processing quad {qid}: {e}")
                    continue

                writer_full.writerows(full_rows)
                writer_pareto.writerows(pareto_rows)
                completed += 1
                if completed % 10 == 0 or completed == num_quads:
                    logging.info(f"[NTF] Completed {completed}/{num_quads} quads")

    logging.info(
        f"[NTF] Quad training data generation finished.\n  points_csv={points_csv}\n  quads_csv={quads_csv}\n  pareto_csv={pareto_csv}")


# ========== Data Augmentation and Filtering ==========

def _detect_bimodal(eps_arr: np.ndarray, gap_ratio_threshold: float = 10.0,
                    min_gap_threshold: float = 0.3) -> Tuple[bool, float]:
    """Detect if epsilon distribution is bimodal.

    A bimodal distribution has a large gap in the middle, e.g., epsilon jumps
    from ~0.1 to ~1.6 without intermediate values.

    Args:
        eps_arr: Array of epsilon values for a single quad
        gap_ratio_threshold: Ratio threshold for detecting bimodal (max_gap / median_gap)
        min_gap_threshold: Minimum absolute gap size to consider

    Returns:
        (is_bimodal, max_gap): Detection result and the maximum gap value
    """
    if len(eps_arr) < 3:
        return False, 0.0

    sorted_eps = np.sort(eps_arr)
    gaps = np.diff(sorted_eps)

    if len(gaps) == 0:
        return False, 0.0

    max_gap = float(gaps.max())
    median_gap = float(np.median(gaps))

    if max_gap < min_gap_threshold:
        return False, max_gap

    if median_gap < 1e-8:
        return True, max_gap

    gap_ratio = max_gap / median_gap
    return gap_ratio > gap_ratio_threshold, max_gap


@dataclass
class _QuadStats:
    """Statistics for a single quad's epsilon distribution."""
    quad_id: int
    v0: int
    v1: int
    v2: int
    v3: int
    epsilon_min: float
    epsilon_max: float
    samples: List[Tuple[float, int, int]]  # List of (epsilon, inner_rate, outter_rate)


def augment_pareto_csv(
        pareto_csv: str,
        output_csv: Optional[str] = None,
        max_eps_threshold: float = 1.0,
        num_augment_samples: int = 10,
) -> str:
    """Filter bad quads and augment data by filling missing epsilon ranges.

    This function performs two main operations:
    1. Filter out "bad quads":
       - Quads with very high minimum epsilon (cannot approximate surface well)
       - Quads with bimodal epsilon distribution (discontinuous behavior)

    2. Augment data for each remaining quad:
       - If quad's epsilon_max < global_eps_max: fill with rate=(1,1) (far camera)
       - If quad's epsilon_min > global_eps_min: fill with quad's max rate (close camera)

    Args:
        pareto_csv: Input Pareto CSV file (from generate_quad_training_csv)
        output_csv: Output augmented CSV file. If None, uses <base>_augmented.csv
        max_eps_threshold: Maximum acceptable minimum epsilon for filtering (default 1.0)
        num_augment_samples: Number of synthetic samples per gap (default 10)

    Returns:
        Path to the output augmented CSV file
    """
    from collections import defaultdict

    logging.info(f"[NTF] Augmenting Pareto CSV: {pareto_csv}")

    # Derive output path
    if output_csv is None:
        base, ext = os.path.splitext(pareto_csv)
        if base.endswith("_pareto"):
            base = base[:-7]
        output_csv = f"{base}_augmented.csv"

    # ========== Step 1: Load data and compute per-quad statistics ==========
    quad_data: dict = defaultdict(list)

    with open(pareto_csv, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            qid = int(row["quad_id"])
            v0 = int(row["v0"])
            v1 = int(row["v1"])
            v2 = int(row["v2"])
            v3 = int(row["v3"])
            eps = float(row["epsilon_target"])
            inner_rate = int(row["inner_rate"])
            outter_rate = int(row["outter_rate"])

            quad_data[qid].append((v0, v1, v2, v3, eps, inner_rate, outter_rate))

    # Compute per-quad statistics (without global range yet)
    quad_stats: dict = {}

    for qid, samples in quad_data.items():
        if not samples:
            continue

        v0, v1, v2, v3 = samples[0][:4]
        epsilons = [s[4] for s in samples]
        eps_arr = np.array(epsilons)

        quad_stats[qid] = _QuadStats(
            quad_id=qid,
            v0=v0, v1=v1, v2=v2, v3=v3,
            epsilon_min=float(eps_arr.min()),
            epsilon_max=float(eps_arr.max()),
            samples=[(s[4], s[5], s[6]) for s in samples]  # (eps, inner, outter)
        )

    logging.info(f"[NTF] Loaded {len(quad_stats)} quads")

    # ========== Step 2: Filter bad quads FIRST ==========
    bad_quads: set = set()
    bad_reasons: dict = {}

    for qid, stats in quad_stats.items():
        eps_arr = np.array([s[0] for s in stats.samples])

        reason = None

        # Condition 1: Very high minimum epsilon
        if stats.epsilon_min > max_eps_threshold:
            reason = f"high_min_eps ({stats.epsilon_min:.4f} > {max_eps_threshold})"
        else:
            # Condition 2: Bimodal distribution
            is_bimodal, max_gap = _detect_bimodal(eps_arr)
            if is_bimodal:
                reason = f"bimodal (max_gap={max_gap:.4f}, range=[{stats.epsilon_min:.4f}, {stats.epsilon_max:.4f}])"

        if reason:
            bad_quads.add(qid)
            bad_reasons[qid] = reason

    logging.info(f"[NTF] Found {len(bad_quads)} bad quads to filter")
    if bad_quads:
        example_ids = list(bad_quads)[:10]
        for qid in example_ids:
            logging.warning(f"[NTF]   Quad {qid}: {bad_reasons.get(qid, 'unknown')}")

    # ========== Step 3: Compute global epsilon range from GOOD quads only ==========
    global_eps_min = float('inf')
    global_eps_max = float('-inf')

    for qid, stats in quad_stats.items():
        if qid in bad_quads:
            continue  # Skip bad quads when computing global range

        global_eps_min = min(global_eps_min, stats.epsilon_min)
        global_eps_max = max(global_eps_max, stats.epsilon_max)

    logging.info(f"[NTF] Global epsilon range (good quads only): [{global_eps_min:.6f}, {global_eps_max:.6f}]")

    # ========== Step 3: Generate augmented samples ==========
    augmented_samples: List[Tuple[int, int, int, int, int, float, int, int]] = []

    for qid, stats in quad_stats.items():
        if qid in bad_quads:
            continue

        # Sort samples by epsilon
        samples_sorted = sorted(stats.samples, key=lambda x: x[0])

        if not samples_sorted:
            continue

        min_eps_sample = samples_sorted[0]  # Lowest epsilon -> highest rate
        max_eps_sample = samples_sorted[-1]  # Highest epsilon -> lowest rate

        # Rate at lowest epsilon (highest rate for this quad)
        rate_at_min_eps = (min_eps_sample[1], min_eps_sample[2])

        v0, v1, v2, v3 = stats.v0, stats.v1, stats.v2, stats.v3

        # Fill gap above this quad's max epsilon -> use (1, 1) rate
        # This is the "far camera" scenario where we want minimal tessellation
        if stats.epsilon_max < global_eps_max:
            gap_start = stats.epsilon_max
            gap_end = global_eps_max

            fill_epsilons = np.linspace(gap_start, gap_end, num_augment_samples + 2)[1:]

            for eps in fill_epsilons:
                augmented_samples.append((qid, v0, v1, v2, v3, float(eps), 1, 1))

        # Fill gap below this quad's min epsilon -> use the quad's max rate
        # This is the "close camera" scenario where we want maximum tessellation
        if stats.epsilon_min > global_eps_min:
            gap_start = global_eps_min
            gap_end = stats.epsilon_min

            fill_epsilons = np.linspace(gap_start, gap_end, num_augment_samples + 2)[:-1]

            for eps in fill_epsilons:
                augmented_samples.append((qid, v0, v1, v2, v3, float(eps),
                                          rate_at_min_eps[0], rate_at_min_eps[1]))

    logging.info(f"[NTF] Generated {len(augmented_samples)} augmented samples")

    # ========== Step 4: Write output CSV ==========
    original_count = 0
    augmented_count = 0

    with open(output_csv, "w", newline="", encoding="utf-8") as f_out:
        writer = csv.writer(f_out)
        writer.writerow([
            "quad_id", "v0", "v1", "v2", "v3",
            "epsilon_target", "inner_rate", "outter_rate"
        ])

        # Write original samples (excluding bad quads)
        for qid, stats in quad_stats.items():
            if qid in bad_quads:
                continue

            for eps, inner_rate, outter_rate in stats.samples:
                writer.writerow([qid, stats.v0, stats.v1, stats.v2, stats.v3,
                                 eps, inner_rate, outter_rate])
                original_count += 1

        # Write augmented samples
        for sample in augmented_samples:
            writer.writerow(sample)
            augmented_count += 1

    total_count = original_count + augmented_count
    good_quads = len(quad_stats) - len(bad_quads)

    logging.info(f"[NTF] Output written to {output_csv}")
    logging.info(f"[NTF] Total samples: {total_count} (original: {original_count}, augmented: {augmented_count})")
    logging.info(f"[NTF] Good quads: {good_quads}/{len(quad_stats)} ({100 * good_quads / len(quad_stats):.1f}%)")

    return output_csv


if __name__ == "__main__":
    # Windows multi-process must use freeze_support
    mp.freeze_support()

    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s',
        handlers=[logging.StreamHandler(sys.stdout)]
    )

    orig_mesh = "assets/Bayon Lion.obj"
    baked_mesh = "assets/Bayon Lion_baked.obj"
    disp_exr = "assets/Bayon Lion_baked_disp.exr"
    resolution = 1024
    sample_per_dim = 20
    max_rate = 5

    # Step 1: Generate raw training data (points + quads + pareto CSVs)
    generate_quad_training_csv(orig_mesh, baked_mesh, disp_exr, resolution, sample_per_dim, max_rate)

    # Step 2: Filter bad quads and augment data
    base, _ = os.path.splitext(baked_mesh)
    pareto_csv = f"{base}_quads_pareto.csv"
    augmented_csv = augment_pareto_csv(
        pareto_csv=pareto_csv,
        max_eps_threshold=1.0,
        num_augment_samples=10,
    )

    logging.info(f"[NTF] Pipeline complete. Use {augmented_csv} for training.")
