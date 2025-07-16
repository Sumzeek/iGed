module;
#include "iGeMacro.h"
#include <nlohmann/json.hpp>

export module iGe.Renderer:Shader;

import std;
import glm;
import iGe.Log;
import iGe.SmartPointer;

namespace iGe
{

export enum class ShaderStage : int {
    None = 0,
    Vertex,
    TessellationControl,
    TessellationEvaluation,
    Geometry,
    Fragment,
    Compute,
};

ShaderStage ShaderStageFromString(const std::string& stageStr);

std::unordered_map<ShaderStage, std::filesystem::path> ParseShaderEntryMap(const std::filesystem::path& jsonFilePath);

export class IGE_API Shader {
public:
    virtual ~Shader() = default;

    static Ref<Shader> Create(const std::filesystem::path& filepath);
    static Ref<Shader> Create(const std::string& name, const std::filesystem::path& filepath);

    virtual const std::string& GetName() const = 0;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
};

export class IGE_API ShaderLibrary {
public:
    void Add(const std::string& name, const Ref<Shader>& shader);
    void Add(const Ref<Shader>& shader);

    Ref<Shader> Load(const std::filesystem::path& filepath);
    Ref<Shader> Load(const std::string& name, const std::filesystem::path& filepath);

    Ref<Shader> Get(const std::string& name);

    bool Exists(const std::string& name) const;

private:
    std::unordered_map<std::string, Ref<Shader>> m_Shaders;
};

} // namespace iGe