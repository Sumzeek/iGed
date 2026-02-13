"""
Tessellation Comparison Script

Compare three tessellation methods:
1. Distance-Based Tessellation
2. Screen-Space Adaptive Tessellation
3. Neural Tessellation Field (NTF)

For each method, compute:
- Total triangle count
- World-space error via ray casting

Author: Generated for comparison experiments
"""

import os
import sys
import logging
import struct
import math
import time
from dataclasses import dataclass
from typing import List, Tuple, Dict, Optional
from concurrent.futures import ProcessPoolExecutor, as_completed
import multiprocessing as mp
import csv

import numpy as np
import torch
import pymeshlab

from Preprocessor import parse_quad_mesh, QuadFace
from Tessellator import DisplacementSampler, QuadTessellatorFast, QuadTessParams, Vertex
from Trainer import NTFRegressor, NTFConfig
from OptixBaker import import_optixbaker


# =============================================================================
# Configuration
# =============================================================================

@dataclass
class ComparisonConfig:
    """Configuration for the comparison experiment."""
    # Screen resolution
    screen_width: int = 1920
    screen_height: int = 1080

    # Camera settings
    fov_degrees: float = 45.0
    near_plane: float = 0.01
    far_plane: float = 1000.0

    # Distance-based tessellation parameters
    dist_min: float = 10.0
    dist_max: float = 100.0
    dist_min_tess: float = 1.0
    dist_max_tess: float = 5.0

    # Screen-space tessellation parameters
    target_pixel: float = 4.0

    # Ray casting samples per quad
    samples_per_dim: int = 50


# =============================================================================
# Multi-process Context
# =============================================================================

@dataclass
class ComparisonContext:
    """Context data shared across all worker processes (initialized once)."""
    # Mesh data
    verts: np.ndarray
    norms: np.ndarray
    uvs: np.ndarray
    quads: list
    orig_verts: np.ndarray
    orig_indices: np.ndarray

    # Displacement data
    disp_exr_path: str
    resolution: int

    # Config
    config: ComparisonConfig

    # Camera projection matrix (constant)
    proj_matrix: np.ndarray

    # NTF model data
    ntf_model_path: str
    epsilon_mean: float
    epsilon_std: float
    epsilon_min: float
    epsilon_max: float
    ntf_state_dict: dict
    ntf_config_dict: dict


# Global worker state
_worker_context: Optional[ComparisonContext] = None
_worker_disp_sampler: Optional[DisplacementSampler] = None
_worker_optix = None
_worker_ntf_model = None
_worker_orig_gas_cached: bool = False  # 标记原始模型 GAS 是否已缓存


def _worker_initializer(context: ComparisonContext) -> None:
    """Initialize worker process with shared context."""
    global _worker_context, _worker_disp_sampler, _worker_optix, _worker_orig_gas_cached, _worker_ntf_model
    _worker_context = context

    # Initialize per-worker resources
    _worker_disp_sampler = DisplacementSampler(context.disp_exr_path, context.resolution)
    _worker_optix = import_optixbaker()

    # 在 worker 初始化时缓存原始模型的 GAS（只执行一次）
    _worker_optix.build_cached_gas(context.orig_verts, context.orig_indices)
    _worker_orig_gas_cached = True
    logging.info(
        f"[Worker] Cached original mesh GAS: {context.orig_verts.shape[0]} vertices, {context.orig_indices.shape[0]} triangles")

    # Initialize NTF model in worker
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    ntf_config = NTFConfig(**context.ntf_config_dict)
    in_dim_raw = 4 * 3 + 1  # 4 vertices * 3 coords + 1 epsilon
    _worker_ntf_model = NTFRegressor(in_dim_raw=in_dim_raw, config=ntf_config).to(device)
    _worker_ntf_model.load_state_dict(context.ntf_state_dict)
    _worker_ntf_model.eval()
    logging.info(f"[Worker] NTF model loaded on {device}")


# =============================================================================
# Camera and Transformation Utilities
# =============================================================================

def compute_projection_matrix(fov_deg: float, aspect: float, near: float, far: float) -> np.ndarray:
    """Compute perspective projection matrix (row-major, for NumPy matrix multiplication)."""
    fov_rad = math.radians(fov_deg)
    tan_half_fov = math.tan(fov_rad / 2.0)

    proj = np.zeros((4, 4), dtype=np.float32)
    proj[0, 0] = 1.0 / (aspect * tan_half_fov)
    proj[1, 1] = 1.0 / tan_half_fov
    proj[2, 2] = -(far + near) / (far - near)
    proj[2, 3] = -(2.0 * far * near) / (far - near)  # Fixed: was proj[3, 2]
    proj[3, 2] = -1.0  # Fixed: was proj[2, 3]

    return proj


