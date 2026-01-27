import math
from typing import List, Tuple, Optional, Dict
from dataclasses import dataclass

import OpenEXR
import Imath
import numpy as np


class DisplacementSampler:
    def __init__(self, exr_path: str, resolution: int):
        self.resolution = resolution
        self._load_exr(exr_path)

    def sample(self, u_px: float, v_px: float) -> float:
        """
        Sample displacement value using bilinear interpolation.

        Args:
            u_px: U coordinate in pixel space
            v_px: V coordinate in pixel space

        Returns:
            Interpolated displacement value
        """
        # Compute integer pixel coordinates and fractional offsets
        base_u = int(np.floor(u_px))
        base_v = int(np.floor(v_px))
        f_u = u_px - base_u
        f_v = v_px - base_v

        # Clamp coordinates to valid range
        H, W = self.disp.shape
        base_u = np.clip(base_u, 0, W - 1)
        base_v = np.clip(base_v, 0, H - 1)
        u1 = min(base_u + 1, W - 1)
        v1 = min(base_v + 1, H - 1)

        # Sample displacement values from the four neighboring texels
        s00 = self.disp[base_v, base_u]
        s10 = self.disp[base_v, u1]
        s01 = self.disp[v1, base_u]
        s11 = self.disp[v1, u1]

        # Perform bilinear interpolation
        sx0 = s00 + (s10 - s00) * f_u  # lerp(s00, s10, f_u)
        sx1 = s01 + (s11 - s01) * f_u  # lerp(s01, s11, f_u)
        displacement = sx0 + (sx1 - sx0) * f_v  # lerp(sx0, sx1, f_v)

        return float(displacement)

    def sample_batch(self, u_px: np.ndarray, v_px: np.ndarray) -> np.ndarray:
        """
        Vectorized batch sampling of displacement values using bilinear interpolation.

        Args:
            u_px: U coordinates in pixel space, shape (N,)
            v_px: V coordinates in pixel space, shape (N,)

        Returns:
            Interpolated displacement values, shape (N,)
        """
        H, W = self.disp.shape

        # Compute integer pixel coordinates and fractional offsets
        base_u = np.floor(u_px).astype(np.int32)
        base_v = np.floor(v_px).astype(np.int32)
        f_u = u_px - base_u
        f_v = v_px - base_v

        # Clamp coordinates to valid range
        base_u = np.clip(base_u, 0, W - 1)
        base_v = np.clip(base_v, 0, H - 1)
        u1 = np.minimum(base_u + 1, W - 1)
        v1 = np.minimum(base_v + 1, H - 1)

        # Sample displacement values from the four neighboring texels
        s00 = self.disp[base_v, base_u]
        s10 = self.disp[base_v, u1]
        s01 = self.disp[v1, base_u]
        s11 = self.disp[v1, u1]

        # Perform bilinear interpolation
        sx0 = s00 + (s10 - s00) * f_u
        sx1 = s01 + (s11 - s01) * f_u
        displacement = sx0 + (sx1 - sx0) * f_v

        return displacement

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


@dataclass
class Vertex:
    position: Tuple[float, float, float]
    normal: Tuple[float, float, float]
    uv: Tuple[float, float]


Triangle = Tuple[int, int, int]


@dataclass
class QuadTessParams:
    edge: Tuple[int, int, int, int]
    inner: Tuple[int, int]
    lift: Optional[Tuple[float, float, float, float]] = None
    disp_sampler: Optional[DisplacementSampler] = None


