module TessUtils;

import Common;

public uint PackTessFactor(float3 v[3], float4x4 mvp, uint2 screenSize) {
    // Transform to clip space → NDC → screen space
    float4 clip[3];
    [unroll] for (uint i = 0; i < 3; ++i) {
        clip[i] = mul(mvp, float4(v[i], 1.0));
    }

    float3 ndc[3];
    [unroll] for (uint i = 0; i < 3; ++i) {
        float w = max(abs(clip[i].w), 1e-6f) * (clip[i].w < 0.0f ? -1.0f : 1.0f);
        ndc[i] = clip[i].xyz / w;
    }

    float2 screen[3];
    [unroll] for (uint i = 0; i < 3; ++i) {
        screen[i] = (ndc[i].xy * 0.5 + 0.5) * float2(screenSize);
    }

    // Compute edge lengths in screen space
    float len[3];
    len[0] = length(screen[1] - screen[2]);
    len[1] = length(screen[2] - screen[0]);
    len[2] = length(screen[0] - screen[1]);

    // Convert lengths to tess factors
    uint tessEdge[3];
    for (uint i = 0; i < 3; ++i) {
        tessEdge[i] = uint(clamp(len[i], 0.0, 255.0));
    }
    uint tessInside = (tessEdge[0] + tessEdge[1] + tessEdge[2]) / 3;

    // Pack tess factors into RGB
    return PackRGBA8ToUint(tessEdge[0], tessEdge[1], tessEdge[2], tessInside);
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

public float3x3 BitToXform(uint bit) {
    float s = float(bit) - 0.5f;
    float3 c1 = float3(s, -0.5f, 0.0f);
    float3 c2 = float3(-0.5f, -s, 0.0f);
    float3 c3 = float3(0.5f, 0.5f, 1.0f);
    return float3x3(c1 , c2 , c3);
}

public float3x3 KeyToXform(uint key) {
    float3x3 xf = float3x3(1.0f);
    while (key > 1u) {
      xf = BitToXform(key & 1u) * xf;
      key = key >> 1u;
    }
    return xf;
}

public uint ParentKey(uint key) {
  return key >> 1u;
}

public void ChildrenKeys(uint key ,out uint children[2]) {
  children[0] = (key << 1u) | 0u;
  children[1] = (key << 1u) | 1u;
}

public bool IsChildZeroKey(uint key) { return ((key & 1u) == 0u); }
public bool IsRootKey(uint key) { return key == 1u; }
public bool IsLeafKey(uint key) { return firstbithigh(key) == 31; }

// Get 3D Coordinates According To Vector Weights
public float3 Berp(float3 v[3], float2 u) {
    return v[0] + u.x * (v[1] - v[0]) + u.y * (v[2] - v[0]);
}

// subdivision routine ( vertex position only )
public void Subd (uint key ,float3 v_in[3], out float3 v_out[3]) {
    float3x3 xf = KeyToXform(key);

    float2 u1 = mul(xf, float3(0.0f, 0.0f, 1.0f)).xy;
    float2 u2 = mul(xf, float3(1.0f, 0.0f, 1.0f)).xy;
    float2 u3 = mul(xf, float3(0.0f, 1.0f, 1.0f)).xy;

    v_out[0] = Berp(v_in ,u1);
    v_out[1] = Berp(v_in ,u2);
    v_out[2] = Berp(v_in ,u3);
}

public float DistanceToLod(float3 pos, float4x4 mvp) {
    float4 clip = mul(mvp, float4(pos, 1.0f));
    float lod = (clip / clip.w).z;

    lod = clamp(lod, 0.0f, 1.0f);
    return -2.0f * log2(lod);
}
