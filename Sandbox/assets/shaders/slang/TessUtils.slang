module TessUtils;

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
