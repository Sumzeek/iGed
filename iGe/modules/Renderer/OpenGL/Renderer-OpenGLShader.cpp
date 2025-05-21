module;
#include "iGeMacro.h"

#include <glad/gl.h>

module iGe.Renderer;
import :OpenGLShader;
import iGe.Log;

namespace iGe
{

namespace Utils
{

static GLenum ShaderTypeFromString(const std::string_view type) {
    if (type == "vertex") { return GL_VERTEX_SHADER; }
    if (type == "fragment" || type == "pixel") { return GL_FRAGMENT_SHADER; }
    if (type == "compute") { return GL_COMPUTE_SHADER; }
    IGE_CORE_ASSERT(false, "Unknown shader type!");
    return 0;
}

static std::string ReadFile(const std::filesystem::path& filepath) {
    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
    if (in) {
        in.seekg(0, std::ios::end);
        size_t size = in.tellg();
        if (size != -1) {
            result.resize(size);
            in.seekg(0, std::ios::beg);
            in.read(&result[0], size);
        } else {
            IGE_CORE_ERROR("Could not read from file '{0}'", filepath.string());
        }
    } else {
        IGE_CORE_ERROR("Could not open file '{0}'", filepath.string());
    }

    return result;
}

static std::string ExtractNameFromPath(const std::string& filepath) {
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    return filepath.substr(lastSlash, count);
}

static std::unordered_map<GLenum, std::string> PreProcess(const std::string& source) {
    std::unordered_map<GLenum, std::string> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLength = std::strlen(typeToken);
    size_t pos = source.find(typeToken, 0); //Start of shader type declaration line
    while (pos != std::string::npos) {
        size_t eol = source.find_first_of("\r\n", pos); //End of shader type declaration line
        IGE_CORE_ASSERT(eol != std::string::npos, "Syntax error");
        size_t begin = pos + typeTokenLength + 1; //Start of shader type name (after "#type " keyword)
        std::string type = source.substr(begin, eol - begin);
        IGE_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified");

        size_t nextLinePos =
                source.find_first_not_of("\r\n", eol); //Start of shader code after shader type declaration line
        IGE_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
        pos = source.find(typeToken, nextLinePos); //Start of next shader type declaration line

        shaderSources[ShaderTypeFromString(type)] =
                (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
}

static std::optional<GLuint> CompileAndLinkProgram(const std::unordered_map<GLenum, std::string>& sources,
                                                   const std::string& debugOuput = "Unknown Debug Output") {
    // Compile Shaders
    std::unordered_map<GLenum, GLuint> shaders;
    for (auto&& [stage, source]: sources) {
        // Create an empty fragment shader handle
        GLuint shaderID = glCreateShader(stage);

        // Send the fragment shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        const GLchar* sourceCStr = (const GLchar*) source.c_str();
        glShaderSource(shaderID, 1, &sourceCStr, 0);

        // Compile the fragment shader
        glCompileShader(shaderID);

        GLint isCompiled = 0;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shaderID, maxLength, &maxLength, &infoLog[0]);

            // We don't need the shader anymore.
            glDeleteShader(shaderID);
            // Either of them. Don't leak shaders.
            for (auto& [_, id]: shaders) { glDeleteShader(id); }

            shaders.clear();
            IGE_CORE_ERROR("{0}", infoLog.data());

            return std::nullopt;
        }

        shaders[stage] = shaderID;
    }

    // Link Program
    GLuint program = glCreateProgram();

    // Link shaders together into a program.
    for (auto&& [stage, shaderID]: shaders) { glAttachShader(program, shaderID); }

    // Link our program
    glLinkProgram(program);

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
        IGE_CORE_ERROR("Shader linking failed ({0}):\n{1}", debugOuput, infoLog.data());

        // We don't need the program anymore
        glDeleteProgram(program);

        // Don't leak shaders either
        for (auto&& [stage, shaderID]: shaders) { glDeleteShader(shaderID); }

        return std::nullopt;
    }

    // Always detach shaders after a successful link
    for (auto&& [stage, shaderID]: shaders) {
        glDetachShader(program, shaderID);
        glDeleteShader(shaderID);
    }

    return program;
}

} // namespace Utils

/////////////////////////////////////////////////////////////////////////////
// OpenGLGraphicsShader /////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLGraphicsShader::OpenGLGraphicsShader(const std::filesystem::path& filepath) : m_FilePath(filepath) {
    std::string source = Utils::ReadFile(filepath);
    m_SourceCode = Utils::PreProcess(source);

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCode, m_FilePath.string());
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_FilePath.string());
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;

    m_Name = filepath.stem().string();
}

OpenGLGraphicsShader::OpenGLGraphicsShader(const std::string& name, const std::string& vertexSrc,
                                           const std::string& fragmentSrc)
    : m_Name(name) {
    m_SourceCode[GL_VERTEX_SHADER] = vertexSrc;
    m_SourceCode[GL_FRAGMENT_SHADER] = fragmentSrc;

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCode, m_FilePath.string());
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_FilePath.string());
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;

    m_Name = name;
}

OpenGLGraphicsShader::~OpenGLGraphicsShader() { glDeleteProgram(m_RendererID); }

void OpenGLGraphicsShader::Bind() const { glUseProgram(m_RendererID); }

void OpenGLGraphicsShader::Unbind() const { glUseProgram(0); }

const std::string& OpenGLGraphicsShader::GetName() const { return m_Name; }

/////////////////////////////////////////////////////////////////////////////
// OpenGLComputeShader //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLComputeShader::OpenGLComputeShader(const std::filesystem::path& filepath) : m_FilePath(filepath) {
    std::string source = Utils::ReadFile(filepath);
    m_SourceCode = Utils::PreProcess(source);

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCode, m_FilePath.string());
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_FilePath.string());
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;

    // Extract name from filepath
    m_Name = filepath.stem().string();
}

OpenGLComputeShader::OpenGLComputeShader(const std::string& name, const std::string& computeSrc) : m_Name(name) {
    m_SourceCode[GL_COMPUTE_SHADER] = computeSrc;

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCode, m_FilePath.string());
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_FilePath.string());
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLComputeShader::~OpenGLComputeShader() { glDeleteProgram(m_RendererID); }

void OpenGLComputeShader::Dispatch(std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ) {
    glUseProgram(m_RendererID);
    glDispatchCompute(groupX, groupY, groupZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

const std::string& OpenGLComputeShader::GetName() const { return m_Name; }

} // namespace iGe
