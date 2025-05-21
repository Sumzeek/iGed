module;
#include "iGeMacro.h"

export module iGe.Renderer:Shader;
import std;
import glm;
import iGe.SmartPointer;
import iGe.Log;

namespace iGe
{

class IGE_API ShaderBase {
public:
    virtual ~ShaderBase() = default;
    virtual const std::string& GetName() const = 0;
};

export class IGE_API GraphicsShader : public ShaderBase {
public:
    static Ref<GraphicsShader> Create(const std::string& filepath);
    static Ref<GraphicsShader> Create(const std::string& name, const std::string& vertexSrc,
                                      const std::string& fragmentSrc);
    
    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
};

class IGE_API ComputeShader : public ShaderBase {
public:
    static Ref<ComputeShader> Create(const std::string& filepath);
    static Ref<ComputeShader> Create(const std::string& name, const std::string& computeSrc);

    virtual void Dispatch(std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ) = 0;
};

export template<typename TShader>
class IGE_API ShaderLibrary {
public:
    void Add(const std::string& name, const Ref<TShader>& shader) {
        IGE_CORE_ASSERT(!Exists(name), "Shader already exists!");
        m_Shaders[name] = shader;
    }
    void Add(const Ref<TShader>& shader) {
        auto& name = shader->GetName();
        IGE_CORE_ASSERT(!Exists(name), "Shader already exists!");
        Add(name, shader);
    }

    Ref<TShader> Load(const std::string& filepath) {
        auto shader = TShader::Create(filepath);
        Add(shader);
        return shader;
    }
    Ref<TShader> Load(const std::string& name, const std::string& filepath) {
        auto shader = TShader::Create(filepath);
        Add(name, shader);
        return shader;
    }

    Ref<TShader> Get(const std::string& name) {
        IGE_CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Shaders[name];
    }
    bool Exists(const std::string& name) const { return m_Shaders.find(name) != m_Shaders.end(); }

private:
    std::unordered_map<std::string, Ref<TShader>> m_Shaders;
};

} // namespace iGe