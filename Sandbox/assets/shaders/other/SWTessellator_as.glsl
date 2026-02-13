#version 460

#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#define WORKGROUP_SIZE 32

#define DISTANCE_BASED_TESSELLATION 1
#define SCREEN_SPACE_ADAPTIVE_TESSELLATION 2
#define NORMAL_BASED_TESSELLATION 3
#define NEURAL_TESSELLATION 4
#define LUT_TESSELLATION 5

// 每个 tile 允许的最大内部细分率
// 对于 max_vertices=256, max_primitives=512
// 使用 8x8 网格: 64 顶点 + 边界顶点 ~= 128, 安全
#define MAX_TILE_INNER_RATE 7u

layout(local_size_x = WORKGROUP_SIZE) in;

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

// =============================================================================
// Helper functions
// =============================================================================

vec3 GetPosition(uint vertexId) {
    return vec3(
    positionBuffer.data[vertexId * 3u + 0u],
    positionBuffer.data[vertexId * 3u + 1u],
    positionBuffer.data[vertexId * 3u + 2u]
    );
}

vec3 GetNormal(uint vertexId) {
    return vec3(
    normalBuffer.data[vertexId * 3u + 0u],
    normalBuffer.data[vertexId * 3u + 1u],
    normalBuffer.data[vertexId * 3u + 2u]
    );
}

// =============================================================================
// Distance-Based Tessellation
// =============================================================================
float u_MinTess = 1.0;
float u_MaxTess = 5.0;

float GetTessLevelByDistance(vec3 v0, vec3 v1) {
    // 1. 计算边的中心点 (世界空间)
    vec3 edgeCenter = (v0 + v1) * 0.5;

    // 2. 计算摄像机到边中心的距离
    float dist = distance(edgeCenter, TessellatorData.u_ViewPos);

    // 3. 计算归一化距离因子 t (0.0 = 近, 1.0 = 远)
    // clamp 确保距离在 [min, max] 范围内
    float t = clamp((dist - TessellatorData.u_MinDist) / (TessellatorData.u_MaxDist - TessellatorData.u_MinDist), 0.0, 1.0);

    // 4. 线性插值 (mix)
    // 注意：距离越近(t=0)，细分应越高；距离越远(t=1)，细分应越低
    return mix(u_MaxTess, u_MinTess, t);
}

// =============================================================================
// Screen-Space Adaptive / Edge Length
// =============================================================================
vec2 WorldToScreen(vec3 modelPos) {
    vec4 clipPos = TessellatorData.u_Projection * TessellatorData.u_View * TessellatorData.u_Model * vec4(modelPos, 1.0);

    // 透视除法
    vec3 ndc = clipPos.xyz / clipPos.w;

    // NDC [-1, 1] 转换到屏幕像素坐标 [0, screenSize]
    vec2 screenPos;
    screenPos.x = (ndc.x * 0.5 + 0.5) * float(TessellatorData.u_ScreenSize.x);
    screenPos.y = (ndc.y * 0.5 + 0.5) * float(TessellatorData.u_ScreenSize.y);

    return screenPos;
}

// 计算边在屏幕空间中的长度（像素）
float ComputeScreenSpaceEdgeLength(vec3 v0, vec3 v1) {
    vec2 screen0 = WorldToScreen(v0);
    vec2 screen1 = WorldToScreen(v1);

    //    return clamp(length(screen1 - screen0) / TessellatorData.u_TargetPixel, u_MinTess, u_MaxTess);
    return length(screen1 - screen0) / TessellatorData.u_TargetPixel;
}

