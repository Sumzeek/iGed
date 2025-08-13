struct VertexInput {
    float3 a_Position   : POSITION;
    float2 a_TexCoord   : TEX_COORD;
};

struct VertexOutput {
    float4 v_Position   : SV_Position;
    float2 v_TexCoord   : TEX_COORD;
};

[[gl::binding(0)]]
cbuffer SceneData {
    float4x4 u_View;
    float4x4 u_Projection;
    float4x4 u_ProjectionView;
    float4x4 u_Transform;
};

[shader("vertex")]
VertexOutput vsMain(VertexInput input)
{
    VertexOutput out;
    out.v_Position = mul(u_ProjectionView, mul(u_Transform, float4(input.a_Position, 1.0f)));
    out.v_TexCoord = input.a_TexCoord;
    return out;
}

// Texture2D u_Texture;
// SamplerState u_Sampler;
[[gl::binding(1)]]
Sampler2D u_Texture;

[shader("fragment")]
float4 fsMain(VertexOutput input) : SV_Target
{
    return u_Texture.Sample(input.v_TexCoord);
}
