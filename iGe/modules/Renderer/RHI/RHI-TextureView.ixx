module;
#include "iGeMacro.h"

export module iGe.RHI:RHITextureView;
import :RHIResource;
import :RHITexture;
import iGe.Common;

namespace iGe
{

export enum class RHIComponentSwizzle : uint32 {
    Identity = 0,
    Zero,
    One,
    R,
    G,
    B,
    A,

    Count
};

export struct RHIComponentMapping {
    RHIComponentSwizzle R = RHIComponentSwizzle::Identity;
    RHIComponentSwizzle G = RHIComponentSwizzle::Identity;
    RHIComponentSwizzle B = RHIComponentSwizzle::Identity;
    RHIComponentSwizzle A = RHIComponentSwizzle::Identity;

    static RHIComponentMapping Identity() {
        return {RHIComponentSwizzle::Identity, RHIComponentSwizzle::Identity, RHIComponentSwizzle::Identity,
                RHIComponentSwizzle::Identity};
    }
};

export enum class RHITextureViewType : uint32 {
    View1D = 0,
    View2D,
    View3D,
    ViewCube,
    View1DArray,
    View2DArray,
    ViewCubeArray,

    Count
};

export struct RHITextureViewCreateInfo {
    RHITextureViewType ViewType = RHITextureViewType::View2D;
    RHIFormat Format = RHIFormat::Unknown; // If Unknown, use texture's format

    // Component swizzle (for channel remapping)
    RHIComponentMapping Components = RHIComponentMapping::Identity();
};

// =================================================================================================
// Texture View Class
// =================================================================================================

export class IGE_API RHITextureView : public RHIResource {
public:
    ~RHITextureView() override = default;

    RHITextureViewType GetViewType() const { return m_ViewType; }
    RHIFormat GetFormat() const { return m_Format; }

protected:
    RHITextureView(const RHITextureViewCreateInfo& info)
        : RHIResource(RHIResourceType::TextureView), m_ViewType(info.ViewType), m_Format(info.Format),
          m_Components(info.Components) {}

    RHITextureViewType m_ViewType;
    RHIFormat m_Format;
    RHIComponentMapping m_Components;
};

} // namespace iGe
