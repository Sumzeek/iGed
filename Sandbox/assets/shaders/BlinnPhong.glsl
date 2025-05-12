#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_Position;
out vec3 v_Color;

void main() {
    v_Position = vec3(u_Transform * vec4(a_Position, 1.0f));
    v_Color = vec3(1.0f, 1.0f, 1.0f);

    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0f);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 f_Color;

uniform vec3 u_ViewPos;

in vec3 v_Position;
in vec3 v_Color;

vec3 ambient = vec3(0.4f, 0.4f, 0.4f);
struct Light {
    vec3 direction;
    vec3 color;
};
Light light = Light(
vec3(0.0f, 0.0f, -1.0f),
vec3(1.0f, 1.0f, 1.0f)
);

vec3 BlinnPhong(vec3 normal, vec3 fragPos, Light light)
{
    // diffuse
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0f);
    vec3 diffuse = diff * light.color * 0.5f;
    // specular
    vec3 viewDir = normalize(u_ViewPos - fragPos);
    float spec = 0.0f;
    vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
    vec3 specular = spec * light.color * 0.5f;

    return diffuse + specular;
    //    return diffuse;
}

void main() {
    vec3 color = vec3(0.0f, 0.0f, 0.0f);

    vec3 fdx = dFdx(v_Position);
    vec3 fdy = dFdy(v_Position);
    vec3 normal = normalize(cross(fdx, fdy));

    // ambient
    color += ambient * v_Color;

    // lighting
    vec3 lighting = BlinnPhong(normal, v_Position, light);
    color += lighting * v_Color;

    f_Color = vec4(color, 1.0f);
}
