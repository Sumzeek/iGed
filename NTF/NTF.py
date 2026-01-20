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

    fflevels: int = 8
    hidden_dim: int = 64
    max_rate: int = 16


class NTFMLP(nn.Module):
    """MLP: [positional encoding of quad + epsilon] -> 4 continuous edge rates.

    Instead of classification over discrete rates, we regress 4 real-valued
    edge rates (bottom, right, top, left). The consumer can later round or
    clamp as needed.
    """

    def __init__(self, in_dim: int, hidden_dim: int = 64) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(in_dim, hidden_dim),
            nn.LeakyReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.LeakyReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.LeakyReLU(),
            nn.Linear(hidden_dim, 4),  # 4 edges, continuous rates
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
    We apply Fourier positional encoding, then an MLP that regresses 4
    continuous edge rates (bottom, right, top, left).
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
        logging.info(f"  max_rate     = {self.config.max_rate}")

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """Forward pass.

        Args:
            x: tensor of shape (B, D_raw), where each row is
               [p0(3), p1(3), p2(3), p3(3), eps].
        Returns:
            rates: tensor of shape (B, 4) with continuous edge rates
                    (bottom, right, top, left).
        """
        enc = self.encoder(x)
        rates = self.mlp(enc)
        return rates

    # Convenience wrapper for prediction, mirroring `Trainer.predict` logic.
    def predict_rate(self, x: torch.Tensor) -> torch.Tensor:
        """Return predicted 4 continuous edge rates from raw feature tensor x.

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
    max_tess_rate: int
    disp_exr_path: str
    resolution: int


# 全局变量，用于在子进程中存储上下文
_worker_context: Optional[QuadProcessingContext] = None
_worker_disp_sampler: Optional[DisplacementSampler] = None
_worker_optix = None


def _worker_initializer(context: QuadProcessingContext) -> None:
    """Initialize worker process with shared context."""
    global _worker_context, _worker_disp_sampler, _worker_optix
    _worker_context = context
    # 每个子进程独立加载 DisplacementSampler 和 optix
    _worker_disp_sampler = DisplacementSampler(context.disp_exr_path, context.resolution)
    _worker_optix = import_optixbaker()


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


def _intersect_rays_with_mesh(origins, dirs, vertices, indices):
    """Ray/mesh intersection using Optix - vectorized t calculation."""
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


def _rate_score(sb: int, sr: int, st: int, sl: int) -> Tuple[float, float]:
    """Return (sum, variance) score; smaller is better."""
    rates = np.array([sb, sr, st, sl], dtype=np.float32)
    return float(rates.sum()), float(rates.var())


def process_single_quad(qid: int):
    """Process a single quad - designed to run in a worker process.

    Optimized version with:
    - Vectorized ray generation
    - Pre-created quad vertices (reused across all rate combinations)
    - Efficient numpy operations
    """
    global _worker_context, _worker_disp_sampler
    ctx = _worker_context

    face = ctx.quads_baked[qid]
    p = [np.asarray(ctx.verts_baked[i], dtype=np.float32) for i in face.verts]
    n = [np.asarray(ctx.norms_baked[i], dtype=np.float32) for i in face.norms]
    uv_px = [np.asarray(ctx.uvs_baked[i][:2], dtype=np.float32) for i in face.uvs]

    S = ctx.samples_per_dim
    num_rays = S * S

    # Vectorized ray generation - use meshgrid and batch bilinear
    us_grid, vs_grid = np.meshgrid(ctx.sample_us, ctx.sample_vs)
    us_flat = us_grid.ravel().astype(np.float32)
    vs_flat = vs_grid.ravel().astype(np.float32)

    # Batch bilinear interpolation
    origins = _bilinear_batch(p, us_flat, vs_flat)
    dirs = _bilinear_batch(n, us_flat, vs_flat)

    # Normalize directions
    norms = np.linalg.norm(dirs, axis=1, keepdims=True)
    dirs = dirs / (norms + 1e-8)

    # Offset origins slightly along normals
    origins = origins - 1e-6 * dirs

    # Reference distances on original mesh
    t_ref = _intersect_rays_with_mesh(origins, dirs, ctx.orig_vertices, ctx.orig_indices)

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
    full_rows: List[Tuple[int, int, int, int, int, int, int, int, float]] = []

    v0, v1, v2, v3 = face.verts
    max_tess_rate = ctx.max_tess_rate

    # Generate all rate combinations using itertools for efficiency
    from itertools import product
    rate_range = range(1, max_tess_rate + 1)

    for s_bottom, s_right, s_top, s_left in product(rate_range, repeat=4):
        params = QuadTessParams(
            edge=(s_bottom, s_right, s_top, s_left),
            inner=((s_bottom + s_top) // 2, (s_right + s_left) // 2),
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
                          s_bottom, s_right, s_top, s_left, epsilon))

    # Sort and build Pareto frontier
    full_rows_sorted = sorted(full_rows, key=lambda r: r[-1])

    pareto_rows: List[Tuple[int, int, int, int, float, int, int, int, int]] = []
    best_score: Optional[Tuple[float, float]] = None
    best_rates: Optional[Tuple[int, int, int, int]] = None

    for (qid_, v0_, v1_, v2_, v3_, sb, sr, st, sl, eps) in full_rows_sorted:
        if not np.isfinite(eps) or eps < 0.0:
            continue
        cur_score = _rate_score(sb, sr, st, sl)
        if best_score is None or cur_score < best_score:
            best_score = cur_score
            best_rates = (sb, sr, st, sl)

        pareto_rows.append((qid_, v0_, v1_, v2_, v3_, eps,
                            best_rates[0], best_rates[1], best_rates[2], best_rates[3]))

    return full_rows, pareto_rows


def generate_quad_training_csv(
        orig_mesh: str,
        baked_mesh: str,
        baked_disp_exr: str,
        resolution: int,
        max_tess_rate: int,
        samples_per_dim: int,
) -> None:
    """Generate CSV files for NTF training.

    1) <base>_points.csv:
       Columns: point_id,x,y,z
       Data: all vertices from the baked mesh.

    2) <base>_quads.csv:
       Full combination data: for each quad and each
          (sample_rate_bottom, sample_rate_right, sample_rate_top, sample_rate_left)
       store epsilon.

    3) <base>_quads_pareto.csv:
       For each quad_id, traverse the full-combination data in 2) sorted
       by epsilon ascending, maintain a "current best" rate combination
       (first prefer smaller sum of the 4 rates, then smaller variance for
       more uniform distribution). Whenever a new combination is strictly
       better than the current best, emit a record with:
         epsilon_target = the epsilon of this row,
         rate = the updated best rate combination.
    """
    logging.info("[NTF] Generating quad training CSVs (points + quads + pareto)")

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

    # 4. Precompute sample grid on [0,1]^2
    S = samples_per_dim
    sample_us = np.linspace(0.0, 1.0, S, dtype=np.float32)
    sample_vs = np.linspace(0.0, 1.0, S, dtype=np.float32)

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
        max_tess_rate=max_tess_rate,
        disp_exr_path=baked_disp_exr,
        resolution=resolution,
    )

    logging.info(f"[NTF] Writing quad CSV to {quads_csv}, max_rate={max_tess_rate}, samples_per_quad={S * S}")

    max_workers = min(60, int(os.cpu_count() * 0.5))
    logging.info(f"[NTF] Using ProcessPoolExecutor with max_workers={max_workers}")

    with open(quads_csv, "w", newline="", encoding="utf-8") as f_q, \
            open(pareto_csv, "w", newline="", encoding="utf-8") as f_p:
        writer_full = csv.writer(f_q)
        writer_pareto = csv.writer(f_p)

        writer_full.writerow([
            "quad_id", "v0", "v1", "v2", "v3",
            "sample_rate_bottom", "sample_rate_right", "sample_rate_top", "sample_rate_left", "epsilon",
        ])
        writer_pareto.writerow([
            "quad_id", "v0", "v1", "v2", "v3",
            "epsilon_target",
            "sample_rate_bottom", "sample_rate_right", "sample_rate_top", "sample_rate_left",
        ])

        with ProcessPoolExecutor(
                max_workers=max_workers,
                initializer=_worker_initializer,
                initargs=(context,)
        ) as executor:
            futures = {executor.submit(process_single_quad, qid): qid for qid in range(num_quads)}
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
    sample_per_dim = 100
    max_tess_rate = 8

    generate_quad_training_csv(orig_mesh, baked_mesh, disp_exr, resolution, max_tess_rate, sample_per_dim)
