module;
#include <glad/gl.h>

module iGe.Renderer;
import :OpenGLShader;

namespace iGe
{
namespace Utils
{
static GLenum ShaderTypeFromStage(const ShaderStage stage) {
    if (stage == ShaderStage::Vertex) { return GL_VERTEX_SHADER; }
    if (stage == ShaderStage::TessellationControl) { return GL_TESS_CONTROL_SHADER; }
    if (stage == ShaderStage::TessellationEvaluation) { return GL_TESS_EVALUATION_SHADER; }
    if (stage == ShaderStage::Geometry) { return GL_GEOMETRY_SHADER; }
    if (stage == ShaderStage::Fragment) { return GL_FRAGMENT_SHADER; }
    if (stage == ShaderStage::Compute) { return GL_COMPUTE_SHADER; }

    Internal::Assert(false, "Unknown shader type!");
    return 0;
}
} // namespace Utils

/////////////////////////////////////////////////////////////////////////////
// OpenGLShader /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLShader::OpenGLShader(const std::filesystem::path& filepath) : m_FilePath(filepath) {
    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = ReadFile(shaderFile);
    }

    Compile(m_SourceCodes);
    CreateProgram();
    m_Name = filepath.stem().string();
}

OpenGLShader::OpenGLShader(const string& name, const std::filesystem::path& filepath)
    : m_Name(name), m_FilePath(filepath) {
    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = ReadFile(shaderFile);
    }

    Compile(m_SourceCodes);
    CreateProgram();
}

OpenGLShader::~OpenGLShader() { glDeleteProgram(m_RendererID); }

void OpenGLShader::Bind() const { glUseProgram(m_RendererID); }

void OpenGLShader::Unbind() const { glUseProgram(0); }

const string& OpenGLShader::GetName() const { return m_Name; }

string OpenGLShader::ReadFile(const std::filesystem::path& filepath) {
    // Check if file exists
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        Internal::LogError("File does not exist: '{0}' (error: {1})", filepath.string(), ec.message());
        return {};
    }

    // Get the file size
    auto fileSize = std::filesystem::file_size(filepath, ec);
    if (ec) {
        Internal::LogError("Could not get file size: '{0}' (error: {1})", filepath.string(), ec.message());
        return {};
    }

    // Read file content
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        Internal::LogError("Could not open file: '{0}'", filepath.string());
        return {};
    }

    string content;
    content.resize(fileSize);
    file.read(content.data(), fileSize);

    if (!file) {
        Internal::LogError("Could not read file: '{0}'", filepath.string());
        return {};
    }

    return content;
}

void OpenGLShader::Compile(std::unordered_map<ShaderStage, string> shaderSources) {
    m_Shaders.clear();
    for (auto&& [stage, source]: shaderSources) {
        // Create an empty fragment shader handle
        GLenum type = Utils::ShaderTypeFromStage(stage);
        GLuint shaderID = glCreateShader(type);

        // Send the fragment shader source code to GL
        // Note that string's .c_str is NULL character terminated.
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
            for (auto& [_, id]: m_Shaders) { glDeleteShader(id); }

            m_Shaders.clear();
            Internal::LogError("{0}", infoLog.data());
            Internal::Assert(false, "Fragment shader compilation failure!");
            return;
        }

        m_Shaders[stage] = shaderID;
    }

    return;
}

void OpenGLShader::CreateProgram() {
    GLuint program = glCreateProgram();

    // Link shaders together into a program.
    std::vector<GLuint> shaderIDs;
    for (auto&& [stage, shaderID]: m_Shaders) {
        //GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
        //glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size() * sizeof(uint32_t));
        //glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
        glAttachShader(program, shaderID);
    }

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
        Internal::LogError("Shader linking failed ({0}):\n{1}", m_Name, infoLog.data());

        // We don't need the program anymore
        glDeleteProgram(program);

        // Don't leak shaders either
        for (auto id: shaderIDs) { glDeleteShader(id); }
    }

    // Always detach shaders after a successful link
    for (auto id: shaderIDs) {
        glDetachShader(program, id);
        glDeleteShader(id);
    }

    m_RendererID = program;
}
} // namespace iGe
