import os
import sys
import logging
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from typing import List, Tuple
import csv

import numpy as np
import pymeshlab

from Tessellator import DisplacementSampler, QuadTessellator, QuadTessParams, Vertex


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
        logging.info(
            f"[Trainer] Loaded limited mesh, verts={limited_vertices.shape[0]}, faces={limited_indices.shape[0]}")

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
        logging.info(f"[Trainer] Writing training data to {csv_path}, max_rate={max_rate}, samples_per_quad={S * S}")

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
                    # Create quad vertices with position, normal, and UV
                    quad = [
                        Vertex(position=tuple(p[0]), normal=tuple(n[0]), uv=tuple(uv_px[0])),
                        Vertex(position=tuple(p[1]), normal=tuple(n[1]), uv=tuple(uv_px[1])),
                        Vertex(position=tuple(p[2]), normal=tuple(n[2]), uv=tuple(uv_px[2])),
                        Vertex(position=tuple(p[3]), normal=tuple(n[3]), uv=tuple(uv_px[3])),
                    ]

                    # Create tessellation params with displacement sampler
                    params = QuadTessParams(
                        edge=(iu, iu, iv, iv),
                        inner=(iu, iv),
                        disp_sampler=disp_sampler
                    )

                    # Tessellate the quad
                    tessellator = QuadTessellator(params)
                    verts, tris = tessellator.tessellate(quad)

                    # Convert vertices to numpy arrays for ray intersection
                    mesh_vertices = np.array([v.position for v in verts], dtype=np.float32)
                    mesh_indices = np.array(tris, dtype=np.uint32)

                    t2 = intersect_rays_with_mesh(origins, dirs, mesh_vertices, mesh_indices, optix)

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
