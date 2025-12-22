import TessUtils;
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
    uint u_TargetTessFactor;
    uint u_LineOption;
};

[[gl::binding(3)]]
Sampler2D u_DisplaceMap;

[[gl::binding(4)]]
Sampler2D u_NormalMap;

struct TaskData {
    uint baseID;
};

[shader("amplification")]
[numthreads(32, 1, 1)]
void ASMain(
    uint3 gid : SV_GroupID,
    uint gtid : SV_GroupThreadID,
    out payload TaskData payload)
{
    // if (gtid == 0) {
    //     payload.baseID = gid.x;
    // }
    // DispatchMesh(1, 1, 1, payload);
}

struct MeshVertex {
    float4 position : SV_Position;
    float3 color    : COLOR;
};

[shader("mesh")]
[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void msMain(
    uint gtid : SV_GroupThreadID,
    in payload TaskData payload,
    OutputVertices<MeshVertex, 64> verts,
    OutputIndices<uint3, 126> triangles
) {
    // Set counts
    SetMeshOutputCounts(3, 1);

    // Write vertices and primitives
    verts[0].position = mul(u_ProjectionView, mul(u_Transform, float4(-0.5f, -0.5f, 0.0f, 1.0f)));
    verts[0].color = float3(1.0f, 0.0f, 0.0f);
    verts[1].position = mul(u_ProjectionView, mul(u_Transform, float4(0.5f, -0.5f, 0.0f, 1.0f)));
    verts[1].color = float3(1.0f, 0.0f, 0.0f);
    verts[2].position = mul(u_ProjectionView, mul(u_Transform, float4(0.0f, 0.5f, 0.0f, 1.0f)));
    verts[2].color = float3(1.0f, 0.0f, 0.0f);
    triangles[0] = uint3(0, 1, 2);
}

[shader("fragment")]
float4 fsMain(MeshVertex input) : SV_Target
{
    return float4(input.color, 1.0f);
}
