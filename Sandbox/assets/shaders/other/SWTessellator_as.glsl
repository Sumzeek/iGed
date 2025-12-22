#version 460

#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#define WORKGROUP_SIZE 32
layout(local_size_x = WORKGROUP_SIZE) in;

taskNV out Task {
    uint taskGroupID;
    uint taskGroupSize;
} task_out;

void main()
{
    uint gid = gl_WorkGroupID.x;
    uint gtid = gl_LocalInvocationID.x;

    if (gtid == 0) {
        gl_TaskCountNV = WORKGROUP_SIZE;
        task_out.taskGroupID = gid;
        task_out.taskGroupSize = WORKGROUP_SIZE;
    }
}