def compute_view_matrix(camera_pos: np.ndarray, target: np.ndarray, up: np.ndarray = None) -> np.ndarray:
    """Compute view matrix (camera looks at target from camera_pos)."""
    if up is None:
        up = np.array([0.0, 1.0, 0.0], dtype=np.float32)

    z_axis = camera_pos - target
    z_axis = z_axis / (np.linalg.norm(z_axis) + 1e-8)

    x_axis = np.cross(up, z_axis)
    x_axis = x_axis / (np.linalg.norm(x_axis) + 1e-8)

    y_axis = np.cross(z_axis, x_axis)

    view = np.eye(4, dtype=np.float32)
    view[0, :3] = x_axis
    view[1, :3] = y_axis
    view[2, :3] = z_axis
    view[0, 3] = -np.dot(x_axis, camera_pos)
    view[1, 3] = -np.dot(y_axis, camera_pos)
    view[2, 3] = -np.dot(z_axis, camera_pos)

    return view


def world_to_screen(pos: np.ndarray, view_proj: np.ndarray, screen_size: Tuple[int, int]) -> np.ndarray:
    """Transform world position to screen coordinates."""
    clip = view_proj @ np.append(pos, 1.0)
    if abs(clip[3]) < 1e-8:
        return np.array([0.0, 0.0])

    ndc = clip[:3] / clip[3]
    screen_x = (ndc[0] * 0.5 + 0.5) * screen_size[0]
    screen_y = (ndc[1] * 0.5 + 0.5) * screen_size[1]

    return np.array([screen_x, screen_y])


# =============================================================================
# Worker Functions for Tessellation Factor Computation
# =============================================================================

def _distance_based_factor(v0: np.ndarray, v1: np.ndarray,
                           camera_pos: np.ndarray, config: ComparisonConfig) -> int:
    """Compute tessellation factor based on distance."""
    edge_center = (v0 + v1) * 0.5
    dist = np.linalg.norm(edge_center - camera_pos)

    t = np.clip((dist - config.dist_min) /
                (config.dist_max - config.dist_min), 0.0, 1.0)

    factor = config.dist_max_tess * (1 - t) + config.dist_min_tess * t
    return max(1, int(round(factor)))


def _screen_space_factor(v0: np.ndarray, v1: np.ndarray,
                         view_proj: np.ndarray, config: ComparisonConfig) -> int:
    """Compute tessellation factor based on screen-space edge length."""
    screen_size = (config.screen_width, config.screen_height)
    screen0 = world_to_screen(v0, view_proj, screen_size)
    screen1 = world_to_screen(v1, view_proj, screen_size)

    screen_length = np.linalg.norm(screen1 - screen0)
    factor = screen_length / config.target_pixel

    return max(1, int(round(factor)))


def _compute_adaptive_epsilon(v0: np.ndarray, v1: np.ndarray,
                              v2: np.ndarray, v3: np.ndarray,
                              view_matrix: np.ndarray, proj_matrix: np.ndarray,
                              config: ComparisonConfig) -> float:
    """Compute adaptive epsilon based on screen pixel size."""

    def to_view_z(v):
        view_pos = view_matrix @ np.append(v, 1.0)
        return abs(view_pos[2])

    d_min = min(to_view_z(v0), to_view_z(v1), to_view_z(v2), to_view_z(v3))
    d_min = max(d_min, 0.01)

    tan_half_fov = 1.0 / proj_matrix[1, 1]
    pixel_world_size = (2.0 * d_min * tan_half_fov) / config.screen_height

    return pixel_world_size


def _ntf_predict(quad_verts: np.ndarray, epsilon: float) -> Tuple[int, int]:
    """Predict tessellation factors using NTF model."""
    global _worker_context, _worker_ntf_model
    ctx = _worker_context

    # Handle out-of-range epsilon
    if epsilon > ctx.epsilon_max:
        return (1, 1)

    epsilon = np.clip(epsilon, ctx.epsilon_min, ctx.epsilon_max)
    eps_norm = (epsilon - ctx.epsilon_mean) / ctx.epsilon_std

    geom = quad_verts.astype(np.float32).flatten()
    feat = np.concatenate([geom, np.array([eps_norm], dtype=np.float32)])

    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    x = torch.from_numpy(feat).unsqueeze(0).to(device)

    with torch.no_grad():
        rates = _worker_ntf_model.predict_rates(x)[0]

    inner_rate = max(1, int(round(rates[0].item())))
    outter_rate = max(1, int(round(rates[1].item())))

    return (inner_rate, outter_rate)


