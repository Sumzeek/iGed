module Common;

public uint PackRGBA8ToUint(uint r, uint g, uint b, uint a) {
    r = clamp(r, 0, 255);
    g = clamp(g, 0, 255);
    b = clamp(b, 0, 255);
    a = clamp(a, 0, 255);

    return (r << 24) | (g << 16) | (b << 8) | a;
}

public void UnpackUintToRGBA8(uint packed, out uint r, out uint g, out uint b, out uint a) {
    r = (packed >> 24) & 0xFF;
    g = (packed >> 16) & 0xFF;
    b = (packed >> 8)  & 0xFF;
    a =  packed        & 0xFF;
}

public uint PackIdBarycentricToUint(uint primID, float3 bary) {
    bary = clamp(bary, 0.0f, 1.0f);
    bary /= (bary.x + bary.y + bary.z);

    uint u8 = (uint)(bary.x * 255.0f + 0.5f);
    uint v8 = (uint)(bary.y * 255.0f + 0.5f);

    if (u8 + v8 > 255) {
        float scale = 255.0f / (u8 + v8);
        u8 = (uint)(u8 * scale);
        v8 = (uint)(v8 * scale);
    }

    return (primID << 16) | (u8 << 8) | v8;
}

public void UnpackIdBarycentricFromUint(uint packed, out uint primID, out float3 bary) {
    primID = packed >> 16;

    uint u8 = (packed >> 8) & 0xFF;
    uint v8 = packed & 0xFF;

    float u = float(u8) / 255.0f;
    float v = float(v8) / 255.0f;
    float w = 1.0f - u - v;

    if (w < 0.0f) {
        w = 0.0f;
        float sum = u + v;
        u /= sum;
        v /= sum;
    }

    bary = float3(u, v, w);
}
