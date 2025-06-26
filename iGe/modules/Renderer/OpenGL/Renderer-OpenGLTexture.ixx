module;
#include "iGeMacro.h"

#include <glad/gl.h>

export module iGe.Renderer:OpenGLTexture;
import std;
import :Texture;

namespace iGe
{

export class IGE_API OpenGLTexture2D : public Texture2D {
public:
    OpenGLTexture2D(const TextureSpecification& specification);
    OpenGLTexture2D(const std::filesystem::path& path);
    virtual ~OpenGLTexture2D();

    virtual const TextureSpecification& GetSpecification() const override;

    virtual uint32_t GetWidth() const override;
    virtual uint32_t GetHeight() const override;
    virtual uint32_t GetRendererID() const override;

    virtual const std::filesystem::path& GetPath() const override;

    virtual void SetData(void* data, uint32_t size) override;

    virtual void Bind(uint32_t slot = 0) const override;
    virtual void BindImage(uint32_t binding) const override;

    virtual bool IsLoaded() const override;

    virtual bool operator==(const Texture& other) const override;

private:
    TextureSpecification m_Specification;

    std::filesystem::path m_Path;
    bool m_IsLoaded = false;
    uint32_t m_Width, m_Height;
    uint32_t m_RendererID;
    GLenum m_InternalFormat, m_DataFormat;
};

} // namespace iGe
