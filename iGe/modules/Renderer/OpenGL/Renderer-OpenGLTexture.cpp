module;
#include <glad/gl.h>
#include <stb_image.h>

module iGe.Renderer;
import :Texture;
import :OpenGLTexture;

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
    }

    Internal::Assert(false);
    return 0;
}

static GLenum iGeImageFormatToGLInternalFormat(ImageFormat format) {
    switch (format) {
        case ImageFormat::RGB8:
            return GL_RGB8;
        case ImageFormat::RGBA8:
            return GL_RGBA8;
    }

    Internal::Assert(false);
    return 0;
}
} // namespace Utils

/////////////////////////////////////////////////////////////////////////////
// OpenGLContext ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification)
    : m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height) {
    m_InternalFormat = Utils::iGeImageFormatToGLInternalFormat(m_Specification.Format);
    m_DataFormat = Utils::iGeImageFormatToGLDataFormat(m_Specification.Format);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

OpenGLTexture2D::OpenGLTexture2D(const string& path) : m_Path(path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = nullptr;
    {
        //IGE_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const string&)");
        data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }

    if (data) {
        m_IsLoaded = true;

        m_Width = width;
        m_Height = height;

        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4) {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        } else if (channels == 3) {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }

        m_InternalFormat = internalFormat;
        m_DataFormat = dataFormat;

        Internal::Assert(internalFormat & dataFormat, "Format not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    } else {
        Internal::LogWarn("Failed to load image from path: {0}", path);
    }
}

OpenGLTexture2D::~OpenGLTexture2D() { glDeleteTextures(1, &m_RendererID); }

const TextureSpecification& OpenGLTexture2D::GetSpecification() const { return m_Specification; }

uint32 OpenGLTexture2D::GetWidth() const { return m_Width; }

uint32 OpenGLTexture2D::GetHeight() const { return m_Height; }

uint32 OpenGLTexture2D::GetRendererID() const { return m_RendererID; }

const string& OpenGLTexture2D::GetPath() const { return m_Path; }

void OpenGLTexture2D::SetData(void* data, uint32 size) {
    uint32 bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    Internal::Assert(size == m_Width * m_Height * bpp, "Data must be entire texture!");
    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
}

void OpenGLTexture2D::Bind(uint32 slot) const { glBindTextureUnit(slot, m_RendererID); }

bool OpenGLTexture2D::IsLoaded() const { return m_IsLoaded; }

bool OpenGLTexture2D::operator==(const Texture& other) const { return m_RendererID == other.GetRendererID(); }
} // namespace iGe
