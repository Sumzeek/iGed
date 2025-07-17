import BlinnPhong;

struct VertexInput {
    float3 a_Position  : POSITION;
    float3 a_Normal    : NORMAL;
    float2 a_TexCoord  : TEXCOORD;
    float3 a_Tangent   : TANGENT;
    float3 a_BiTangent : BITANGENT;
};

struct VertexOutput {
    float3 v_Position  : POSITION;
    float3 v_Normal    : NORMAL;
    float2 v_TexCoord  : TEXCOORD;
    float3 v_Tangent   : TANGENT;
    float3 v_BiTangent : BITANGENT;
};

[shader("vertex")]
VertexOutput vsMain(VertexInput input, uint vertexID : SV_VertexID)
{
    VertexOutput out;
    out.v_Position  = input.a_Position;
    out.v_TexCoord  = input.a_TexCoord;
    out.v_Normal    = input.a_Normal;
    out.v_Tangent   = input.a_Tangent;
    out.v_BiTangent = input.a_BiTangent;
    return out;
}

struct TessControlOutput {
    float3 tc_Position  : POSITION;
    float3 tc_Normal    : NORMAL;
    float2 tc_TexCoord  : TEXCOORD;
    float3 tc_Tangent   : TANGENT;
    float3 tc_BiTangent : BITANGENT;
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
    out.tc_Tangent = input[i].v_Tangent;
    out.tc_BiTangent = input[i].v_BiTangent;

    return out;
}

// Patch Constant Function
PatchConstantData computeTessFactors(InputPatch<VertexOutput, 3> input) {
    PatchConstantData pcd;

    pcd.EdgeTessFactor[0] = 32.0;
    pcd.EdgeTessFactor[1] = 32.0;
    pcd.EdgeTessFactor[2] = 32.0;

    pcd.InsideTessFactor[0] = 32.0;

    return pcd;
}

struct TessEvalOutput {
    float4 te_Position  : SV_Position;
    float3 te_WorldPos  : POSITION;
    float3 te_Normal    : NORMAL;
    float2 te_TexCoord  : TEXCOORD;
    float3 te_Tangent   : TANGENT;
    float3 te_BiTangent : BITANGENT;
    float3x3 te_TBN     : TBN;
};

[[gl::binding(0)]]
cbuffer SceneData {
    float4x4 u_ProjectionView;
    float4x4 u_Transform;
};

[[gl::binding(1)]]
cbuffer CameraData {
    float3 u_ViewPos;
    float u_DisplaceMapScale;
    float4x4 u_Normal;
};

[[gl::binding(2)]]
Sampler2D u_DisplaceMap;

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
    float3 interpolatedNormal = n0 * bary.x + n1 * bary.y + n2 * bary.z;

    float2 uv0 = input[0].tc_TexCoord;
    float2 uv1 = input[1].tc_TexCoord;
    float2 uv2 = input[2].tc_TexCoord;
    float2 interpolatedTexCoord = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;

    float3 t0 = input[0].tc_Tangent;
    float3 t1 = input[1].tc_Tangent;
    float3 t2 = input[2].tc_Tangent;
    float3 interpolatedTangent = t0 * bary.x + t1 * bary.y + t2 * bary.z;

    float3 bt0 = input[0].tc_BiTangent;
    float3 bt1 = input[1].tc_BiTangent;
    float3 bt2 = input[2].tc_BiTangent;
    float3 interpolatedBiTangent = bt0 * bary.x + bt1 * bary.y + bt2 * bary.z;

    TessEvalOutput out;

    // Displacement Mapping
    float displacement = u_DisplaceMap.Sample(interpolatedTexCoord).r - 0.5f;
    float3 position = interpolatedPosition + displacement * interpolatedNormal * u_DisplaceMapScale;

    // Calculate TBN
    float3 t = normalize(mul(u_Normal, float4(interpolatedTangent, 0.0f)).xyz);
    float3 b = normalize(mul(u_Normal, float4(interpolatedBiTangent, 0.0f)).xyz);
    float3 n = normalize(mul(u_Normal, float4(interpolatedNormal, 0.0f)).xyz);
    float3x3 tbn = float3x3(t, b, n);

    out.te_Position = mul(u_ProjectionView, mul(u_Transform, float4(position, 1.0f)));
    out.te_WorldPos = mul(u_Transform, float4(position, 1.0f)).xyz;
    out.te_Normal = interpolatedNormal;
    out.te_TexCoord = interpolatedTexCoord;
    out.te_Tangent = interpolatedTangent;
    out.te_BiTangent = interpolatedBiTangent;
    out.te_TBN = tbn;

    return out;
}

