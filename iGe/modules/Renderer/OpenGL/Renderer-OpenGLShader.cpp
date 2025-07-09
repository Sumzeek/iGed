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

static GLenum ShaderTypeFromString(const std::string& type) {
    if (type == "vertex") { return GL_VERTEX_SHADER; }
    if (type == "fragment" || type == "pixel") { return GL_FRAGMENT_SHADER; }
    if (type == "compute") { return GL_COMPUTE_SHADER; }
    IGE_CORE_ASSERT(false, "Unknown shader type!");
    return 0;
}

} // namespace Utils

/////////////////////////////////////////////////////////////////////////////
// OpenGLShader /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLShader::OpenGLShader(const std::string& filepath) : m_FilePath(filepath) {
    std::string source = ReadFile(filepath);
    auto sources = PreProcess(source);

    Compile(sources);
    CreateProgram();

    // Extract name from filepath
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);
}

//OpenGLShader::OpenGLShader(const std::string& vertFilepath, const std::string& fragFilepath) {
//    std::unordered_map<GLenum, std::string> sources;
//    sources[GL_VERTEX_SHADER] = ReadFile(vertFilepath);
//    sources[GL_FRAGMENT_SHADER] = ReadFile(fragFilepath);
//
//    Compile(sources);
//    CreateProgram();
//}

OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
    : m_Name(name) {
    std::unordered_map<GLenum, std::string> sources;
    sources[GL_VERTEX_SHADER] = vertexSrc;
    sources[GL_FRAGMENT_SHADER] = fragmentSrc;

    Compile(sources);
    CreateProgram();
}

OpenGLShader::~OpenGLShader() { glDeleteProgram(m_RendererID); }

void OpenGLShader::Bind() const { glUseProgram(m_RendererID); }

void OpenGLShader::Unbind() const { glUseProgram(0); }

const std::string& OpenGLShader::GetName() const { return m_Name; }

std::string OpenGLShader::ReadFile(const std::string& filepath) {
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
            IGE_CORE_ERROR("Could not read from file '{0}'", filepath);
        }
    } else {
        IGE_CORE_ERROR("Could not open file '{0}'", filepath);
    }

    return result;
}

std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source) {
    std::unordered_map<GLenum, std::string> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLength = std::strlen(typeToken);
    size_t pos = source.find(typeToken, 0); //Start of shader type declaration line
    while (pos != std::string::npos) {
        size_t eol = source.find_first_of("\r\n", pos); //End of shader type declaration line
        IGE_CORE_ASSERT(eol != std::string::npos, "Syntax error");
        size_t begin = pos + typeTokenLength + 1; //Start of shader type name (after "#type " keyword)
        std::string type = source.substr(begin, eol - begin);
        IGE_CORE_ASSERT(Utils::ShaderTypeFromString(type), "Invalid shader type specified");

        size_t nextLinePos =
                source.find_first_not_of("\r\n", eol); //Start of shader code after shader type declaration line
        IGE_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
        pos = source.find(typeToken, nextLinePos); //Start of next shader type declaration line

        shaderSources[Utils::ShaderTypeFromString(type)] =
                (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
}

void OpenGLShader::Compile(std::unordered_map<GLenum, std::string> shaderSources) {
    m_Shaders.clear();
    for (auto&& [stage, source]: shaderSources) {
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
            for (auto& [_, id]: m_Shaders) { glDeleteShader(id); }

            m_Shaders.clear();
            IGE_CORE_ERROR("{0}", infoLog.data());
            IGE_CORE_ASSERT(false, "Fragment shader compilation failure!");
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
        IGE_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_FilePath, infoLog.data());

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
