module;
#include "iGeMacro.h"

export module iGe.Renderer:Texture;
import std;
import iGe.SmartPointer;

namespace iGe
{

export enum class ImageFormat : int { None = 0, R8, RGB8, RGBA8, R32UI, R32F, RG32F, RGB32F };

export struct TextureSpecification {
    std::uint32_t Width = 1;
    uint32_t Height = 1;
    ImageFormat Format = ImageFormat::RGBA8;
    bool GenerateMips = true;
};

export struct IGE_API Texture {
public:
    virtual ~Texture() = default;

    virtual const TextureSpecification& GetSpecification() const = 0;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual uint32_t GetRendererID() const = 0;

    virtual const std::filesystem::path& GetPath() const = 0;

    virtual void SetData(void* data, uint32_t size) = 0;

    virtual void Bind(uint32_t slot = 0) const = 0;
    virtual void BindImage(uint32_t binding = 0) const = 0;

    virtual bool IsLoaded() const = 0;

    virtual bool operator==(const Texture& other) const = 0;
};

export class IGE_API Texture2D : public Texture {
public:
    static Ref<Texture2D> Create(const TextureSpecification& specification);
    static Ref<Texture2D> Create(const std::string& path);
};

} // namespace iGe
