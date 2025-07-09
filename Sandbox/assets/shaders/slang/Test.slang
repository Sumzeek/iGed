import BlinnPhong;

struct VertexInput {
    float3 a_Position   : POSITION;
    float3 a_Normal     : NORMAL;
    float2 a_TexCoord   : TEXCOORD;
    float3 a_Tangent    : TANGENT;
    float3 a_BiTangent  : BITANGENT;
};

struct VertexOutput {
    float4 v_Position   : SV_Position;
    float3 v_WorldPos   : POSITION;
    float3 v_Normal     : NORMAL;
    float3x3 v_NormalM : NOR;
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
Sampler2D u_NormalMap;

[[gl::binding(3)]]
Sampler2D u_DisplaceMap;

[shader("vertex")]
VertexOutput vsMain(VertexInput input)
{
    float displacement = u_DisplaceMap.Sample(input.a_TexCoord).r - 0.5f;
    float3 position = input.a_Position + displacement * input.a_Normal * u_DisplaceMapScale;

    // Calculate TBN
    float3 T = normalize(mul(u_Normal, float4(input.a_Tangent, 0.0f)).xyz);
    float3 B = normalize(mul(u_Normal, float4(input.a_BiTangent, 0.0f)).xyz);
    float3 N = normalize(mul(u_Normal, float4(input.a_Normal, 0.0f)).xyz);
    float3x3 TBN = float3x3(T, B, N);

    // Normal mapping
    float3 normal = normalize(u_NormalMap.Sample(input.a_TexCoord).xyz);
    normal = normalize(normal * 2.0f - 1.0f);
    normal = normalize(mul(TBN, normal));

    VertexOutput out;
    out.v_Position = mul(u_ProjectionView, mul(u_Transform, float4(position, 1.0f)));
    out.v_WorldPos = mul(u_Transform, float4(position, 1.0f)).xyz;
    out.v_Normal = normal;
    return out;
}

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

static const float3 ambient = float3(0.4f, 0.4f, 0.4f);
static const Light light = {
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
};

[shader("pixel")]
float4 psMain(VertexOutput input, uint primID : SV_PrimitiveID) : SV_Target
{
    // float3 color = colors[primID % 11];
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);

    float3 color = float3(0.0f, 0.0f, 0.0f);

    // float3 fdx = ddx(input.v_WorldPos);
    // float3 fdy = ddy(input.v_WorldPos);
    // float3 normal = normalize(cross(fdx, fdy));
    float3 normal = normalize(input.v_Normal);

    // ambient
    color += ambient * baseColor;

    // lighing
    float3 lighting = BlinnPhong(u_ViewPos, input.v_WorldPos, normal, light);
    color += lighting * baseColor;

    return float4(color, 1.0f);
}
