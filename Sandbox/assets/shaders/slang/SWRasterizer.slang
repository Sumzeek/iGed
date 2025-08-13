import RasterizerUtils;
import BlinnPhong;

// GLSL need #extension GL_NV_shader_atomic_int64 : require

#define SUBPIXEL_SAMPLES 4
#define BACKFACE_CULL false

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
    float u_DisplaceMapScale;
};

[[gl::binding(5)]]
RWTexture2D<uint> u_DepthBuffer;

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

	RasterTri tri = SetupTriangle<BACKFACE_CULL>(raster, verts, primID);

    Scanline.RasterizeTri(u_Packed64Buffer, tri, u_ScreenSize);
    // Scanline.RasterizeTri(u_DepthBuffer, tri);
	// EdgeFunction.RasterizeTri(u_DepthBuffer, tri);
}
