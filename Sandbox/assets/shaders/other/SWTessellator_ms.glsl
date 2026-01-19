#version 460

#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : require

#define WORKGROUP_SIZE 32
#define MAX_VERTEX_COUNT 256
#define MAX_PRIMITIVE_COUNT 512
#define FLOAT_EPSILON 0.000001

#define NTF_MAX_FFLEVELS 8
#define NTF_MAX_HIDDEN_DIM 64
#define NTF_IN_DIM_RAW 13// 4*3 坐标 + 1 epsilon
#define NTF_MAX_ENC_DIM (NTF_IN_DIM_RAW * 2 * NTF_MAX_FFLEVELS)// 13 * 2 * 8 = 208

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

// NTF
layout(std140, binding = 10) uniform NTFConfigBlock {
    int fflevels;// offset 0
    int hidden_dim;// offset 4
    int max_rate;// offset 8
    int in_dim_raw;// offset 12
    float epsilon_mean;// offset 16
    float epsilon_std;// offset 20
    vec2 _pad;// offset 24 (vec2 不像数组那样强制16字节对齐)
    ivec4 layer_info[4];// offset 32 (weight_offset, bias_offset, in_features, out_features)
} ntf_config;

layout(std430, binding = 11) readonly buffer NTFWeightsBlock {
    float weights[];
} ntf_weights;

layout(std430, binding = 12) readonly buffer NTFBiasesBlock {
    float biases[];
} ntf_biases;

taskNV in Task {
    uint taskGroupID;
    uint taskGroupSize;
} task_in;

// out for fragment shader
out PerVertexData {
    vec3 mcPosition;
    vec3 vcPosition;
    vec2 texcoord;
} v_out[];

// use for tessellation
struct VertexData {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
};

uvec4 ntf_predict(vec3 v0, vec3 v1, vec3 v2, vec3 v3, float epsilon);
float compute_adaptive_epsilon(vec3 v0, vec3 v1, vec3 v2, vec3 v3);

shared VertexData sVertices[MAX_VERTEX_COUNT];
shared uvec3      sIndices[MAX_PRIMITIVE_COUNT];

VertexData GetInputVertex(uint vertexId);
uint GetNearestIndexI0(float lo, float hi, uint rate, float x);
uint GetNearestIndexI1(float lo, float hi, uint rate, float x);
VertexData GetDisplacedVertex(VertexData quadVerts[4], float u, float v);

void main()
{
    uint gid = gl_WorkGroupID.x;
    uint gtid = gl_LocalInvocationID.x;
    uint quadId = task_in.taskGroupID * task_in.taskGroupSize + gid;

    // Task shader guarantees quadId < u_QuadSize, so no bounds check needed
    // Composite quad vertices
    VertexData quadVerts[4];
    quadVerts[0] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 0u]);
    quadVerts[1] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 1u]);
    quadVerts[2] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 2u]);
    quadVerts[3] = GetInputVertex(quadIndexBuffer.data[quadId * 4u + 3u]);

    // 计算屏幕空间自适应 epsilon
    // epsilon 代表在当前视距下，单个像素对应的世界空间大小
    float epsilon = compute_adaptive_epsilon(
    quadVerts[0].position,
    quadVerts[1].position,
    quadVerts[2].position,
    quadVerts[3].position
    );

    // edge tess factors: bottom, right, top, left
    uvec4 edge = ntf_predict(
    quadVerts[0].position,
    quadVerts[1].position,
    quadVerts[2].position,
    quadVerts[3].position,
    epsilon
    );

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
            mid = (float(local) + 0.5) / float(e_bottom);
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
            mid = leftLift + (float(local) + 0.5) * du;
            id = GetNearestIndexI1(0.0, 1.0, e_bottom, mid);

            j0 = local;
            j1 = local + 1u;
            j2 = inner_vcnt + id;
        } else if (i < inner_u + inner_v) {
            local = i - inner_u;
            mid = bottomLift + (float(local) + 0.5) * dv;
            id = GetNearestIndexI1(0.0, 1.0, e_right, mid);

            j0 = (inner_u + 1u) * (local + 1u) - 1u;
            j1 = (inner_u + 1u) * (local + 2u) - 1u;
            j2 = inner_vcnt + e_bottom + id;
        } else if (i < 2u * inner_u + inner_v) {
            local = i - (inner_u + inner_v);
            mid = rightLift + (float(local) + 0.5) * du;
            id = GetNearestIndexI1(0.0, 1.0, e_top, mid);

            j0 = (inner_u + 1u) * (inner_v + 1u) - 1u - local;
            j1 = (inner_u + 1u) * (inner_v + 1u) - 2u - local;
            j2 = inner_vcnt + e_bottom + e_right + id;
        } else {
            local = i - (2u * inner_u + inner_v);
            mid = topLift + (float(local) + 0.5) * dv;
            id = GetNearestIndexI1(0.0, 1.0, e_left, mid);

            j0 = (inner_u + 1u) * (inner_v - local);
            j1 = (inner_u + 1u) * (inner_v - local - 1u);
            j2 = inner_vcnt + e_bottom + e_right + e_top + id;
        }

        sIndices[inner_tcnt + edge_tcnt0 + i] = uvec3(j2, j1, j0);
    }

    // Synchronize all threads before reading from shared memory
    barrier();

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

    // Set the primitive count (only thread 0 needs to do this)
    if (gtid == 0u) {
        gl_PrimitiveCountNV = tcnt;
    }
}

