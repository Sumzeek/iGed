import Common;
import BlinnPhong;

[[gl::binding(0)]]
cbuffer SceneData {
    float4x4 u_View;
    float4x4 u_Projection;
    float4x4 u_ProjectionView;
    float4x4 u_Transform;
};

[[gl::binding(1)]]
cbuffer PerFrameData {
    float3 u_ViewPos;
    float _padding_u_ViewPos;
    float4x4 u_Normal;
};

[[gl::binding(2)]]
cbuffer TessellatorData {
    uint2 u_ScreenSize;
    uint u_TriSize; // Number of coarse triangles
    uint u_DisplaceMapScale;
};

[[gl::binding(5)]]
RWTexture2D<float> u_DepthBuffer;

[[gl::binding(6)]]
RWStructuredBuffer<uint64_t> u_Packed64Buffer; // high 32 bit: depth value, low 32 bit: color

[[gl::binding(10)]]
RWStructuredBuffer<float> u_VertexBuffer;

[[gl::binding(11)]]
RWStructuredBuffer<uint> u_IndexBuffer;

float3 GetVertex(uint vertexIndex) {
    uint base = vertexIndex * 3;
    return float3(u_VertexBuffer[base], u_VertexBuffer[base + 1], u_VertexBuffer[base + 2]);
}

struct VertexOutput {
    float4 v_Position : SV_Position;
    float2 v_TexCoord : TEXCOORD;
};

[shader("vertex")]
VertexOutput vsMain(uint vertexID : SV_VertexID)
{
    VertexOutput out;

    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    out.v_Position = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    out.v_TexCoord = uv;

    return out;
}

static const float3 ambient = float3(0.4f, 0.4f, 0.4f);
static const Light light = {
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
};

[shader("fragment")]
float4 fsMain(VertexOutput input) : SV_Target
{
    // uint2 pixelPos = uint2(input.v_TexCoord * float2(u_ScreenSize.x, u_ScreenSize.y));
    // float depth = u_DepthBuffer[pixelPos];
    // return float4(depth.xxx, 1.0);

    // Parse visibility buffer
    uint2 pixelPos = uint2(input.v_TexCoord * float2(u_ScreenSize));
    uint pixelIndex = pixelPos.y * u_ScreenSize.x + pixelPos.x;

    uint64_t visBuffer = u_Packed64Buffer[pixelIndex];
    uint depthBits = uint(visBuffer >> 32);
    uint packedPrimBary = uint(visBuffer & 0xFFFFFFFFul);

    // Check if the pixel is invalid
    if (depthBits == 0xFFFFFFFFu) {
        return float4(0.5f, 0.5f, 0.5f, 1.0f); // Debug fallback color
    }

    float depth = asfloat(depthBits);

    // Unpack primitive ID and barycentric coordinates
    uint primID;
    float3 barycentric;
    UnpackIdBarycentricFromUint(packedPrimBary, primID, barycentric);

    // Fetch triangle vertices in local space
    uint baseIndex = primID * 3;
    float3 vertices[3] = {
        GetVertex(u_IndexBuffer[baseIndex]),
        GetVertex(u_IndexBuffer[baseIndex + 1]),
        GetVertex(u_IndexBuffer[baseIndex + 2])
    };

    // Interpolate position in local space
    float3 localPos = vertices[0] * barycentric.x +
                      vertices[1] * barycentric.y +
                      vertices[2] * barycentric.z;

    // Transform vertices and position to world space
    float3 worldVertices[3];
    [unroll]
    for (uint i = 0; i < 3; ++i) {
        worldVertices[i] = mul(u_Transform, float4(vertices[i], 1.0f)).xyz;
    }
    float3 worldPos = mul(u_Transform, float4(localPos, 1.0f)).xyz;

    // Compute face normal
    float3 normal = normalize(cross(worldVertices[1] - worldVertices[0],
                                    worldVertices[2] - worldVertices[0]));

    // Flip normal if back-facing
    float3 viewDir = normalize(u_ViewPos - worldPos);
    if (dot(normal, viewDir) < 0.0f) {
        normal = -normal;
    }

    // Lighting calculations
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);
    float3 color = ambient * baseColor +
                   BlinnPhong(u_ViewPos, worldPos, normal, light) * baseColor;

    return float4(color, 1.0f);
}
