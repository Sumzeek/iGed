module;
#include "iGeMacro.h"

export module iGe.Renderer:Shader;

import std;
import glm;
import iGe.SmartPointer;

namespace iGe
{

export class IGE_API Shader {
public:
    virtual ~Shader() = default;

    static Ref<Shader> Create(const std::string filepath);
    static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);

    virtual const std::string& GetName() const = 0;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
};

export class IGE_API ShaderLibrary {
public:
    void Add(const std::string& name, const Ref<Shader>& shader);
    void Add(const Ref<Shader>& shader);

    Ref<Shader> Load(const std::string& filepath);
    Ref<Shader> Load(const std::string& name, const std::string& filepath);

    Ref<Shader> Get(const std::string& name);

    bool Exists(const std::string& name) const;

private:
    std::unordered_map<std::string, Ref<Shader>> m_Shaders;
};

} // namespace iGe