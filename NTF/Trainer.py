import math
import os
import sys
import logging
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from typing import List, Tuple, Optional, Dict
import csv

import numpy as np
import OpenEXR
import Imath
import pymeshlab


def clamp(x: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, x))


Vec3 = Tuple[float, float, float]
Tri = Tuple[int, int, int]


@dataclass
class TessParams:
    edge: Tuple[int, int, int, int]
    inner: Tuple[int, int]
    lift: Optional[Tuple[float, float, float, float]] = None
    i0_pref_lower: bool = True


def generate_boundary_samples(rate: int, t_min: float, t_max: float) -> List[float]:
    if rate <= 0:
        return []
    k0 = math.ceil(rate * t_min - 1e-9)
    k1 = math.floor(rate * t_max + 1e-9)
    k0 = int(clamp(k0, 0, rate))
    k1 = int(clamp(k1, 0, rate))
    if k1 < k0:
        return []
    return [k / rate for k in range(k0, k1 + 1)]


def nearest_index_with_tie(x: float, lo: int, hi: int, prefer_up: bool, eps: float = 1e-12) -> int:
    base = math.floor(x)
    frac = x - base
    if frac > 0.5 + eps:
        idx = base + 1
    elif frac < 0.5 - eps:
        idx = base
    else:
        idx = base + (1 if prefer_up else 0)
    return int(clamp(idx, lo, hi))


def compute_lift(params: TessParams) -> Tuple[float, float, float, float]:
    if params.lift is not None:
        b, r, t, l = params.lift
        res = (clamp(b, 0.0, 0.49), clamp(r, 0.0, 0.49), clamp(t, 0.0, 0.49), clamp(l, 0.0, 0.49))
        params.lift = res
        return res

    iu, iv = params.inner
    e_bottom, e_right, e_top, e_left = params.edge
    denom = e_bottom + e_right + e_top + e_left + 2 * (iu + iv)
    if denom <= 0:
        lift = 0.0
    else:
        t = (iu * iv * 2) / denom
        lift = (1 - math.sqrt(max(0.0, 1 - 1 / (t + 1)))) / 2
    res = (lift, lift, lift, lift)
    params.lift = res
    return res


def bilinear(u: float, v: float, p00: Vec3, p10: Vec3, p01: Vec3, p11: Vec3) -> Vec3:
    w00 = (1 - u) * (1 - v)
    w10 = u * (1 - v)
    w01 = (1 - u) * v
    w11 = u * v
    x = w00 * p00[0] + w10 * p10[0] + w01 * p01[0] + w11 * p11[0]
    y = w00 * p00[1] + w10 * p10[1] + w01 * p01[1] + w11 * p11[1]
    z = w00 * p00[2] + w10 * p10[2] + w01 * p01[2] + w11 * p11[2]
    return (x, y, z)


