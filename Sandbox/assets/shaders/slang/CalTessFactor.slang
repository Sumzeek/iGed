import TessUtils;

[[gl::binding(0)]]
cbuffer SceneData {
    float4x4 u_ProjectionView;
    float4x4 u_Transform;
};

[[gl::binding(1)]]
cbuffer TessData {
    uint2 u_ScreenSize;
    uint u_TriSize; // Number of coarse triangles
    uint _padding_u_TriSize;
};

[[gl::binding(10)]]
RWStructuredBuffer<float> u_VertexBuffer;

[[gl::binding(11)]]
RWStructuredBuffer<uint> u_IndexBuffer;

[[gl::binding(12)]]
RWStructuredBuffer<uint2> u_TessFactorBuffer;

[[gl::binding(13)]]
RWStructuredBuffer<uint> u_CounterBuffer;

float3 GetVertex(uint vertexIndex) {
    uint base = vertexIndex * 3;
    return float3(u_VertexBuffer[base], u_VertexBuffer[base + 1], u_VertexBuffer[base + 2]);
}

[shader("compute")]
[numthreads(32, 1, 1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint primID = threadId.x;
    if (primID >= u_TriSize) { return; }

    // Get coarse triangle
    float3 v[3]= float3[3](
        GetVertex(u_IndexBuffer[primID * 3]),
        GetVertex(u_IndexBuffer[primID * 3 + 1]),
        GetVertex(u_IndexBuffer[primID * 3 + 2])
    );

    // Calculate tessellation factor
    float4x4 mvp = mul(u_ProjectionView, u_Transform);
    uint packed = PackTessFactor(v, mvp, u_ScreenSize);

    uint index;
    InterlockedAdd(u_CounterBuffer[0], 1, index);
    u_TessFactorBuffer[index] = uint2(packed, primID);
}