// =============================================================================
// Normal-Based Tessellation
// =============================================================================
float GetTessLevelByNormal(vec3 n0, vec3 n1) {
    // 1. 计算两个法线的点积
    // dot结果: 1.0 表示平行(平面), < 1.0 表示有夹角(曲面)
    // 也就是 cos(theta)
    float cosTheta = dot(normalize(n0), normalize(n1));

    // 2. 将点积转换为角度 (弧度)
    // clamp 防止浮点误差导致 acos 输入越界
    float angle = acos(clamp(cosTheta, -1.0, 1.0));

    // 3. 计算归一化曲率因子 t
    // 角度越大，t 越接近 1.0 (需要高细分)
    float t = clamp(angle * 3.1415926 * TessellatorData.u_MaxCurvature, 0.0, 1.0);

    // 4. 线性插值
    // 曲率越小(t=0)，用 MinTess；曲率越大(t=1)，用 MaxTess
    return mix(u_MinTess, u_MaxTess, t);
}

// =============================================================================
// Neural Tessellation Field
// =============================================================================
layout(std430, binding = 9) readonly buffer QuadEdgeIdBuffer {
    uvec4 data[];
} quadEdgeIdBuffer;

layout(std430, binding = 10) /*readonly*/ buffer EdgeTessFactorBuffer {
    uint data[];
} edgeTessFactorBuffer;

layout(std430, binding = 11) buffer InnerTessFactorBuffer {
    uvec2 data[];
} innerTessFactorBuffer;

// 传递给 Mesh Shader 的数据
taskNV out Task {
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
} task_out;

