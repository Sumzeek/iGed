#version 460

#extension GL_NV_gpu_shader5 : require

#define WORKGROUP_SIZE 32
#define FLOAT_EPSILON 0.000001

#define NTF_MAX_FFLEVELS 8
#define NTF_MAX_HIDDEN_DIM 64
#define NTF_IN_DIM_RAW 13// 4*3 坐标 + 1 epsilon
#define NTF_MAX_ENC_DIM (NTF_IN_DIM_RAW * 2 * NTF_MAX_FFLEVELS)// 13 * 2 * 8 = 208
#define NTF_OUTPUT_DIM 2// 2 rates: inner_rate, outter_rate

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
// Each quad has 4 edge IDs stored as uvec4 (e_bottom_id, e_right_id, e_top_id, e_left_id)
layout(std430, binding = 9) readonly buffer QuadEdgeIdBuffer {
    uvec4 data[];
} quadEdgeIdBuffer;

// Output: per-edge accumulated tessellation factors
// Each edge has a uint accumulator that will be summed from adjacent quads
layout(std430, binding = 10) buffer EdgeTessFactorBuffer {
    uint data[];
} edgeTessFactorBuffer;

layout(std430, binding = 11) buffer InnerTessFactorBuffer {
    uvec2 data[];
} innerTessFactorBuffer;

// NTF configuration - Regression mode v2
// 注意: std140 布局规则
layout(std140, binding = 12) uniform NTFConfigBlock {
// offset 0: 基础配置 (打包为 ivec4)
    ivec4 config0;// (fflevels, hidden_dim, in_dim_raw, pad)
// offset 16: epsilon 配置 (打包为 vec4)
    vec4 config1;// (epsilon_mean, epsilon_std, epsilon_min, epsilon_max)
// offset 32: 坐标归一化中心 (打包为 vec4)
    vec4 coord_center;// (x, y, z, pad)
// offset 48: 坐标归一化缩放 (打包为 vec4)
    vec4 coord_scale;// (x, y, z, pad)
// offset 64: layer_info
    ivec4 layer_info[3];// (weight_offset, bias_offset, in_features, out_features)
} ntf_config;

// 辅助宏访问配置
#define NTF_FFLEVELS      ntf_config.config0.x
#define NTF_HIDDEN_DIM    ntf_config.config0.y
#define NTF_IN_DIM_RAW_V  ntf_config.config0.z
#define NTF_EPSILON_MEAN  ntf_config.config1.x
#define NTF_EPSILON_STD   ntf_config.config1.y
#define NTF_EPSILON_MIN   ntf_config.config1.z
#define NTF_EPSILON_MAX   ntf_config.config1.w
#define NTF_COORD_CENTER  ntf_config.coord_center.xyz
#define NTF_COORD_SCALE   ntf_config.coord_scale.xyz

layout(std430, binding = 13) readonly buffer NTFWeightsBlock {
    float weights[];
} ntf_weights;

layout(std430, binding = 14) readonly buffer NTFBiasesBlock {
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
// NTF Neural Network - Regression Mode
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
    int fflevels = NTF_FFLEVELS;
    int in_dim = NTF_IN_DIM_RAW_V;

    for (int level = 0; level < fflevels; level++) {
        float freq = pow(2.0, float(level));

        for (int i = 0; i < in_dim; i++) {
            float val = raw[i] * freq;
            enc[enc_idx++] = sin(val);
        }
        for (int i = 0; i < in_dim; i++) {
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

// NTF 预测函数 - 回归模式 v2
// 返回 vec2(inner_rate, outter_rate)
vec2 ntf_predict(vec3 v0, vec3 v1, vec3 v2, vec3 v3, float epsilon) {
    // 0. 处理超出训练范围的 epsilon
    if (epsilon > NTF_EPSILON_MAX) {
        return vec2(1.0, 1.0);// 最低细分率
    } else if (epsilon < NTF_EPSILON_MIN) {
        return vec2(5.0, 5.0);// 最高细分率
    }
    // Clamp epsilon 到训练数据范围，避免外推导致的不稳定预测
    epsilon = clamp(epsilon, NTF_EPSILON_MIN, NTF_EPSILON_MAX);

    // 1. 坐标归一化: (coord - center) / scale -> [-1, 1]
    vec3 center = NTF_COORD_CENTER;
    vec3 scale = NTF_COORD_SCALE;
    vec3 v0_norm = (v0 - center) / scale;
    vec3 v1_norm = (v1 - center) / scale;
    vec3 v2_norm = (v2 - center) / scale;
    vec3 v3_norm = (v3 - center) / scale;

    // 2. 构建原始输入向量 (使用归一化后的坐标)
    float raw[NTF_IN_DIM_RAW];
    raw[0] = v0_norm.x; raw[1] = v0_norm.y; raw[2] = v0_norm.z;
    raw[3] = v1_norm.x; raw[4] = v1_norm.y; raw[5] = v1_norm.z;
    raw[6] = v2_norm.x; raw[7] = v2_norm.y; raw[8] = v2_norm.z;
    raw[9] = v3_norm.x; raw[10] = v3_norm.y; raw[11] = v3_norm.z;

    // 归一化 epsilon (已经过 clamp)
    raw[12] = (epsilon - NTF_EPSILON_MEAN) / NTF_EPSILON_STD;

    // 2. 位置编码
    float encoded[NTF_MAX_ENC_DIM];
    ntf_positional_encoding(raw, encoded);

    // 3. MLP 前向传播
    float hidden1[NTF_MAX_HIDDEN_DIM];
    float hidden2[NTF_MAX_HIDDEN_DIM];

    int hidden_dim = NTF_HIDDEN_DIM;

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

    // Layer 2: hidden_dim -> 2 (inner_rate, outter_rate)
    float hidden2_ext[NTF_MAX_ENC_DIM];
    for (int i = 0; i < hidden_dim; i++) {
        hidden2_ext[i] = hidden2[i];
    }
    float out_rates[NTF_MAX_HIDDEN_DIM];
    ntf_linear_layer(hidden2_ext, out_rates, 2,
    ntf_config.layer_info[2].z,
    ntf_config.layer_info[2].w, false);

    // 4. 返回 inner_rate 和 outter_rate（连续值）
    // 网络输出可能需要 clamp 到合理范围 [1, max_rate]
    float inner_rate = max(out_rates[0], 1.0);
    float outter_rate = max(out_rates[1], 1.0);

    return vec2(inner_rate, outter_rate);
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

    // Predict tessellation factors using NTF (regression mode)
    // Returns vec2(inner_rate, outter_rate)
    vec2 rates = ntf_predict(v0, v1, v2, v3, epsilon);

    // Round to nearest integer for tessellation
    uint inner_rate = uint(round(rates.x));
    uint outter_rate = uint(round(rates.y));

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
