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

static GLenum ShaderTypeFromStage(const ShaderStage stage) {
    if (stage == ShaderStage::Vertex) { return GL_VERTEX_SHADER; }
    if (stage == ShaderStage::TessellationControl) { return GL_TESS_CONTROL_SHADER; }
    if (stage == ShaderStage::TessellationEvaluation) { return GL_TESS_EVALUATION_SHADER; }
    if (stage == ShaderStage::Geometry) { return GL_GEOMETRY_SHADER; }
    if (stage == ShaderStage::Fragment) { return GL_FRAGMENT_SHADER; }
    if (stage == ShaderStage::Compute) { return GL_COMPUTE_SHADER; }
    if (stage == ShaderStage::Amplification) { return GL_TASK_SHADER_NV; }
    if (stage == ShaderStage::Mesh) { return GL_MESH_SHADER_NV; }

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

static std::optional<GLuint> CompileAndLinkProgram(const std::unordered_map<ShaderStage, std::string>& sources,
                                                   const std::string& debugOuput = "Unknown Debug Output") {
    // Compile Shaders
    std::unordered_map<GLenum, GLuint> shaders;
    for (auto&& [stage, source]: sources) {
        // Create an empty fragment shader handle
        GLuint shaderID = glCreateShader(ShaderTypeFromStage(stage));

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

        shaders[ShaderTypeFromStage(stage)] = shaderID;
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
OpenGLGraphicsShader::OpenGLGraphicsShader(const std::filesystem::path& filepath)
    : m_FilePath(filepath), m_Name(filepath.stem().string()) {
    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = Utils::ReadFile(shaderFile);
    }

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCodes);
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_Name);
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLGraphicsShader::OpenGLGraphicsShader(const std::string& name, const std::filesystem::path& filepath)
    : m_FilePath(filepath), m_Name(name) {
    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = Utils::ReadFile(shaderFile);
    }

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCodes);
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_Name);
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLGraphicsShader::~OpenGLGraphicsShader() { glDeleteProgram(m_RendererID); }

void OpenGLGraphicsShader::Bind() const { glUseProgram(m_RendererID); }

void OpenGLGraphicsShader::Unbind() const { glUseProgram(0); }

const std::string& OpenGLGraphicsShader::GetName() const { return m_Name; }

/////////////////////////////////////////////////////////////////////////////
// OpenGLComputeShader //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLComputeShader::OpenGLComputeShader(const std::filesystem::path& filepath) {
    m_Name = filepath.stem().string();

    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = Utils::ReadFile(shaderFile);
    }

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCodes);
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_Name);
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLComputeShader::OpenGLComputeShader(const std::string& name, const std::filesystem::path& filepath)
    : m_Name(name) {
    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = Utils::ReadFile(shaderFile);
    }

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCodes);
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_Name);
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLComputeShader::~OpenGLComputeShader() { glDeleteProgram(m_RendererID); }

void OpenGLComputeShader::Bind() const { glUseProgram(m_RendererID); }

void OpenGLComputeShader::Unbind() const { glUseProgram(0); }

void OpenGLComputeShader::Dispatch(std::uint32_t groupX, std::uint32_t groupY, std::uint32_t groupZ) {
    glUseProgram(m_RendererID);
    glDispatchCompute(groupX, groupY, groupZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

const std::string& OpenGLComputeShader::GetName() const { return m_Name; }

/////////////////////////////////////////////////////////////////////////////
// OpenGLMeshShader /////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLMeshShader::OpenGLMeshShader(const std::filesystem::path& filepath) {
    m_Name = filepath.stem().string();

    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = Utils::ReadFile(shaderFile);
    }

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCodes);
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_Name);
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLMeshShader::OpenGLMeshShader(const std::string& name, const std::filesystem::path& filepath) : m_Name(name) {
    auto shaderMap = ParseShaderEntryMap(filepath);
    for (const auto& shaderPair: shaderMap) {
        ShaderStage stage = shaderPair.first;
        const auto& shaderFile = shaderPair.second;
        m_SourceCodes[stage] = Utils::ReadFile(shaderFile);
    }

    // Create Shader
    auto maybeProgram = Utils::CompileAndLinkProgram(m_SourceCodes);
    if (!maybeProgram.has_value()) {
        IGE_CORE_ERROR("Shader creation failed: {}", m_Name);
        IGE_CORE_ASSERT(false, "shader compilation failure!");
        return;
    }
    m_RendererID = *maybeProgram;
}

OpenGLMeshShader::~OpenGLMeshShader() { glDeleteProgram(m_RendererID); }

void OpenGLMeshShader::Bind() const { glUseProgram(m_RendererID); }

void OpenGLMeshShader::Unbind() const { glUseProgram(0); }

void OpenGLMeshShader::DispatchTask(std::uint32_t offset, std::uint32_t count) {
    glUseProgram(m_RendererID);
    glDrawMeshTasksNV(offset, count);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

const std::string& OpenGLMeshShader::GetName() const { return m_Name; }

} // namespace iGe
