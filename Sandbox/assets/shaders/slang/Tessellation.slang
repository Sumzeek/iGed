// hello-world.slang
[[gl::binding(0)]]
StructuredBuffer<float> buffer0;

[[gl::binding(1)]]
StructuredBuffer<float> buffer1;

[[gl::binding(2)]]
RWStructuredBuffer<float> result;

[shader("compute")]
[numthreads(1,1,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    result[index] = buffer0[index] + buffer1[index];
}