def tessellate_quad(p00: Vec3, p10: Vec3, p01: Vec3, p11: Vec3, params: TessParams) -> Tuple[List[Vec3], List[Tri]]:
    bottom, right, top, left = compute_lift(params)

    iu, iv = params.inner
    e_bottom, e_right, e_top, e_left = params.edge

    if left + right >= 0.999 or bottom + top >= 0.999:
        raise ValueError("lift too large, inner region collapsed")

    du = (1.0 - left - right) / iu if iu > 0 else 0.0
    dv = (1.0 - bottom - top) / iv if iv > 0 else 0.0

    verts: List[Vec3] = []
    triangles: List[Tri] = []
    vtx_index: Dict[Tuple[float, float], int] = {}

    def add_vtx(u: float, v: float) -> int:
        u = clamp(u, 0.0, 1.0)
        v = clamp(v, 0.0, 1.0)
        key = (round(u, 9), round(v, 9))
        idx = vtx_index.get(key)
        if idx is not None:
            return idx
        pos = bilinear(u, v, p00, p10, p01, p11)
        idx = len(verts)
        verts.append(pos)
        vtx_index[key] = idx
        return idx

    def prefer_high_for_I0() -> bool:
        return not params.i0_pref_lower

    def prefer_high_for_I1() -> bool:
        return params.i0_pref_lower

    # Inner grid
    if iu > 0 and iv > 0:
        for jv in range(iv):
            v0 = bottom + jv * dv
            v1 = bottom + (jv + 1) * dv
            for ju in range(iu):
                u0 = left + ju * du
                u1 = left + (ju + 1) * du
                a = add_vtx(u0, v0)
                b = add_vtx(u1, v0)
                c = add_vtx(u0, v1)
                d = add_vtx(u1, v1)
                triangles.append((a, b, c))
                triangles.append((b, d, c))

    # Bottom
    if e_bottom > 0 and iu > 0 and bottom > 0:
        edge_samples = generate_boundary_samples(e_bottom, 0.0, 1.0)
        p_cnt = max(0, len(edge_samples) - 1)
        q = iu
        inner_us = [left + j * du for j in range(q + 1)]
        for k in range(p_cnt):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - left) / du, 0, q, prefer_up=prefer_high_for_I0()) if du > 1e-12 else 0
            v0 = add_vtx(edge_samples[k], 0.0)
            v1 = add_vtx(edge_samples[k + 1], 0.0)
            v2 = add_vtx(inner_us[r], bottom)
            triangles.append((v0, v1, v2))
        for j in range(q):
            t_mid = left + (j + 0.5) * du
            k = nearest_index_with_tie(e_bottom * t_mid, 0, e_bottom, prefer_up=prefer_high_for_I1())
            t_k = clamp(k / e_bottom, 0.0, 1.0)
            v0 = add_vtx(inner_us[j + 1], bottom)
            v1 = add_vtx(inner_us[j], bottom)
            v2 = add_vtx(t_k, 0.0)
            triangles.append((v0, v1, v2))

    # Top
    if e_top > 0 and iu > 0 and top > 0:
        edge_samples = generate_boundary_samples(e_top, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iu
        inner_us = [left + j * du for j in range(q + 1)]
        # I0
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - left) / du, 0, q,
                                       prefer_up=prefer_high_for_I0()) if du > 1e-12 else 0
            v0 = add_vtx(edge_samples[k + 1], 1.0)
            v1 = add_vtx(edge_samples[k], 1.0)
            v2 = add_vtx(inner_us[r], 1.0 - top)
            triangles.append((v0, v1, v2))
        # I1
        for j in range(q):
            t_mid = left + (j + 0.5) * du
            k = nearest_index_with_tie(e_top * t_mid, 0, e_top, prefer_up=prefer_high_for_I1())
            t_k = clamp(k / e_top, 0.0, 1.0)
            v0 = add_vtx(inner_us[j], 1.0 - top)
            v1 = add_vtx(inner_us[j + 1], 1.0 - top)
            v2 = add_vtx(t_k, 1.0)
            triangles.append((v0, v1, v2))

    # Right
    if e_right > 0 and iv > 0 and right > 0:
        edge_samples = generate_boundary_samples(e_right, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iv
        inner_vs = [bottom + j * dv for j in range(q + 1)]
        # I0
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - bottom) / dv, 0, q,
                                       prefer_up=prefer_high_for_I0()) if dv > 1e-12 else 0
            v0 = add_vtx(1.0, edge_samples[k + 1])
            v1 = add_vtx(1.0, edge_samples[k])
            v2 = add_vtx(1.0 - right, inner_vs[r])
            triangles.append((v0, v1, v2))
        # I1
        for j in range(q):
            t_mid = bottom + (j + 0.5) * dv
            k = nearest_index_with_tie(e_right * t_mid, 0, e_right, prefer_up=prefer_high_for_I1())
            t_k = clamp(k / e_right, 0.0, 1.0)
            v0 = add_vtx(1.0 - right, inner_vs[j])
            v1 = add_vtx(1.0 - right, inner_vs[j + 1])
            v2 = add_vtx(1.0, t_k)
            triangles.append((v0, v1, v2))

    # Left
    if e_left > 0 and iv > 0 and left > 0:
        edge_samples = generate_boundary_samples(e_left, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iv
        inner_vs = [bottom + j * dv for j in range(q + 1)]
        # I0
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - bottom) / dv, 0, q,
                                       prefer_up=prefer_high_for_I0()) if dv > 1e-12 else 0
            v0 = add_vtx(0.0, edge_samples[k])
            v1 = add_vtx(0.0, edge_samples[k + 1])
            v2 = add_vtx(left, inner_vs[r])
            triangles.append((v0, v1, v2))
        # I1
        for j in range(q):
            t_mid = bottom + (j + 0.5) * dv
            k = nearest_index_with_tie(e_left * t_mid, 0, e_left, prefer_up=prefer_high_for_I1())
            t_k = clamp(k / e_left, 0.0, 1.0)
            v0 = add_vtx(left, inner_vs[j + 1])
            v1 = add_vtx(left, inner_vs[j])
            v2 = add_vtx(0.0, t_k)
            triangles.append((v0, v1, v2))

    return verts, triangles


