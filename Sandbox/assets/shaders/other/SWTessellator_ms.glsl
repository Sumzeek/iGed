#version 460

#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : require

#define WORKGROUP_SIZE 32
#define MAX_VERTEX_COUNT 256
#define MAX_PRIMITIVE_COUNT 512
#define FLOAT_EPSILON 0.000001

// 每个 tile 的最大细分率，必须与 task shader 中的定义一致
#define MAX_TILE_INNER_RATE 7u

layout(local_size_x = WORKGROUP_SIZE) in;

layout(triangles) out;
layout(max_vertices = MAX_VERTEX_COUNT, max_primitives = MAX_PRIMITIVE_COUNT) out;

layout(binding = 0, std140) uniform SceneDataBlock_std140 {
    mat4 u_View;
    mat4 u_Projection;
    mat4 u_ProjectionView;
    mat4 u_Transform;
} SceneData;

layout(binding = 1, std140) uniform PerFrameDataBlock_std140 {
    vec3 u_ViewPos;
    float _padding_u_ViewPos;
    mat4 u_Normal;
} PerFrameData;

layout(binding = 2, std140) uniform TessellatorDataBlock_std140 {
    uvec2 u_ScreenSize;
    uint u_TessllationMode;
    uint u_TargetTessFactor;
    uint u_LineOption;
    uint u_EpsilonOption;
    uint u_QuadSize;
    float u_MinDist;
    float u_MaxDist;
    float u_TargetPixel;
    float u_MaxCurvature;
    float u_EpsilonCoefficient;
    vec3 u_ViewPos;
    float _padding_u_ViewPos;
    mat4 u_Model;
    mat4 u_View;
    mat4 u_Projection;
} TessellatorData;

layout(binding = 3) uniform sampler2D u_DisplaceMap;
layout(binding = 4) uniform sampler2D u_NormalMap;

// vertex information
layout(std430, binding = 5) readonly buffer PositionBuffer {
    float data[];// vec3
} positionBuffer;

layout(std430, binding = 6) readonly buffer NormalBuffer {
    float data[];// vec3
} normalBuffer;

layout(std430, binding = 7) readonly buffer TexCoordBuffer {
    float data[];// vec2
} texcoordBuffer;

layout(std430, binding = 8) readonly buffer QuadIndexBuffer {
    uint data[];
} quadIndexBuffer;

taskNV in Task {
    uint quadId;// 原始 quad ID
    uint innerU;// 内部 U 方向细分率
    uint innerV;// 内部 V 方向细分率
    uint tilesU;// U 方向 tile 数
    uint tilesV;// V 方向 tile 数
// 边界细分因子
    uint edgeBottom;
    uint edgeRight;
    uint edgeTop;
    uint edgeLeft;
} task_in;

// out for fragment shader
out PerVertexData {
    vec3 mcPosition;
    vec3 vcPosition;
    vec2 texcoord;
    float height;
} v_out[];

// use for tessellation
struct VertexData {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
};

void EmitVertex(uint vertexId, VertexData vertex);
void EmitPrimitive(uint primitiveId, uvec3 indices);

VertexData GetInputVertex(uint vertexId);
uint GetNearestIndexI0(float lo, float hi, uint rate, float x);
uint GetNearestIndexI1(float lo, float hi, uint rate, float x);
VertexData GetDisplacedVertex(VertexData quadVerts[4], float u, float v);
VertexData BilinearQuad(VertexData v0, VertexData v1, VertexData v2, VertexData v3, float u, float v);

// 全局参数（使用 shared memory 以便所有线程访问）
shared float s_leftLift, s_rightLift, s_bottomLift, s_topLift;
shared float s_du, s_dv;

