import BlinnPhong;

struct VertexInput {
    float3 a_Position   : POSITION;
};

struct VertexOutput {
    float4 v_Position   : SV_Position;
    float3 v_WorldPos   : POSITION;
    float3 v_Color      : COLOR;
};

[[gl::binding(0)]]
cbuffer SceneData {
    float4x4 u_ProjectionView;
    float4x4 u_Transform;
};

[[gl::binding(1)]]
cbuffer CameraData {
    float3 u_ViewPos;
};

[shader("vertex")]
VertexOutput vsMain(VertexInput input)
{
    VertexOutput out;
    out.v_Position = mul(u_ProjectionView, mul(u_Transform, float4(input.a_Position, 1.0f)));
    out.v_WorldPos = mul(u_Transform, float4(input.a_Position, 1.0f)).xyz;
    out.v_Color = float3(1.0f, 1.0f, 1.0f);
    return out;
}

static const float3 ambient = float3(0.4f, 0.4f, 0.4f);
static const Light light = {
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
};

[shader("pixel")]
float4 psMain(VertexOutput input) : SV_Target
{
    float3 color = float3(0.0f, 0.0f, 0.0f);

    float3 fdx = ddx(input.v_WorldPos);
    float3 fdy = ddy(input.v_WorldPos);
    float3 normal = normalize(cross(fdx, fdy));

    // ambient
    color += ambient * input.v_Color;

    // lighting
    float3 lighting = BlinnPhong(u_ViewPos, input.v_WorldPos, normal, light);
    color += lighting * input.v_Color;

    return float4(color, 1.0f);
}
