module;
#include "iGeMacro.h"
#include <glad/gl.h>

export module iGe.Renderer:OpenGLShader;
import :Shader;

import std;

namespace iGe
{

export class IGE_API OpenGLShader : public Shader {
public:
    OpenGLShader(const std::filesystem::path& filepath);
    OpenGLShader(const std::string& name, const std::filesystem::path& filepath);
    virtual ~OpenGLShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual const std::string& GetName() const override;

private:
    std::string ReadFile(const std::filesystem::path& filepath);

    void Compile(std::unordered_map<ShaderStage, std::string>);
    void CreateProgram();

    std::string m_Name;
    std::uint32_t m_RendererID;

    std::filesystem::path m_FilePath;
    std::unordered_map<ShaderStage, std::string> m_SourceCodes;
    std::unordered_map<ShaderStage, GLuint> m_Shaders;
};

} // namespace iGe
