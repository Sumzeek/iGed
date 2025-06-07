struct VertexInput {
    float3 a_Position   : POSITION;
};

struct VertexOutput {
    float4 v_Position   : SV_Position;
    float3 v_WorldPos   : POSITION;
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
    return out;
}

static const float3 colors[12] = {
    float3(1.0f, 0.0f, 0.0f), // Red
    float3(0.0f, 1.0f, 0.0f), // Green
    float3(0.0f, 0.0f, 1.0f), // Blue
    float3(1.0f, 1.0f, 0.0f), // Yellow
    float3(1.0f, 0.0f, 1.0f), // Magenta
    float3(0.0f, 1.0f, 1.0f), // Cyan
    float3(1.0f, 0.5f, 0.0f), // Orange
    float3(0.5f, 0.0f, 1.0f), // Purple
    float3(0.5f, 1.0f, 0.0f), // Lime
    float3(0.0f, 0.5f, 1.0f), // Sky Blue
    float3(1.0f, 0.0f, 0.5f), // Pink
};

[shader("pixel")]
float4 psMain(VertexOutput input, uint primID : SV_PrimitiveID) : SV_Target
{
    float3 color = colors[primID % 11];

    return float4(color, 1.0f);
}
