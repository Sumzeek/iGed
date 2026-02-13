#version 460

#extension GL_NV_gpu_shader5 : require

#define WORKGROUP_SIZE 32
#define FLOAT_EPSILON 0.000001

layout(local_size_x = WORKGROUP_SIZE) in;

// Scene data for computing adaptive epsilon
layout(binding = 0, std140) uniform SceneDataBlock_std140 {
    mat4 u_View;
    mat4 u_Projection;
    mat4 u_ProjectionView;
    mat4 u_Transform;
} SceneData;

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

// Vertex information
layout(std430, binding = 5) readonly buffer PositionBuffer {
    float data[];// vec3
} positionBuffer;

layout(std430, binding = 8) readonly buffer QuadIndexBuffer {
    uint data[];
} quadIndexBuffer;

// Edge mapping buffer: maps each quad's 4 edges to global edge IDs
layout(std430, binding = 9) readonly buffer QuadEdgeIdBuffer {
    uvec4 data[];
} quadEdgeIdBuffer;

// Output: per-edge accumulated tessellation factors
layout(std430, binding = 10) buffer EdgeTessFactorBuffer {
    uint data[];
} edgeTessFactorBuffer;

layout(std430, binding = 11) buffer InnerTessFactorBuffer {
    uvec2 data[];
} innerTessFactorBuffer;

// LUT buffers
// LUTEntry: (error, inner_factor, outer_factor, padding) - 16 bytes each
layout(std430, binding = 15) readonly buffer LUTEntriesBuffer {
    vec4 data[];  // x=error, y=inner_factor, z=outer_factor, w=padding
} lutEntriesBuffer;

// QuadLUTInfo: (start_index, entry_count, padding0, padding1) - 16 bytes each
layout(std430, binding = 16) readonly buffer QuadLUTInfoBuffer {
    uvec4 data[];  // x=start_index, y=entry_count, z=padding, w=padding
} quadLUTInfoBuffer;

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

float compute_adaptive_epsilon(vec3 v0, vec3 v1, vec3 v2, vec3 v3) {
    // 将四个顶点从模型空间变换到相机空间
    vec4 view0 = TessellatorData.u_View * TessellatorData.u_Model * vec4(v0, 1.0);
    vec4 view1 = TessellatorData.u_View * TessellatorData.u_Model * vec4(v1, 1.0);
    vec4 view2 = TessellatorData.u_View * TessellatorData.u_Model * vec4(v2, 1.0);
    vec4 view3 = TessellatorData.u_View * TessellatorData.u_Model * vec4(v3, 1.0);

    // 在相机空间中，相机看向 -Z 方向
    float dist0 = abs(view0.z);
    float dist1 = abs(view1.z);
    float dist2 = abs(view2.z);
    float dist3 = abs(view3.z);

    float d_min = min(min(dist0, dist1), min(dist2, dist3));

    const float MIN_DISTANCE = 0.01;
    d_min = max(d_min, MIN_DISTANCE);

    float tanHalfFov = 1.0 / TessellatorData.u_Projection[1][1];
    float screenHeight = float(TessellatorData.u_ScreenSize.y);
    float pixel_world_size = (2.0 * d_min * tanHalfFov) / screenHeight;

    return pixel_world_size;
}

// =============================================================================
// LUT Lookup with Binary Search and Interpolation
// =============================================================================

// 在给定quad的LUT条目中进行二分查找和插值
// 返回 vec2(inner_factor, outer_factor)
vec2 lut_lookup(uint quadId, float epsilon) {
    uvec4 quadInfo = quadLUTInfoBuffer.data[quadId];
    uint startIndex = quadInfo.x;
    uint entryCount = quadInfo.y;
    
    // 边界情况：只有一个条目
    if (entryCount <= 1) {
        vec4 entry = lutEntriesBuffer.data[startIndex];
        return vec2(entry.y, entry.z);  // inner_factor, outer_factor
    }
    
    // 获取第一个和最后一个条目用于边界检查
    vec4 firstEntry = lutEntriesBuffer.data[startIndex];
    vec4 lastEntry = lutEntriesBuffer.data[startIndex + entryCount - 1];
    
    // 边界情况：epsilon小于最小error -> 返回最高细分（最小error对应的因子）
    if (epsilon <= firstEntry.x) {
        return vec2(firstEntry.y, firstEntry.z);
    }
    
    // 边界情况：epsilon大于最大error -> 返回最低细分（最大error对应的因子）
    if (epsilon >= lastEntry.x) {
        return vec2(lastEntry.y, lastEntry.z);
    }
    
    // 二分查找：找到epsilon所在的区间 [entries[lo], entries[hi]]
    uint lo = 0;
    uint hi = entryCount - 1;
    
    while (hi - lo > 1) {
        uint mid = (lo + hi) / 2;
        vec4 midEntry = lutEntriesBuffer.data[startIndex + mid];
        
        if (epsilon < midEntry.x) {
            hi = mid;
        } else {
            lo = mid;
        }
    }
    
    // 获取相邻的两个条目进行插值
    vec4 entryLo = lutEntriesBuffer.data[startIndex + lo];
    vec4 entryHi = lutEntriesBuffer.data[startIndex + hi];
    
    // 线性插值因子
    float errorRange = entryHi.x - entryLo.x;
    float t = 0.5;  // 默认值
    if (errorRange > FLOAT_EPSILON) {
        t = (epsilon - entryLo.x) / errorRange;
    }
    t = clamp(t, 0.0, 1.0);
    
    // 线性插值 inner_factor 和 outer_factor
    float inner_factor = mix(entryLo.y, entryHi.y, t);
    float outer_factor = mix(entryLo.z, entryHi.z, t);
    
    return vec2(inner_factor, outer_factor);
}

// =============================================================================
// Main
// =============================================================================

void main() {
    uint gid = gl_WorkGroupID.x;
    uint gtid = gl_LocalInvocationID.x;
    uint globalID = gl_GlobalInvocationID.x;

    // Bounds check
    uint quadId = globalID;
    if (quadId >= TessellatorData.u_QuadSize) {
        return;
    }

    // Get quad vertex positions
    vec3 v0 = GetPosition(quadIndexBuffer.data[quadId * 4u + 0u]);
    vec3 v1 = GetPosition(quadIndexBuffer.data[quadId * 4u + 1u]);
    vec3 v2 = GetPosition(quadIndexBuffer.data[quadId * 4u + 2u]);
    vec3 v3 = GetPosition(quadIndexBuffer.data[quadId * 4u + 3u]);

    // Compute adaptive epsilon
    float epsilon = compute_adaptive_epsilon(v0, v1, v2, v3);
    epsilon *= TessellatorData.u_EpsilonCoefficient;

    // Lookup tessellation factors from LUT with interpolation
    vec2 factors = lut_lookup(quadId, epsilon);

    // Round to nearest integer for tessellation
    uint inner_rate = uint(round(factors.x));
    uint outter_rate = uint(round(factors.y));

    // Ensure minimum value of 1
    inner_rate = max(inner_rate, 1u);
    outter_rate = max(outter_rate, 1u);

    // Get the edge IDs for this quad
    uvec4 edgeIds = quadEdgeIdBuffer.data[quadId];

    // All 4 edges use the same outter_rate
    // Atomically accumulate tessellation factors to edge buffer
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.x], outter_rate);
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.y], outter_rate);
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.z], outter_rate);
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.w], outter_rate);

    // Inner tessellation uses inner_rate for both U and V
    innerTessFactorBuffer.data[quadId] = uvec2(inner_rate, inner_rate);
}