class QuadTessellator:
    def __init__(self, params: QuadTessParams):
        # compute lift values
        if params.lift is None:
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

            # # 1. 提取基础参数 (Segments Count)
            # iu, iv = params.inner
            # eb, er, et, el = params.edge
            #
            # # 2. 统计各区域三角形数量 (Triangle Counts)
            # # 中间矩形区 (每个格子2个三角形)
            # tris_center = 2 * iu * iv
            #
            # # 垂直方向的梯形带 (下梯形 + 上梯形)
            # # 下梯形三角形数 = u方向段数 + 增加的边数
            # tris_v_band = (iu + eb) + (iu + et)
            #
            # # 水平方向的梯形带 (右梯形 + 左梯形)
            # tris_h_band = (iv + er) + (iv + el)
            #
            # # 3. 计算总数与目标几何特征
            # total_tris = tris_center + tris_v_band + tris_h_band
            #
            # # [关键点] 目标内部矩形的面积 = 内部三角形数 / 总三角形数
            # # 因为总面积是1，且要求单位三角形面积密度一致
            # target_inner_area = tris_center / total_tris
            #
            # # 宽与高的差值 (Width - Height)
            # # 由 (垂直带总数 - 水平带总数) / 总数 决定
            # diff_wh = (tris_v_band - tris_h_band) / total_tris
            #
            # # 4. 解方程求内部矩形尺寸 (Solving quadratic equation)
            # # h^2 + diff * h - area = 0
            # inner_h = (-diff_wh + math.sqrt(diff_wh ** 2 + 4 * target_inner_area)) / 2
            # inner_w = inner_h + diff_wh
            #
            # # 5. 计算四周留白 (Margins/Lifts)
            # # 垂直剩余空间分配给下、上
            # margin_v = 1.0 - inner_h
            # # 分母即为垂直带的总三角形数 (tris_v_band)
            # val_x = margin_v * (iu + eb) / tris_v_band  # Bottom
            # val_z = margin_v * (iu + et) / tris_v_band  # Top
            #
            # # 水平剩余空间分配给右、左
            # margin_h = 1.0 - inner_w
            # # 分母即为水平带的总三角形数 (tris_h_band)
            # val_y = margin_h * (iv + er) / tris_h_band  # Right
            # val_w = margin_h * (iv + el) / tris_h_band  # Left
            #
            # params.lift = (val_x, val_y, val_z, val_w)

        self.params = params

    def tessellate(self, quad) -> Tuple[List[Vertex], List[Triangle]]:
        e_bottom, e_right, e_top, e_left = self.params.edge
        iu, iv = self.params.inner
        bottom, right, top, left = self.params.lift

        du = (1.0 - left - right) / iu if iu > 0 else 0.0
        dv = (1.0 - bottom - top) / iv if iv > 0 else 0.0

        verts: List[Vertex] = []
        tris: List[Triangle] = []

        vtx_index: Dict[Tuple[float, float], int] = {}

        def add_vtx(u: float, v: float) -> int:
            """插入(u,v)对应的三维点，返回顶点索引；带 1e-9 舍入以归并数值误差。"""
            u = self._clamp(u, 0.0, 1.0)
            v = self._clamp(v, 0.0, 1.0)
            key = (round(u, 9), round(v, 9))
            idx = vtx_index.get(key)

            if idx is not None:
                return idx
            vertex = self._bilinear(quad, u, v)

            if self.params.disp_sampler is not None:
                disp_sampler = self.params.disp_sampler
                displacement = disp_sampler.sample(vertex.uv[0], vertex.uv[1])
                pos = tuple(
                    vertex.position[i] + displacement * vertex.normal[i]
                    for i in range(3)
                )
                vertex.position = pos

            idx = len(verts)
            verts.append(vertex)
            vtx_index[key] = idx
            return idx

        # inner
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
                    tris.append((a, b, c))
                    tris.append((b, d, c))

        # Bottom: v ∈ [0, bottom]
        if e_bottom > 0 and iu > 0 and bottom > 0:
            edge_samples = QuadTessellator._generate_boundary_samples(e_bottom, 0.0, 1.0)
            p = max(0, len(edge_samples) - 1)
            q = iu
            inner_us = [left + j * du for j in range(q + 1)]
            # I0: 以边为底边，顶点在内线
            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = QuadTessellator._nearest_index_with_tie((t_mid - left) / du, 0, q,
                                                            True) if du > 1e-12 else 0
                v0 = add_vtx(edge_samples[k], 0.0)
                v1 = add_vtx(edge_samples[k + 1], 0.0)
                v2 = add_vtx(inner_us[r], bottom)
                tris.append((v0, v1, v2))
            # I1: 以内线为底边，顶点在边
            for j in range(q):
                t_mid = left + (j + 0.5) * du
                k = QuadTessellator._nearest_index_with_tie(e_bottom * t_mid, 0, e_bottom, False)
                t_k = QuadTessellator._clamp(k / e_bottom, 0.0, 1.0)
                v0 = add_vtx(inner_us[j + 1], bottom)
                v1 = add_vtx(inner_us[j], bottom)
                v2 = add_vtx(t_k, 0.0)
                tris.append((v0, v1, v2))

        # Top
        if e_top > 0 and iu > 0 and top > 0:
            edge_samples = QuadTessellator._generate_boundary_samples(e_top, 0.0, 1.0)
            p = max(0, len(edge_samples) - 1)
            q = iu
            inner_us = [left + j * du for j in range(q + 1)]
            # I0
            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = QuadTessellator._nearest_index_with_tie((t_mid - left) / du, 0, q,
                                                            True) if du > 1e-12 else 0
                v0 = add_vtx(edge_samples[k + 1], 1.0)
                v1 = add_vtx(edge_samples[k], 1.0)
                v2 = add_vtx(inner_us[r], 1.0 - top)
                tris.append((v0, v1, v2))
            # I1
            for j in range(q):
                t_mid = left + (j + 0.5) * du
                k = QuadTessellator._nearest_index_with_tie(e_top * t_mid, 0, e_top, False)
                t_k = QuadTessellator._clamp(k / e_top, 0.0, 1.0)
                v0 = add_vtx(inner_us[j], 1.0 - top)
                v1 = add_vtx(inner_us[j + 1], 1.0 - top)
                v2 = add_vtx(t_k, 1.0)
                tris.append((v0, v1, v2))

        # Right
        if e_right > 0 and iv > 0 and right > 0:
            edge_samples = QuadTessellator._generate_boundary_samples(e_right, 0.0, 1.0)
            p = max(0, len(edge_samples) - 1)
            q = iv
            inner_vs = [bottom + j * dv for j in range(q + 1)]
            # I0
            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = QuadTessellator._nearest_index_with_tie((t_mid - bottom) / dv, 0, q,
                                                            True) if dv > 1e-12 else 0
                v0 = add_vtx(1.0, edge_samples[k + 1])
                v1 = add_vtx(1.0, edge_samples[k])
                v2 = add_vtx(1.0 - right, inner_vs[r])
                tris.append((v0, v1, v2))
            # I1
            for j in range(q):
                t_mid = bottom + (j + 0.5) * dv
                k = QuadTessellator._nearest_index_with_tie(e_right * t_mid, 0, e_right, False)
                t_k = QuadTessellator._clamp(k / e_right, 0.0, 1.0)
                v0 = add_vtx(1.0 - right, inner_vs[j])
                v1 = add_vtx(1.0 - right, inner_vs[j + 1])
                v2 = add_vtx(1.0, t_k)
                tris.append((v0, v1, v2))

        # Left
        if e_left > 0 and iv > 0 and left > 0:
            edge_samples = QuadTessellator._generate_boundary_samples(e_left, 0.0, 1.0)
            p = max(0, len(edge_samples) - 1)
            q = iv
            inner_vs = [bottom + j * dv for j in range(q + 1)]
            # I0
            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = QuadTessellator._nearest_index_with_tie((t_mid - bottom) / dv, 0, q,
                                                            True) if dv > 1e-12 else 0
                v0 = add_vtx(0.0, edge_samples[k])
                v1 = add_vtx(0.0, edge_samples[k + 1])
                v2 = add_vtx(left, inner_vs[r])
                tris.append((v0, v1, v2))
            # I1
            for j in range(q):
                t_mid = bottom + (j + 0.5) * dv
                k = QuadTessellator._nearest_index_with_tie(e_left * t_mid, 0, e_left, False)
                t_k = QuadTessellator._clamp(k / e_left, 0.0, 1.0)
                v0 = add_vtx(left, inner_vs[j + 1])
                v1 = add_vtx(left, inner_vs[j])
                v2 = add_vtx(0.0, t_k)
                tris.append((v0, v1, v2))

        return verts, tris

    @staticmethod
    def _clamp(x: float, lo: float, hi: float) -> float:
        return max(lo, min(hi, x))

    @staticmethod
    def _lerp(a, b, t):
        return tuple(ai + (bi - ai) * t for ai, bi in zip(a, b))

    @staticmethod
    def _normalize(v):
        length = math.sqrt(sum(x * x for x in v))
        if length == 0:
            return v
        return tuple(x / length for x in v)

    @staticmethod
    def _bilinear(quad, u: float, v: float) -> Vertex:
        # Quad vertex order: v0→v1→v2→v3→v0 forms a quad
        # v0=(0,0), v1=(1,0), v2=(1,1), v3=(0,1)
        v0, v1, v2, v3 = quad

        w00 = (1 - u) * (1 - v)  # v0: (0,0)
        w10 = u * (1 - v)  # v1: (1,0)
        w11 = u * v  # v2: (1,1)
        w01 = (1 - u) * v  # v3: (0,1)

        def wsum(a0, a1, a2, a3):
            return tuple(
                a0[i] * w00 +
                a1[i] * w10 +
                a2[i] * w11 +
                a3[i] * w01
                for i in range(len(a0))
            )

        pos = wsum(v0.position, v1.position, v2.position, v3.position)
        normal = wsum(v0.normal, v1.normal, v2.normal, v3.normal)
        normal = QuadTessellator._normalize(normal)
        uv = wsum(v0.uv, v1.uv, v2.uv, v3.uv)

        return Vertex(pos, normal, uv)

    @staticmethod
    def _generate_boundary_samples(rate: int, t_min: float, t_max: float) -> List[float]:
        if rate <= 0:
            return []
        k0 = math.ceil(rate * t_min - 1e-9)
        k1 = math.floor(rate * t_max + 1e-9)
        k0 = int(QuadTessellator._clamp(k0, 0, rate))
        k1 = int(QuadTessellator._clamp(k1, 0, rate))
        if k1 < k0:
            return []
        return [k / rate for k in range(k0, k1 + 1)]

    @staticmethod
    def _nearest_index_with_tie(x: float, lo: int, hi: int, prefer_up: bool, eps: float = 1e-12) -> int:
        """
        最近整数索引，显式处理 .5 平局:
        - prefer_up=True 时在 .5 平局取上邻(等价 ceil)
        - prefer_up=False 时在 .5 平局取下邻(等价 floor)
        """
        base = math.floor(x)
        frac = x - base
        if frac > 0.5 + eps:
            idx = base + 1
        elif frac < 0.5 - eps:
            idx = base
        else:
            idx = base + (1 if prefer_up else 0)
        return int(QuadTessellator._clamp(idx, lo, hi))


