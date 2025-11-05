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
    uint u_LineOption;
};

[[gl::binding(3)]]
Sampler2D u_DisplaceMap;

[[gl::binding(4)]]
Sampler2D u_NormalMap;

struct VertexInput {
    float3 a_Position  : POSITION;
    float3 a_Normal    : NORMAL;
    float2 a_TexCoord  : TEXCOORD;
    //float a_Curvature  : CURVATURE;
};

struct VertexOutput {
    float3 v_Position  : POSITION;
    float3 v_Normal    : NORMAL;
    float2 v_TexCoord  : TEXCOORD;
    //float v_Curvature  : CURVATURE;
};

[shader("vertex")]
VertexOutput vsMain(VertexInput input, uint vertexID : SV_VertexID)
{
    VertexOutput out;
    out.v_Position  = input.a_Position;
    out.v_TexCoord  = input.a_TexCoord;
    out.v_Normal    = input.a_Normal;
    //out.v_Curvature = input.a_Curvature;
    return out;
}

struct TessControlOutput {
    float3 tc_Position  : POSITION;
    float3 tc_Normal    : NORMAL;
    float2 tc_TexCoord  : TEXCOORD;
};

struct PatchConstantData {
    float EdgeTessFactor[4] : SV_TessFactor;
    float InsideTessFactor[2] : SV_InsideTessFactor;
};

[shader("tesscontrol")]
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("computeTessFactors")]
TessControlOutput tcsMain(InputPatch<VertexOutput, 4> input, uint i : SV_OutputControlPointID)
{
    TessControlOutput out;
    out.tc_Position = input[i].v_Position;
    out.tc_Normal = input[i].v_Normal;
    out.tc_TexCoord = input[i].v_TexCoord;

    return out;
}

// Patch Constant Function
PatchConstantData computeTessFactors(InputPatch<VertexOutput, 4> input) {
    float3 v[4]= float3[4](
        input[0].v_Position,
        input[1].v_Position,
        input[2].v_Position,
        input[3].v_Position,
    );

    float4x4 mvp = mul(u_ProjectionView, u_Transform);
    uint packed = PackTessFactorQuad(v, mvp, u_ScreenSize);

    uint r,g,b,a;
    UnpackUintToRGBA8(packed, r, g, b, a);

    uint targetPixelSize = 8; // in pixels

    PatchConstantData pcd;
    // pcd.EdgeTessFactor[0] = max(max(input[0].v_Curvature, r / targetPixelSize), 1.0f);
    // pcd.EdgeTessFactor[1] = max(max(input[1].v_Curvature, g / targetPixelSize), 1.0f);
    // pcd.EdgeTessFactor[2] = max(max(input[2].v_Curvature, b / targetPixelSize), 1.0f);
    // pcd.EdgeTessFactor[3] = max(max(input[3].v_Curvature, b / targetPixelSize), 1.0f);
    // pcd.InsideTessFactor[0] = max(a / targetPixelSize, 1);
    pcd.EdgeTessFactor[0] = u_TargetTessFactor;
    pcd.EdgeTessFactor[1] = u_TargetTessFactor;
    pcd.EdgeTessFactor[2] = u_TargetTessFactor;
    pcd.EdgeTessFactor[3] = u_TargetTessFactor;
    pcd.InsideTessFactor[0] = u_TargetTessFactor;
    pcd.InsideTessFactor[1] = u_TargetTessFactor;

    return pcd;
}

struct TessEvalOutput {
    float4 te_Position  : SV_Position;
    float3 te_WorldPos  : POSITION;
    float3 te_Normal    : NORMAL;
    float2 te_TexCoord  : TEXCOORD;
    float te_T;
};

