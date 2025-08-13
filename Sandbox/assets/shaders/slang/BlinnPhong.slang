module BlinnPhong;

public struct Light {
    public float3 Direction;
    public float3 Color;
};

public float3 BlinnPhong(float3 viewPos, float3 fragPos, float3 normal, Light light) {
    // diffuse
    float3 lightDir = normalize(-light.Direction);
    float diff = max(dot(lightDir, normal), 0.0f);
    float3 diffuse = diff * light.Color * 0.5f;
    // specular
    float3 viewDir = normalize(viewPos - fragPos);
    float spec = 0.0f;
    float3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
    float3 specular = spec * light.Color * 0.5f;

    return diffuse + specular;
    // return diffuse;
}