def _compute_triangle_count(edge_factors: Tuple[int, int, int, int],
                            inner_factors: Tuple[int, int]) -> int:
    """Compute total triangle count for a quad."""
    e_bottom, e_right, e_top, e_left = edge_factors
    inner_u, inner_v = inner_factors

    count = inner_u * inner_v * 2
    count += (inner_u + inner_v) * 2
    count += e_bottom + e_right + e_top + e_left

    return count


def _bilinear_batch(corners: List[np.ndarray], us: np.ndarray, vs: np.ndarray) -> np.ndarray:
    """Vectorized bilinear interpolation."""
    w00 = ((1 - us) * (1 - vs))[:, None]
    w10 = (us * (1 - vs))[:, None]
    w11 = (us * vs)[:, None]
    w01 = ((1 - us) * vs)[:, None]
    return w00 * corners[0] + w10 * corners[1] + w11 * corners[2] + w01 * corners[3]


def _ray_cast_cached(origins: np.ndarray, dirs: np.ndarray,
                     vertices: np.ndarray, indices: np.ndarray) -> np.ndarray:
    """Cast rays against cached original mesh using intersect_cached."""
    global _worker_optix
    hits = _worker_optix.intersect_cached(origins, dirs)
    hits = np.asarray(hits, dtype=np.int32)

    num_rays = origins.shape[0]
    t_out = np.full(num_rays, -1.0, dtype=np.float32)

    valid_mask = hits >= 0
    valid_indices = np.where(valid_mask)[0]

    if len(valid_indices) == 0:
        return t_out

    valid_tri_ids = hits[valid_indices]
    tri_v_indices = indices[valid_tri_ids]

    v0 = vertices[tri_v_indices[:, 0]]
    v1 = vertices[tri_v_indices[:, 1]]
    v2 = vertices[tri_v_indices[:, 2]]

    ori = origins[valid_indices]
    d = dirs[valid_indices]

    # Möller–Trumbore intersection
    e1 = v1 - v0
    e2 = v2 - v0
    pvec = np.cross(d, e2)
    det = np.einsum('ij,ij->i', e1, pvec)

    valid_det_mask = np.abs(det) > 1e-8
    sub_indices = valid_indices[valid_det_mask]

    if len(sub_indices) == 0:
        return t_out

    inv_det = 1.0 / det[valid_det_mask]
    tvec = ori[valid_det_mask] - v0[valid_det_mask]
    qvec = np.cross(tvec, e1[valid_det_mask])
    t = np.einsum('ij,ij->i', e2[valid_det_mask], qvec) * inv_det

    t_out[sub_indices] = np.abs(t)
    return t_out


def _ray_cast(origins: np.ndarray, dirs: np.ndarray,
              vertices: np.ndarray, indices: np.ndarray) -> np.ndarray:
    """Cast rays against non-cached mesh (for tessellated quads)."""
    global _worker_optix
    hits = _worker_optix.intersect(origins, dirs, vertices, indices)
    hits = np.asarray(hits, dtype=np.int32)

    num_rays = origins.shape[0]
    t_out = np.full(num_rays, -1.0, dtype=np.float32)

    valid_mask = hits >= 0
    valid_indices = np.where(valid_mask)[0]

    if len(valid_indices) == 0:
        return t_out

    valid_tri_ids = hits[valid_indices]
    tri_v_indices = indices[valid_tri_ids]

    v0 = vertices[tri_v_indices[:, 0]]
    v1 = vertices[tri_v_indices[:, 1]]
    v2 = vertices[tri_v_indices[:, 2]]

    ori = origins[valid_indices]
    d = dirs[valid_indices]

    # Möller–Trumbore intersection
    e1 = v1 - v0
    e2 = v2 - v0
    pvec = np.cross(d, e2)
    det = np.einsum('ij,ij->i', e1, pvec)

    valid_det_mask = np.abs(det) > 1e-8
    sub_indices = valid_indices[valid_det_mask]

    if len(sub_indices) == 0:
        return t_out

    inv_det = 1.0 / det[valid_det_mask]
    tvec = ori[valid_det_mask] - v0[valid_det_mask]
    qvec = np.cross(tvec, e1[valid_det_mask])
    t = np.einsum('ij,ij->i', e2[valid_det_mask], qvec) * inv_det

    t_out[sub_indices] = np.abs(t)
    return t_out


