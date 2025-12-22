module;
#include "iGeMacro.h"
#include <glad/gl.h>

export module iGe.Renderer:OpenGLShader;
import :Shader;
import std;

namespace iGe
{
export class IGE_API OpenGLGraphicsShader : public GraphicsShader {
public:
    OpenGLGraphicsShader(const std::filesystem::path& filepath);
    OpenGLGraphicsShader(const std::string& name, const std::filesystem::path& filepath);
    virtual ~OpenGLGraphicsShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual const std::string& GetName() const override;

private:
    std::string m_Name;
    std::uint32_t m_RendererID;

    std::filesystem::path m_FilePath;
    std::unordered_map<ShaderStage, std::string> m_SourceCodes;
    std::unordered_map<ShaderStage, GLuint> m_Shaders;
};

export class IGE_API OpenGLComputeShader : public ComputeShader {
public:
    OpenGLComputeShader(const std::filesystem::path& filepath);
    OpenGLComputeShader(const std::string& name, const std::filesystem::path& filepath);
    virtual ~OpenGLComputeShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;
    virtual void Dispatch(std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ) override;

    virtual const std::string& GetName() const override;

private:
    std::string m_Name;
    std::uint32_t m_RendererID;

    std::filesystem::path m_FilePath;
    std::unordered_map<ShaderStage, std::string> m_SourceCodes;
    std::unordered_map<ShaderStage, GLuint> m_Shaders;
};

export class IGE_API OpenGLMeshShader : public MeshShader {
public:
    OpenGLMeshShader(const std::filesystem::path& filepath);
    OpenGLMeshShader(const std::string& name, const std::filesystem::path& filepath);
    virtual ~OpenGLMeshShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;
    virtual void DispatchTask(std::uint32_t offset, std::uint32_t count) override;

    virtual const std::string& GetName() const override;

private:
    std::string m_Name;
    std::uint32_t m_RendererID;

    std::filesystem::path m_FilePath;
    std::unordered_map<ShaderStage, std::string> m_SourceCodes;
    std::unordered_map<ShaderStage, GLuint> m_Shaders;
};

} // namespace iGe