void main()
{
    uint tileId = gl_WorkGroupID.x;// tile index within this quad
    uint gtid = gl_LocalInvocationID.x;

    // Bounds check
    uint quadId = task_in.quadId;
    if (quadId >= TessellatorData.u_QuadSize) {
        return;
    }

    // 计算当前 tile 的坐标
    uint tilesU = task_in.tilesU;
    uint tilesV = task_in.tilesV;
    uint tileX = tileId % tilesU;
    uint tileY = tileId / tilesU;

    // 获取原始 quad 的顶点
    VertexData quadVerts[4];
    quadVerts[0] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 0u]);
    quadVerts[1] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 1u]);
    quadVerts[2] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 2u]);
    quadVerts[3] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 3u]);

    // 全局细分参数
    uint inner_u = task_in.innerU;
    uint inner_v = task_in.innerV;
    uint e_bottom = task_in.edgeBottom;
    uint e_right = task_in.edgeRight;
    uint e_top = task_in.edgeTop;
    uint e_left = task_in.edgeLeft;

    // 计算 lift 值
    float denom = float(e_bottom + e_right + e_top + e_left) + 2.0 * float(inner_u + inner_v);
    float t = (denom > 0.0) ? (2.0 * float(inner_u * inner_v) / denom) : 0.0;
    float lift = (t > -1.0) ? ((1.0 - sqrt(max(0.0, 1.0 - 1.0 / (t + 1.0)))) * 0.5) : 0.0;

    float bottomLift = lift;
    float rightLift  = lift;
    float topLift    = lift;
    float leftLift   = lift;

    // // 计算 lift 值（根据各边三角形分布独立计算四边 lift）
    // float iu_f = float(inner_u);
    // float iv_f = float(inner_v);
    // float eb_f = float(e_bottom);
    // float er_f = float(e_right);
    // float et_f = float(e_top);
    // float el_f = float(e_left);
    //
    // // 1. 统计各区域三角形数量
    // float tris_center = 2.0 * iu_f * iv_f;
    // float tris_v_band = (iu_f + eb_f) + (iu_f + et_f);// 下梯形 + 上梯形
    // float tris_h_band = (iv_f + er_f) + (iv_f + el_f);// 右梯形 + 左梯形
    // float total_tris = tris_center + tris_v_band + tris_h_band;
    //
    // // 2. 目标内部矩形面积与宽高差
    // float target_inner_area = (total_tris > 0.0) ? (tris_center / total_tris) : 0.0;
    // float diff_wh = (total_tris > 0.0) ? ((tris_v_band - tris_h_band) / total_tris) : 0.0;
    //
    // // 3. 解方程求内部矩形尺寸: h^2 + diff * h - area = 0
    // float inner_h = (-diff_wh + sqrt(diff_wh * diff_wh + 4.0 * target_inner_area)) / 2.0;
    // float inner_w = inner_h + diff_wh;
    //
    // // 4. 计算四周 lift
    // float margin_v = 1.0 - inner_h;
    // float bottomLift = (tris_v_band > 0.0) ? (margin_v * (iu_f + eb_f) / tris_v_band) : 0.0;
    // float topLift    = (tris_v_band > 0.0) ? (margin_v * (iu_f + et_f) / tris_v_band) : 0.0;
    //
    // float margin_h = 1.0 - inner_w;
    // float rightLift  = (tris_h_band > 0.0) ? (margin_h * (iv_f + er_f) / tris_h_band) : 0.0;
    // float leftLift   = (tris_h_band > 0.0) ? (margin_h * (iv_f + el_f) / tris_h_band) : 0.0;

    float du = (1.0 - leftLift - rightLift) / float(inner_u);
    float dv = (1.0 - bottomLift - topLift) / float(inner_v);

    // 存储到 shared memory
    if (gtid == 0u) {
        s_leftLift = leftLift;
        s_rightLift = rightLift;
        s_bottomLift = bottomLift;
        s_topLift = topLift;
        s_du = du;
        s_dv = dv;
    }
    barrier();

    // 计算当前 tile 处理的内部网格范围（全局索引）
    uint global_u_start = tileX * MAX_TILE_INNER_RATE;
    uint global_v_start = tileY * MAX_TILE_INNER_RATE;
    uint global_u_end = min(global_u_start + MAX_TILE_INNER_RATE, inner_u);
    uint global_v_end = min(global_v_start + MAX_TILE_INNER_RATE, inner_v);

    // 当前 tile 的内部网格大小
    uint tile_u_size = global_u_end - global_u_start;// 内部网格的"格子"数量
    uint tile_v_size = global_v_end - global_v_start;

    // 判断边界
    bool isLeftBoundary   = (tileX == 0u);
    bool isRightBoundary  = (tileX == tilesU - 1u);
    bool isBottomBoundary = (tileY == 0u);
    bool isTopBoundary    = (tileY == tilesV - 1u);

    // tile 内部网格的顶点数
    uint inner_cols = tile_u_size + 1u;
    uint inner_rows = tile_v_size + 1u;
    uint tile_inner_vcnt = inner_cols * inner_rows;

    // 预计算边界信息
    // 策略：每个 tile 负责从 edge_start 到 edge_end 的边界顶点
    // 为了确保所有边段都被覆盖，且不重复：
    // - 非最后一个 tile 的 edge_end 需要扩展以包含与下一个 tile 共享的顶点
    // - 这样边段 (edge_end, edge_end+1) 由当前 tile 负责
    uint bottom_edge_start = 0u, bottom_edge_end = 0u, bottom_edge_cnt = 0u;
    if (isBottomBoundary) {
        float tile_u_lo = leftLift + float(global_u_start) * du;
        float tile_u_hi = leftLift + float(global_u_end) * du;

        if (tileX == 0u) {
            bottom_edge_start = 0u;
        } else {
            // 从上一个 tile 的结束位置开始（不重复生成顶点对应的 I0）
            bottom_edge_start = uint(floor(tile_u_lo * float(e_bottom) + FLOAT_EPSILON));
        }

        if (tileX == tilesU - 1u) {
            bottom_edge_end = e_bottom;
        } else {
            // 扩展到包含下一个顶点，确保边段不丢失
            bottom_edge_end = uint(ceil(tile_u_hi * float(e_bottom) + FLOAT_EPSILON));
        }
        if (bottom_edge_start > bottom_edge_end) bottom_edge_start = bottom_edge_end;
        bottom_edge_cnt = bottom_edge_end - bottom_edge_start + 1u;
    }

    uint right_edge_start = 0u, right_edge_end = 0u, right_edge_cnt = 0u;
    if (isRightBoundary) {
        float tile_v_lo = bottomLift + float(global_v_start) * dv;
        float tile_v_hi = bottomLift + float(global_v_end) * dv;

        if (tileY == 0u) {
            right_edge_start = 0u;
        } else {
            right_edge_start = uint(floor(tile_v_lo * float(e_right) + FLOAT_EPSILON));
        }
        if (tileY == tilesV - 1u) {
            right_edge_end = e_right;
        } else {
            right_edge_end = uint(ceil(tile_v_hi * float(e_right) + FLOAT_EPSILON));
        }
        if (right_edge_start > right_edge_end) right_edge_start = right_edge_end;
        right_edge_cnt = right_edge_end - right_edge_start + 1u;
    }

    uint top_edge_start = 0u, top_edge_end = 0u, top_edge_cnt = 0u;
    if (isTopBoundary) {
        float tile_u_lo = leftLift + float(global_u_start) * du;
        float tile_u_hi = leftLift + float(global_u_end) * du;

        // top 边是从右到左编号的，所以逻辑相反
        if (tileX == tilesU - 1u) {
            top_edge_start = 0u;
        } else {
            top_edge_start = uint(floor((1.0 - tile_u_hi) * float(e_top) + FLOAT_EPSILON));
        }
        if (tileX == 0u) {
            top_edge_end = e_top;
        } else {
            top_edge_end = uint(ceil((1.0 - tile_u_lo) * float(e_top) + FLOAT_EPSILON));
        }
        if (top_edge_start > top_edge_end) top_edge_start = top_edge_end;
        top_edge_cnt = top_edge_end - top_edge_start + 1u;
    }

    uint left_edge_start = 0u, left_edge_end = 0u, left_edge_cnt = 0u;
    if (isLeftBoundary) {
        float tile_v_lo = bottomLift + float(global_v_start) * dv;
        float tile_v_hi = bottomLift + float(global_v_end) * dv;

        // left 边是从上到下编号的，所以逻辑相反
        if (tileY == tilesV - 1u) {
            left_edge_start = 0u;
        } else {
            left_edge_start = uint(floor((1.0 - tile_v_hi) * float(e_left) + FLOAT_EPSILON));
        }
        if (tileY == 0u) {
            left_edge_end = e_left;
        } else {
            left_edge_end = uint(ceil((1.0 - tile_v_lo) * float(e_left) + FLOAT_EPSILON));
        }
        if (left_edge_start > left_edge_end) left_edge_start = left_edge_end;
        left_edge_cnt = left_edge_end - left_edge_start + 1u;
    }

    // 计算顶点和三角形的偏移量
    uint vcnt = tile_inner_vcnt;
    uint bottom_vcnt_base = vcnt;  vcnt += bottom_edge_cnt;
    uint right_vcnt_base  = vcnt;  vcnt += right_edge_cnt;
    uint top_vcnt_base    = vcnt;  vcnt += top_edge_cnt;
    uint left_vcnt_base   = vcnt;  vcnt += left_edge_cnt;

    uint tile_inner_tcnt = tile_u_size * tile_v_size * 2u;
    uint tcnt = tile_inner_tcnt;

    // 边界三角形数量 = I0 + I1
    // I0: edge_cnt - 1 (每条边的边段数) - 但如果边界不在此 tile，则为 0
    // I1: tile_size (内部网格在该边的边段数) - 但如果边界不在此 tile，则为 0
    uint bottom_i0_cnt = (isBottomBoundary && bottom_edge_cnt > 1u) ? (bottom_edge_cnt - 1u) : 0u;
    uint right_i0_cnt  = (isRightBoundary && right_edge_cnt > 1u) ? (right_edge_cnt - 1u) : 0u;
    uint top_i0_cnt    = (isTopBoundary && top_edge_cnt > 1u) ? (top_edge_cnt - 1u) : 0u;
    uint left_i0_cnt   = (isLeftBoundary && left_edge_cnt > 1u) ? (left_edge_cnt - 1u) : 0u;

    uint bottom_i1_cnt = isBottomBoundary ? tile_u_size : 0u;
    uint right_i1_cnt  = isRightBoundary ? tile_v_size : 0u;
    uint top_i1_cnt    = isTopBoundary ? tile_u_size : 0u;
    uint left_i1_cnt   = isLeftBoundary ? tile_v_size : 0u;

    uint bottom_tcnt_base = tcnt;
    uint bottom_i0_base = bottom_tcnt_base;
    uint bottom_i1_base = bottom_i0_base + bottom_i0_cnt;
    tcnt += bottom_i0_cnt + bottom_i1_cnt;

    uint right_tcnt_base = tcnt;
    uint right_i0_base = right_tcnt_base;
    uint right_i1_base = right_i0_base + right_i0_cnt;
    tcnt += right_i0_cnt + right_i1_cnt;

    uint top_tcnt_base = tcnt;
    uint top_i0_base = top_tcnt_base;
    uint top_i1_base = top_i0_base + top_i0_cnt;
    tcnt += top_i0_cnt + top_i1_cnt;

    uint left_tcnt_base = tcnt;
    uint left_i0_base = left_tcnt_base;
    uint left_i1_base = left_i0_base + left_i0_cnt;
    tcnt += left_i0_cnt + left_i1_cnt;

    // ========== 生成 tile 的内部网格顶点 ==========
    for (uint i = gtid; i < tile_inner_vcnt; i += WORKGROUP_SIZE) {
        uint local_u = i % inner_cols;
        uint local_v = i / inner_cols;

        uint global_u = global_u_start + local_u;
        uint global_v = global_v_start + local_v;

        float u = leftLift + float(global_u) * du;
        float v = bottomLift + float(global_v) * dv;

        EmitVertex(i, GetDisplacedVertex(quadVerts, u, v));
    }

    // ========== 生成 tile 的内部网格三角形 ==========
    for (uint i = gtid; i < tile_inner_tcnt; i += WORKGROUP_SIZE) {
        uint quad_idx = i / 2u;
        uint tri_id = i % 2u;

        uint qu = quad_idx % tile_u_size;
        uint qv = quad_idx / tile_u_size;

        uint v00 = qv * inner_cols + qu;
        uint v10 = qv * inner_cols + qu + 1u;
        uint v01 = (qv + 1u) * inner_cols + qu;
        uint v11 = (qv + 1u) * inner_cols + qu + 1u;

        if (tri_id == 0u) {
            EmitPrimitive(i, uvec3(v00, v10, v11));
        } else {
            EmitPrimitive(i, uvec3(v00, v11, v01));
        }
    }

    // ========== 边界顶点和三角形 ==========

    // --- Bottom 边界 (v=0) ---
    if (isBottomBoundary && bottom_edge_cnt > 0u) {
        // 生成边界顶点
        for (uint i = gtid; i < bottom_edge_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = bottom_edge_start + i;
            float u = float(global_edge_idx) / float(e_bottom);
            float v = 0.0;
            EmitVertex(bottom_vcnt_base + i, GetDisplacedVertex(quadVerts, u, v));
        }

        // I0 三角形: 边界边为底，内部点为顶
        for (uint i = gtid; i < bottom_i0_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = bottom_edge_start + i;
            float mid = (float(global_edge_idx) + 0.5) / float(e_bottom);
            uint global_inner_idx = GetNearestIndexI0(leftLift, 1.0 - rightLift, inner_u, mid);

            // clamp to tile range
            uint clamped_inner = clamp(global_inner_idx, global_u_start, global_u_end);
            uint local_inner_idx = clamped_inner - global_u_start;
            uint j0 = bottom_vcnt_base + i;
            uint j1 = bottom_vcnt_base + i + 1u;
            uint j2 = local_inner_idx;
            EmitPrimitive(bottom_i0_base + i, uvec3(j0, j1, j2));
        }

        // I1 三角形: 内部边为底，边界点为顶
        for (uint i = gtid; i < bottom_i1_cnt; i += WORKGROUP_SIZE) {
            uint global_inner_idx = global_u_start + i;
            float mid = leftLift + (float(global_inner_idx) + 0.5) * du;
            uint global_edge_idx = GetNearestIndexI1(0.0, 1.0, e_bottom, mid);

            // clamp to edge range
            uint clamped_edge = clamp(global_edge_idx, bottom_edge_start, bottom_edge_end);
            uint local_edge_idx = clamped_edge - bottom_edge_start;
            uint j0 = i;
            uint j1 = i + 1u;
            uint j2 = bottom_vcnt_base + local_edge_idx;
            EmitPrimitive(bottom_i1_base + i, uvec3(j2, j1, j0));
        }
    }

    // --- Right 边界 (u=1) ---
    if (isRightBoundary && right_edge_cnt > 0u) {
        for (uint i = gtid; i < right_edge_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = right_edge_start + i;
            float u = 1.0;
            float v = float(global_edge_idx) / float(e_right);
            EmitVertex(right_vcnt_base + i, GetDisplacedVertex(quadVerts, u, v));
        }

        for (uint i = gtid; i < right_i0_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = right_edge_start + i;
            float mid = (float(global_edge_idx) + 0.5) / float(e_right);
            uint global_inner_idx = GetNearestIndexI0(bottomLift, 1.0 - topLift, inner_v, mid);

            uint clamped_inner = clamp(global_inner_idx, global_v_start, global_v_end);
            uint local_inner_idx = clamped_inner - global_v_start;
            uint j0 = right_vcnt_base + i;
            uint j1 = right_vcnt_base + i + 1u;
            uint j2 = (local_inner_idx + 1u) * inner_cols - 1u;
            EmitPrimitive(right_i0_base + i, uvec3(j0, j1, j2));
        }

        for (uint i = gtid; i < right_i1_cnt; i += WORKGROUP_SIZE) {
            uint global_inner_idx = global_v_start + i;
            float mid = bottomLift + (float(global_inner_idx) + 0.5) * dv;
            uint global_edge_idx = GetNearestIndexI1(0.0, 1.0, e_right, mid);

            uint clamped_edge = clamp(global_edge_idx, right_edge_start, right_edge_end);
            uint local_edge_idx = clamped_edge - right_edge_start;
            uint j0 = (i + 1u) * inner_cols - 1u;
            uint j1 = (i + 2u) * inner_cols - 1u;
            uint j2 = right_vcnt_base + local_edge_idx;
            EmitPrimitive(right_i1_base + i, uvec3(j2, j1, j0));
        }
    }

    // --- Top 边界 (v=1) ---
    if (isTopBoundary && top_edge_cnt > 0u) {
        for (uint i = gtid; i < top_edge_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = top_edge_start + i;
            float u = 1.0 - float(global_edge_idx) / float(e_top);
            float v = 1.0;
            EmitVertex(top_vcnt_base + i, GetDisplacedVertex(quadVerts, u, v));
        }

        for (uint i = gtid; i < top_i0_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = top_edge_start + i;
            float mid = (float(global_edge_idx) + 0.5) / float(e_top);
            // top 边从右到左编号，mid 在反向参数空间中，lo/hi 需要对应反向
            uint global_inner_idx = GetNearestIndexI0(rightLift, 1.0 - leftLift, inner_u, mid);
            uint global_u = inner_u - global_inner_idx;

            uint clamped_u = clamp(global_u, global_u_start, global_u_end);
            uint local_u = clamped_u - global_u_start;
            uint j0 = top_vcnt_base + i;
            uint j1 = top_vcnt_base + i + 1u;
            uint j2 = tile_v_size * inner_cols + local_u;
            EmitPrimitive(top_i0_base + i, uvec3(j0, j1, j2));
        }

        for (uint i = gtid; i < top_i1_cnt; i += WORKGROUP_SIZE) {
            uint reversed_local = tile_u_size - 1u - i;
            uint global_inner_idx = global_u_start + reversed_local;
            float mid = rightLift + (float(inner_u - 1u - global_inner_idx) + 0.5) * du;
            uint global_edge_idx = GetNearestIndexI1(0.0, 1.0, e_top, mid);

            uint clamped_edge = clamp(global_edge_idx, top_edge_start, top_edge_end);
            uint local_edge_idx = clamped_edge - top_edge_start;
            uint local_u_from_right = tile_u_size - i;
            uint j0 = tile_v_size * inner_cols + local_u_from_right;
            uint j1 = tile_v_size * inner_cols + local_u_from_right - 1u;
            uint j2 = top_vcnt_base + local_edge_idx;
            EmitPrimitive(top_i1_base + i, uvec3(j2, j1, j0));
        }
    }

    // --- Left 边界 (u=0) ---
    if (isLeftBoundary && left_edge_cnt > 0u) {
        for (uint i = gtid; i < left_edge_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = left_edge_start + i;
            float u = 0.0;
            float v = 1.0 - float(global_edge_idx) / float(e_left);
            EmitVertex(left_vcnt_base + i, GetDisplacedVertex(quadVerts, u, v));
        }

        for (uint i = gtid; i < left_i0_cnt; i += WORKGROUP_SIZE) {
            uint global_edge_idx = left_edge_start + i;
            float mid = (float(global_edge_idx) + 0.5) / float(e_left);
            // left 边从上到下编号，mid 在反向参数空间中，lo/hi 需要对应反向
            uint global_inner_idx = GetNearestIndexI0(topLift, 1.0 - bottomLift, inner_v, mid);
            uint global_v = inner_v - global_inner_idx;

            uint clamped_v = clamp(global_v, global_v_start, global_v_end);
            uint local_v = clamped_v - global_v_start;
            uint j0 = left_vcnt_base + i;
            uint j1 = left_vcnt_base + i + 1u;
            uint j2 = local_v * inner_cols;
            EmitPrimitive(left_i0_base + i, uvec3(j0, j1, j2));
        }

        for (uint i = gtid; i < left_i1_cnt; i += WORKGROUP_SIZE) {
            uint reversed_local = tile_v_size - 1u - i;
            uint global_inner_idx = global_v_start + reversed_local;
            float mid = topLift + (float(inner_v - 1u - global_inner_idx) + 0.5) * dv;
            uint global_edge_idx = GetNearestIndexI1(0.0, 1.0, e_left, mid);

            uint clamped_edge = clamp(global_edge_idx, left_edge_start, left_edge_end);
            uint local_edge_idx = clamped_edge - left_edge_start;
            uint local_v_from_top = tile_v_size - i;
            uint j0 = local_v_from_top * inner_cols;
            uint j1 = (local_v_from_top - 1u) * inner_cols;
            uint j2 = left_vcnt_base + local_edge_idx;
            EmitPrimitive(left_i1_base + i, uvec3(j2, j1, j0));
        }
    }

    if (gtid == 0u) {
        gl_PrimitiveCountNV = tcnt;
    }
}

