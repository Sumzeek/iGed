import math
from dataclasses import dataclass
from typing import List, Tuple, Optional, Dict

try:
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D  # noqa: F401  # 触发 3D 支持
    from mpl_toolkits.mplot3d.art3d import Line3DCollection
except Exception:
    plt = None

# 基础类型(三维)
Vec3 = Tuple[float, float, float]
Tri = Tuple[int, int, int]


@dataclass
class TessParams:
    # 边缘细分率: (bottom, right, top, left)
    edge: Tuple[int, int, int, int]
    # 内部网格细分率: (iu 沿 u, iv 沿 v)
    inner: Tuple[int, int]
    # 过渡带宽度(参数空间 [0,1]): (bottom, right, top, left)
    # 若为 None 则按启发式自动推导
    lift: Optional[Tuple[float, float, float, float]] = None
    # I0/I1 的平局(0.5)选边策略:
    # 若 i0_pref_lower=True: I0 选低索引(左/下), I1 选高索引(右/上); 反之亦然
    i0_pref_lower: bool = True


def clamp(x: float, lo: float, hi: float) -> float:
    """夹取范围 [lo, hi]。"""
    return max(lo, min(hi, x))


def bilinear(u: float, v: float, p00: Vec3, p10: Vec3, p01: Vec3, p11: Vec3) -> Vec3:
    """
    双线性映射(三维): 将参数域 [0,1]^2 映射至三维四边形(p00,p10,p01,p11)。
    注意: 该四边形通常为双线性插值形成的曲面片，而非严格平面。
    """
    w00 = (1 - u) * (1 - v)
    w10 = u * (1 - v)
    w01 = (1 - u) * v
    w11 = u * v
    x = w00 * p00[0] + w10 * p10[0] + w01 * p01[0] + w11 * p11[0]
    y = w00 * p00[1] + w10 * p10[1] + w01 * p01[1] + w11 * p11[1]
    z = w00 * p00[2] + w10 * p10[2] + w01 * p01[2] + w11 * p11[2]
    return (x, y, z)


def nearest_index_with_tie(x: float, lo: int, hi: int, prefer_up: bool, eps: float = 1e-12) -> int:
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
    return int(clamp(idx, lo, hi))


def generate_boundary_samples(rate: int, t_min: float, t_max: float) -> List[float]:
    """
    依据细分率 rate，在 [t_min, t_max] 上生成单调不降采样点(包含端点)。
    采样为 k/rate, k∈[0,rate] 的子集并映射至区间。
    """
    if rate <= 0:
        return []
    k0 = math.ceil(rate * t_min - 1e-9)
    k1 = math.floor(rate * t_max + 1e-9)
    k0 = int(clamp(k0, 0, rate))
    k1 = int(clamp(k1, 0, rate))
    if k1 < k0:
        return []
    return [k / rate for k in range(k0, k1 + 1)]


def compute_lift(params: TessParams) -> Tuple[float, float, float, float]:
    """
    计算(或规范化)过渡带宽度:
    - 若 params.lift 已给出，则仅做夹取并返回；
    - 否则按启发式从细分率推导对称的过渡带，更新 params.lift 并返回。
    """
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
        # 数值保护: sqrt 内部不为负
        lift = (1 - math.sqrt(max(0.0, 1 - 1 / (t + 1)))) / 2
    res = (lift, lift, lift, lift)
    params.lift = res
    return res