def _tessellate_and_compute_error(quad_idx: int,
                                  edge_factors: Tuple[int, int, int, int],
                                  inner_factors: Tuple[int, int],
                                  samples_per_dim: int = 50) -> float:
    """Tessellate a quad and compute world-space error via ray casting."""
    global _worker_context, _worker_disp_sampler
    ctx = _worker_context

    face = ctx.quads[quad_idx]
    positions = [ctx.verts[i] for i in face.verts]
    normals = [ctx.norms[i] for i in face.norms]

    # Create Vertex objects for tessellation
    vertices = []
    for i in range(4):
        v_idx = face.verts[i]
        n_idx = face.norms[i]
        uv_idx = face.uvs[i]
        vertices.append(Vertex(
            position=tuple(ctx.verts[v_idx]),
            normal=tuple(ctx.norms[n_idx]),
            uv=tuple(ctx.uvs[uv_idx][:2])
        ))

    # Create tessellation parameters
    params = QuadTessParams(
        edge=edge_factors,
        inner=inner_factors,
        disp_sampler=_worker_disp_sampler
    )

    # Tessellate
    tessellator = QuadTessellatorFast(params)
    mesh_verts, mesh_tris = tessellator.tessellate_arrays(vertices)

    if mesh_verts.shape[0] == 0 or mesh_tris.shape[0] == 0:
        return 0.0

    # Generate sample rays
    S = samples_per_dim
    us, vs = np.meshgrid(
        np.linspace(0.0, 1.0, S),
        np.linspace(0.0, 1.0, S)
    )
    us_flat = us.ravel().astype(np.float32)
    vs_flat = vs.ravel().astype(np.float32)

    # Bilinear interpolation for ray origins and directions
    p = [np.array(positions[i]) for i in range(4)]
    n = [np.array(normals[i]) for i in range(4)]

    origins = _bilinear_batch(p, us_flat, vs_flat)
    dirs = _bilinear_batch(n, us_flat, vs_flat)

    # Normalize directions
    norms = np.linalg.norm(dirs, axis=1, keepdims=True)
    dirs = dirs / (norms + 1e-8)

    # Offset origins slightly
    origins = origins - 1e-6 * dirs

    # Ray cast against original mesh (using cached GAS for performance)
    t_ref = _ray_cast_cached(origins, dirs, ctx.orig_verts, ctx.orig_indices)

    # Ray cast against tessellated mesh (non-cached, small mesh)
    t_tess = _ray_cast(origins, dirs, mesh_verts, mesh_tris)

    # Compute error
    valid_mask = (t_ref > 0) & (t_tess > 0)
    if not np.any(valid_mask):
        return -1.0

    errors = np.abs(t_ref[valid_mask] - t_tess[valid_mask])
    max_error = float(np.max(errors))

    return max_error