@dataclass
class TessellatedMesh:
    vertices: np.ndarray  # (N, 3) float32
    indices: np.ndarray   # (M, 3) uint32


class DisplacementSampler:
    """Utility to sample displacement EXR using integer pixel-space UV (0..res-1).

    This assumes the EXR is a single-channel (R) float32 image of size res x res.
    """

    def __init__(self, exr_path: str, resolution: int):
        self.resolution = resolution
        self._load_exr(exr_path)

    def _load_exr(self, path: str):
        exr = OpenEXR.InputFile(path)
        header = exr.header()
        dw = header['dataWindow']
        W = dw.max.x - dw.min.x + 1
        H = dw.max.y - dw.min.y + 1
        if W != self.resolution or H != self.resolution:
            # Allow mismatch but warn; we still reshape according to header
            print(f"[Trainer] Warning: EXR size ({W},{H}) != resolution {self.resolution}")
            self.resolution = W
        pt = Imath.PixelType(Imath.PixelType.FLOAT)
        raw = exr.channel('R', pt)
        arr = np.frombuffer(raw, dtype=np.float32)
        self.disp = arr.reshape(H, W)
        exr.close()

    def sample(self, u_px: float, v_px: float) -> float:
        """Sample displacement at pixel-space (u_px, v_px) with nearest neighbor.

        u_px, v_px: in [0, res-1], can be float; they are clamped and rounded.
        """
        res = self.resolution
        x = int(round(clamp(u_px, 0.0, float(res - 1))))
        y = int(round(clamp(v_px, 0.0, float(res - 1))))
        return float(self.disp[y, x])


def tessellate_quad_with_displacement(
    p00: Vec3,
    p10: Vec3,
    p01: Vec3,
    p11: Vec3,
    uv00: Tuple[float, float],
    uv10: Tuple[float, float],
    uv01: Tuple[float, float],
    uv11: Tuple[float, float],
    params: TessParams,
    disp_sampler: DisplacementSampler,
    ray_dir_hint: Vec3 = (0.0, 0.0, 1.0),
) -> TessellatedMesh:
    """Tessellate a quad and apply displacement map sampling per vertex.

    - Input positions are the four quad corners in object space.
    - Input uvs are in *pixel* coordinates (0..res-1) for the same four corners.
    - We first call tessellate_quad to get topology in parametric (u,v) space,
      then for each vertex param (u,v) we bilinearly interpolate both position
      and uv, sample displacement from EXR, and offset the position along a
      chosen direction (by default a fixed ray_dir_hint).

    Returns a TessellatedMesh with displaced vertices and triangle indices.
    """
    base_verts, tris = tessellate_quad(p00, p10, p01, p11, params)
    p00_np = np.array(p00, dtype=np.float32)
    p10_np = np.array(p10, dtype=np.float32)
    p01_np = np.array(p01, dtype=np.float32)
    p11_np = np.array(p11, dtype=np.float32)

    def solve_uv_from_bilinear(pos: np.ndarray) -> Tuple[float, float]:
        best_u = 0.0
        best_v = 0.0
        best_err = 1e30
        steps = 10
        for iu in range(steps + 1):
            u = iu / steps
            for iv in range(steps + 1):
                v = iv / steps
                w00 = (1 - u) * (1 - v)
                w10 = u * (1 - v)
                w01 = (1 - u) * v
                w11 = u * v
                cand = (
                    w00 * p00_np +
                    w10 * p10_np +
                    w01 * p01_np +
                    w11 * p11_np
                )
                err = float(np.linalg.norm(cand - pos))
                if err < best_err:
                    best_err = err
                    best_u = u
                    best_v = v
        return best_u, best_v

    uv00_np = np.array(uv00, dtype=np.float32)
    uv10_np = np.array(uv10, dtype=np.float32)
    uv01_np = np.array(uv01, dtype=np.float32)
    uv11_np = np.array(uv11, dtype=np.float32)

    displaced_verts: List[np.ndarray] = []
    ray_dir = np.array(ray_dir_hint, dtype=np.float32)
    ray_dir /= (np.linalg.norm(ray_dir) + 1e-8)

    for vpos in base_verts:
        pos_np = np.array(vpos, dtype=np.float32)
        u, v = solve_uv_from_bilinear(pos_np)

        # 双线性插值像素 UV
        w00 = (1 - u) * (1 - v)
        w10 = u * (1 - v)
        w01 = (1 - u) * v
        w11 = u * v
        uv_px = (
            w00 * uv00_np +
            w10 * uv10_np +
            w01 * uv01_np +
            w11 * uv11_np
        )
        du = float(uv_px[0])
        dv = float(uv_px[1])

        disp = disp_sampler.sample(du, dv)
        displaced = pos_np + ray_dir * disp
        displaced_verts.append(displaced)

    vertices = np.stack(displaced_verts, axis=0).astype(np.float32)
    indices = np.array(tris, dtype=np.uint32)

    return TessellatedMesh(vertices=vertices, indices=indices)


