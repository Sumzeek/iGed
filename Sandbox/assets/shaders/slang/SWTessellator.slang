import TessUtils;

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

[[gl::binding(2)]]
cbuffer TessellatorData {
    uint2 u_ScreenSize;
    uint u_TriSize; // Number of coarse triangles
    float u_DisplaceMapScale;
};

[[gl::binding(10)]]
RWStructuredBuffer<float3> u_VertexBuffer;

[[gl::binding(11)]]
RWStructuredBuffer<uint> u_IndexBuffer;

[[gl::binding(12)]]
RWStructuredBuffer<uint2> u_SubdBufferIn; // x is key, y is triangleID

[[gl::binding(20)]]
RWStructuredBuffer<uint2> u_SubdBufferCounter; // x is 0, y is last counter

[[gl::binding(21)]]
RWStructuredBuffer<uint2> u_SubdBufferOut; // x is key, y is triangleID

// write a key to the subdivision buffer
void WriteKey(uint key, uint primID) {
    uint index;
    InterlockedAdd(u_SubdBufferCounter[0].x, 1, index);
    u_SubdBufferOut[index] = uint2(key, primID);
}

// general routine to update the subdivision buffer
void UpdateSubdBuffer(uint key, uint primID, int targetLod) {
    // extract subdivision level associated to the key
    int keyLod = firstbithigh(key);

    // update the key accordingly
    if (/* subdivide ? */ keyLod < targetLod && !IsLeafKey(key)) {
        uint children [2];
        ChildrenKeys(key, children);

        WriteKey(children[0], primID);
        WriteKey(children[1], primID);
    }
    else if (/* keep ? */ keyLod == targetLod ) {
        WriteKey(key, primID);
    }
    else /* merge ? */ {
        if (/* is root ? */ IsRootKey(key)) {
          WriteKey(key, primID);
        } else if (/* is zero child ? */ IsChildZeroKey(key)) {
          WriteKey(ParentKey(key), primID);
        }
    }
}

[shader("compute")]
[numthreads(32, 1, 1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    if (index >= u_SubdBufferCounter[0].y) { return; }

    // get coarse triangle associated to the key
    uint primID = u_SubdBufferIn[index].y;
    float3 v_in[3]= float3[3](
        u_VertexBuffer[u_IndexBuffer[primID * 3]],
        u_VertexBuffer[u_IndexBuffer[primID * 3+ 1]],
        u_VertexBuffer[u_IndexBuffer[primID * 3+ 2]],
    );

    // // compute distance - based LOD
    // uint key = u_SubdBufferIn[index].x;
    // float3 v[3];
    //
    // // what does this process do? Get The subdivision triangle's coordinates
    // Subd(key ,v_in, v);
    // float3 midPoint = (v[1] + v[2]) / 2.0f;
    // int targetLod = int(DistanceToLod(u_ViewPos, mul(u_ProjectionView, u_Transform)));

    uint key = u_SubdBufferIn[index].x;

    // write to u_SubdBufferOut
    UpdateSubdBuffer(key, primID, 4);
}
