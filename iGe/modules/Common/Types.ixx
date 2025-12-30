export module iGe.Types;

export import std;
export import glm;

// =====================
// Basic Integer Types
// =====================
export using uint8 = std::uint8_t;
export using uint16 = std::uint16_t;
export using uint32 = std::uint32_t;
export using uint64 = std::uint64_t;

export using int8 = std::int8_t;
export using int16 = std::int16_t;
export using int32 = std::int32_t;
export using int64 = std::int64_t;

// =====================
// Floating Point Types
// =====================
export using float32 = float;
export using float64 = double;

// =====================
// Pointer Integer Types
// =====================
export using intptr = std::intptr_t;
export using uintptr = std::uintptr_t;

// =====================
// Size / Index Types
// =====================
export using size32 = std::uint32_t;
export using size64 = std::uint64_t;

// =====================
// Math Types (GLM)
// =====================
export using float2 = glm::vec2;
export using float3 = glm::vec3;
export using float4 = glm::vec4;

export using float3x3 = glm::mat3;
export using float4x4 = glm::mat4;

// =====================
// String
// =====================
export using string = std::string;