void EmitVertex(uint vertexId, VertexData vd) {
    vec4 worldPos = SceneData.u_Transform * vec4(vd.position, 1.0);
    vec4 viewPos = SceneData.u_View * worldPos;
    vec4 clipPos  = SceneData.u_Projection * viewPos;

    gl_MeshVerticesNV[vertexId].gl_Position = clipPos;

    v_out[vertexId].mcPosition  = worldPos.xyz;
    v_out[vertexId].vcPosition  = viewPos.xyz;
    v_out[vertexId].texcoord    = vd.texcoord;

    // Displacement Mapping
    ivec2 base = ivec2(floor(vd.texcoord));
    vec2 f = fract(vd.texcoord);

    float s00 = texelFetch(u_DisplaceMap, base, 0).r;
    float s10 = texelFetch(u_DisplaceMap, base + ivec2(1, 0), 0).r;
    float s01 = texelFetch(u_DisplaceMap, base + ivec2(0, 1), 0).r;
    float s11 = texelFetch(u_DisplaceMap, base + ivec2(1, 1), 0).r;

    float sx0 = mix(s00, s10, f.x);
    float sx1 = mix(s01, s11, f.x);
    float disp = mix(sx0, sx1, f.y);

    v_out[vertexId].height = disp;
}

void EmitPrimitive(uint primitiveId, uvec3 indices) {
    gl_PrimitiveIndicesNV[3u * primitiveId + 0u] = indices.x;
    gl_PrimitiveIndicesNV[3u * primitiveId + 1u] = indices.y;
    gl_PrimitiveIndicesNV[3u * primitiveId + 2u] = indices.z;
}

