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
    OpenGLGraphicsShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    virtual ~OpenGLGraphicsShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual const std::string& GetName() const override;

private:
    std::uint32_t m_RendererID;
    std::filesystem::path m_FilePath;
    std::string m_Name;

    std::unordered_map<GLenum, std::string> m_SourceCode;
};

export class IGE_API OpenGLComputeShader : public ComputeShader {
public:
    OpenGLComputeShader(const std::filesystem::path& filepath);
    OpenGLComputeShader(const std::string& name, const std::string& computeSrc);
    virtual ~OpenGLComputeShader();

    virtual void Dispatch(std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ) override;

    virtual const std::string& GetName() const override;

private:
    std::uint32_t m_RendererID;
    std::filesystem::path m_FilePath;
    std::string m_Name;

    std::unordered_map<GLenum, std::string> m_SourceCode;
};

} // namespace iGe
