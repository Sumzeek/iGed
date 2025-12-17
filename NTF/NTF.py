from __future__ import annotations

import sys
import csv
import logging
import os
from dataclasses import dataclass
from typing import Optional, Tuple, List

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
from Tessellator import DisplacementSampler, QuadTessellator, QuadTessParams, Vertex


def generate_quad_training_csv(
        baked_mesh: str,
        limited_mesh: str,
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

    # 1. Parse baked mesh quads (geometry in baked space)
    verts_baked, norms_baked, uvs_baked, quads_baked = parse_quad_mesh(baked_mesh)
    num_quads = len(quads_baked)
    logging.info(f"[NTF] Parsed baked mesh {baked_mesh}, quads={num_quads}")

    # 2. Limited mesh as reference surface (used for epsilon computation)
    ms = pymeshlab.MeshSet()
    if not os.path.isfile(limited_mesh):
        logging.error(f"[NTF] Limited mesh not found: {limited_mesh}")
        return
    ms.load_new_mesh(limited_mesh)
    m = ms.current_mesh()
    limited_vertices = np.array(m.vertex_matrix(), dtype=np.float32)
    limited_indices = np.array(m.face_matrix(), dtype=np.uint32)
    logging.info(
        f"[NTF] Loaded limited mesh {limited_mesh}, verts={limited_vertices.shape[0]}, faces={limited_indices.shape[0]}"
    )

    # 3. Displacement sampler over baked_disp_exr
    if not os.path.isfile(baked_disp_exr):
        logging.error(f"[NTF] Displacement EXR not found: {baked_disp_exr}")
        return
    disp_sampler = DisplacementSampler(baked_disp_exr, resolution)

    # 4. Precompute sample grid on [0,1]^2
    S = samples_per_dim
    sample_us = np.linspace(0.0, 1.0, S, dtype=np.float32)
    sample_vs = np.linspace(0.0, 1.0, S, dtype=np.float32)

    # 5. Helper: bilinear interpolation of four points/normals
    def bilinear(p: List[np.ndarray], u: float, v: float) -> np.ndarray:
        w00 = (1 - u) * (1 - v)
        w10 = u * (1 - v)
        w01 = u * v
        w11 = (1 - u) * v
        return w00 * p[0] + w10 * p[1] + w01 * p[2] + w11 * p[3]

    # 6. Ray/mesh intersection helper (Möller–Trumbore) using Optix hits
    optix = import_optixbaker()

    def intersect_rays_with_mesh(origins, dirs, vertices, indices):
        hits = optix.intersect(origins, dirs, vertices, indices)
        t_out = np.full(origins.shape[0], -1.0, dtype=np.float32)
        for i, tri_id in enumerate(hits):
            if tri_id < 0:
                continue
            i0, i1, i2 = indices[tri_id]
            v0 = vertices[i0]
            v1 = vertices[i1]
            v2 = vertices[i2]
            ori = origins[i]
            d = dirs[i]
            e1 = v1 - v0
            e2 = v2 - v0
            pvec = np.cross(d, e2)
            det = np.dot(e1, pvec)
            if abs(det) < 1e-8:
                continue
            inv_det = 1.0 / det
            tvec = ori - v0
            u = np.dot(tvec, pvec) * inv_det
            qvec = np.cross(tvec, e1)
            v = np.dot(d, qvec) * inv_det
            t = np.dot(e2, qvec) * inv_det
            t_out[i] = abs(t)
        return t_out

    # 7. Write points.csv from baked mesh vertices
    logging.info(f"[NTF] Writing points CSV to {points_csv}")
    with open(points_csv, "w", newline="", encoding="utf-8") as f_pts:
        writer = csv.writer(f_pts)
        writer.writerow(["point_id", "x", "y", "z"])
        for pid, (x, y, z) in enumerate(verts_baked):
            writer.writerow([pid, float(x), float(y), float(z)])

    # 8. Per-quad epsilon computation: write full quads.csv and, in memory,
    #    build a per-quad Pareto-compressed view used for quads_pareto.csv.
    from concurrent.futures import ThreadPoolExecutor, as_completed
    import collections

    logging.info(f"[NTF] Writing quad CSV to {quads_csv}, max_rate={max_tess_rate}, samples_per_quad={S * S}")

    def rate_score(sb: int, sr: int, st: int, sl: int) -> Tuple[float, float]:
        """Return (sum, variance) score; smaller is better.

        We first minimize the sum of the 4 rates (cheaper overall tessellation),
        and in case of ties we minimize the variance to prefer more uniform
        distributions such as (2,2,2,2) over (1,4,4,1).
        """
        rates = np.array([sb, sr, st, sl], dtype=np.float32)
        return float(rates.sum()), float(rates.var())

    def process_single_quad(qid: int):
        face = quads_baked[qid]
        p = [np.asarray(verts_baked[i], dtype=np.float32) for i in face.verts]
        n = [np.asarray(norms_baked[i], dtype=np.float32) for i in face.norms]
        uv_px = [np.asarray(uvs_baked[i][:2], dtype=np.float32) for i in face.uvs]

        num_rays = S * S
        origins = np.zeros((num_rays, 3), dtype=np.float32)
        dirs = np.zeros((num_rays, 3), dtype=np.float32)

        idx = 0
        for vi in range(S):
            v_ = float(sample_vs[vi])
            for ui in range(S):
                u_ = float(sample_us[ui])
                pos = bilinear(p, u_, v_)
                nor = bilinear(n, u_, v_)
                nor = nor / (np.linalg.norm(nor) + 1e-8)
                origins[idx] = pos - 1e-6 * nor
                dirs[idx] = nor
                idx += 1

        # Reference distances on limited mesh
        t_ref = intersect_rays_with_mesh(origins, dirs, limited_vertices, limited_indices)

        # All full rows (one per rate combination) for this quad
        full_rows: List[Tuple[int, int, int, int, int, int, int, int, float]] = []

        # Per-quad Pareto frontier records (local to this quad only)
        pareto_rows: List[Tuple[int, int, int, int, float, int, int, int, int]] = []
        best_score: Optional[Tuple[float, float]] = None
        best_rates: Optional[Tuple[int, int, int, int]] = None

        v0, v1, v2, v3 = face.verts

        for s_bottom in range(1, max_tess_rate + 1):
            for s_right in range(1, max_tess_rate + 1):
                for s_top in range(1, max_tess_rate + 1):
                    for s_left in range(1, max_tess_rate + 1):
                        quad_verts = [
                            Vertex(position=(float(p[0][0]), float(p[0][1]), float(p[0][2])),
                                   normal=(float(n[0][0]), float(n[0][1]), float(n[0][2])),
                                   uv=(float(uv_px[0][0]), float(uv_px[0][1]))),
                            Vertex(position=(float(p[1][0]), float(p[1][1]), float(p[1][2])),
                                   normal=(float(n[1][0]), float(n[1][1]), float(n[1][2])),
                                   uv=(float(uv_px[1][0]), float(uv_px[1][1]))),
                            Vertex(position=(float(p[2][0]), float(p[2][1]), float(p[2][2])),
                                   normal=(float(n[2][0]), float(n[2][1]), float(n[2][2])),
                                   uv=(float(uv_px[2][0]), float(uv_px[2][1]))),
                            Vertex(position=(float(p[3][0]), float(p[3][1]), float(p[3][2])),
                                   normal=(float(n[3][0]), float(n[3][1]), float(n[3][2])),
                                   uv=(float(uv_px[3][0]), float(uv_px[3][1]))),
                        ]

                        params = QuadTessParams(
                            edge=(s_bottom, s_right, s_top, s_left),
                            inner=((s_bottom + s_top) // 2, (s_right + s_left) // 2),
                            disp_sampler=disp_sampler,
                        )

                        tessellator = QuadTessellator(params)
                        verts_tess, tris_tess = tessellator.tessellate(quad_verts)

                        mesh_vertices = np.array([v.position for v in verts_tess], dtype=np.float32)
                        mesh_indices = np.array(tris_tess, dtype=np.uint32)

                        t_tess = intersect_rays_with_mesh(origins, dirs, mesh_vertices, mesh_indices)

                        mask = (t_ref > 0) & (t_tess > 0)
                        if not np.any(mask):
                            epsilon = -1.0
                        else:
                            epsilon = float(np.max(np.abs(t_ref[mask] - t_tess[mask])))

                        full_rows.append((qid, v0, v1, v2, v3,
                                          s_bottom, s_right, s_top, s_left, epsilon))

        # For this quad, sort full_rows by epsilon ascending and build a
        # local prefix-optimal frontier over rate combinations.
        # For each epsilon (after sorting ascending), we maintain the
        # best rate combination seen so far (using rate_score) and emit
        # one record. This means each valid epsilon has a corresponding
        # best rate, and later (worse) combinations are effectively
        # replaced by earlier, cheaper/more-uniform ones.
        full_rows_sorted = sorted(full_rows, key=lambda r: r[-1])
        for (qid_, v0_, v1_, v2_, v3_, sb, sr, st, sl, eps) in full_rows_sorted:
            if not np.isfinite(eps) or eps < 0.0:
                continue
            cur_score = rate_score(sb, sr, st, sl)
            if best_score is None or cur_score < best_score:
                best_score = cur_score
                best_rates = (sb, sr, st, sl)

            # Always emit the current best rates for this epsilon
            pareto_rows.append((qid_, v0_, v1_, v2_, v3_, eps,
                                best_rates[0], best_rates[1], best_rates[2], best_rates[3]))

        return full_rows, pareto_rows

    max_workers = min(8, os.cpu_count() or 4)
    logging.info(f"[NTF] Using ThreadPoolExecutor with max_workers={max_workers}")

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

        with ThreadPoolExecutor(max_workers=max_workers) as executor:
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
                logging.info(f"[NTF] Completed {completed}/{num_quads} quads")

    logging.info(
        f"[NTF] Quad training data generation finished.\n  points_csv={points_csv}\n  quads_csv={quads_csv}\n  pareto_csv={pareto_csv}")


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s',
        handlers=[logging.StreamHandler(sys.stdout)]
    )

    baked_mesh = "assets/Icosphere_baked.obj"
    limited_mesh = "assets/Icosphere_baked_limited.obj"
    disp_exr = "assets/Icosphere_baked_disp.exr"
    resolution = 1024
    sample_per_dim = 16
    max_tess_rate = 4

    generate_quad_training_csv(baked_mesh, limited_mesh, disp_exr, resolution, max_tess_rate, sample_per_dim)
