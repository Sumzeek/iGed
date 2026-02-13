module;
#include "iGeMacro.h"

export module iGed.LUT;
import iGe;
import glm;
import std;

export namespace LUT
{
// 单个查找表条目：(error, inner_factor, outer_factor)
struct LUTEntry {
    float error;
    float inner_factor;
    float outer_factor;
    float _padding; // 对齐到 16 字节
};

// 每个quad的索引信息：起始位置和条目数
struct QuadLUTInfo {
    std::uint32_t start_index;
    std::uint32_t entry_count;
    std::uint32_t _padding0;
    std::uint32_t _padding1;
};

struct LUTData {
    std::vector<LUTEntry> entries;      // 所有条目的平铺数组
    std::vector<QuadLUTInfo> quad_info; // 每个quad的索引信息
    std::uint32_t quad_count = 0;
    std::uint32_t total_entries = 0;
};

// 从pareto.csv文件加载LUT数据
inline LUTData LoadFromCSV(const std::string& filepath) {
    LUTData data;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        IGE_ERROR("Failed to open LUT file: {}", filepath);
        return data;
    }

    // 临时存储：quad_id -> 条目列表
    std::map<std::uint32_t, std::vector<LUTEntry>> quad_entries;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ',')) { tokens.push_back(token); }

        // 格式: quadId, v0, v1, v2, v3, error, inner_factor, outer_factor
        if (tokens.size() < 8) continue;

        try {
            std::uint32_t quad_id = static_cast<std::uint32_t>(std::stoi(tokens[0]));
            float error = std::stof(tokens[5]);
            float inner_factor = std::stof(tokens[6]);
            float outer_factor = std::stof(tokens[7]);

            LUTEntry entry;
            entry.error = error;
            entry.inner_factor = inner_factor;
            entry.outer_factor = outer_factor;
            entry._padding = 0.0f;

            quad_entries[quad_id].push_back(entry);
        } catch (const std::exception& e) {
            // 跳过解析错误的行
            continue;
        }
    }
    file.close();

    // 找到最大的quad_id来确定quad数量
    std::uint32_t max_quad_id = 0;
    for (const auto& [quad_id, entries]: quad_entries) { max_quad_id = std::max(max_quad_id, quad_id); }
    data.quad_count = max_quad_id + 1;

    // 为每个quad排序条目（按error升序）并构建平铺数组
    data.quad_info.resize(data.quad_count);

    std::uint32_t current_offset = 0;
    for (std::uint32_t quad_id = 0; quad_id < data.quad_count; ++quad_id) {
        auto it = quad_entries.find(quad_id);
        if (it == quad_entries.end() || it->second.empty()) {
            // 没有数据的quad，使用默认值
            data.quad_info[quad_id].start_index = current_offset;
            data.quad_info[quad_id].entry_count = 1;
            data.quad_info[quad_id]._padding0 = 0;
            data.quad_info[quad_id]._padding1 = 0;

            LUTEntry default_entry;
            default_entry.error = 0.0f;
            default_entry.inner_factor = 1.0f;
            default_entry.outer_factor = 1.0f;
            default_entry._padding = 0.0f;
            data.entries.push_back(default_entry);
            current_offset += 1;
        } else {
            auto& entries = it->second;
            // 按error升序排序
            std::sort(entries.begin(), entries.end(),
                      [](const LUTEntry& a, const LUTEntry& b) { return a.error < b.error; });

            data.quad_info[quad_id].start_index = current_offset;
            data.quad_info[quad_id].entry_count = static_cast<std::uint32_t>(entries.size());
            data.quad_info[quad_id]._padding0 = 0;
            data.quad_info[quad_id]._padding1 = 0;

            for (const auto& entry: entries) { data.entries.push_back(entry); }
            current_offset += static_cast<std::uint32_t>(entries.size());
        }
    }

    data.total_entries = static_cast<std::uint32_t>(data.entries.size());

    IGE_INFO("LUT loaded: {} quads, {} total entries", data.quad_count, data.total_entries);

    return data;
}

// GPU缓冲区管理类
class Buffers {
public:
    void Create(const LUTData& data) {
        if (data.entries.empty() || data.quad_info.empty()) {
            IGE_ERROR("Cannot create LUT buffers from empty data");
            return;
        }

        // 创建条目缓冲区
        m_EntriesBuffer = iGe::Buffer::Create(const_cast<void*>(static_cast<const void*>(data.entries.data())),
                                              data.entries.size() * sizeof(LUTEntry));

        // 创建quad索引信息缓冲区
        m_QuadInfoBuffer = iGe::Buffer::Create(const_cast<void*>(static_cast<const void*>(data.quad_info.data())),
                                               data.quad_info.size() * sizeof(QuadLUTInfo));

        m_QuadCount = data.quad_count;
        m_TotalEntries = data.total_entries;
    }

    void Bind(std::uint32_t entries_binding, std::uint32_t quad_info_binding) const {
        if (m_EntriesBuffer) { m_EntriesBuffer->Bind(entries_binding, iGe::BufferType::Storage); }
        if (m_QuadInfoBuffer) { m_QuadInfoBuffer->Bind(quad_info_binding, iGe::BufferType::Storage); }
    }

    std::uint32_t GetQuadCount() const { return m_QuadCount; }
    std::uint32_t GetTotalEntries() const { return m_TotalEntries; }

private:
    iGe::Ref<iGe::Buffer> m_EntriesBuffer;
    iGe::Ref<iGe::Buffer> m_QuadInfoBuffer;
    std::uint32_t m_QuadCount = 0;
    std::uint32_t m_TotalEntries = 0;
};

} // namespace LUT
