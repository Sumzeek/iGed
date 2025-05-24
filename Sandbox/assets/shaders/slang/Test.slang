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
struct Light {
    float3 direction;
    float3 color;
};
static const Light light = Light(
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
);

float3 BlinnPhong(float3 normal, float3 fragPos, Light light) {
    // diffuse
    float3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0f);
    float3 diffuse = diff * light.color * 0.5f;
    // specular
    float3 viewDir = normalize(u_ViewPos - fragPos);
    float spec = 0.0f;
    float3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
    float3 specular = spec * light.color * 0.5f;

    return diffuse + specular;
    // return diffuse;
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
    float3 color = float3(0.0f, 0.0f, 0.0f);

    float3 fdx = ddx(input.v_WorldPos);
    float3 fdy = ddy(input.v_WorldPos);
    float3 normal = normalize(cross(fdx, fdy));

    // ambient
    color += ambient * input.v_Color;

    // lighting
    float3 lighting = BlinnPhong(normal, input.v_WorldPos, light);
    color += lighting * input.v_Color;

    return float4(colors[primID % 11], 1.0f);
}