@dataclass
class TrainerConfig:
    resolution: int
    max_rate: int = 4
    samples_per_dim: int = 10  # S


class Trainer:
    """Offline trainer to generate tessellation epsilon data.

    This class mirrors the style of Preprocessor: it loads baked and limited
    meshes, prepares rays, performs tessellation with displacement and writes
    a CSV of (quad_id, iu, iv, epsilon).
    """

    def __init__(self, input_mesh: str, config: TrainerConfig):
        self.input_mesh = input_mesh
        self.config = config
        base, ext = os.path.splitext(input_mesh)
        self.baked_mesh = f"{base}_baked{ext}"
        self.limited_mesh = f"{base}_baked_limited{ext}"
        self.baked_disp_exr = f"{base}_baked_disp.exr"

        logging.info(f"[Trainer] init input={self.input_mesh} res={config.resolution} max_rate={config.max_rate}")

    # --- public API ---

    def run(self):
        self._generate_training_data()

    def _generate_training_data(self):
        from TessDemo import TessParams

        logging.info("[Trainer] Generating training data (multithreaded per-quad, with displacement)")

        baked = self._parse_baked_mesh()
        num_quads = len(baked["quads"])
        logging.info(f"[Trainer] Parsed baked mesh, quads={num_quads}")

        # Limited mesh as reference surface
        ms = pymeshlab.MeshSet()
        if not os.path.isfile(self.limited_mesh):
            logging.error(f"[Trainer] Limited mesh not found: {self.limited_mesh}")
            return
        ms.load_new_mesh(self.limited_mesh)
        m = ms.current_mesh()
        limited_vertices = np.array(m.vertex_matrix(), dtype=np.float32)
        limited_indices = np.array(m.face_matrix(), dtype=np.uint32)
        logging.info(f"[Trainer] Loaded limited mesh, verts={limited_vertices.shape[0]}, faces={limited_indices.shape[0]}")

        # Displacement sampler over baked_disp_exr
        if not os.path.isfile(self.baked_disp_exr):
            logging.error(f"[Trainer] Displacement EXR not found: {self.baked_disp_exr}")
            return
        disp_sampler = DisplacementSampler(self.baked_disp_exr, self.config.resolution)

        S = self.config.samples_per_dim
        sample_us = np.linspace(0.0, 1.0, S, dtype=np.float32)
        sample_vs = np.linspace(0.0, 1.0, S, dtype=np.float32)

        base, _ = os.path.splitext(self.input_mesh)
        max_rate = self.config.max_rate
        csv_path = f"{base}_training_{max_rate}.csv"
        logging.info(f"[Trainer] Writing training data to {csv_path}, max_rate={max_rate}, samples_per_quad={S*S}")

        def bilinear_point_normal(u, v, p, n):
            w00 = (1 - u) * (1 - v)
            w10 = u * (1 - v)
            w01 = u * v
            w11 = (1 - u) * v
            pos = (
                w00 * p[0] +
                w10 * p[1] +
                w01 * p[2] +
                w11 * p[3]
            )
            nor = (
                w00 * n[0] +
                w10 * n[1] +
                w01 * n[2] +
                w11 * n[3]
            )
            nor = nor / (np.linalg.norm(nor) + 1e-8)
            return pos, nor

        def intersect_rays_with_mesh(origins, dirs, vertices, indices, optix):
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

        # We need optix; let Preprocessor create it and pass in, or import here.
        from Preprocessor import Preprocessor  # avoid circular at module top
        optix = Preprocessor(self.input_mesh, self.config.resolution)._import_optixbaker()

        def process_single_quad(qid: int) -> List[Tuple[int, int, int, float]]:
            face = baked["quads"][qid]
            p = [np.array(baked["vertices"][i], dtype=np.float32) for i in face.verts]
            n = [np.array(baked["normals"][i], dtype=np.float32) for i in face.norms]
            uv_px = [np.array(baked["uvs"][i][:2], dtype=np.float32) for i in face.uvs]

            num_rays = S * S
            origins = np.zeros((num_rays, 3), dtype=np.float32)
            dirs = np.zeros((num_rays, 3), dtype=np.float32)

            idx = 0
            for vi in range(S):
                v = float(sample_vs[vi])
                for ui in range(S):
                    u = float(sample_us[ui])
                    pos, nor = bilinear_point_normal(u, v, p, n)
                    origins[idx] = pos - 1e-6 * nor
                    dirs[idx] = nor
                    idx += 1

            t1 = intersect_rays_with_mesh(origins, dirs, limited_vertices, limited_indices, optix)

            rows: List[Tuple[int, int, int, float]] = []
            for iu in range(1, max_rate + 1):
                for iv in range(1, max_rate + 1):
                    params = TessParams(edge=(iu, iu, iv, iv), inner=(iu, iv))
                    mesh = tessellate_quad_with_displacement(
                        tuple(p[0]), tuple(p[1]), tuple(p[2]), tuple(p[3]),
                        tuple(uv_px[0]), tuple(uv_px[1]), tuple(uv_px[2]), tuple(uv_px[3]),
                        params,
                        disp_sampler,
                        ray_dir_hint=(0.0, 0.0, 1.0),
                    )

                    t2 = intersect_rays_with_mesh(origins, dirs, mesh.vertices, mesh.indices, optix)

                    mask = (t1 > 0) & (t2 > 0)
                    if not np.any(mask):
                        epsilon = -1.0
                    else:
                        epsilon = float(np.max(np.abs(t1[mask] - t2[mask])))

                    rows.append((qid, iu, iv, epsilon))

            return rows

        max_workers = min(8, os.cpu_count() or 4)
        logging.info(f"[Trainer] Using ThreadPoolExecutor with max_workers={max_workers}")

        with open(csv_path, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["quad_id", "sample_rate_u", "sample_rate_v", "epsilon"])

            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                futures = {executor.submit(process_single_quad, qid): qid for qid in range(num_quads)}
                completed = 0
                for future in as_completed(futures):
                    qid = futures[future]
                    try:
                        rows = future.result()
                    except Exception as e:
                        logging.exception(f"[Trainer] Exception while processing quad {qid}: {e}")
                        continue

                    writer.writerows(rows)

                    completed += 1
                    logging.info(f"[Trainer] Completed {completed}/{num_quads} quads")

        logging.info("[Trainer] Training data generation finished")

    # --- helpers ---

    def _parse_baked_mesh(self):
        from Preprocessor import Preprocessor  # reuse its parser
        # Instantiate a temporary Preprocessor just to call its _parse_baked_mesh
        tmp = Preprocessor(self.input_mesh, self.config.resolution)
        return tmp._parse_baked_mesh()


if __name__ == '__main__':
    # Console logging at INFO level
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s',
        handlers=[logging.StreamHandler(sys.stdout)]
    )

    input_mesh = "assets/Icosphere.obj"
    resolution = 1024

    cfg = TrainerConfig(resolution=resolution, max_rate=16, samples_per_dim=32)
    trainer = Trainer(input_mesh, cfg)
    trainer.run()