VertexData GetInputVertex(uint vertexId) {
    VertexData vd;

    vd.position = vec3(
    positionBuffer.data[vertexId * 3u + 0u],
    positionBuffer.data[vertexId * 3u + 1u],
    positionBuffer.data[vertexId * 3u + 2u]);

    vd.normal = vec3(
    normalBuffer.data[vertexId * 3u + 0u],
    normalBuffer.data[vertexId * 3u + 1u],
    normalBuffer.data[vertexId * 3u + 2u]);

    vd.texcoord = vec2(
    texcoordBuffer.data[vertexId * 2u + 0u],
    texcoordBuffer.data[vertexId * 2u + 1u]);

    return vd;
}

VertexData BilinearQuad(VertexData v0, VertexData v1, VertexData v2, VertexData v3, float u, float v) {
    float w00 = (1.0 - u) * (1.0 - v);
    float w10 =        u  * (1.0 - v);
    float w11 =        u  *        v;
    float w01 = (1.0 - u) *        v;

    VertexData outV;
    outV.position = v0.position * w00 + v1.position * w10 + v2.position * w11 + v3.position * w01;

    outV.normal = v0.normal * w00 + v1.normal * w10 + v2.normal * w11 + v3.normal * w01;
    outV.normal = normalize(outV.normal);

    outV.texcoord = v0.texcoord * w00 + v1.texcoord * w10 + v2.texcoord * w11 + v3.texcoord * w01;
    return outV;
}

