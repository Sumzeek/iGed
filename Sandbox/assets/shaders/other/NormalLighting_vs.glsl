#version 460

struct _MatrixStorage_float4x4_ColMajorstd140
{
    vec4 data[4];
};

layout(binding = 0, std140) uniform SLANG_ParameterGroup_SceneData_std140
{
    _MatrixStorage_float4x4_ColMajorstd140 u_View;
    _MatrixStorage_float4x4_ColMajorstd140 u_Projection;
    _MatrixStorage_float4x4_ColMajorstd140 u_ProjectionView;
    _MatrixStorage_float4x4_ColMajorstd140 u_Transform;
} SceneData;

layout(binding = 1, std140) uniform SLANG_ParameterGroup_PerFrameData_std140
{
    vec3 u_ViewPos;
    float _padding_u_ViewPos;
    _MatrixStorage_float4x4_ColMajorstd140 u_Normal;
} PerFrameData;

layout(location = 0) in vec3 input_a_Position;
layout(location = 1) in vec3 input_a_Normal;
layout(location = 0) out vec3 entryPointParam_vsMain_v_WorldPos;
layout(location = 1) out vec3 entryPointParam_vsMain_v_Normal;
layout(location = 2) out vec3 entryPointParam_vsMain_v_Color;

float _194;

void main()
{
    vec4 _86 = vec4(input_a_Position, 1.0);
    gl_Position = (_86 * mat4(vec4(SceneData.u_Transform.data[0].x, SceneData.u_Transform.data[1].x, SceneData.u_Transform.data[2].x, SceneData.u_Transform.data[3].x), vec4(SceneData.u_Transform.data[0].y, SceneData.u_Transform.data[1].y, SceneData.u_Transform.data[2].y, SceneData.u_Transform.data[3].y), vec4(SceneData.u_Transform.data[0].z, SceneData.u_Transform.data[1].z, SceneData.u_Transform.data[2].z, SceneData.u_Transform.data[3].z), vec4(SceneData.u_Transform.data[0].w, SceneData.u_Transform.data[1].w, SceneData.u_Transform.data[2].w, SceneData.u_Transform.data[3].w))) * mat4(vec4(SceneData.u_ProjectionView.data[0].x, SceneData.u_ProjectionView.data[1].x, SceneData.u_ProjectionView.data[2].x, SceneData.u_ProjectionView.data[3].x), vec4(SceneData.u_ProjectionView.data[0].y, SceneData.u_ProjectionView.data[1].y, SceneData.u_ProjectionView.data[2].y, SceneData.u_ProjectionView.data[3].y), vec4(SceneData.u_ProjectionView.data[0].z, SceneData.u_ProjectionView.data[1].z, SceneData.u_ProjectionView.data[2].z, SceneData.u_ProjectionView.data[3].z), vec4(SceneData.u_ProjectionView.data[0].w, SceneData.u_ProjectionView.data[1].w, SceneData.u_ProjectionView.data[2].w, SceneData.u_ProjectionView.data[3].w));
    entryPointParam_vsMain_v_WorldPos = (_86 * mat4(vec4(SceneData.u_Transform.data[0].x, SceneData.u_Transform.data[1].x, SceneData.u_Transform.data[2].x, SceneData.u_Transform.data[3].x), vec4(SceneData.u_Transform.data[0].y, SceneData.u_Transform.data[1].y, SceneData.u_Transform.data[2].y, SceneData.u_Transform.data[3].y), vec4(SceneData.u_Transform.data[0].z, SceneData.u_Transform.data[1].z, SceneData.u_Transform.data[2].z, SceneData.u_Transform.data[3].z), vec4(SceneData.u_Transform.data[0].w, SceneData.u_Transform.data[1].w, SceneData.u_Transform.data[2].w, SceneData.u_Transform.data[3].w))).xyz;
    entryPointParam_vsMain_v_Normal = input_a_Normal * mat3(vec4(PerFrameData.u_Normal.data[0].x, PerFrameData.u_Normal.data[1].x, PerFrameData.u_Normal.data[2].x, _194).xyz, vec4(PerFrameData.u_Normal.data[0].y, PerFrameData.u_Normal.data[1].y, PerFrameData.u_Normal.data[2].y, _194).xyz, vec4(PerFrameData.u_Normal.data[0].z, PerFrameData.u_Normal.data[1].z, PerFrameData.u_Normal.data[2].z, _194).xyz);
    entryPointParam_vsMain_v_Color = vec3(1.0);
}

