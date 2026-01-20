#version 460

#extension GL_NV_gpu_shader5 : require

#define WORKGROUP_SIZE 32
#define FLOAT_EPSILON 0.000001

#define NTF_MAX_FFLEVELS 8
#define NTF_MAX_HIDDEN_DIM 64
#define NTF_IN_DIM_RAW 13// 4*3 坐标 + 1 epsilon
#define NTF_MAX_ENC_DIM (NTF_IN_DIM_RAW * 2 * NTF_MAX_FFLEVELS)// 13 * 2 * 8 = 208

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
    uint u_QuadSize;
    uint u_LineOption;
} TessellatorData;

// Vertex information
layout(std430, binding = 5) readonly buffer PositionBuffer {
    float data[];// vec3
} positionBuffer;

layout(std430, binding = 8) readonly buffer QuadIndexBuffer {
    uint data[];
} quadIndexBuffer;

// Edge mapping buffer: maps each quad's 4 edges to global edge IDs
// Each quad has 4 edge IDs stored as uvec4 (e_bottom_id, e_right_id, e_top_id, e_left_id)
layout(std430, binding = 9) readonly buffer QuadEdgeIdBuffer {
    uvec4 data[];
} quadEdgeIdBuffer;

// Output: per-edge accumulated tessellation factors
// Each edge has a uint accumulator that will be summed from adjacent quads
layout(std430, binding = 10) buffer EdgeTessFactorBuffer {
    uint data[];
} edgeTessFactorBuffer;

// NTF configuration
layout(std140, binding = 11) uniform NTFConfigBlock {
    int fflevels;// offset 0
    int hidden_dim;// offset 4
    int max_rate;// offset 8
    int in_dim_raw;// offset 12
    float epsilon_mean;// offset 16
    float epsilon_std;// offset 20
    vec2 _pad;// offset 24
    ivec4 layer_info[4];// offset 32 (weight_offset, bias_offset, in_features, out_features)
} ntf_config;

layout(std430, binding = 12) readonly buffer NTFWeightsBlock {
    float weights[];
} ntf_weights;

layout(std430, binding = 13) readonly buffer NTFBiasesBlock {
    float biases[];
} ntf_biases;

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
    vec4 view0 = SceneData.u_View * SceneData.u_Transform * vec4(v0, 1.0);
    vec4 view1 = SceneData.u_View * SceneData.u_Transform * vec4(v1, 1.0);
    vec4 view2 = SceneData.u_View * SceneData.u_Transform * vec4(v2, 1.0);
    vec4 view3 = SceneData.u_View * SceneData.u_Transform * vec4(v3, 1.0);

    // 在相机空间中，相机看向 -Z 方向
    float dist0 = abs(view0.z);
    float dist1 = abs(view1.z);
    float dist2 = abs(view2.z);
    float dist3 = abs(view3.z);

    float d_min = min(min(dist0, dist1), min(dist2, dist3));

    const float MIN_DISTANCE = 0.01;
    d_min = max(d_min, MIN_DISTANCE);

    float tanHalfFov = 1.0 / SceneData.u_Projection[1][1];
    float screenHeight = float(TessellatorData.u_ScreenSize.y);
    float pixel_world_size = (2.0 * d_min * tanHalfFov) / screenHeight;

    return pixel_world_size;
}

// =============================================================================
// NTF Neural Network
// =============================================================================

float ntf_leaky_relu(float x) {
    return x > 0.0 ? x : 0.01 * x;
}

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

        for (int i = 0; i < ntf_config.in_dim_raw; i++) {
            float val = raw[i] * freq;
            enc[enc_idx++] = sin(val);
        }
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
    // 1. 构建原始输入向量
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

    int hidden_dim = ntf_config.hidden_dim;

    // Layer 0: enc_dim -> hidden_dim
    ntf_linear_layer(encoded, hidden1, 0,
    ntf_config.layer_info[0].z,
    ntf_config.layer_info[0].w, true);

    // Layer 1: hidden_dim -> hidden_dim
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

    // Layer 3: hidden_dim -> 4
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

// =============================================================================
// Main
// =============================================================================

void main() {
    uint quadId = gl_GlobalInvocationID.x;

    // Bounds check
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

    // Predict tessellation factors using NTF
    uvec4 tessFactors = ntf_predict(v0, v1, v2, v3, epsilon);

    // Get the edge IDs for this quad
    uvec4 edgeIds = quadEdgeIdBuffer.data[quadId];

    // Atomically accumulate tessellation factors to edge buffer
    // tessFactors: (e_bottom, e_right, e_top, e_left)
    // edgeIds: (e_bottom_id, e_right_id, e_top_id, e_left_id)
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.x], tessFactors.x);
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.y], tessFactors.y);
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.z], tessFactors.z);
    atomicAdd(edgeTessFactorBuffer.data[edgeIds.w], tessFactors.w);
}
