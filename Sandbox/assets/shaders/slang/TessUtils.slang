module TessUtils;

public uint PackTessFactor(float3 v[3], float4x4 mvp, uint2 screenSize) {
    // Transform to clip space → NDC → screen space
    float4 clip[3];
    for (uint i = 0; i < 3; ++i) {
        clip[i] = mul(mvp, float4(v[i], 1.0));
    }

    float3 ndc[3];
    for (uint i = 0; i < 3; ++i) {
        ndc[i] = clip[i].xyz / clip[i].w;
    }

    float2 screen[3];
    for (uint i = 0; i < 3; ++i) {
        screen[i] = (ndc[i].xy * 0.5 + 0.5) * float2(screenSize);
    }

    // Compute edge lengths in screen space
    float len[3];
    len[0] = length(screen[0] - screen[1]);
    len[1] = length(screen[1] - screen[2]);
    len[2] = length(screen[2] - screen[0]);

    // Convert lengths to tess factors
    uint tess[3];
    for (uint i = 0; i < 3; ++i) {
        tess[i] = uint(clamp(len[i] / 20.0, 0.0, 255.0));
    }

    // Pack tess factors into RGB
    return PackRGBA8ToUint(tess[0], tess[1], tess[2], 0);
}

uint PackRGBA8ToUint(uint r, uint g, uint b, uint a) {
    r = clamp(r, 0, 255);
    g = clamp(g, 0, 255);
    b = clamp(b, 0, 255);
    a = clamp(a, 0, 255);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void UnpackUintToRGBA8(uint packed, out uint r, out uint g, out uint b, out uint a) {
    a = (packed >> 24) & 0xFF;
    r = (packed >> 16) & 0xFF;
    g = (packed >> 8)  & 0xFF;
    b =  packed        & 0xFF;
}

public float CalTrianglePixelArea(float3 v[3], float4x4 mvp, uint2 screenSize) {
    // Transform to clip space → NDC → screen space
    float4 clip[3];
    for (uint i = 0; i < 3; ++i) {
        clip[i] = mul(mvp, float4(v[i], 1.0));
    }

    float3 ndc[3];
    for (uint i = 0; i < 3; ++i) {
        ndc[i] = clip[i].xyz / clip[i].w;
    }

    float2 screenPos[3];
    for (uint i = 0; i < 3; ++i) {
        screenPos[i] = (ndc[i].xy * 0.5 + 0.5) * float2(screenSize);
    }

    // Compute 2D area via cross product
    float2 edge1 = screenPos[1] - screenPos[0];
    float2 edge2 = screenPos[2] - screenPos[0];
    float crossProduct = edge1.x * edge2.y - edge1.y * edge2.x;

    return 0.5 * abs(crossProduct); // in pixels²
}
