import Common;

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
RWTexture2D<uint> u_DepthBuffer;

[[gl::binding(6)]]
RWStructuredBuffer<uint64_t> u_Packed64Buffer; // high 32 bit: depth value, low 32 bit: color

[shader("compute")]
[numthreads(8, 8, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
    if (threadId.x >= u_ScreenSize.x || threadId.y >= u_ScreenSize.y) {
        return;
    }

    uint2 pixelPos = uint2(threadId.x, threadId.y);
    u_DepthBuffer[pixelPos] = asuint(1.0f);

    uint pixelIndex = threadId.y * u_ScreenSize.x + threadId.x;
    u_Packed64Buffer[pixelIndex] = uint64_t(0xFFFFFFFF) << 32;
}