class QuadTessellatorFast:
    """
    Optimized QuadTessellator using NumPy vectorization.

    This version first collects all unique (u, v) coordinates, then performs
    batch bilinear interpolation and batch displacement sampling.
    Returns numpy arrays directly for better integration with ray tracing.
    """

    def __init__(self, params: QuadTessParams):
        # compute lift values
        if params.lift is None:
            iu, iv = params.inner
            e_bottom, e_right, e_top, e_left = params.edge
            denom = e_bottom + e_right + e_top + e_left + 2 * (iu + iv)
            if denom <= 0:
                lift = 0.0
            else:
                t = (iu * iv * 2) / denom
                lift = (1 - math.sqrt(max(0.0, 1 - 1 / (t + 1)))) / 2
            params.lift = (lift, lift, lift, lift)

        self.params = params

    def tessellate_arrays(self, quad) -> Tuple[np.ndarray, np.ndarray]:
        """
        Tessellate and return numpy arrays directly.

        Args:
            quad: List/tuple of 4 Vertex objects

        Returns:
            vertices: np.ndarray of shape (N, 3) - vertex positions
            triangles: np.ndarray of shape (M, 3) - triangle indices
        """
        e_bottom, e_right, e_top, e_left = self.params.edge
        iu, iv = self.params.inner
        bottom, right, top, left = self.params.lift

        du = (1.0 - left - right) / iu if iu > 0 else 0.0
        dv = (1.0 - bottom - top) / iv if iv > 0 else 0.0

        # Collect all (u, v) coordinates first
        uv_list = []
        tri_uv_indices = []  # Will store indices into uv_list for each triangle
        uv_to_idx: Dict[Tuple[float, float], int] = {}

        def add_uv(u: float, v: float) -> int:
            """Add a (u,v) coordinate and return its index."""
            u = max(0.0, min(1.0, u))
            v = max(0.0, min(1.0, v))
            key = (round(u, 9), round(v, 9))
            idx = uv_to_idx.get(key)
            if idx is not None:
                return idx
            idx = len(uv_list)
            uv_list.append((u, v))
            uv_to_idx[key] = idx
            return idx

        tris = []

        # Inner grid
        if iu > 0 and iv > 0:
            for jv in range(iv):
                v0 = bottom + jv * dv
                v1 = bottom + (jv + 1) * dv
                for ju in range(iu):
                    u0 = left + ju * du
                    u1 = left + (ju + 1) * du
                    a = add_uv(u0, v0)
                    b = add_uv(u1, v0)
                    c = add_uv(u0, v1)
                    d = add_uv(u1, v1)
                    tris.append((a, b, c))
                    tris.append((b, d, c))

        # Bottom edge
        if e_bottom > 0 and iu > 0 and bottom > 0:
            edge_samples = self._generate_boundary_samples_fast(e_bottom, 0.0, 1.0)
            p = len(edge_samples) - 1
            q = iu
            inner_us = np.array([left + j * du for j in range(q + 1)])

            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = self._nearest_index_fast((t_mid - left) / du, 0, q, True) if du > 1e-12 else 0
                a = add_uv(edge_samples[k], 0.0)
                b = add_uv(edge_samples[k + 1], 0.0)
                c = add_uv(inner_us[r], bottom)
                tris.append((a, b, c))

            for j in range(q):
                t_mid = left + (j + 0.5) * du
                k = self._nearest_index_fast(e_bottom * t_mid, 0, e_bottom, False)
                t_k = max(0.0, min(1.0, k / e_bottom))
                a = add_uv(inner_us[j + 1], bottom)
                b = add_uv(inner_us[j], bottom)
                c = add_uv(t_k, 0.0)
                tris.append((a, b, c))

        # Top edge
        if e_top > 0 and iu > 0 and top > 0:
            edge_samples = self._generate_boundary_samples_fast(e_top, 0.0, 1.0)
            p = len(edge_samples) - 1
            q = iu
            inner_us = np.array([left + j * du for j in range(q + 1)])

            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = self._nearest_index_fast((t_mid - left) / du, 0, q, True) if du > 1e-12 else 0
                a = add_uv(edge_samples[k + 1], 1.0)
                b = add_uv(edge_samples[k], 1.0)
                c = add_uv(inner_us[r], 1.0 - top)
                tris.append((a, b, c))

            for j in range(q):
                t_mid = left + (j + 0.5) * du
                k = self._nearest_index_fast(e_top * t_mid, 0, e_top, False)
                t_k = max(0.0, min(1.0, k / e_top))
                a = add_uv(inner_us[j], 1.0 - top)
                b = add_uv(inner_us[j + 1], 1.0 - top)
                c = add_uv(t_k, 1.0)
                tris.append((a, b, c))

        # Right edge
        if e_right > 0 and iv > 0 and right > 0:
            edge_samples = self._generate_boundary_samples_fast(e_right, 0.0, 1.0)
            p = len(edge_samples) - 1
            q = iv
            inner_vs = np.array([bottom + j * dv for j in range(q + 1)])

            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = self._nearest_index_fast((t_mid - bottom) / dv, 0, q, True) if dv > 1e-12 else 0
                a = add_uv(1.0, edge_samples[k + 1])
                b = add_uv(1.0, edge_samples[k])
                c = add_uv(1.0 - right, inner_vs[r])
                tris.append((a, b, c))

            for j in range(q):
                t_mid = bottom + (j + 0.5) * dv
                k = self._nearest_index_fast(e_right * t_mid, 0, e_right, False)
                t_k = max(0.0, min(1.0, k / e_right))
                a = add_uv(1.0 - right, inner_vs[j])
                b = add_uv(1.0 - right, inner_vs[j + 1])
                c = add_uv(1.0, t_k)
                tris.append((a, b, c))

        # Left edge
        if e_left > 0 and iv > 0 and left > 0:
            edge_samples = self._generate_boundary_samples_fast(e_left, 0.0, 1.0)
            p = len(edge_samples) - 1
            q = iv
            inner_vs = np.array([bottom + j * dv for j in range(q + 1)])

            for k in range(p):
                t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
                r = self._nearest_index_fast((t_mid - bottom) / dv, 0, q, True) if dv > 1e-12 else 0
                a = add_uv(0.0, edge_samples[k])
                b = add_uv(0.0, edge_samples[k + 1])
                c = add_uv(left, inner_vs[r])
                tris.append((a, b, c))

            for j in range(q):
                t_mid = bottom + (j + 0.5) * dv
                k = self._nearest_index_fast(e_left * t_mid, 0, e_left, False)
                t_k = max(0.0, min(1.0, k / e_left))
                a = add_uv(left, inner_vs[j + 1])
                b = add_uv(left, inner_vs[j])
                c = add_uv(0.0, t_k)
                tris.append((a, b, c))

        if len(uv_list) == 0:
            return np.zeros((0, 3), dtype=np.float32), np.zeros((0, 3), dtype=np.uint32)

        # Convert to numpy arrays
        uv_arr = np.array(uv_list, dtype=np.float32)  # (N, 2)
        us = uv_arr[:, 0]
        vs = uv_arr[:, 1]

        # Extract quad corner data as numpy arrays for vectorized computation
        v0, v1, v2, v3 = quad
        pos0 = np.array(v0.position, dtype=np.float32)
        pos1 = np.array(v1.position, dtype=np.float32)
        pos2 = np.array(v2.position, dtype=np.float32)
        pos3 = np.array(v3.position, dtype=np.float32)

        norm0 = np.array(v0.normal, dtype=np.float32)
        norm1 = np.array(v1.normal, dtype=np.float32)
        norm2 = np.array(v2.normal, dtype=np.float32)
        norm3 = np.array(v3.normal, dtype=np.float32)

        uv0 = np.array(v0.uv, dtype=np.float32)
        uv1 = np.array(v1.uv, dtype=np.float32)
        uv2 = np.array(v2.uv, dtype=np.float32)
        uv3 = np.array(v3.uv, dtype=np.float32)

        # Compute bilinear weights
        w00 = ((1 - us) * (1 - vs))[:, None]  # (N, 1)
        w10 = (us * (1 - vs))[:, None]
        w11 = (us * vs)[:, None]
        w01 = ((1 - us) * vs)[:, None]

        # Batch bilinear interpolation for positions
        positions = w00 * pos0 + w10 * pos1 + w11 * pos2 + w01 * pos3  # (N, 3)

        # Batch bilinear interpolation for normals
        normals = w00 * norm0 + w10 * norm1 + w11 * norm2 + w01 * norm3  # (N, 3)

        # Normalize normals
        norms_len = np.linalg.norm(normals, axis=1, keepdims=True)
        normals = normals / (norms_len + 1e-8)

        # Batch bilinear interpolation for UVs
        uvs = w00[:, :1].squeeze(-1)[:, None] * uv0 + \
              w10[:, :1].squeeze(-1)[:, None] * uv1 + \
              w11[:, :1].squeeze(-1)[:, None] * uv2 + \
              w01[:, :1].squeeze(-1)[:, None] * uv3
        # Fix: recalculate properly
        w00_2d = ((1 - us) * (1 - vs))[:, None]
        w10_2d = (us * (1 - vs))[:, None]
        w11_2d = (us * vs)[:, None]
        w01_2d = ((1 - us) * vs)[:, None]
        uvs = w00_2d * uv0 + w10_2d * uv1 + w11_2d * uv2 + w01_2d * uv3  # (N, 2)

        # Apply displacement if sampler is available
        if self.params.disp_sampler is not None:
            displacements = self.params.disp_sampler.sample_batch(uvs[:, 0], uvs[:, 1])  # (N,)
            positions = positions + displacements[:, None] * normals

        # Convert triangles to numpy array
        triangles = np.array(tris, dtype=np.uint32)  # (M, 3)

        return positions.astype(np.float32), triangles

    def tessellate(self, quad) -> Tuple[List[Vertex], List[Triangle]]:
        """
        Tessellate and return Vertex objects (for backward compatibility).

        Note: Use tessellate_arrays() for better performance.
        """
        positions, triangles = self.tessellate_arrays(quad)

        # Convert back to Vertex objects (slower, for compatibility)
        verts = []
        for i in range(positions.shape[0]):
            v = Vertex(
                position=tuple(positions[i].tolist()),
                normal=(0.0, 0.0, 1.0),  # Note: normals not preserved in this conversion
                uv=(0.0, 0.0)
            )
            verts.append(v)

        tris = [tuple(triangles[i].tolist()) for i in range(triangles.shape[0])]
        return verts, tris

    @staticmethod
    def _generate_boundary_samples_fast(rate: int, t_min: float, t_max: float) -> np.ndarray:
        if rate <= 0:
            return np.array([], dtype=np.float32)
        k0 = int(np.ceil(rate * t_min - 1e-9))
        k1 = int(np.floor(rate * t_max + 1e-9))
        k0 = max(0, min(rate, k0))
        k1 = max(0, min(rate, k1))
        if k1 < k0:
            return np.array([], dtype=np.float32)
        return np.arange(k0, k1 + 1, dtype=np.float32) / rate

    @staticmethod
    def _nearest_index_fast(x: float, lo: int, hi: int, prefer_up: bool, eps: float = 1e-12) -> int:
        base = int(np.floor(x))
        frac = x - base
        if frac > 0.5 + eps:
            idx = base + 1
        elif frac < 0.5 - eps:
            idx = base
        else:
            idx = base + (1 if prefer_up else 0)
        return max(lo, min(hi, idx))