// =============================================================================
// compute_adaptive_epsilon: 计算屏幕空间自适应 epsilon
//
// 原理:
// 1. 将四个顶点变换到相机空间 (View Space)
// 2. 取到相机的最小距离作为投影平面距离
// 3. 在该距离处，计算单个像素对应的世界空间大小作为 epsilon
//
// 透视投影公式:
// - 在距离 d 处，视口垂直高度 = 2 * d * tan(fov/2)
// - 单个像素的世界空间大小 = (2 * d * tan(fov/2)) / screenHeight
// =============================================================================
float compute_adaptive_epsilon(vec3 v0, vec3 v1, vec3 v2, vec3 v3) {
    // 将四个顶点从模型空间变换到相机空间
    vec4 view0 = SceneData.u_View * SceneData.u_Transform * vec4(v0, 1.0);
    vec4 view1 = SceneData.u_View * SceneData.u_Transform * vec4(v1, 1.0);
    vec4 view2 = SceneData.u_View * SceneData.u_Transform * vec4(v2, 1.0);
    vec4 view3 = SceneData.u_View * SceneData.u_Transform * vec4(v3, 1.0);

    // 在相机空间中，相机看向 -Z 方向，所以顶点到相机的距离是 -z (取绝对值)
    float dist0 = abs(view0.z);
    float dist1 = abs(view1.z);
    float dist2 = abs(view2.z);
    float dist3 = abs(view3.z);

    // 取最小距离作为投影平面距离
    float d_min = min(min(dist0, dist1), min(dist2, dist3));

    // 防止除零，并设置一个合理的最小距离
    const float MIN_DISTANCE = 0.01;
    d_min = max(d_min, MIN_DISTANCE);

    // 从投影矩阵中提取 FOV 信息
    // 标准透视投影矩阵: P[1][1] = 1 / tan(fov/2)
    // 所以 tan(fov/2) = 1 / P[1][1]
    float tanHalfFov = 1.0 / SceneData.u_Projection[1][1];

    // 获取屏幕高度
    float screenHeight = float(TessellatorData.u_ScreenSize.y);

    // 计算在距离 d_min 处，单个像素对应的世界空间大小
    // pixel_size = (2 * d * tan(fov/2)) / screenHeight
    float pixel_world_size = (2.0 * d_min * tanHalfFov) / screenHeight;

    return pixel_world_size;
}

float ntf_leaky_relu(float x) { return x > 0.0 ? x : 0.01 * x; }

float ntf_get_weight(int layer, int out_idx, int in_idx) {
    int offset = ntf_config.layer_info[layer].x;
    int in_features = ntf_config.layer_info[layer].z;
    return ntf_weights.weights[offset + out_idx * in_features + in_idx];
}

float ntf_get_bias(int layer, int out_idx) {
    int offset = ntf_config.layer_info[layer].y;
    return ntf_biases.biases[offset + out_idx];
}

