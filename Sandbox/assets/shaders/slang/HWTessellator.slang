import Common;
import BlinnPhong;
import TessUtils;

// uint(gl_InvocationID) must be replace to gl_InvocationID in GLSL

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
    float u_DisplaceMapScale;
};

[[gl::binding(3)]]
Sampler2D u_DisplaceMap;

struct VertexInput {
    float3 a_Position  : POSITION;
    float3 a_Normal    : NORMAL;
    float2 a_TexCoord  : TEXCOORD;
};

struct VertexOutput {
    float3 v_Position  : POSITION;
    float3 v_Normal    : NORMAL;
    float2 v_TexCoord  : TEXCOORD;
};

[shader("vertex")]
VertexOutput vsMain(VertexInput input, uint vertexID : SV_VertexID)
{
    VertexOutput out;
    out.v_Position  = input.a_Position;
    out.v_TexCoord  = input.a_TexCoord;
    out.v_Normal    = input.a_Normal;
    return out;
}

struct TessControlOutput {
    float3 tc_Position  : POSITION;
    float3 tc_Normal    : NORMAL;
    float2 tc_TexCoord  : TEXCOORD;
};

struct PatchConstantData {
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor[1] : SV_InsideTessFactor;
};

[shader("tesscontrol")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("computeTessFactors")]
TessControlOutput tcsMain(InputPatch<VertexOutput, 3> input, uint i : SV_OutputControlPointID)
{
    TessControlOutput out;
    out.tc_Position = input[i].v_Position;
    out.tc_Normal = input[i].v_Normal;
    out.tc_TexCoord = input[i].v_TexCoord;

    return out;
}

// Patch Constant Function
PatchConstantData computeTessFactors(InputPatch<VertexOutput, 3> input) {
    float3 v[3]= float3[3](
        input[0].v_Position,
        input[1].v_Position,
        input[2].v_Position
    );

    float4x4 mvp = mul(u_ProjectionView, u_Transform);
    uint packed = PackTessFactor(v, mvp, u_ScreenSize);

    uint r,g,b,a;
    UnpackUintToRGBA8(packed, r, g, b, a);

    PatchConstantData pcd;
    pcd.EdgeTessFactor[0] = u_TargetTessFactor;
    pcd.EdgeTessFactor[1] = u_TargetTessFactor;
    pcd.EdgeTessFactor[2] = u_TargetTessFactor;
    pcd.InsideTessFactor[0] = u_TargetTessFactor;
    // pcd.EdgeTessFactor[0] = r;
    // pcd.EdgeTessFactor[1] = g;
    // pcd.EdgeTessFactor[2] = b;
    // pcd.InsideTessFactor[0] = a;

    return pcd;
}

struct TessEvalOutput {
    float4 te_Position  : SV_Position;
    float3 te_WorldPos  : POSITION;
    float3 te_Normal    : NORMAL;
    float2 te_TexCoord  : TEXCOORD;
};

[shader("tesseval")]
[domain("tri")]
TessEvalOutput tesMain(
    float3 bary : SV_DomainLocation,
    const OutputPatch<TessControlOutput, 3> input,
    PatchConstantData tessData)
{
    float3 p0 = input[0].tc_Position;
    float3 p1 = input[1].tc_Position;
    float3 p2 = input[2].tc_Position;
    float3 interpolatedPosition = p0 * bary.x + p1 * bary.y + p2 * bary.z;

    float3 n0 = input[0].tc_Normal;
    float3 n1 = input[1].tc_Normal;
    float3 n2 = input[2].tc_Normal;
    float3 interpolatedNormal = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);

    float2 uv0 = input[0].tc_TexCoord;
    float2 uv1 = input[1].tc_TexCoord;
    float2 uv2 = input[2].tc_TexCoord;
    float2 interpolatedTexCoord = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;

    TessEvalOutput out;

    // Displacement Mapping
    float displacement = u_DisplaceMap.Sample(interpolatedTexCoord).r;
    float3 position = interpolatedPosition + displacement * interpolatedNormal * u_DisplaceMapScale;

    out.te_Position = mul(u_ProjectionView, mul(u_Transform, float4(position, 1.0f)));
    out.te_WorldPos = mul(u_Transform, float4(position, 1.0f)).xyz;
    out.te_Normal = interpolatedNormal;
    out.te_TexCoord = interpolatedTexCoord;

    return out;
}

static const float3 ambient = float3(0.4f, 0.4f, 0.4f);
static const Light light = {
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
};

[shader("fragment")]
float4 fsMain(TessEvalOutput input) : SV_Target
{
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);
    float3 color = float3(0.0f, 0.0f, 0.0f);

    float3 fdx = ddx(input.te_WorldPos);
    float3 fdy = ddy(input.te_WorldPos);
    float3 normal = normalize(cross(fdx, fdy));
    float3 viewPosition = mul(u_View, float4(input.te_WorldPos, 1.0f)).xyz;
    if (dot(normal, viewPosition) > 0.0f) {
        normal = -1.0f * normal;
    }

    // ambient
    color += ambient * baseColor;

    // lighing
    float3 lighting = BlinnPhong(u_ViewPos, input.te_WorldPos, normal, light);
    color += lighting * baseColor;

    return float4(color, 1.0f);
}
