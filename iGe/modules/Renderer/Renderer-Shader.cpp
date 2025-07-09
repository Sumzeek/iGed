module;
#include "iGeMacro.h"

module iGe.Renderer;
import :Shader;
import :Renderer;
import :OpenGLShader;

import std;
import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Shader ///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<Shader> Shader::Create(const std::string filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLShader>(filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// ShaderLibrary ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader) {
    IGE_CORE_ASSERT(!Exists(name), "Shader already exists!");
    m_Shaders[name] = shader;
}

void ShaderLibrary::Add(const Ref<Shader>& shader) {
    auto& name = shader->GetName();
    IGE_CORE_ASSERT(!Exists(name), "Shader already exists!");
    Add(name, shader);
}

Ref<Shader> ShaderLibrary::Load(const std::string& filepath) {
    auto shader = Shader::Create(filepath);
    Add(shader);
    return shader;
}

Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath) {
    auto shader = Shader::Create(filepath);
    Add(name, shader);
    return shader;
}

Ref<Shader> ShaderLibrary::Get(const std::string& name) {
    IGE_CORE_ASSERT(Exists(name), "Shader not found!");
    return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name) const { return m_Shaders.find(name) != m_Shaders.end(); }

} // namespace iGe