module;
#include "iGeMacro.h"
#include <cstdint>

export module iGed.NTF;
import iGe;
import std;

export namespace NTF
{

struct Config {
    int32_t fflevels = 2;      // 傅里叶特征级数
    int32_t hidden_dim = 16;   // 隐藏层维度
    int32_t in_dim_raw = 13;   // 原始输入维度 (4*3 + 1)
    float epsilon_mean = 0.0f; // epsilon 归一化均值
    float epsilon_std = 1.0f;  // epsilon 归一化标准差
    float epsilon_min = 0.0f;  // epsilon 范围最小值 (用于 clamp)
    float epsilon_max = 1.0f;  // epsilon 范围最大值 (用于 clamp)

    // 坐标归一化参数 (v2)
    glm::vec3 coord_center = glm::vec3(0.0f); // 坐标中心点
    glm::vec3 coord_scale = glm::vec3(1.0f);  // 坐标缩放因子

    // 计算编码后的输入维度
    int32_t encoded_dim() const { return in_dim_raw * 2 * fflevels; }
};

struct LayerWeights {
    int32_t in_features = 0;
    int32_t out_features = 0;
    std::vector<float> weight; // out_features * in_features (row-major)
    std::vector<float> bias;   // out_features
};

struct Model {
    Config config;
    std::vector<LayerWeights> layers; // 3 层