def process_single_quad(args: Tuple[int, np.ndarray, np.ndarray, np.ndarray]) -> Dict:
    """Process a single quad using distance-based, screen-space, and NTF tessellation.

    Args:
        args: Tuple of (qid, camera_pos, view_matrix, view_proj)

    Returns a dict with results for all three methods.
    """
    qid, camera_pos, view_matrix, view_proj = args

    global _worker_context
    ctx = _worker_context
    config = ctx.config

    face = ctx.quads[qid]

    # Get quad data
    positions = [ctx.verts[i] for i in face.verts]

    p0, p1, p2, p3 = [np.array(p) for p in positions]

    results = {}

    # 1. Distance-based tessellation
    e_bottom = _distance_based_factor(p0, p1, camera_pos, config)
    e_right = _distance_based_factor(p1, p2, camera_pos, config)
    e_top = _distance_based_factor(p3, p2, camera_pos, config)
    e_left = _distance_based_factor(p0, p3, camera_pos, config)
    inner_u = max((e_bottom + e_top) // 2, 1)
    inner_v = max((e_right + e_left) // 2, 1)

    edge_dist = (e_bottom, e_right, e_top, e_left)
    inner_dist = (inner_u, inner_v)
    tri_count_dist = _compute_triangle_count(edge_dist, inner_dist)
    error_dist = _tessellate_and_compute_error(qid, edge_dist, inner_dist, config.samples_per_dim)

    results['distance'] = {'tri_count': tri_count_dist, 'error': error_dist}

    # 2. Screen-space tessellation
    e_bottom = _screen_space_factor(p0, p1, view_proj, config)
    e_right = _screen_space_factor(p1, p2, view_proj, config)
    e_top = _screen_space_factor(p3, p2, view_proj, config)
    e_left = _screen_space_factor(p0, p3, view_proj, config)
    inner_u = max((e_bottom + e_top) // 2, 1)
    inner_v = max((e_right + e_left) // 2, 1)

    edge_ss = (e_bottom, e_right, e_top, e_left)
    inner_ss = (inner_u, inner_v)
    tri_count_ss = _compute_triangle_count(edge_ss, inner_ss)
    error_ss = _tessellate_and_compute_error(qid, edge_ss, inner_ss, config.samples_per_dim)

    results['screen_space'] = {'tri_count': tri_count_ss, 'error': error_ss}

    # 3. NTF tessellation
    quad_verts = np.array([p0, p1, p2, p3], dtype=np.float32)
    epsilon = _compute_adaptive_epsilon(p0, p1, p2, p3, view_matrix, ctx.proj_matrix, config)
    inner_rate, outer_rate = _ntf_predict(quad_verts, epsilon)

    # NTF uses uniform tessellation factors
    edge_ntf = (outer_rate, outer_rate, outer_rate, outer_rate)
    inner_ntf = (inner_rate, inner_rate)
    tri_count_ntf = _compute_triangle_count(edge_ntf, inner_ntf)
    error_ntf = _tessellate_and_compute_error(qid, edge_ntf, inner_ntf, config.samples_per_dim)

    results['ntf'] = {'tri_count': tri_count_ntf, 'error': error_ntf}

    return results


# =============================================================================
# NTF Model Loading (for main process)
# =============================================================================

def load_ntf_model_data(model_path: str) -> Tuple[dict, dict, float, float, float, float]:
    """Load NTF model data for serialization to workers.

    Returns: (state_dict, config_dict, epsilon_mean, epsilon_std, epsilon_min, epsilon_max)
    """
    device = 'cpu'  # Load on CPU for serialization

    if model_path.endswith('.pt'):
        checkpoint = torch.load(model_path, map_location=device)
        ntf_config = checkpoint['ntf_config']

        epsilon_mean = checkpoint.get('epsilon_mean', 0.0)
        epsilon_std = checkpoint.get('epsilon_std', 1.0)
        epsilon_min = checkpoint.get('epsilon_min', 0.0)
        epsilon_max = checkpoint.get('epsilon_max', 1.0)

        # Convert state dict tensors to CPU numpy for pickling
        state_dict = {k: v.cpu() for k, v in checkpoint['state_dict'].items()}

        return state_dict, ntf_config, epsilon_mean, epsilon_std, epsilon_min, epsilon_max
    else:
        raise ValueError(f"Unsupported model format: {model_path}")


# =============================================================================
# Main Comparison Function
# =============================================================================

def run_comparison(
        baked_mesh_path: str,
        original_mesh_path: str,
        displacement_exr_path: str,
        ntf_model_path: str,
        resolution: int = 1024,
        output_csv: str = "tessellation_comparison.csv",
        max_workers: int = None
):
    """Run the tessellation comparison experiment using multiprocessing."""

    config = ComparisonConfig()

    # Load baked mesh (quad-based)
    logging.info("[Comparison] Loading meshes...")
    verts, norms, uvs, quads = parse_quad_mesh(baked_mesh_path)
    verts = np.array(verts, dtype=np.float32)
    norms = np.array(norms, dtype=np.float32)
    uvs = np.array(uvs, dtype=np.float32)

    # Load original high-poly mesh
    ms = pymeshlab.MeshSet()
    ms.load_new_mesh(original_mesh_path)
    m = ms.current_mesh()
    orig_verts = np.array(m.vertex_matrix(), dtype=np.float32)
    orig_indices = np.array(m.face_matrix(), dtype=np.uint32)

    # Compute model center and radius
    model_center = (verts.min(axis=0) + verts.max(axis=0)) / 2.0
    model_radius = np.linalg.norm(verts.max(axis=0) - verts.min(axis=0)) / 2.0

    logging.info(f"[Comparison] Loaded baked mesh: {len(quads)} quads")
    logging.info(f"[Comparison] Model center: {model_center}")
    logging.info(f"[Comparison] Model radius: {model_radius}")

    # Load NTF model data
    logging.info("[Comparison] Loading NTF model...")
    ntf_state_dict, ntf_config_dict, epsilon_mean, epsilon_std, epsilon_min, epsilon_max = \
        load_ntf_model_data(ntf_model_path)

    logging.info(f"[NTF] epsilon_mean: {epsilon_mean:.6f}")
    logging.info(f"[NTF] epsilon_std:  {epsilon_std:.6f}")
    logging.info(f"[NTF] epsilon_min:  {epsilon_min:.6f}")
    logging.info(f"[NTF] epsilon_max:  {epsilon_max:.6f}")

    # Setup multiprocessing
    if max_workers is None:
        max_workers = min(60, max(1, int(os.cpu_count() * 0.5)))
    logging.info(f"[Comparison] Using ProcessPoolExecutor with max_workers={max_workers}")

    # Camera distances: [1R, 2R, 3R, 4R, 5R, 6R]
    distance_factors = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]

    # Compute projection matrix (constant across distances)
    aspect = config.screen_width / config.screen_height
    proj_matrix = compute_projection_matrix(
        config.fov_degrees, aspect, config.near_plane, config.far_plane
    )

    results = []
    num_quads = len(quads)

    # Update distance-based parameters based on model scale (constant)
    config.dist_min = 0.0
    config.dist_max = model_radius * 6.0

    # Create static context (shared by all workers, initialized once)
    context = ComparisonContext(
        verts=verts,
        norms=norms,
        uvs=uvs,
        quads=quads,
        orig_verts=orig_verts,
        orig_indices=orig_indices,
        disp_exr_path=displacement_exr_path,
        resolution=resolution,
        config=config,
        proj_matrix=proj_matrix,
        ntf_model_path=ntf_model_path,
        epsilon_mean=epsilon_mean,
        epsilon_std=epsilon_std,
        epsilon_min=epsilon_min,
        epsilon_max=epsilon_max,
        ntf_state_dict=ntf_state_dict,
        ntf_config_dict=ntf_config_dict,
    )

    # Create ProcessPoolExecutor ONCE, outside the distance loop
    # Workers and their cached GAS will persist across all distance tests
    logging.info("[Comparison] Initializing worker pool (GAS will be built once per worker)...")

    with ProcessPoolExecutor(
            max_workers=max_workers,
            initializer=_worker_initializer,
            initargs=(context,)
    ) as executor:

        for dist_factor in distance_factors:
            logging.info(f"\n{'=' * 60}")
            logging.info(f"Testing distance: {dist_factor}R")
            logging.info(f"{'=' * 60}")

            # Set camera position (along Z axis from model center)
            camera_dist = dist_factor * model_radius
            camera_pos = model_center + np.array([0.0, 0.0, camera_dist])
            view_matrix = compute_view_matrix(camera_pos, model_center)
            view_proj = proj_matrix @ view_matrix

            # Store results for each method
            method_results = {
                'distance': {'tri_count': 0, 'errors': []},
                'screen_space': {'tri_count': 0, 'errors': []},
                'ntf': {'tri_count': 0, 'errors': []}
            }

            start_time = time.time()
            completed = 0

            # Submit tasks with camera parameters (camera params change per distance)
            task_args = [(qid, camera_pos, view_matrix, view_proj) for qid in range(num_quads)]
            futures = {executor.submit(process_single_quad, args): args[0] for args in task_args}

            for future in as_completed(futures):
                qid = futures[future]
                try:
                    quad_results = future.result()

                    for method in ['distance', 'screen_space', 'ntf']:
                        method_results[method]['tri_count'] += quad_results[method]['tri_count']
                        if quad_results[method]['error'] >= 0:
                            method_results[method]['errors'].append(quad_results[method]['error'])

                except Exception as e:
                    logging.exception(f"[Comparison] Exception while processing quad {qid}: {e}")
                    continue

                completed += 1
                if completed % 100 == 0 or completed == num_quads:
                    elapsed = time.time() - start_time
                    rate = completed / elapsed if elapsed > 0 else 0
                    eta = (num_quads - completed) / rate if rate > 0 else 0
                    logging.info(f"  Processed {completed}/{num_quads} quads "
                                 f"({elapsed:.1f}s elapsed, ETA: {eta:.1f}s)")

            # Compute error statistics for each method (inside dist_factor loop)
            for method in method_results:
                errors = np.array(method_results[method]['errors'])
                if len(errors) > 0:
                    method_results[method]['max_error'] = float(np.max(errors))
                    method_results[method]['min_error'] = float(np.min(errors))
                    method_results[method]['mean_error'] = float(np.mean(errors))
                    method_results[method]['median_error'] = float(np.median(errors))
                    method_results[method]['std_error'] = float(np.std(errors))
                    method_results[method]['rmse'] = float(np.sqrt(np.mean(errors ** 2)))
                    method_results[method]['p90_error'] = float(np.percentile(errors, 90))
                    method_results[method]['p95_error'] = float(np.percentile(errors, 95))
                    method_results[method]['p99_error'] = float(np.percentile(errors, 99))
                    method_results[method]['valid_quads'] = len(errors)
                else:
                    method_results[method]['max_error'] = 0.0
                    method_results[method]['min_error'] = 0.0
                    method_results[method]['mean_error'] = 0.0
                    method_results[method]['median_error'] = 0.0
                    method_results[method]['std_error'] = 0.0
                    method_results[method]['rmse'] = 0.0
                    method_results[method]['p90_error'] = 0.0
                    method_results[method]['p95_error'] = 0.0
                    method_results[method]['p99_error'] = 0.0
                    method_results[method]['valid_quads'] = 0

            # Log results for this distance
            logging.info(f"\nResults for distance {dist_factor}R:")
            for method in ['distance', 'screen_space', 'ntf']:
                r = method_results[method]
                logging.info(f"  {method:15s}: triangles={r['tri_count']:8d}, "
                             f"max={r['max_error']:.6f}, mean={r['mean_error']:.6f}, "
                             f"rmse={r['rmse']:.6f}, p95={r['p95_error']:.6f}")

            # Store results with all metrics
            result_row = {'distance_factor': dist_factor}
            for method in ['distance', 'screen_space', 'ntf']:
                r = method_results[method]
                result_row[f'{method}_tri_count'] = r['tri_count']
                result_row[f'{method}_valid_quads'] = r['valid_quads']
                result_row[f'{method}_max_error'] = r['max_error']
                result_row[f'{method}_min_error'] = r['min_error']
                result_row[f'{method}_mean_error'] = r['mean_error']
                result_row[f'{method}_median_error'] = r['median_error']
                result_row[f'{method}_std_error'] = r['std_error']
                result_row[f'{method}_rmse'] = r['rmse']
                result_row[f'{method}_p90_error'] = r['p90_error']
                result_row[f'{method}_p95_error'] = r['p95_error']
                result_row[f'{method}_p99_error'] = r['p99_error']
            results.append(result_row)

    # Save results to CSV
    with open(output_csv, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)

    logging.info(f"\n{'=' * 60}")
    logging.info(f"Results saved to {output_csv}")
    logging.info(f"{'=' * 60}")

    # Print summary tables
    _print_summary_tables(results)

    return results


def _print_summary_tables(results: List[Dict]) -> None:
    """Print formatted summary tables for distance, screen-space, and NTF methods."""

    # Table 1: Triangle Counts
    print("\n" + "=" * 75)
    print("TRIANGLE COUNT COMPARISON")
    print("=" * 75)
    print(f"{'Distance':>10s} | {'Distance-Based':>18s} | {'Screen-Space':>18s} | {'NTF':>18s}")
    print("-" * 75)
    for r in results:
        print(f"{r['distance_factor']:>10.1f} | "
              f"{r['distance_tri_count']:>18,d} | "
              f"{r['screen_space_tri_count']:>18,d} | "
              f"{r['ntf_tri_count']:>18,d}")
    print("=" * 75)

    # Table 2: Max Error
    print("\n" + "=" * 75)
    print("MAX ERROR COMPARISON")
    print("=" * 75)
    print(f"{'Distance':>10s} | {'Distance-Based':>18s} | {'Screen-Space':>18s} | {'NTF':>18s}")
    print("-" * 75)
    for r in results:
        print(f"{r['distance_factor']:>10.1f} | "
              f"{r['distance_max_error']:>18.6f} | "
              f"{r['screen_space_max_error']:>18.6f} | "
              f"{r['ntf_max_error']:>18.6f}")
    print("=" * 75)

    # Table 3: Mean Error
    print("\n" + "=" * 75)
    print("MEAN ERROR COMPARISON")
    print("=" * 75)
    print(f"{'Distance':>10s} | {'Distance-Based':>18s} | {'Screen-Space':>18s} | {'NTF':>18s}")
    print("-" * 75)
    for r in results:
        print(f"{r['distance_factor']:>10.1f} | "
              f"{r['distance_mean_error']:>18.6f} | "
              f"{r['screen_space_mean_error']:>18.6f} | "
              f"{r['ntf_mean_error']:>18.6f}")
    print("=" * 75)

    # Table 4: RMSE
    print("\n" + "=" * 75)
    print("RMSE COMPARISON")
    print("=" * 75)
    print(f"{'Distance':>10s} | {'Distance-Based':>18s} | {'Screen-Space':>18s} | {'NTF':>18s}")
    print("-" * 75)
    for r in results:
        print(f"{r['distance_factor']:>10.1f} | "
              f"{r['distance_rmse']:>18.6f} | "
              f"{r['screen_space_rmse']:>18.6f} | "
              f"{r['ntf_rmse']:>18.6f}")
    print("=" * 75)

    # Table 5: P95 Error
    print("\n" + "=" * 75)
    print("P95 ERROR COMPARISON")
    print("=" * 75)
    print(f"{'Distance':>10s} | {'Distance-Based':>18s} | {'Screen-Space':>18s} | {'NTF':>18s}")
    print("-" * 75)
    for r in results:
        print(f"{r['distance_factor']:>10.1f} | "
              f"{r['distance_p95_error']:>18.6f} | "
              f"{r['screen_space_p95_error']:>18.6f} | "
              f"{r['ntf_p95_error']:>18.6f}")
    print("=" * 75)

    # Table 6: Detailed Metrics for Each Distance
    print("\n" + "=" * 90)
    print("DETAILED METRICS SUMMARY (All Distances)")
    print("=" * 90)

    for r in results:
        print(f"\n--- Distance: {r['distance_factor']}R ---")
        print(f"{'Metric':<20s} | {'Distance-Based':>20s} | {'Screen-Space':>20s} | {'NTF':>20s}")
        print("-" * 90)

        metrics = [
            ('Triangle Count', 'tri_count', 'd'),
            ('Valid Quads', 'valid_quads', 'd'),
            ('Max Error', 'max_error', '.6f'),
            ('Min Error', 'min_error', '.6f'),
            ('Mean Error', 'mean_error', '.6f'),
            ('Median Error', 'median_error', '.6f'),
            ('Std Deviation', 'std_error', '.6f'),
            ('RMSE', 'rmse', '.6f'),
            ('P90 Error', 'p90_error', '.6f'),
            ('P95 Error', 'p95_error', '.6f'),
            ('P99 Error', 'p99_error', '.6f'),
        ]

        for metric_name, metric_key, fmt in metrics:
            dist_val = r[f'distance_{metric_key}']
            ss_val = r[f'screen_space_{metric_key}']
            ntf_val = r[f'ntf_{metric_key}']

            if fmt == 'd':
                print(f"{metric_name:<20s} | {dist_val:>20,d} | {ss_val:>20,d} | {ntf_val:>20,d}")
            else:
                print(f"{metric_name:<20s} | {dist_val:>20{fmt}} | {ss_val:>20{fmt}} | {ntf_val:>20{fmt}}")

    print("\n" + "=" * 90)


# =============================================================================
# Entry Point
# =============================================================================

if __name__ == '__main__':
    # Windows multiprocess requires freeze_support
    mp.freeze_support()

    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s',
        handlers=[logging.StreamHandler(sys.stdout)]
    )

    # Default paths - adjust these to your model files
    baked_mesh = "assets/Bayon Lion_baked.obj"
    original_mesh = "assets/Bayon Lion.obj"
    displacement_exr = "assets/Bayon Lion_baked_disp.exr"
    ntf_model = "assets/Bayon Lion_baked_quads_augmented_ntf.pt"
    resolution = 1024
    output_csv = "Bayon Lion_tessellation_comparison_results.csv"

    # Check if files exist
    for path in [baked_mesh, original_mesh, displacement_exr, ntf_model]:
        if not os.path.isfile(path):
            logging.error(f"File not found: {path}")
            sys.exit(1)

    # Run comparison with multiprocessing
    results = run_comparison(
        baked_mesh_path=baked_mesh,
        original_mesh_path=original_mesh,
        displacement_exr_path=displacement_exr,
        ntf_model_path=ntf_model,
        resolution=resolution,
        output_csv=output_csv,
        max_workers=None  # Auto-detect
    )
