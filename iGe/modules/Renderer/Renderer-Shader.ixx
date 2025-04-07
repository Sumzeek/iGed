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

    virtual void SetInt(const std::string& name, int value) = 0;
    virtual void SetIntArray(const std::string& name, int* values, std::uint32_t count) = 0;
    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual void SetFloat2(const std::string& name, const glm::vec2& value) = 0;
    virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
    virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
    virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

    virtual const std::string& GetName() const = 0;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
};

} // namespace iGe