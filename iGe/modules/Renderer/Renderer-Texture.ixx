module;
#include "iGeMacro.h"

export module iGe.Renderer:Texture;
import iGe.Common;

namespace iGe
{
export enum class ImageFormat : int { None = 0, R8, RGB8, RGBA8, RGBA32F };

export struct TextureSpecification {
    uint32 Width = 1;
    uint32 Height = 1;
    ImageFormat Format = ImageFormat::RGBA8;
    bool GenerateMips = true;
};

export struct IGE_API Texture {
public:
    virtual ~Texture() = default;

    virtual const TextureSpecification& GetSpecification() const = 0;

    virtual uint32 GetWidth() const = 0;
    virtual uint32 GetHeight() const = 0;
    virtual uint32 GetRendererID() const = 0;

    virtual const string& GetPath() const = 0;

    virtual void SetData(void* data, uint32 size) = 0;

    virtual void Bind(uint32 slot = 0) const = 0;

    virtual bool IsLoaded() const = 0;

    virtual bool operator==(const Texture& other) const = 0;
};

export class IGE_API Texture2D : public Texture {
public:
    static Ref<Texture2D> Create(const TextureSpecification& specification);
    static Ref<Texture2D> Create(const string& path);
};
} // namespace iGe