uint GetNearestIndexI0(float lo, float hi, uint rate, float x) {
    float t = (x - lo) * float(rate) / (hi - lo) + FLOAT_EPSILON;
    int id = int(round(t));
    return uint(clamp(id, 0, int(rate)));
}

uint GetNearestIndexI1(float lo, float hi, uint rate, float x) {
    float t = (x - lo) * float(rate) / (hi - lo) - FLOAT_EPSILON;
    int id = int(round(t));
    return uint(clamp(id, 0, int(rate)));
}

VertexData GetDisplacedVertex(VertexData quadVerts[4], float u, float v) {
    VertexData vd = BilinearQuad(quadVerts[0], quadVerts[1], quadVerts[2], quadVerts[3], u, v);

    // Displacement Mapping
    ivec2 base = ivec2(floor(vd.texcoord));
    vec2 f = fract(vd.texcoord);

    // Sample displacement values from the four neighboring texels
    float s00 = texelFetch(u_DisplaceMap, base, 0).r;
    float s10 = texelFetch(u_DisplaceMap, base + ivec2(1, 0), 0).r;
    float s01 = texelFetch(u_DisplaceMap, base + ivec2(0, 1), 0).r;
    float s11 = texelFetch(u_DisplaceMap, base + ivec2(1, 1), 0).r;

    // Perform bilinear interpolation
    float sx0 = mix(s00, s10, f.x);
    float sx1 = mix(s01, s11, f.x);
    float disp = mix(sx0, sx1, f.y);
    vd.position += vd.normal * disp;

    return vd;
}
