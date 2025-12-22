#version 460

#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : require

#define WORKGROUP_SIZE 32
#define MAX_VERTEX_COUNT 256
#define MAX_PRIMITIVE_COUNT 512
#define FLOAT_EPSILON 0.000001

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
    uint u_QuadSize;
    uint u_LineOption;
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
    uint taskGroupID;
    uint taskGroupSize;
} task_in;

// out for fargment shader
out PerVertexData {
    vec3 mcPosition;
    vec3 vcPosition;
    vec2 texcoord;
    vec3 barycentric;
} v_out[];

// use for tessellation
struct VertexData {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
};

shared VertexData sVertices[MAX_VERTEX_COUNT];
shared uvec3      sIndices[MAX_PRIMITIVE_COUNT];

VertexData GetInputVertex(uint vertexId);
uint GenerateBoundarySamples(uint rate, float t_min, float t_max, out float samples[256]);
uint GetNearestIndexI0(float lo, float hi, uint rate, float x);
uint GetNearestIndexI1(float lo, float hi, uint rate, float x);
VertexData GetDisplacedVertex(VertexData quadVerts[4], float u, float v);

void main()
{
    uint gid = gl_WorkGroupID.x;
    uint gtid = gl_LocalInvocationID.x;
    uint quadId = task_in.taskGroupID * task_in.taskGroupSize + gid;

    if (quadId >= TessellatorData.u_QuadSize) {
        return;
    }

    // Composite quad vertices
    VertexData quadVerts[4];
    quadVerts[0] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 0u]);
    quadVerts[1] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 1u]);
    quadVerts[2] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 2u]);
    quadVerts[3] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 3u]);

    // edge tess factors: bottom, right, top, left
    const uvec4 edge = uvec4(5, 5, 5, 5);

    uint e_bottom = edge.x;
    uint e_right  = edge.y;
    uint e_top    = edge.z;
    uint e_left   = edge.w;
    uint inner_u  = (e_bottom + e_top)  / 2u;
    uint inner_v  = (e_right  + e_left) / 2u;

    float denom = float(e_bottom + e_right + e_top + e_left) + 2.0 * float(inner_u + inner_v);
    float t = (denom > 0.0) ? (2.0 * float(inner_u * inner_v) / denom) : 0.0;
    float lift = (t > -1.0) ? ((1.0 - sqrt(max(0.0, 1.0 - 1.0 / (t + 1.0)))) * 0.5) : 0.0;

    float bottomLift = lift;
    float rightLift  = lift;
    float topLift    = lift;
    float leftLift   = lift;

    float du = (1.0 - leftLift - rightLift) / float(inner_u);
    float dv = (1.0 - bottomLift - topLift) / float(inner_v);

    // ---------- inner region ----------
    uint inner_vcnt = (inner_u + 1u) * (inner_v + 1u);
    for (uint i = gtid; i < inner_vcnt; i += WORKGROUP_SIZE) {
        uint ju = i % (inner_u + 1u);
        uint jv = i / (inner_u + 1u);

        float u = leftLift + float(ju) * du;
        float v = bottomLift + float(jv) * dv;

        sVertices[i] = GetDisplacedVertex(quadVerts, u, v);
    }

    uint inner_tcnt = inner_u * inner_v * 2;
    for (uint i = gtid; i < inner_tcnt; i += WORKGROUP_SIZE) {
        uint quad_id = i / 2u;
        uint tri_id  = i % 2u;

        uint qu = quad_id % inner_u;
        uint qv = quad_id / inner_u;

        uint row = inner_u + 1u;

        uint v00 = qv       * row + qu;
        uint v10 = qv       * row + qu + 1u;
        uint v01 = (qv + 1) * row + qu;
        uint v11 = (qv + 1) * row + qu + 1u;

        uint base = i * 3u;

        // CCW
        if (tri_id == 0u) {
            sIndices[i] = uvec3(v00, v10, v11);
        } else {
            sIndices[i] = uvec3(v00, v11, v01);
        }
    }

    // ---------- boundary regions ----------
    uint edge_vcnt = e_bottom + e_right + e_top + e_left;
    for (uint i = gtid; i < edge_vcnt; i += WORKGROUP_SIZE) {
        float u, v;
        uint local;

        if (i < e_bottom) {
            local = i;
            u = float(local) / float(e_bottom);
            v = 0.0;
        } else if (i < e_bottom + e_right) {
            local = i - e_bottom;
            u = 1.0;
            v = float(local) / float(e_right);
        } else if (i < e_bottom + e_right + e_top) {
            local = i - (e_bottom + e_right);
            u = 1.0 - float(local) / float(e_top);
            v = 1.0;
        } else {
            local = i - (e_bottom + e_right + e_top);
            u = 0.0;
            v = 1.0 - float(local) / float(e_left);
        }

        sVertices[inner_vcnt + i] = GetDisplacedVertex(quadVerts, u, v);
    }

    // I0: edge as base, apex on inner line
    uint edge_tcnt0 = e_bottom + e_right + e_top + e_left;
    for (uint i = gtid; i < edge_tcnt0; i += WORKGROUP_SIZE) {
        uint local, id;
        float mid;

        uint j0, j1, j2;
        j0 = inner_vcnt + i;
        j1 = inner_vcnt + ((i + 1u) % edge_vcnt);

        if (i < e_bottom) {
            local = i;
            mid = (float(local) + 0.5) / (float(e_bottom));
            id = GetNearestIndexI0(leftLift, 1.0 - rightLift, inner_u, mid);
            j2 = id;
        } else if (i < e_bottom + e_right) {
            local = i - e_bottom;
            mid = (float(local) + 0.5) / float(e_right);
            id = GetNearestIndexI0(bottomLift, 1.0 - topLift, inner_v, mid);
            j2 = (inner_u + 1u) * (id + 1u) - 1u;
        } else if (i < e_bottom + e_right + e_top) {
            local = i - (e_bottom + e_right);
            mid = (float(local) + 0.5) / float(e_top);
            id = GetNearestIndexI0(leftLift, 1.0 - rightLift, inner_u, mid);
            j2 = (inner_u + 1u) * (inner_v + 1u) - 1u - id;
        } else {
            local = i - (e_bottom + e_right + e_top);
            mid = (float(local) + 0.5) / float(e_left);
            id = GetNearestIndexI0(bottomLift, 1.0 - topLift, inner_v, mid);
            j2 = (inner_u + 1u) * (inner_v - id);
        }

        sIndices[inner_tcnt + i] = uvec3(j0, j1, j2);
    }

    // I1: inner line as base, apex on edge
    uint edge_tcnt1 = inner_u * 2u + inner_v * 2u;
    for (uint i = gtid; i < edge_tcnt1; i += WORKGROUP_SIZE) {
        uint local, id;
        float mid;

        uint j0, j1, j2;

        if (i < inner_u) {
            local = i;
            mid = leftLift + (local + 0.5) * du;
            id = GetNearestIndexI1(0.0, 1.0, e_bottom, mid);

            j0 = local;
            j1 = local + 1u;
            j2 = inner_vcnt + id;
        } else if (i < inner_u + inner_v) {
            local = i - inner_u;
            mid = bottomLift + (local + 0.5) * dv;
            id = GetNearestIndexI1(0.0, 1.0, e_right, mid);

            j0 = (inner_u + 1u) * (local + 1u) - 1u;
            j1 = (inner_u + 1u) * (local + 2u) - 1u;
            j2 = inner_vcnt + e_bottom + id;
        } else if (i < 2u * inner_u + inner_v) {
            local = i - (inner_u + inner_v);
            mid = rightLift + (local + 0.5) * du;
            id = GetNearestIndexI1(0.0, 1.0, e_top, mid);

            j0 = (inner_u + 1u) * (inner_v + 1u) - 1u - local;
            j1 = (inner_u + 1u) * (inner_v + 1u) - 2u - local;
            j2 = inner_vcnt + e_bottom + e_right + id;
        } else {
            local = i - (2u * inner_u + inner_v);
            mid = topLift + (local + 0.5) * dv;
            id = GetNearestIndexI1(0.0, 1.0, e_left, mid);

            j0 = (inner_u + 1u) * (inner_v - local);
            j1 = (inner_u + 1u) * (inner_v - local - 1);
            j2 = inner_vcnt + e_bottom + e_right + e_top + id;
        }

        sIndices[inner_tcnt + edge_tcnt0 + i] = uvec3(j2, j1, j0);
    }

    // ---------- write mesh outputs ----------
    uint vcnt = inner_vcnt + edge_vcnt;
    for (uint i = gtid; i < vcnt; i += WORKGROUP_SIZE) {
        VertexData vd = sVertices[i];

        vec4 worldPos = SceneData.u_Transform * vec4(vd.position, 1.0);
        vec4 viewPos = SceneData.u_View * worldPos;
        vec4 clipPos  = SceneData.u_Projection * viewPos;

        gl_MeshVerticesNV[i].gl_Position = clipPos;

        v_out[i].mcPosition  = worldPos.xyz;
        v_out[i].vcPosition  = viewPos.xyz;
        v_out[i].texcoord    = vd.texcoord;
    }

    uint tcnt = inner_tcnt + edge_tcnt0 + edge_tcnt1;
    for (uint i = gtid; i < tcnt; i += WORKGROUP_SIZE) {
        gl_PrimitiveIndicesNV[3u * i + 0u] = sIndices[i].x;
        gl_PrimitiveIndicesNV[3u * i + 1u] = sIndices[i].y;
        gl_PrimitiveIndicesNV[3u * i + 2u] = sIndices[i].z;
    }

    if (gtid == 0u) {
        gl_PrimitiveCountNV = uint(tcnt);
    }
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

    // Displacement Mapping (currently disabled for geometry position)
    // Compute integer pixel coordinates and fractional offsets
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