[shader("tesseval")]
[domain("quad")]
TessEvalOutput tesMain(
    float2 uv : SV_DomainLocation,
    const OutputPatch<TessControlOutput, 4> input,
    PatchConstantData tessData)
{
    float3 p0 = input[0].tc_Position;
    float3 p1 = input[1].tc_Position;
    float3 p2 = input[2].tc_Position;
    float3 p3 = input[3].tc_Position;
    // Assume quad index order: 0:(0,0), 1:(1,0), 2:(1,1), 3:(0,1)
    float3 interpolatedPosition =
          p0 * (1 - uv.x) * (1 - uv.y)
        + p1 * uv.x * (1 - uv.y)
        + p2 * uv.x * uv.y
        + p3 * (1 - uv.x) * uv.y;

    float3 n0 = input[0].tc_Normal;
    float3 n1 = input[1].tc_Normal;
    float3 n2 = input[2].tc_Normal;
    float3 n3 = input[3].tc_Normal;
    float3 interpolatedNormal =
          n0 * (1 - uv.x) * (1 - uv.y)
        + n1 * uv.x * (1 - uv.y)
        + n2 * uv.x * uv.y
        + n3 * (1 - uv.x) * uv.y;
    interpolatedNormal = normalize(interpolatedNormal);

    float2 uv0 = input[0].tc_TexCoord;
    float2 uv1 = input[1].tc_TexCoord;
    float2 uv2 = input[2].tc_TexCoord;
    float2 uv3 = input[3].tc_TexCoord;
    float2 interpolatedTexCoord =
          uv0 * (1 - uv.x) * (1 - uv.y)
        + uv1 * uv.x * (1 - uv.y)
        + uv2 * uv.x * uv.y
        + uv3 * (1 - uv.x) * uv.y;

    TessEvalOutput out;

    // Displacement Mapping (currently disabled for geometry position)
    // Compute integer pixel coordinates and fractional offsets
    int2 base = int2(floor(interpolatedTexCoord));
    float2 f = frac(interpolatedTexCoord);

    // Sample displacement values from the four neighboring texels
    float s00 = u_DisplaceMap.Load(int3(base, 0)).r;
    float s10 = u_DisplaceMap.Load(int3(base + int2(1, 0), 0)).r;
    float s01 = u_DisplaceMap.Load(int3(base + int2(0, 1), 0)).r;
    float s11 = u_DisplaceMap.Load(int3(base + int2(1, 1), 0)).r;

    // Perform bilinear interpolation
    float sx0 = lerp(s00, s10, f.x);
    float sx1 = lerp(s01, s11, f.x);
    float displacement = lerp(sx0, sx1, f.y);

    // Offset the vertex along its normal by the displacement value
    float3 position = interpolatedPosition + displacement * interpolatedNormal;;

    out.te_Position = mul(u_ProjectionView, mul(u_Transform, float4(position, 1.0f)));
    out.te_WorldPos = mul(u_Transform, float4(position, 1.0f)).xyz;
    out.te_Normal = interpolatedNormal;
    out.te_TexCoord = interpolatedTexCoord;
    out.te_T = displacement;

    return out;
}

struct GeometryOutput {
    float4 g_Position  : SV_Position;
    float3 g_WorldPos  : POSITION;
    float3 g_Normal    : NORMAL;
    float2 g_TexCoord  : TEXCOORD;
    float3 g_Barycentric : BARYCENTRIC;
    float3 g_T;
};

[shader("geometry")]
[maxvertexcount(6)]
void gsMain(
    triangle TessEvalOutput input[3],
    inout TriangleStream<GeometryOutput> outputStream,
    uint primitiveID : SV_PrimitiveID)
{
    float3 barycentrics[3] = {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };

    for (int i = 0; i < 3; ++i) {
        GeometryOutput outputVertex;
        outputVertex.g_Position = input[i].te_Position;
        outputVertex.g_WorldPos = input[i].te_WorldPos;
        outputVertex.g_Normal = input[i].te_Normal;
        outputVertex.g_Barycentric = barycentrics[i];
        outputVertex.g_TexCoord = input[i].te_TexCoord;
        outputVertex.g_T = input[i].te_T;
        outputStream.Append(outputVertex);
    }
}

static const float3 ambient = float3(0.4f, 0.4f, 0.4f);
static const Light light = {
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
};

[shader("fragment")]
float4 fsMain(GeometryOutput input) : SV_Target
{
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);
    float3 color = float3(0.0f, 0.0f, 0.0f);

    // Sample normal values from the four neighboring texels
    int2 base = int2(floor(input.g_TexCoord));
    float2 f = frac(input.g_TexCoord);
    float3 s00 = u_NormalMap.Load(int3(base, 0)).rgb;
    float3 s10 = u_NormalMap.Load(int3(base + int2(1, 0), 0)).rgb;
    float3 s01 = u_NormalMap.Load(int3(base + int2(0, 1), 0)).rgb;
    float3 s11 = u_NormalMap.Load(int3(base + int2(1, 1), 0)).rgb;
    float3 sx0 = lerp(s00, s10, f.x);
    float3 sx1 = lerp(s01, s11, f.x);
    float3 normal = mul(float3x3(u_Normal), lerp(sx0, sx1, f.y));
    normal = normalize(normal);

    float3 viewPosition = mul(u_View, float4(input.g_WorldPos, 1.0f)).xyz;
    if (dot(normal, viewPosition) > 0.0f) {
        normal = -1.0f * normal;
    }

    // ambient
    color += ambient * baseColor;

    // lighting
    float3 lighting = BlinnPhong(u_ViewPos, input.g_WorldPos, normal, light);
    color += lighting * baseColor;

    // line
    const float edgeWidth = 1.5; // in pixels
    float3 bc = input.g_Barycentric;
    float3 w = fwidth(bc);
    float3 a3 = smoothstep(0.0, w * edgeWidth, bc);
    float edgeFactor = min(min(a3.x, a3.y), a3.z); // 0 at edge, 1 inside
    if (u_LineOption == 1) {  color = lerp(float3(0.0, 0.0, 0.0), color, edgeFactor); }

    return float4(color, 1.0f);
}