    // 从文件加载
    static Model Load(const std::string& path) {
        Model model;

        std::ifstream fin(path, std::ios::binary);
        if (!fin.is_open()) { IGE_ERROR("Failed to open NTF file: {}", path); }

        // 读取头部配置 (回归模式)
        fin.read(reinterpret_cast<char*>(&model.config.fflevels), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.hidden_dim), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.in_dim_raw), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.epsilon_mean), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.epsilon_std), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.epsilon_min), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.epsilon_max), sizeof(float));

        // 读取坐标归一化参数 (v2)
        fin.read(reinterpret_cast<char*>(&model.config.coord_center.x), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.coord_center.y), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.coord_center.z), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.coord_scale.x), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.coord_scale.y), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.coord_scale.z), sizeof(float));

        IGE_INFO("[NTF::Model::Load] Load ntf config from {}:", path);
        IGE_INFO("    fflevels     = {}", model.config.fflevels);
        IGE_INFO("    hidden_dim   = {}", model.config.hidden_dim);
        IGE_INFO("    in_dim_raw   = {}", model.config.in_dim_raw);
        IGE_INFO("    epsilon_mean = {}", model.config.epsilon_mean);
        IGE_INFO("    epsilon_std  = {}", model.config.epsilon_std);
        IGE_INFO("    epsilon_min  = {}", model.config.epsilon_min);
        IGE_INFO("    epsilon_max  = {}", model.config.epsilon_max);
        IGE_INFO("    coord_center = ({}, {}, {})", model.config.coord_center.x, model.config.coord_center.y,
                 model.config.coord_center.z);
        IGE_INFO("    coord_scale  = ({}, {}, {})", model.config.coord_scale.x, model.config.coord_scale.y,
                 model.config.coord_scale.z);
        IGE_INFO("    encoded_dim  = {}", model.config.encoded_dim());

        // 读取 3 层权重
        model.layers.resize(3);
        for (int i = 0; i < 3; i++) {
            auto& layer = model.layers[i];

            fin.read(reinterpret_cast<char*>(&layer.in_features), sizeof(int32_t));
            fin.read(reinterpret_cast<char*>(&layer.out_features), sizeof(int32_t));

            size_t weight_size = layer.in_features * layer.out_features;
            layer.weight.resize(weight_size);
            layer.bias.resize(layer.out_features);

            fin.read(reinterpret_cast<char*>(layer.weight.data()), weight_size * sizeof(float));
            fin.read(reinterpret_cast<char*>(layer.bias.data()), layer.out_features * sizeof(float));
            IGE_INFO("    Layer {}: ({}, {})", i, layer.in_features, layer.out_features);
        }

        if (!fin.good()) { IGE_ERROR("Error reading NTF file: {}", path); }
        return model;
    }

    /**
     * @brief 评估网络，预测 inner_rate 和 outter_rate (回归模式)
     * @param p0 四边形顶点0 (左下)
     * @param p1 四边形顶点1 (右下)
     * @param p2 四边形顶点2 (右上)
     * @param p3 四边形顶点3 (左上)
     * @param epsilon 误差阈值 (会被归一化)
     * @return glm::vec2 预测的细分率 (inner_rate, outter_rate)，连续浮点值
     */
    glm::vec2 Eval(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                   float epsilon) const {
        // 1. 坐标归一化: (coord - center) / scale -> [-1, 1]
        glm::vec3 p0_norm = (p0 - config.coord_center) / config.coord_scale;
        glm::vec3 p1_norm = (p1 - config.coord_center) / config.coord_scale;
        glm::vec3 p2_norm = (p2 - config.coord_center) / config.coord_scale;
        glm::vec3 p3_norm = (p3 - config.coord_center) / config.coord_scale;

        // 2. 构建原始输入向量 [p0.x, p0.y, p0.z, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z, eps]
        std::vector<float> raw_input(config.in_dim_raw);
        raw_input[0] = p0_norm.x;
        raw_input[1] = p0_norm.y;
        raw_input[2] = p0_norm.z;
        raw_input[3] = p1_norm.x;
        raw_input[4] = p1_norm.y;
        raw_input[5] = p1_norm.z;
        raw_input[6] = p2_norm.x;
        raw_input[7] = p2_norm.y;
        raw_input[8] = p2_norm.z;
        raw_input[9] = p3_norm.x;
        raw_input[10] = p3_norm.y;
        raw_input[11] = p3_norm.z;
        // 归一化 epsilon
        raw_input[12] = (epsilon - config.epsilon_mean) / config.epsilon_std;

        // 2. 位置编码: 对每个输入维度应用 Fourier 特征
        // 编码后维度 = in_dim_raw * 2 * fflevels
        std::vector<float> encoded(config.encoded_dim());
        int idx = 0;
        for (int level = 0; level < config.fflevels; ++level) {
            float k = static_cast<float>(1 << level); // 2^level
            for (int d = 0; d < config.in_dim_raw; ++d) {
                float v = k * raw_input[d];
                encoded[idx++] = std::sin(v);
            }
            for (int d = 0; d < config.in_dim_raw; ++d) {
                float v = k * raw_input[d];
                encoded[idx++] = std::cos(v);
            }
        }

        // 3. 网络前向传播
        std::vector<float> current = encoded;
        std::vector<float> next;

        for (size_t layer_idx = 0; layer_idx < layers.size(); ++layer_idx) {
            const auto& layer = layers[layer_idx];
            next.resize(layer.out_features);

            // 线性层: y = Wx + b
            for (int out_i = 0; out_i < layer.out_features; ++out_i) {
                float sum = layer.bias[out_i];
                for (int in_i = 0; in_i < layer.in_features; ++in_i) {
                    // weight 是 row-major: weight[out_i * in_features + in_i]
                    sum += layer.weight[out_i * layer.in_features + in_i] * current[in_i];
                }

                // LeakyReLU (除了最后一层)
                if (layer_idx < layers.size() - 1) { sum = sum > 0.0f ? sum : 0.01f * sum; }

                next[out_i] = sum;
            }

            current = std::move(next);
        }

        // 4. 回归模式：输出 2 个连续值 (inner_rate, outter_rate)
        // current[0] = inner_rate, current[1] = outter_rate
        float inner_rate = std::max(current[0], 1.0f);
        float outter_rate = std::max(current[1], 1.0f);

        return glm::vec2(inner_rate, outter_rate);
    }

    /**
     * @brief 评估网络，预测 inner_rate 和 outter_rate，并四舍五入为整数
     * @param p0 四边形顶点0 (左下)
     * @param p1 四边形顶点1 (右下)
     * @param p2 四边形顶点2 (右上)
     * @param p3 四边形顶点3 (左上)
     * @param epsilon 误差阈值 (会被归一化)
     * @return glm::ivec2 预测的细分率 (inner_rate, outter_rate)，整数值
     */
    glm::ivec2 EvalRounded(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                           float epsilon) const {
        glm::vec2 rates = Eval(p0, p1, p2, p3, epsilon);
        return glm::ivec2(std::max(static_cast<int>(std::round(rates.x)), 1),
                          std::max(static_cast<int>(std::round(rates.y)), 1));
    }
};

struct Buffers {
    iGe::Ref<iGe::Buffer> config_ubo;
    iGe::Ref<iGe::Buffer> weights_ssbo;
    iGe::Ref<iGe::Buffer> biases_ssbo;

    // 权重偏移信息 (用于 shader 中索引)
    struct LayerOffsets {
        int32_t weight_offset; // 权重在 buffer 中的起始位置
        int32_t bias_offset;   // 偏置在 buffer 中的起始位置
        int32_t in_features;
        int32_t out_features;
    };
    std::vector<LayerOffsets> layer_offsets;

    Config config;