def tessellate_quad(
        p00: Vec3,
        p10: Vec3,
        p01: Vec3,
        p11: Vec3,
        params: TessParams,
) -> Tuple[List[Vec3], List[Tri]]:
    """
    对四边形进行基于规则的自适应三角化(结果为三维点集):
    - 内部规则网格(若 iu, iv > 0)
    - 四条边各自的过渡带(遵循 Rule1/Rule2: 中点最近 + 可配置平局规则)
    - 退化情形: 无内部格且四边细分为 1，采用巴黎分割

    返回:
    - 顶点数组: List[(x,y,z)]
    - 三角形索引: List[(i,j,k)]
    """
    # 计算或规范化过渡带
    bottom, right, top, left = compute_lift(params)

    iu, iv = params.inner
    e_bottom, e_right, e_top, e_left = params.edge

    # 确保内部可用区域
    if left + right >= 0.999 or bottom + top >= 0.999:
        raise ValueError("过渡带过大，内部区域被完全压缩")

    # 内部网格步长
    du = (1.0 - left - right) / iu if iu > 0 else 0.0
    dv = (1.0 - bottom - top) / iv if iv > 0 else 0.0

    verts: List[Vec3] = []
    triangles: List[Tri] = []

    # 顶点缓存: 避免重复插入相同(u,v)点
    vtx_index: Dict[Tuple[float, float], int] = {}

    def add_vtx(u: float, v: float) -> int:
        """插入(u,v)对应的三维点，返回顶点索引；带 1e-9 舍入以归并数值误差。"""
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

    # I0/I1 的平局方向
    def prefer_high_for_I0() -> bool:
        # I0: 若偏低索引，则不取上邻；否则取上邻
        return not params.i0_pref_lower

    def prefer_high_for_I1() -> bool:
        # I1: 与 I0 相反
        return params.i0_pref_lower

    # 1) 内部规则网格 -> 每个四边形拆成两个三角形
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

    # 2) 四条边的过渡带(按照中点最近规则 + 可配置平局策略)
    # Bottom: v ∈ [0, bottom]
    if e_bottom > 0 and iu > 0 and bottom > 0:
        edge_samples = generate_boundary_samples(e_bottom, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iu
        inner_us = [left + j * du for j in range(q + 1)]
        # I0: 以边为底边，顶点在内线
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - left) / du, 0, q, prefer_up=prefer_high_for_I0()) if du > 1e-12 else 0
            v0 = add_vtx(edge_samples[k], 0.0)
            v1 = add_vtx(edge_samples[k + 1], 0.0)
            v2 = add_vtx(inner_us[r], bottom)
            triangles.append((v0, v1, v2))
        # I1: 以内线为底边，顶点在边
        for j in range(q):
            t_mid = left + (j + 0.5) * du
            k = nearest_index_with_tie(e_bottom * t_mid, 0, e_bottom, prefer_up=prefer_high_for_I1())
            t_k = clamp(k / e_bottom, 0.0, 1.0)
            v0 = add_vtx(inner_us[j + 1], bottom)
            v1 = add_vtx(inner_us[j], bottom)
            v2 = add_vtx(t_k, 0.0)
            triangles.append((v0, v1, v2))

    # Top: v ∈ [1-top, 1]
    if e_top > 0 and iu > 0 and top > 0:
        edge_samples = generate_boundary_samples(e_top, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iu
        inner_us = [left + j * du for j in range(q + 1)]
        # I0
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - left) / du, 0, q, prefer_up=prefer_high_for_I0()) if du > 1e-12 else 0
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

    # Right: u ∈ [1-right, 1]
    if e_right > 0 and iv > 0 and right > 0:
        edge_samples = generate_boundary_samples(e_right, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iv
        inner_vs = [bottom + j * dv for j in range(q + 1)]
        # I0
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - bottom) / dv, 0, q, prefer_up=prefer_high_for_I0()) if dv > 1e-12 else 0
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

    # Left: u ∈ [0, left]
    if e_left > 0 and iv > 0 and left > 0:
        edge_samples = generate_boundary_samples(e_left, 0.0, 1.0)
        p = max(0, len(edge_samples) - 1)
        q = iv
        inner_vs = [bottom + j * dv for j in range(q + 1)]
        # I0
        for k in range(p):
            t_mid = (edge_samples[k] + edge_samples[k + 1]) * 0.5
            r = nearest_index_with_tie((t_mid - bottom) / dv, 0, q, prefer_up=prefer_high_for_I0()) if dv > 1e-12 else 0
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

    # 3) 退化情形: 无内部网格且四边细分均为 1 -> 巴黎分割
    if iu == 0 and iv == 0 and e_bottom == e_right == e_top == e_left == 1:
        a = add_vtx(0.0, 0.0)
        b = add_vtx(1.0, 0.0)
        c = add_vtx(0.0, 1.0)
        d = add_vtx(1.0, 1.0)
        triangles.append((a, b, c))
        triangles.append((b, d, c))

    return verts, triangles


def _set_axes_equal(ax):
    """设置 3D 坐标轴等比例，避免视觉拉伸。"""
    import numpy as np

    limits = np.array([
        ax.get_xlim3d(),
        ax.get_ylim3d(),
        ax.get_zlim3d(),
    ])
    spans = limits[:, 1] - limits[:, 0]
    centers = np.mean(limits, axis=1)
    radius = 0.5 * max(spans)
    ax.set_xlim3d([centers[0] - radius, centers[0] + radius])
    ax.set_ylim3d([centers[1] - radius, centers[1] + radius])
    ax.set_zlim3d([centers[2] - radius, centers[2] + radius])


def plot_tris(verts: List[Vec3], tris: List[Tri], out_path: Optional[str] = None):
    """三维可视化三角形连接关系(若安装了 matplotlib)。"""
    if plt is None:
        print("matplotlib 未安装，跳过可视化。")
        return

    # 构造线段集合
    segments = []  # 每个元素为 [(x1,y1,z1), (x2,y2,z2)]
    for i, j, k in tris:
        a, b, c = verts[i], verts[j], verts[k]
        segments.append([a, b])
        segments.append([b, c])
        segments.append([c, a])

    fig = plt.figure(figsize=(7, 6))
    ax = fig.add_subplot(111, projection='3d')

    # 绘制三角形边
    lc = Line3DCollection(segments, colors="k", linewidths=0.6, alpha=0.9)
    ax.add_collection3d(lc)

    # 绘制顶点散点
    xs = [p[0] for p in verts]
    ys = [p[1] for p in verts]
    zs = [p[2] for p in verts]
    ax.scatter(xs, ys, zs, s=8, c="r", alpha=0.7, depthshade=True)

    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title(f"Triangles: {len(tris)} | Verts: {len(verts)}")

    # 自适应缩放并设置等比例
    ax.autoscale()
    try:
        _set_axes_equal(ax)
    except Exception:
        pass

    if out_path:
        plt.savefig(out_path, dpi=150, bbox_inches='tight')
        print(f"保存到 {out_path}")
    else:
        plt.show()


def demo():
    """示例: 构造一个略带起伏的四边形曲面并执行细分(三维)。"""
    # 四个角点(三维)。可调整 z 值以观察 3D 效果
    p00 = (0.0, 0.0, 0.0)
    p10 = (1.0, 0.0, 0.2)
    p01 = (0.0, 1.0, 0.0)
    p11 = (1.0, 1.0, 0.4)

    # 策略: I0 偏向左/下; I1 偏向右/上
    # 可传入 lift 显式指定过渡带宽度，否则自动推导
    params = TessParams(edge=(1, 1, 1, 1), inner=(1, 1), i0_pref_lower=True)

    verts, tris = tessellate_quad(p00, p10, p01, p11, params)
    print(f"Generated {len(tris)} triangles and {len(verts)} vertices")

    plot_tris(verts, tris, out_path="tess_preview_3d.png")


if __name__ == "__main__":
    demo()