def show(verts: List[Vertex], tris: List[Triangle]):
    """
    使用 matplotlib 三维可视化显示顶点和三角形网格
    """
    try:
        import matplotlib.pyplot as plt
        from mpl_toolkits.mplot3d import Axes3D
        from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    except ImportError:
        print("Error: matplotlib is required for visualization. Install it with: pip install matplotlib")
        return

    if not verts:
        print("No vertices to display")
        return

    # 提取顶点位置
    positions = [v.position for v in verts]
    xs = [p[0] for p in positions]
    ys = [p[1] for p in positions]
    zs = [p[2] for p in positions]

    # 创建3D图形
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    # 绘制顶点
    ax.scatter(xs, ys, zs, c='blue', marker='o', s=20, alpha=0.6, label='Vertices')

    # 绘制三角形边
    if tris:
        for tri in tris:
            i0, i1, i2 = tri
            # 绘制三角形的三条边
            points = [
                positions[i0], positions[i1], positions[i2], positions[i0]
            ]
            edge_xs = [p[0] for p in points]
            edge_ys = [p[1] for p in points]
            edge_zs = [p[2] for p in points]
            ax.plot(edge_xs, edge_ys, edge_zs, 'k-', linewidth=0.5, alpha=0.3)

        # 绘制三角形面（半透明）
        tri_verts = []
        for tri in tris:
            i0, i1, i2 = tri
            tri_verts.append([positions[i0], positions[i1], positions[i2]])

        poly_collection = Poly3DCollection(tri_verts, alpha=0.2, facecolor='cyan',
                                           edgecolor='none')
        ax.add_collection3d(poly_collection)

    # 设置标签和标题
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title(f'Tessellation Result\n{len(verts)} vertices, {len(tris)} triangles')
    ax.legend()

    # 设置相等的坐标轴比例
    max_range = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)) if xs else 1.0
    mid_x = (max(xs) + min(xs)) * 0.5 if xs else 0
    mid_y = (max(ys) + min(ys)) * 0.5 if ys else 0
    mid_z = (max(zs) + min(zs)) * 0.5 if zs else 0
    ax.set_xlim(mid_x - max_range * 0.6, mid_x + max_range * 0.6)
    ax.set_ylim(mid_y - max_range * 0.6, mid_y + max_range * 0.6)
    ax.set_zlim(mid_z - max_range * 0.6, mid_z + max_range * 0.6)

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    # Example usage
    quad = (
        Vertex((0, 0, 0), (0, 0, 1), (0, 0)),
        Vertex((1, 0, 0), (0, 0, 1), (10, 0)),
        Vertex((1, 1, 0), (0, 0, 1), (10, 10)),
        Vertex((0, 1, 0), (0, 0, 1), (0, 10)),
    )
    params = QuadTessParams(edge=(2, 3, 4, 5), inner=(3, 4),
                            disp_sampler=DisplacementSampler("assets/Bayon Lion_baked_disp.exr", 1024))
    tessellator = QuadTessellator(params)
    verts, tris = tessellator.tessellate(quad)

    # show
    show(verts, tris)