[[gl::binding(3)]]
Sampler2D u_NormalMap;

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

    // Normal mapping
    // float3 normal = normalize(u_NormalMap.Sample(input.te_TexCoord).rgb);
    // normal = normalize(normal * 2.0f - 1.0f);
    // normal = normalize(mul(input.te_TBN, normal));

    // ambient
    color += ambient * baseColor;

    // lighing
    float3 lighting = BlinnPhong(u_ViewPos, input.te_WorldPos, normal, light);
    color += lighting * baseColor;

    return float4(color, 1.0f);
}

// struct VertexInput {};
//
// struct VertexOutput {
//     float3 v_Position : POSITION;
// };
//
// [shader("vertex")]
// VertexOutput vsMain(VertexInput input, uint vertexID : SV_VertexID)
// {
//     VertexOutput out;
//
//     float2 positions[3] = {
//         float2(0.0f,  0.5f),
//         float2(0.5f, -0.5f),
//         float2(-0.5f, -0.5f)
//     };
//
//     out.v_Position = float3(positions[vertexID], 0.0f);
//     return out;
// }
//
// struct TessControlOutput {
//     float3 tc_Position : POSITION;
// };
//
// struct PatchConstantData {
//     float EdgeTessFactor[3] : SV_TessFactor;
//     float InsideTessFactor[1] : SV_InsideTessFactor;
// };
//
// [shader("tesscontrol")]
// [domain("tri")]
// [partitioning("integer")]
// [outputtopology("triangle_cw")]
// [outputcontrolpoints(3)]
// [patchconstantfunc("computeTessFactors")]
// TessControlOutput tcsMain(InputPatch<VertexOutput, 3> input, uint i : SV_OutputControlPointID)
// {
//     TessControlOutput out;
//     out.tc_Position = input[i].v_Position;
//     return out;
// }
//
// // Patch Constant Function
// PatchConstantData computeTessFactors(InputPatch<VertexOutput, 3> input) {
//     PatchConstantData pcd;
//
//     pcd.EdgeTessFactor[0] = 8.0;
//     pcd.EdgeTessFactor[1] = 8.0;
//     pcd.EdgeTessFactor[2] = 8.0;
//
//     pcd.InsideTessFactor[0] = 8.0;
//
//     return pcd;
// }
//
// struct TessEvalOutput {
//     float4 te_Position : SV_Position;
//     uint te_PrimID        : PRIMID;
// };
//
// [shader("tesseval")]
// [domain("tri")]
// TessEvalOutput tesMain(
//     float3 bary : SV_DomainLocation,
//     const OutputPatch<TessControlOutput, 3> input,
//     PatchConstantData tessData,
//     uint primID : SV_PrimitiveID)
// {
//     TessEvalOutput out;
//
//     float3 p0 = input[0].tc_Position;
//     float3 p1 = input[1].tc_Position;
//     float3 p2 = input[2].tc_Position;
//
//     float3 interpolatedPosition =
//          p0 * bary.x +
//          p1 * bary.y +
//          p2 * bary.z;
//
//     out.te_Position = float4(interpolatedPosition, 1.0);
//     out.te_PrimID = primID;
//     return out;
// }
//
// static const float3 colors[12] = {
//     float3(1.0f, 0.0f, 0.0f), // Red
//     float3(0.0f, 1.0f, 0.0f), // Green
//     float3(0.0f, 0.0f, 1.0f), // Blue
//     float3(1.0f, 1.0f, 0.0f), // Yellow
//     float3(1.0f, 0.0f, 1.0f), // Magenta
//     float3(0.0f, 1.0f, 1.0f), // Cyan
//     float3(1.0f, 0.5f, 0.0f), // Orange
//     float3(0.5f, 0.0f, 1.0f), // Purple
//     float3(0.5f, 1.0f, 0.0f), // Lime
//     float3(0.0f, 0.5f, 1.0f), // Sky Blue
//     float3(1.0f, 0.0f, 0.5f), // Pink
// };
//
// [shader("fragment")]
// float4 fsMain(TessEvalOutput input) : SV_Target
// {
//     float3 color = colors[input.te_PrimID % 11];
//     return float4(color, 1.0f);
// }
