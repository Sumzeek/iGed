module;
#include "iGeMacro.h"
#include <glad/gl.h>
#include <stb_image.h>

module iGe.Renderer;
import :OpenGLTexture;
import iGe.Log;

namespace iGe
{

namespace Utils
{

static GLenum iGeImageFormatToGLDataFormat(ImageFormat format) {
    switch (format) {
        case ImageFormat::RGB8:
            return GL_RGB;
        case ImageFormat::RGBA8:
            return GL_RGBA;
        case ImageFormat::R32UI:
            return GL_RED_INTEGER;
        case ImageFormat::R32F:
            return GL_RED;
    }

    IGE_CORE_ASSERT(false);
    return 0;
}

static GLenum iGeImageFormatToGLInternalFormat(ImageFormat format) {
    switch (format) {
        case ImageFormat::RGB8:
            return GL_RGB8;
        case ImageFormat::RGBA8:
            return GL_RGBA8;
        case ImageFormat::R32UI:
            return GL_R32UI;
        case ImageFormat::R32F:
            return GL_R32F;
    }

    IGE_CORE_ASSERT(false);
    return 0;
}

static GLenum iGeImageFormatToGLType(ImageFormat format) {
    switch (format) {
        case ImageFormat::RGB8:
        case ImageFormat::RGBA8:
        case ImageFormat::R32UI:
            return GL_UNSIGNED_BYTE;
        case ImageFormat::R32F:
            return GL_FLOAT;
    }

    IGE_CORE_ASSERT(false);
    return 0;
}

} // namespace Utils

/////////////////////////////////////////////////////////////////////////////
// OpenGLContext ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification) : m_Specification(specification) {
    uint32_t width = m_Specification.Width;
    uint32_t height = m_Specification.Height;
    GLenum internalFormat = Utils::iGeImageFormatToGLInternalFormat(m_Specification.Format);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, internalFormat, width, height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

OpenGLTexture2D::OpenGLTexture2D(const std::filesystem::path& path) : m_Path(path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = nullptr;
    {
        //IGE_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
        data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    }

    if (data) {
        m_IsLoaded = true;

        m_Specification.Width = width;
        m_Specification.Height = height;
        if (channels == 4) {
            m_Specification.Format = ImageFormat::RGBA8;
        } else if (channels == 3) {
            m_Specification.Format = ImageFormat::RGB8;
        }

        GLenum internalFormat = Utils::iGeImageFormatToGLInternalFormat(m_Specification.Format);
        GLenum dataFormat = Utils::iGeImageFormatToGLDataFormat(m_Specification.Format);
        GLenum type = Utils::iGeImageFormatToGLType(m_Specification.Format);

        IGE_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, internalFormat, width, height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureSubImage2D(m_RendererID, 0, 0, 0, width, height, dataFormat, type, data);

        stbi_image_free(data);
    } else {
        IGE_CORE_WARN("Failed to load image from path: {0}", path.string());
    }
}

OpenGLTexture2D::~OpenGLTexture2D() { glDeleteTextures(1, &m_RendererID); }

const TextureSpecification& OpenGLTexture2D::GetSpecification() const { return m_Specification; }

uint32_t OpenGLTexture2D::GetWidth() const { return m_Specification.Width; }

uint32_t OpenGLTexture2D::GetHeight() const { return m_Specification.Height; }

uint32_t OpenGLTexture2D::GetRendererID() const { return m_RendererID; }

const std::filesystem::path& OpenGLTexture2D::GetPath() const { return m_Path; }

void OpenGLTexture2D::SetData(void* data, uint32_t size) {
    uint32_t width = m_Specification.Width;
    uint32_t height = m_Specification.Height;
    GLenum dataFormat = Utils::iGeImageFormatToGLDataFormat(m_Specification.Format);
    GLenum type = Utils::iGeImageFormatToGLType(m_Specification.Format);

    uint32_t bpp;
    switch (m_Specification.Format) {
        case ImageFormat::RGB8:
            bpp = 3;
            break;
        case ImageFormat::RGBA8:
        case ImageFormat::R32UI:
        case ImageFormat::R32F:
            bpp = 4;
            break;
        default:
            IGE_CORE_ASSERT(false, "Unsupported texture format in SetData()");
            return;
    }
    IGE_CORE_ASSERT(size == width * height * bpp, "Data must be entire texture!");

    glTextureSubImage2D(m_RendererID, 0, 0, 0, width, height, dataFormat, type, data);
}

void OpenGLTexture2D::Bind(uint32_t slot) const { glBindTextureUnit(slot, m_RendererID); }

void OpenGLTexture2D::BindImage(uint32_t binding) const {
    glBindImageTexture(binding, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

bool OpenGLTexture2D::IsLoaded() const { return m_IsLoaded; }

bool OpenGLTexture2D::operator==(const Texture& other) const { return m_RendererID == other.GetRendererID(); }

} // namespace iGe
