import RasterizerUtils;

#define SUBPIXEL_SAMPLES 4
#define BACKFACE_CULL false

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
RWTexture2D<uint64_t> u_VisBuffer; // pixel's z-rgb

[[gl::binding(5)]]
RWTexture2D<uint> u_DepthBuffer;

[[gl::binding(6)]]
RWTexture2D<float4> u_ColorBuffer;

float3 GetVertex(uint vertexIndex) {
    uint base = vertexIndex * 3;
    return float3(u_VertexBuffer[base], u_VertexBuffer[base + 1], u_VertexBuffer[base + 2]);
}

[shader("compute")]
[numthreads(32, 1, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
    uint primID = threadId.x;
    if (primID >= u_TriSize) { return; }

    // Get coarse triangle
    float3 v[3]= float3[3](
        GetVertex(u_IndexBuffer[primID * 3]),
        GetVertex(u_IndexBuffer[primID * 3 + 1]),
        GetVertex(u_IndexBuffer[primID * 3 + 2])
    );

    // rasterization
    Raster raster = CreateRaster<SUBPIXEL_SAMPLES>(u_ScreenSize);

    float4x4 mvp = mul(u_ProjectionView, u_Transform);
    float4 verts[3];
    for (uint i = 0; i < 3; ++i) {
        float4 pointClip = mul(mvp, float4(v[i], 1.0f));
        verts[i] = CalculateSubpixelCoordinates(raster, pointClip);
    }

	RasterTri tri = SetupTriangle<BACKFACE_CULL>(raster, verts);

    // Scanline.RasterizeTri(u_VisBuffer, tri);
    Scanline.RasterizeTri(u_DepthBuffer, tri);;
	// EdgeFunction.RasterizeTri(u_DepthBuffer, tri);
}
