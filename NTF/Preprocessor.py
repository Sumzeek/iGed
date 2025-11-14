import os
import sys
import math
import logging
from dataclasses import dataclass
from typing import List, Tuple, Optional

import numpy as np
import pymeshlab
import OpenEXR
import Imath


@dataclass
class QuadFace:
    verts: Tuple[int, int, int, int]
    norms: Tuple[int, int, int, int]
    uvs: Tuple[int, int, int, int]


class Preprocessor:
    """
    Mesh preprocessing:
      1. Load and decimate original mesh.
      2. Convert to quads and assign tiled UV atlas.
      3. Bake ray intersections to displacement + normal maps.
      4. Reconstruct limited mesh from per-pixel data.
    """
    EPSILON = 1e-6

    def __init__(self, input_mesh: str, resolution: int):
        self.input_mesh = input_mesh
        self.resolution = resolution

        base, ext = os.path.splitext(input_mesh)
        self.baked_mesh = f"{base}_baked{ext}"
        self.limited_mesh = f"{base}_baked_limited{ext}"
        self.baked_disp_exr = f"{base}_baked_disp.exr"
        self.baked_norm_exr = f"{base}_baked_norm.exr"

        self._optix = self._import_optixbaker()

        # Optional pre-built PTX path
        ptx_default = os.path.join("OptixBaker", "assets", "optix_kernel.ptx")
        if os.path.isfile(ptx_default):
            os.environ["OPTIX_INTERSECT_PTX"] = ptx_default

        logging.info(f"[Init] input={self.input_mesh} resolution={self.resolution}")

    # ---------- Public API ----------

    def run(self):
        logging.info("[Run] Start preprocessing pipeline")
        self._quadrangulate_and_tile()
        baked_data = self._parse_baked_mesh()
        original = self._load_original_mesh()
        disp, norms, origins, dirs, quads = self._bake_surface(original, baked_data)
        self._save_exr_displacement(disp)
        self._save_exr_normal(norms)
        self._reconstruct_limited_mesh(disp, origins, dirs, quads, baked_data["vertices"])
        logging.info("[Run] Completed all stages")

    # ---------- Stage 1: Quadrangulate + UV Atlas ----------

    def _quadrangulate_and_tile(self):
        logging.info("[Stage 1] Load and decimate mesh")
        ms = pymeshlab.MeshSet()
        ms.load_new_mesh(self.input_mesh)

        original_faces = ms.current_mesh().face_number()
        target_faces = max(1, original_faces // 100)
        logging.info(f"[Stage 1] Original faces={original_faces} target={target_faces}")

        ms.meshing_decimation_quadric_edge_collapse(targetfacenum=target_faces, qualitythr=1.0)
        ms.meshing_repair_non_manifold_edges()
        ms.meshing_tri_to_quad_by_smart_triangle_pairing()

        ms.save_current_mesh(
            self.baked_mesh,
            save_vertex_color=False,
            save_vertex_normal=True,
            save_wedge_texcoord=False,
            save_face_color=False
        )
        logging.info(f"[Stage 1] Saved intermediate quad mesh={self.baked_mesh}")

        # Parse quad mesh to extract topology
        vertices, normals, quads_raw = self._read_obj_quads(self.baked_mesh)
        if not quads_raw:
            logging.warning("[Stage 1] No quads found; abort tiling")
            return

        grid = max(1, int(math.ceil(math.sqrt(len(quads_raw)))))
        tile_size = max(1, self.resolution // grid)
        logging.info(f"[Stage 1] Atlas grid={grid} tile_size={tile_size}")

        # Rewrite OBJ with tiled UVs
        with open(self.baked_mesh, "w") as f:
            f.write("# Quad UV tiled atlas\n")
            v_counter = 0
            for qi, (v_idx, n_idx) in enumerate(quads_raw):
                col = qi % grid
                row = qi // grid
                u0 = col * tile_size
                v0 = row * tile_size
                u1 = min(self.resolution - 1, (col + 1) * tile_size - 1)
                v1 = min(self.resolution - 1, (row + 1) * tile_size - 1)
                quad_uvs = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]

                for k, (vi, ni) in enumerate(zip(v_idx, n_idx)):
                    vx, vy, vz = vertices[vi]
                    nx, ny, nz = normals[ni]
                    u, v = quad_uvs[k]
                    f.write(f"v {vx} {vy} {vz}\n")
                    f.write(f"vt {u} {v}\n")
                    f.write(f"vn {nx} {ny} {nz}\n")

                f.write(
                    f"f {v_counter + 1}/{v_counter + 1}/{v_counter + 1} "
                    f"{v_counter + 2}/{v_counter + 2}/{v_counter + 2} "
                    f"{v_counter + 3}/{v_counter + 3}/{v_counter + 3} "
                    f"{v_counter + 4}/{v_counter + 4}/{v_counter + 4}\n"
                )
                v_counter += 4
        logging.info(f"[Stage 1] UV atlas written to {self.baked_mesh}")

    # ---------- Stage 2: Baking ----------

    def _bake_surface(self, original, baked):
        logging.info("[Stage 2] Baking displacement + normals")
        res = self.resolution
        origins = np.zeros((res * res, 3), dtype=np.float32)
        directions = np.zeros((res * res, 3), dtype=np.float32)

        # Generate per-pixel rays
        for face in baked["quads"]:
            p = [np.array(baked["vertices"][i]) for i in face.verts]
            n = [np.array(baked["normals"][i]) for i in face.norms]
            uv = [np.array(baked["uvs"][i][:2]) for i in face.uvs]

            px = [u.astype(int) for u in uv]
            xs = [v[0] for v in px]
            ys = [v[1] for v in px]
            min_x = max(0, min(xs))
            max_x = min(res - 1, max(xs))
            min_y = max(0, min(ys))
            max_y = min(res - 1, max(ys))

            for y in range(min_y, max_y + 1):
                for x in range(min_x, max_x + 1):
                    u = (x - min_x) / max(1, (max_x - min_x))
                    v = (y - min_y) / max(1, (max_y - min_y))
                    pos = (
                        (1 - u) * (1 - v) * p[0] +
                        u * (1 - v) * p[1] +
                        u * v * p[2] +
                        (1 - u) * v * p[3]
                    )
                    nor = (
                        (1 - u) * (1 - v) * n[0] +
                        u * (1 - v) * n[1] +
                        u * v * n[2] +
                        (1 - u) * v * n[3]
                    )
                    nor /= (np.linalg.norm(nor) + 1e-8)
                    idx = y * res + x
                    origins[idx] = pos - self.EPSILON * nor
                    directions[idx] = nor

        logging.info("[Stage 2] Ray grid prepared; launching intersection")
        hits = self._optix.intersect(
            origins, directions,
            original["vertices"], original["indices"]
        )

        H = W = res
        displacements = np.zeros(H * W, dtype=np.float32)
        out_normals = np.zeros((H * W, 3), dtype=np.float32)

        verts = original["vertices"]
        norms = original["normals"]
        faces = original["indices"]

        for i in range(H * W):
            tri_id = hits[i]
            if tri_id < 0:
                continue
            i0, i1, i2 = faces[tri_id]
            v0, v1, v2 = verts[i0], verts[i1], verts[i2]
            n0, n1, n2 = norms[i0], norms[i1], norms[i2]

            ori = origins[i]
            dir_ = directions[i]

            # Moller-Trumbore
            e1 = v1 - v0
            e2 = v2 - v0
            pvec = np.cross(dir_, e2)
            det = np.dot(e1, pvec)
            if abs(det) < 1e-8:
                continue
            inv = 1.0 / det
            tvec = ori - v0
            u = np.dot(tvec, pvec) * inv
            qvec = np.cross(tvec, e1)
            v = np.dot(dir_, qvec) * inv
            t = np.dot(e2, qvec) * inv
            w0 = max(0.0, 1.0 - u - v)
            w1 = max(0.0, u)
            w2 = max(0.0, v)

            displacements[i] = t
            n_interp = w0 * n0 + w1 * n1 + w2 * n2
            n_interp /= (np.linalg.norm(n_interp) + 1e-8)
            out_normals[i] = n_interp

        logging.info("[Stage 2] Baking done")
        return displacements, out_normals, origins, directions, baked["quads"]

    # ---------- Stage 3: Limited Mesh Reconstruction ----------

    def _reconstruct_limited_mesh(self, disp, origins, dirs, quads, baked_vertices):
        logging.info("[Stage 3] Reconstruct limited mesh")
        res = self.resolution
        ms = pymeshlab.MeshSet()
        limited_vertices: List[np.ndarray] = []
        limited_indices: List[int] = []

        def add_vertex(pos: np.ndarray) -> int:
            limited_vertices.append(pos)
            return len(limited_vertices) - 1

        for face in quads:
            uv = [np.array(self._uv_cache[i][:2]) for i in face.uvs]
            px = [u.astype(int) for u in uv]
            xs = [v[0] for v in px]
            ys = [v[1] for v in px]
            min_x = max(0, min(xs))
            max_x = min(res - 1, max(xs))
            min_y = max(0, min(ys))
            max_y = min(res - 1, max(ys))

            gw = max_x - min_x
            gh = max_y - min_y
            grid = np.full((gh + 1, gw + 1), -1, dtype=int)

            for gy in range(gh + 1):
                for gx in range(gw + 1):
                    x = min_x + gx
                    y = min_y + gy
                    idx = y * res + x
                    d = disp[idx]
                    if d <= 0:
                        continue
                    pos = origins[idx] + dirs[idx] * d
                    grid[gy, gx] = add_vertex(pos)

            for cy in range(gh):
                for cx in range(gw):
                    v00 = grid[cy, cx]
                    v10 = grid[cy, cx + 1]
                    v01 = grid[cy + 1, cx]
                    v11 = grid[cy + 1, cx + 1]
                    if -1 in (v00, v10, v01, v11):
                        continue
                    # Two triangles
                    limited_indices.extend([v00, v10, v11])
                    limited_indices.extend([v00, v11, v01])

        if not limited_vertices:
            logging.warning("[Stage 3] No vertices reconstructed")
            return

        mesh = pymeshlab.Mesh(
            vertex_matrix=np.array(limited_vertices, dtype=np.float32),
            face_matrix=np.array(limited_indices, dtype=np.uint32).reshape(-1, 3)
        )
        ms.add_mesh(mesh, "limited_mesh")
        ms.save_current_mesh(self.limited_mesh, save_face_color=False, save_vertex_color=False)
        logging.info(f"[Stage 3] Saved limited mesh={self.limited_mesh} verts={len(limited_vertices)}")

    # ---------- EXR Writers ----------

    def _save_exr_displacement(self, displacement: np.ndarray):
        logging.info(f"[EXR] Writing displacement {self.baked_disp_exr}")
        H = W = self.resolution
        if displacement.ndim == 2:
            arr = displacement.astype(np.float32)
        else:
            arr = displacement.reshape(H, W).astype(np.float32)
        header = OpenEXR.Header(W, H)
        header["channels"] = {"R": Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT))}
        exr = OpenEXR.OutputFile(self.baked_disp_exr, header)
        exr.writePixels({"R": arr.tobytes()})
        exr.close()

    def _save_exr_normal(self, normals: np.ndarray):
        logging.info(f"[EXR] Writing normals {self.baked_norm_exr}")
        H = W = self.resolution
        if normals.ndim == 2:
            normals = normals.reshape(H, W, 3)
        normals = normals.astype(np.float32)
        header = OpenEXR.Header(W, H)
        header["channels"] = {
            "R": Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
            "G": Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
            "B": Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
        }
        exr = OpenEXR.OutputFile(self.baked_norm_exr, header)
        exr.writePixels({
            "R": normals[:, :, 0].tobytes(),
            "G": normals[:, :, 1].tobytes(),
            "B": normals[:, :, 2].tobytes(),
        })
        exr.close()

    # ---------- Helpers ----------

    def _import_optixbaker(self):
        logging.info("[Import] Attempting to import optixbaker")
        assets_path = os.path.join(os.path.dirname(__file__), "OptixBaker", "assets")
        if os.path.isdir(assets_path):
            if assets_path not in sys.path:
                sys.path.insert(0, assets_path)
        try:
            import optixbaker
            logging.info("[Import] optixbaker loaded successfully")
            return optixbaker
        except ImportError as e:
            logging.error("[Import] Failed to import optixbaker. Ensure build or install.\n"
                          "Expected path: OptixBaker/assets")
            raise

    def _load_original_mesh(self):
        ms = pymeshlab.MeshSet()
        ms.load_new_mesh(self.input_mesh)
        m = ms.current_mesh()
        data = {
            "vertices": np.array(m.vertex_matrix(), dtype=np.float32),
            "normals": np.array(m.vertex_normal_matrix(), dtype=np.float32),
            "indices": np.array(m.face_matrix(), dtype=np.uint32)
        }
        logging.info(f"[Original] Loaded vertices={data['vertices'].shape[0]} faces={data['indices'].shape[0]}")
        return data

    def _parse_baked_mesh(self):
        logging.info(f"[Parse] Reading baked mesh {self.baked_mesh}")
        verts = []
        norms = []
        uvs = []
        quads: List[QuadFace] = []
        face_specs = []

        with open(self.baked_mesh, "r") as f:
            for line in f:
                if line.startswith("v "):
                    _, x, y, z = line.strip().split()[:4]
                    verts.append([float(x), float(y), float(z)])
                elif line.startswith("vn "):
                    _, x, y, z = line.strip().split()[:4]
                    norms.append([float(x), float(y), float(z)])
                elif line.startswith("vt "):
                    parts = line.strip().split()
                    # Allow 2 or 3 components (ignore w if present)
                    u = float(parts[1])
                    v = float(parts[2]) if len(parts) > 2 else 0.0
                    uvs.append([u, v])
                elif line.startswith("f "):
                    parts = line.strip().split()[1:]
                    if len(parts) != 4:
                        continue
                    fs = []
                    for p in parts:
                        v_idx, t_idx, n_idx = p.split("/")
                        fs.append((int(v_idx) - 1, int(t_idx) - 1, int(n_idx) - 1))
                    face_specs.append(fs)

        # Build QuadFace objects
        for spec in face_specs:
            v_i = tuple(s[0] for s in spec)
            t_i = tuple(s[1] for s in spec)
            n_i = tuple(s[2] for s in spec)
            quads.append(QuadFace(v_i, n_i, t_i))

        self._uv_cache = uvs  # for later reconstruction
        logging.info(f"[Parse] verts={len(verts)} norms={len(norms)} uvs={len(uvs)} quads={len(quads)}")
        return {"vertices": verts, "normals": norms, "uvs": uvs, "quads": quads}

    def _read_obj_quads(self, path: str):
        verts = []
        norms = []
        quads = []
        with open(path, "r") as f:
            for line in f:
                if line.startswith("v "):
                    _, x, y, z = line.strip().split()[:4]
                    verts.append([float(x), float(y), float(z)])
                elif line.startswith("vn "):
                    _, x, y, z = line.strip().split()[:4]
                    norms.append([float(x), float(y), float(z)])
                elif line.startswith("f "):
                    parts = line.strip().split()[1:]
                    if len(parts) != 4:
                        continue
                    v_idx = []
                    n_idx = []
                    for p in parts:
                        if "//" in p:  # v//vn
                            v, n = p.split("//")
                            v_idx.append(int(v) - 1)
                            n_idx.append(int(n) - 1)
                        else:
                            vals = p.split("/")
                            if len(vals) >= 3:
                                v_idx.append(int(vals[0]) - 1)
                                n_idx.append(int(vals[2]) - 1)
                    if len(v_idx) == 4 and len(n_idx) == 4:
                        quads.append((tuple(v_idx), tuple(n_idx)))
        return verts, norms, quads


if __name__ == '__main__':
    # Console logging at INFO level
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s',
        handlers=[logging.StreamHandler(sys.stdout)]
    )

    input_mesh = "Icosphere.obj"
    resolution = 1024

    preprocessor = Preprocessor(input_mesh, resolution)
    preprocessor.run()
