#version 460

#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#define WORKGROUP_SIZE 32
layout(local_size_x = WORKGROUP_SIZE) in;

layout(binding = 2, std140) uniform TessellatorDataBlock_std140 {
    uvec2 u_ScreenSize;
    uint u_QuadSize;
    uint u_LineOption;
} TessellatorData;

taskNV out Task {
    uint taskGroupID;
    uint taskGroupSize;
} task_out;

void main()
{
    uint gid = gl_WorkGroupID.x;
    uint gtid = gl_LocalInvocationID.x;

    // Calculate how many mesh workgroups we actually need
    uint startQuadId = gid * WORKGROUP_SIZE;
    uint remainingQuads = (startQuadId < TessellatorData.u_QuadSize) ? (TessellatorData.u_QuadSize - startQuadId) : 0u;
    uint taskCount = min(remainingQuads, WORKGROUP_SIZE);

    if (gtid == 0u) {
        gl_TaskCountNV = taskCount;
        task_out.taskGroupID = gid;
        task_out.taskGroupSize = WORKGROUP_SIZE;
    }
}
