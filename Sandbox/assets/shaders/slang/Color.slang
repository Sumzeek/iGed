struct VertexInput {
    float3 a_Position   : POSITION;
    float3 a_Color      : COLOR;
};

struct VertexOutput {
    float4 v_Position   : SV_Position;
    float3 v_Color      : COLOR;
};

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

[shader("vertex")]
VertexOutput vsMain(VertexInput input)
{
    VertexOutput out;
    out.v_Position = mul(u_ProjectionView, mul(u_Transform, float4(input.a_Position, 1.0f)));
    out.v_Color = input.a_Color;
    return out;
}

[shader("fragment")]
float4 fsMain(VertexOutput input) : SV_Target
{
    return float4(input.v_Color, 1.0f);
}
