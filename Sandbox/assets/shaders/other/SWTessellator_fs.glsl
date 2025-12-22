#version 460

layout(binding = 1, std140) uniform PerFrameDataBlock_std140 {
    vec3 u_ViewPos;
    float _padding_u_ViewPos;
    mat4 u_Normal;
} PerFrameData;

layout(binding = 2, std140) uniform TessellatorDataBlock_std140 {
    uvec2 u_ScreenSize;
    uint u_QuadSize;
    uint u_LineOption;
} TessellatorData;

layout(binding = 3) uniform sampler2D u_DisplaceMap;
layout(binding = 4) uniform sampler2D u_NormalMap;

in PerVertexData {
    vec3 mcPosition;
    vec3 vcPosition;
    vec2 texcoord;
    vec3 barycentric;
} fragIn;

layout(location = 0) out vec4 out_ScreenColor;

struct Light {
    vec3 Direction;
    vec3 Color;
};

const vec3 ambient = vec3(0.4f, 0.4f, 0.4f);
const Light light = { vec3(0.0f, 0.0f, -1.0f), vec3(1.0f, 1.0f, 1.0f) };

vec3 BlinnPhong(vec3 viewPos, vec3 fragPos, vec3 normal, Light light) {
    // diffuse
    vec3 lightDir = normalize(-light.Direction);
    float diff = max(dot(lightDir, normal), 0.0f);
    vec3 diffuse = diff * light.Color * 0.5f;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    float spec = 0.0f;
    vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
    vec3 specular = spec * light.Color * 0.5f;

    return diffuse + specular;
    // return diffuse;
}

void main()
{
    vec3 baseColor = vec3(1.0f, 1.0f, 1.0f);
    vec3 color = vec3(0.0f, 0.0f, 0.0f);

    // Sample normal values from the four neighboring texels
    ivec2 base = ivec2(floor(fragIn.texcoord));
    vec2 f = fract(fragIn.texcoord);

    vec3 s00 = texelFetch(u_NormalMap, base, 0).rgb;
    vec3 s10 = texelFetch(u_NormalMap, base + ivec2(1, 0), 0).rgb;
    vec3 s01 = texelFetch(u_NormalMap, base + ivec2(0, 1), 0).rgb;
    vec3 s11 = texelFetch(u_NormalMap, base + ivec2(1, 1), 0).rgb;

    vec3 sx0 = mix(s00, s10, f.x);
    vec3 sx1 = mix(s01, s11, f.x);
    vec3 normal = mat3(PerFrameData.u_Normal) * mix(sx0, sx1, f.y);
    normal = normalize(normal);

    if (dot(normal, fragIn.vcPosition) > 0.0f) {
        normal = -1.0f * normal;
    }

    // ambient
    color += ambient * baseColor;

    // lighting
    vec3 lighting = BlinnPhong(PerFrameData.u_ViewPos, fragIn.mcPosition, normal, light);
    color += lighting * baseColor;

    // line
    const float edgeWidth = 1.5;// in pixels
    vec3 bc = fragIn.barycentric;
    vec3 w  = fwidth(bc);
    vec3 a3 = smoothstep(vec3(0.0), w * edgeWidth, bc);
    float edgeFactor = min(min(a3.x, a3.y), a3.z);// 0 at edge, 1 inside
    if (TessellatorData.u_LineOption == 1u) { color = mix(vec3(0.0, 0.0, 0.0), color, edgeFactor); }

    out_ScreenColor = vec4(color, 1.0f);
}