void ntf_positional_encoding(in float raw[NTF_IN_DIM_RAW], out float enc[NTF_MAX_ENC_DIM]) {
    int enc_idx = 0;

    for (int level = 0; level < ntf_config.fflevels; level++) {
        float freq = pow(2.0, float(level));

        // 先输出所有 sin 值
        for (int i = 0; i < ntf_config.in_dim_raw; i++) {
            float val = raw[i] * freq;
            enc[enc_idx++] = sin(val);
        }
        // 再输出所有 cos 值
        for (int i = 0; i < ntf_config.in_dim_raw; i++) {
            float val = raw[i] * freq;
            enc[enc_idx++] = cos(val);
        }
    }
}

void ntf_linear_layer(
in float input_data[NTF_MAX_ENC_DIM],
out float output_data[NTF_MAX_HIDDEN_DIM],
int layer,
int in_size,
int out_size,
bool apply_activation
) {
    for (int o = 0; o < out_size; o++) {
        float sum = ntf_get_bias(layer, o);

        for (int i = 0; i < in_size; i++) {
            sum += ntf_get_weight(layer, o, i) * input_data[i];
        }

        output_data[o] = apply_activation ? ntf_leaky_relu(sum) : sum;
    }
}

uvec4 ntf_predict(vec3 v0, vec3 v1, vec3 v2, vec3 v3, float epsilon) {
    // 1. 构建原始输入向量 (4*3 坐标 + 1 epsilon)
    float raw[NTF_IN_DIM_RAW];
    raw[0] = v0.x; raw[1] = v0.y; raw[2] = v0.z;
    raw[3] = v1.x; raw[4] = v1.y; raw[5] = v1.z;
    raw[6] = v2.x; raw[7] = v2.y; raw[8] = v2.z;
    raw[9] = v3.x; raw[10] = v3.y; raw[11] = v3.z;

    // 归一化 epsilon
    raw[12] = (epsilon - ntf_config.epsilon_mean) / ntf_config.epsilon_std;

    // 2. 位置编码
    float encoded[NTF_MAX_ENC_DIM];
    ntf_positional_encoding(raw, encoded);

    // 3. MLP 前向传播
    float hidden1[NTF_MAX_HIDDEN_DIM];
    float hidden2[NTF_MAX_HIDDEN_DIM];
    float hidden3[NTF_MAX_HIDDEN_DIM];

    int enc_dim = ntf_config.in_dim_raw * 2 * ntf_config.fflevels;
    int hidden_dim = ntf_config.hidden_dim;

    // Layer 0: enc_dim -> hidden_dim
    ntf_linear_layer(encoded, hidden1, 0,
    ntf_config.layer_info[0].z,
    ntf_config.layer_info[0].w, true);

    // Layer 1: hidden_dim -> hidden_dim
    // 需要把 hidden1 拷贝到兼容的数组类型
    float hidden1_ext[NTF_MAX_ENC_DIM];
    for (int i = 0; i < hidden_dim; i++) {
        hidden1_ext[i] = hidden1[i];
    }
    ntf_linear_layer(hidden1_ext, hidden2, 1,
    ntf_config.layer_info[1].z,
    ntf_config.layer_info[1].w, true);

    // Layer 2: hidden_dim -> hidden_dim
    float hidden2_ext[NTF_MAX_ENC_DIM];
    for (int i = 0; i < hidden_dim; i++) {
        hidden2_ext[i] = hidden2[i];
    }
    ntf_linear_layer(hidden2_ext, hidden3, 2,
    ntf_config.layer_info[2].z,
    ntf_config.layer_info[2].w, true);

    // Layer 3: hidden_dim -> 4 (无激活函数)
    float hidden3_ext[NTF_MAX_ENC_DIM];
    for (int i = 0; i < hidden_dim; i++) {
        hidden3_ext[i] = hidden3[i];
    }
    float output_ext[NTF_MAX_HIDDEN_DIM];
    ntf_linear_layer(hidden3_ext, output_ext, 3,
    ntf_config.layer_info[3].z,
    ntf_config.layer_info[3].w, false);

    // 4. 反归一化并转换为整数细分率
    float max_rate = float(ntf_config.max_rate);
    uint r0 = uint(clamp(round(output_ext[0] * max_rate), 1.0, max_rate));
    uint r1 = uint(clamp(round(output_ext[1] * max_rate), 1.0, max_rate));
    uint r2 = uint(clamp(round(output_ext[2] * max_rate), 1.0, max_rate));
    uint r3 = uint(clamp(round(output_ext[3] * max_rate), 1.0, max_rate));
    return uvec4(r0, r1, r2, r3);
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
