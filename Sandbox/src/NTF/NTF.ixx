module;
#include <cstdint>

export module iGed.NTF;
import iGe;
import std;

export namespace NTF
{

struct Config {
    int32_t fflevels = 8;      // 傅里叶特征级数
    int32_t hidden_dim = 64;   // 隐藏层维度
    int32_t max_rate = 16;     // 最大细分率
    int32_t in_dim_raw = 13;   // 原始输入维度 (4*3 + 1)
    float epsilon_mean = 0.0f; // epsilon 归一化均值
    float epsilon_std = 1.0f;  // epsilon 归一化标准差

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
    std::vector<LayerWeights> layers; // 4 层

    // 从文件加载
    static Model Load(const std::string& path) {
        Model model;

        std::ifstream fin(path, std::ios::binary);
        if (!fin.is_open()) { throw std::runtime_error("Failed to open NTF file: " + path); }

        // 读取头部配置
        fin.read(reinterpret_cast<char*>(&model.config.fflevels), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.hidden_dim), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.max_rate), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.in_dim_raw), sizeof(int32_t));
        fin.read(reinterpret_cast<char*>(&model.config.epsilon_mean), sizeof(float));
        fin.read(reinterpret_cast<char*>(&model.config.epsilon_std), sizeof(float));

        std::cout << "[NTF::Model::Load] 加载配置:" << std::endl;
        std::cout << "  fflevels    = " << model.config.fflevels << std::endl;
        std::cout << "  hidden_dim  = " << model.config.hidden_dim << std::endl;
        std::cout << "  max_rate    = " << model.config.max_rate << std::endl;
        std::cout << "  in_dim_raw  = " << model.config.in_dim_raw << std::endl;
        std::cout << "  epsilon_mean= " << model.config.epsilon_mean << std::endl;
        std::cout << "  epsilon_std = " << model.config.epsilon_std << std::endl;
        std::cout << "  encoded_dim = " << model.config.encoded_dim() << std::endl;

        // 读取 4 层权重
        model.layers.resize(4);
        for (int i = 0; i < 4; i++) {
            auto& layer = model.layers[i];

            fin.read(reinterpret_cast<char*>(&layer.in_features), sizeof(int32_t));
            fin.read(reinterpret_cast<char*>(&layer.out_features), sizeof(int32_t));

            size_t weight_size = layer.in_features * layer.out_features;
            layer.weight.resize(weight_size);
            layer.bias.resize(layer.out_features);

            fin.read(reinterpret_cast<char*>(layer.weight.data()), weight_size * sizeof(float));
            fin.read(reinterpret_cast<char*>(layer.bias.data()), layer.out_features * sizeof(float));
            std::cout << "  Layer " << i << ": (" << layer.in_features << ", " << layer.out_features << ")"
                      << std::endl;
        }

        if (!fin.good()) { throw std::runtime_error("Error reading NTF file: " + path); }

        std::cout << "[NTF::Model::Load] 加载完成: " << path << std::endl;
        return model;
    }

    /**
     * @brief 评估网络，预测四条边的细分率
     * @param p0 四边形顶点0 (左下)
     * @param p1 四边形顶点1 (右下)
     * @param p2 四边形顶点2 (右上)
     * @param p3 四边形顶点3 (左上)
     * @param epsilon 误差阈值 (会被归一化)
     * @return glm::vec4 四条边的预测细分率 (bottom, right, top, left)
     */
    glm::vec4 Eval(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                   float epsilon) const {
        // 1. 构建原始输入向量 [p0.x, p0.y, p0.z, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z, eps]
        std::vector<float> raw_input(config.in_dim_raw);
        raw_input[0] = p0.x;
        raw_input[1] = p0.y;
        raw_input[2] = p0.z;
        raw_input[3] = p1.x;
        raw_input[4] = p1.y;
        raw_input[5] = p1.z;
        raw_input[6] = p2.x;
        raw_input[7] = p2.y;
        raw_input[8] = p2.z;
        raw_input[9] = p3.x;
        raw_input[10] = p3.y;
        raw_input[11] = p3.z;
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

        // 4. 反归一化并返回结果
        // 网络输出是归一化到 [0, 1] 的值，需要乘以 max_rate 得到实际细分率
        float max_rate = static_cast<float>(config.max_rate);
        return glm::vec4(current[0] * max_rate, current[1] * max_rate, current[2] * max_rate, current[3] * max_rate);
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

        layer_offsets.resize(4);
        for (int i = 0; i < 4; i++) {
            layer_offsets[i].weight_offset = static_cast<int32_t>(total_weights);
            layer_offsets[i].bias_offset = static_cast<int32_t>(total_biases);
            layer_offsets[i].in_features = model.layers[i].in_features;
            layer_offsets[i].out_features = model.layers[i].out_features;

            total_weights += model.layers[i].weight.size();
            total_biases += model.layers[i].bias.size();
        }

        // 创建配置 UBO
        // 结构: fflevels, hidden_dim, max_rate, in_dim_raw, epsilon_mean, epsilon_std
        //       layer0_weight_off, layer0_bias_off, layer0_in, layer0_out
        //       layer1_weight_off, layer1_bias_off, layer1_in, layer1_out
        //       ...
        struct ConfigData {
            int32_t fflevels;
            int32_t hidden_dim;
            int32_t max_rate;
            int32_t in_dim_raw;
            float epsilon_mean;
            float epsilon_std;
            float _pad[2]; // 对齐到 16 字节
            // 每层信息: weight_offset, bias_offset, in_features, out_features
            int32_t layer_info[4][4];
        };

        ConfigData cfg_data;
        cfg_data.fflevels = config.fflevels;
        cfg_data.hidden_dim = config.hidden_dim;
        cfg_data.max_rate = config.max_rate;
        cfg_data.in_dim_raw = config.in_dim_raw;
        cfg_data.epsilon_mean = config.epsilon_mean;
        cfg_data.epsilon_std = config.epsilon_std;
        cfg_data._pad[0] = cfg_data._pad[1] = 0.0f;

        for (int i = 0; i < 4; i++) {
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

        std::cout << "[NTFGLBuffers::create] 创建 OpenGL 缓冲区:" << std::endl;
        std::cout << "  Config UBO: " << sizeof(ConfigData) << " bytes" << std::endl;
        std::cout << "  Weights SSBO: " << all_weights.size() * sizeof(float) << " bytes" << std::endl;
        std::cout << "  Biases SSBO: " << all_biases.size() * sizeof(float) << " bytes" << std::endl;
    }

    // 绑定到 shader
    void Bind(uint32_t config_binding = 0, uint32_t weights_binding = 1, uint32_t biases_binding = 2) const {
        config_ubo->Bind(config_binding, iGe::BufferType::Uniform);
        weights_ssbo->Bind(weights_binding, iGe::BufferType::Storage);
        biases_ssbo->Bind(biases_binding, iGe::BufferType::Storage);
    }
};

} // namespace NTF
