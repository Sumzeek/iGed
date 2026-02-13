#version 460

struct _MatrixStorage_float4x4_ColMajorstd140
{
    vec4 data[4];
};

layout(binding = 1, std140) uniform SLANG_ParameterGroup_PerFrameData_std140
{
    vec3 u_ViewPos;
    float _padding_u_ViewPos;
    _MatrixStorage_float4x4_ColMajorstd140 u_Normal;
} PerFrameData;

layout(location = 0) in vec3 input_v_WorldPos;
layout(location = 1) in vec3 input_v_Normal;
layout(location = 2) in vec3 input_v_Color;
layout(location = 0) out vec4 entryPointParam_fsMain;

void main()
{
    vec3 _98 = normalize(-vec3(0.0, 0.0, -1.0));
    entryPointParam_fsMain = vec4(input_v_Color * (vec3(0.4000000059604644775390625) + (((vec3(1.0) * max(dot(_98, input_v_Normal), 0.0)) * 0.5) + ((vec3(1.0) * pow(max(dot(normalize(PerFrameData.u_ViewPos - input_v_WorldPos), reflect(-_98, input_v_Normal)), 0.0), 32.0)) * 0.5))), 1.0);
}

