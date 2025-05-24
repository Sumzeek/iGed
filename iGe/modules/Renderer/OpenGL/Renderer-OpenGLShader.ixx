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
    OpenGLShader(const std::string& filepath);
    OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    virtual ~OpenGLShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual const std::string& GetName() const override;

private:
    std::string ReadFile(const std::string& filepath);
    std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);

    void Compile(std::unordered_map<GLenum, std::string>);
    void CreateProgram();

    std::uint32_t m_RendererID;
    std::string m_FilePath;
    std::string m_Name;

    std::unordered_map<GLenum, std::string> m_SourceCode;
    std::unordered_map<GLenum, GLuint> m_Shaders;
};

} // namespace iGe