void main()
{
    uint gid = gl_WorkGroupID.x;
    uint gtid = gl_LocalInvocationID.x;
    uint globalID = gl_GlobalInvocationID.x;

    // 每个 task shader 工作组处理一个 quad
    uint quadId = gid;
    if (quadId >= TessellatorData.u_QuadSize) {
        return;
    }

    uvec2 innerFactors = uvec2(1u, 1u);
    uvec4 edgeFactors = uvec4(1u, 1u, 1u, 1u);
    uvec4 edgeIds = quadEdgeIdBuffer.data[quadId];
    if (TessellatorData.u_TessllationMode == DISTANCE_BASED_TESSELLATION) {
        // 获取 quad 的四个顶点位置
        // Quad 顶点布局:
        //   v3 ---- v2
        //   |       |
        //   |       |
        //   v0 ---- v1
        vec3 v0 = GetPosition(quadIndexBuffer.data[quadId * 4u + 0u]);
        vec3 v1 = GetPosition(quadIndexBuffer.data[quadId * 4u + 1u]);
        vec3 v2 = GetPosition(quadIndexBuffer.data[quadId * 4u + 2u]);
        vec3 v3 = GetPosition(quadIndexBuffer.data[quadId * 4u + 3u]);

        edgeFactors.x = max(uint(round(GetTessLevelByDistance(v0, v1))), 1u);
        edgeFactors.y = max(uint(round(GetTessLevelByDistance(v1, v2))), 1u);
        edgeFactors.z = max(uint(round(GetTessLevelByDistance(v3, v2))), 1u);
        edgeFactors.w = max(uint(round(GetTessLevelByDistance(v0, v3))), 1u);

        innerFactors.x = max((edgeFactors.x + edgeFactors.z) / 2u, 1u);
        innerFactors.y = max((edgeFactors.y + edgeFactors.w) / 2u, 1u);
    } else if (TessellatorData.u_TessllationMode == SCREEN_SPACE_ADAPTIVE_TESSELLATION) {
        vec3 v0 = GetPosition(quadIndexBuffer.data[quadId * 4u + 0u]);
        vec3 v1 = GetPosition(quadIndexBuffer.data[quadId * 4u + 1u]);
        vec3 v2 = GetPosition(quadIndexBuffer.data[quadId * 4u + 2u]);
        vec3 v3 = GetPosition(quadIndexBuffer.data[quadId * 4u + 3u]);

        edgeFactors.x = max(uint(round(ComputeScreenSpaceEdgeLength(v0, v1))), 1u);
        edgeFactors.y = max(uint(round(ComputeScreenSpaceEdgeLength(v1, v2))), 1u);
        edgeFactors.z = max(uint(round(ComputeScreenSpaceEdgeLength(v3, v2))), 1u);
        edgeFactors.w = max(uint(round(ComputeScreenSpaceEdgeLength(v0, v3))), 1u);

        innerFactors.x = max((edgeFactors.x + edgeFactors.z) / 2u, 1u);
        innerFactors.y = max((edgeFactors.y + edgeFactors.w) / 2u, 1u);
    } else if (TessellatorData.u_TessllationMode == NORMAL_BASED_TESSELLATION) {
        vec3 n0 = GetNormal(quadIndexBuffer.data[quadId * 4u + 0u]);
        vec3 n1 = GetNormal(quadIndexBuffer.data[quadId * 4u + 1u]);
        vec3 n2 = GetNormal(quadIndexBuffer.data[quadId * 4u + 2u]);
        vec3 n3 = GetNormal(quadIndexBuffer.data[quadId * 4u + 3u]);

        edgeFactors.x = max(uint(round(GetTessLevelByNormal(n0, n1))), 1u);
        edgeFactors.y = max(uint(round(GetTessLevelByNormal(n1, n2))), 1u);
        edgeFactors.z = max(uint(round(GetTessLevelByNormal(n3, n2))), 1u);
        edgeFactors.w = max(uint(round(GetTessLevelByNormal(n0, n3))), 1u);

        innerFactors.x = max((edgeFactors.x + edgeFactors.z) / 2u, 1u);
        innerFactors.y = max((edgeFactors.y + edgeFactors.w) / 2u, 1u);
    } else if (TessellatorData.u_TessllationMode == NEURAL_TESSELLATION || TessellatorData.u_TessllationMode == LUT_TESSELLATION) {
        // 读取边细分因子
        edgeFactors.x = uint(round(edgeTessFactorBuffer.data[edgeIds.x] / 2));
        edgeFactors.y = uint(round(edgeTessFactorBuffer.data[edgeIds.y] / 2));
        edgeFactors.z = uint(round(edgeTessFactorBuffer.data[edgeIds.z] / 2));
        edgeFactors.w = uint(round(edgeTessFactorBuffer.data[edgeIds.w] / 2));

        // 读取内部细分因子
        innerFactors.x = innerTessFactorBuffer.data[quadId].x;
        innerFactors.y = innerTessFactorBuffer.data[quadId].y;
    } else {
        return;
    }

    //Testing: 将计算结果写回缓冲区
    if (TessellatorData.u_TessllationMode != NEURAL_TESSELLATION && TessellatorData.u_TessllationMode != LUT_TESSELLATION) {
        edgeTessFactorBuffer.data[edgeIds.x] = edgeFactors.x;
        edgeTessFactorBuffer.data[edgeIds.y] = edgeFactors.y;
        edgeTessFactorBuffer.data[edgeIds.z] = edgeFactors.z;
        edgeTessFactorBuffer.data[edgeIds.w] = edgeFactors.w;
        innerTessFactorBuffer.data[quadId] = uvec2(innerFactors.x, innerFactors.y);
    }

    // 计算需要多少个 tile 来覆盖内部区
    uint tilesU = (innerFactors.x + MAX_TILE_INNER_RATE - 1u) / MAX_TILE_INNER_RATE;
    uint tilesV = (innerFactors.y + MAX_TILE_INNER_RATE - 1u) / MAX_TILE_INNER_RATE;
    uint totalTiles = tilesU * tilesV;

    // 启动对应数量的 mesh shader 工作组
    if (gtid == 0u) {
        gl_TaskCountNV = totalTiles;

        task_out.quadId = quadId;
        task_out.innerU = innerFactors.x;
        task_out.innerV = innerFactors.y;
        task_out.tilesU = tilesU;
        task_out.tilesV = tilesV;
        task_out.edgeBottom = edgeFactors.x;
        task_out.edgeRight = edgeFactors.y;
        task_out.edgeTop = edgeFactors.z;
        task_out.edgeLeft = edgeFactors.w;
    }
}
