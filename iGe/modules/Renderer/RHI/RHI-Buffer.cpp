module iGe.RHI;
import :RHIBuffer;

namespace iGe
{

// =================================================================================================
// Static Method
// =================================================================================================

uint32 UBElementTypeSize(UBElementType type) {
    switch (type) {
        case UBElementType::Float:
            return 4;
        case UBElementType::Float2:
            return 4 * 2;
        case UBElementType::Float3:
            return 4 * 3;
        case UBElementType::Float4:
            return 4 * 4;
        case UBElementType::Int:
            return 4;
        case UBElementType::Int2:
            return 4 * 2;
        case UBElementType::Int3:
            return 4 * 3;
        case UBElementType::Int4:
            return 4 * 4;
        case UBElementType::Bool:
            return 1;
        case UBElementType::Float4x4:
            return 4 * 4 * 4;
    }

    Internal::Assert(false, "Unknown UBElementType!");
    return 0;
}

// =================================================================================================
// UBElement
// =================================================================================================

UBElement::UBElement(UBElementType type, const std::string& name)
    : Name{name}, Type{type}, Offset{0}, Size{UBElementTypeSize(type)} {}

// =================================================================================================
// UniformBufferLayout
// =================================================================================================

UniformBufferLayout::UniformBufferLayout(std::initializer_list<UBElement> elements) : Elements(elements) {
    CalculateOffsets();
}

void UniformBufferLayout::CalculateOffsets() {
    size_t offset = 0;
    for (auto& element: Elements) {
        element.Offset = static_cast<uint32>(offset);
        offset += element.Size;
    }
    Size = static_cast<uint32>(offset);
}

} // namespace iGe