    // 从 NTFModel 创建 OpenGL 缓冲区
    void Create(const Model& model) {
        config = model.config;

        // 计算打包后的权重和偏置大小
        size_t total_weights = 0;
        size_t total_biases = 0;

        layer_offsets.resize(3);
        for (int i = 0; i < 3; i++) {
            layer_offsets[i].weight_offset = static_cast<int32_t>(total_weights);
            layer_offsets[i].bias_offset = static_cast<int32_t>(total_biases);
            layer_offsets[i].in_features = model.layers[i].in_features;
            layer_offsets[i].out_features = model.layers[i].out_features;

            total_weights += model.layers[i].weight.size();
            total_biases += model.layers[i].bias.size();
        }

        // 创建配置 UBO (回归模式 v2) - 匹配 GLSL std140 布局
        // GLSL 布局:
        //   offset 0:  ivec4 config0       (fflevels, hidden_dim, in_dim_raw, pad)
        //   offset 16: vec4  config1       (epsilon_mean, epsilon_std, epsilon_min, epsilon_max)
        //   offset 32: vec4  coord_center  (x, y, z, pad)
        //   offset 48: vec4  coord_scale   (x, y, z, pad)
        //   offset 64: ivec4 layer_info[3] (weight_offset, bias_offset, in_features, out_features)
        struct alignas(16) ConfigData {
            // offset 0: config0 (ivec4)
            int32_t fflevels;
            int32_t hidden_dim;
            int32_t in_dim_raw;
            int32_t pad0;
            // offset 16: config1 (vec4)
            float epsilon_mean;
            float epsilon_std;
            float epsilon_min;
            float epsilon_max;
            // offset 32: coord_center (vec4)
            float coord_center_x;
            float coord_center_y;
            float coord_center_z;
            float pad1;
            // offset 48: coord_scale (vec4)
            float coord_scale_x;
            float coord_scale_y;
            float coord_scale_z;
            float pad2;
            // offset 64: layer_info[3] (ivec4[3])
            int32_t layer_info[3][4];
        };
        static_assert(sizeof(ConfigData) == 112, "ConfigData must be 112 bytes to match std140 layout");

        ConfigData cfg_data{};
        cfg_data.fflevels = config.fflevels;
        cfg_data.hidden_dim = config.hidden_dim;
        cfg_data.in_dim_raw = config.in_dim_raw;
        cfg_data.pad0 = 0;
        cfg_data.epsilon_mean = config.epsilon_mean;
        cfg_data.epsilon_std = config.epsilon_std;
        cfg_data.epsilon_min = config.epsilon_min;
        cfg_data.epsilon_max = config.epsilon_max;
        cfg_data.coord_center_x = config.coord_center.x;
        cfg_data.coord_center_y = config.coord_center.y;
        cfg_data.coord_center_z = config.coord_center.z;
        cfg_data.pad1 = 0.0f;
        cfg_data.coord_scale_x = config.coord_scale.x;
        cfg_data.coord_scale_y = config.coord_scale.y;
        cfg_data.coord_scale_z = config.coord_scale.z;
        cfg_data.pad2 = 0.0f;

        for (int i = 0; i < 3; i++) {
            cfg_data.layer_info[i][0] = layer_offsets[i].weight_offset;
            cfg_data.layer_info[i][1] = layer_offsets[i].bias_offset;
            cfg_data.layer_info[i][2] = layer_offsets[i].in_features;
            cfg_data.layer_info[i][3] = layer_offsets[i].out_features;
        }

        config_ubo = iGe::Buffer::Create(&cfg_data, sizeof(ConfigData));

        // 创建权重 SSBO
        std::vector<float> all_weights;
        all_weights.reserve(total_weights);
        for (const auto& layer: model.layers) {
            all_weights.insert(all_weights.end(), layer.weight.begin(), layer.weight.end());
        }

        weights_ssbo = iGe::Buffer::Create(all_weights.data(), all_weights.size() * sizeof(float));

        // 创建偏置 SSBO
        std::vector<float> all_biases;
        all_biases.reserve(total_biases);
        for (const auto& layer: model.layers) {
            all_biases.insert(all_biases.end(), layer.bias.begin(), layer.bias.end());
        }

        biases_ssbo = iGe::Buffer::Create(all_biases.data(), all_biases.size() * sizeof(float));

        IGE_INFO("[NTF::Buffers::Create] Create OpenGL buffers:");
        IGE_INFO("    Config UBO: {} bytes", sizeof(ConfigData));
        IGE_INFO("    Weights SSBO: {} bytes", all_weights.size() * sizeof(float));
        IGE_INFO("    Biases SSBO: {} bytes", all_biases.size() * sizeof(float));
    }

    // 绑定到 shader
    void Bind(uint32_t config_binding = 0, uint32_t weights_binding = 1, uint32_t biases_binding = 2) const {
        config_ubo->Bind(config_binding, iGe::BufferType::Uniform);
        weights_ssbo->Bind(weights_binding, iGe::BufferType::Storage);
        biases_ssbo->Bind(biases_binding, iGe::BufferType::Storage);
    }
};

} // namespace NTF
