#type vertex
#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 11964 0
struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    vec4  data_0[4];
};


#line 14 1
struct SLANG_ParameterGroup_SceneData_std140_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0 u_ProjectionView_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 u_Transform_0;
};


#line 12
layout(binding = 0)
layout(std140) uniform block_SLANG_ParameterGroup_SceneData_std140_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0 u_ProjectionView_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 u_Transform_0;
}SceneData_0;

#line 1
layout(location = 0)
out vec3 entryPointParam_vsMain_v_Color_0;


#line 1
layout(location = 0)
in vec3 input_a_Position_0;


#line 1
layout(location = 1)
in vec3 input_a_Color_0;



struct VertexOutput_0
{
    vec4 v_Position_0;
    vec3 v_Color_0;
};


#line 18
void main()
{
    VertexOutput_0 out_0;
    out_0.v_Position_0 = ((((((vec4(input_a_Position_0, 1.0)) * (mat4x4(SceneData_0.u_Transform_0.data_0[0][0], SceneData_0.u_Transform_0.data_0[1][0], SceneData_0.u_Transform_0.data_0[2][0], SceneData_0.u_Transform_0.data_0[3][0], SceneData_0.u_Transform_0.data_0[0][1], SceneData_0.u_Transform_0.data_0[1][1], SceneData_0.u_Transform_0.data_0[2][1], SceneData_0.u_Transform_0.data_0[3][1], SceneData_0.u_Transform_0.data_0[0][2], SceneData_0.u_Transform_0.data_0[1][2], SceneData_0.u_Transform_0.data_0[2][2], SceneData_0.u_Transform_0.data_0[3][2], SceneData_0.u_Transform_0.data_0[0][3], SceneData_0.u_Transform_0.data_0[1][3], SceneData_0.u_Transform_0.data_0[2][3], SceneData_0.u_Transform_0.data_0[3][3]))))) * (mat4x4(SceneData_0.u_ProjectionView_0.data_0[0][0], SceneData_0.u_ProjectionView_0.data_0[1][0], SceneData_0.u_ProjectionView_0.data_0[2][0], SceneData_0.u_ProjectionView_0.data_0[3][0], SceneData_0.u_ProjectionView_0.data_0[0][1], SceneData_0.u_ProjectionView_0.data_0[1][1], SceneData_0.u_ProjectionView_0.data_0[2][1], SceneData_0.u_ProjectionView_0.data_0[3][1], SceneData_0.u_ProjectionView_0.data_0[0][2], SceneData_0.u_ProjectionView_0.data_0[1][2], SceneData_0.u_ProjectionView_0.data_0[2][2], SceneData_0.u_ProjectionView_0.data_0[3][2], SceneData_0.u_ProjectionView_0.data_0[0][3], SceneData_0.u_ProjectionView_0.data_0[1][3], SceneData_0.u_ProjectionView_0.data_0[2][3], SceneData_0.u_ProjectionView_0.data_0[3][3]))));
    out_0.v_Color_0 = input_a_Color_0;
    VertexOutput_0 _S1 = out_0;

    #line 23
    gl_Position = out_0.v_Position_0;

    #line 23
    entryPointParam_vsMain_v_Color_0 = _S1.v_Color_0;

    #line 23
    return;
}

#type fragment
#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 6 0
layout(location = 0)
out vec4 entryPointParam_psMain_0;


#line 6
layout(location = 0)
in vec3 input_v_Color_0;


#line 27
void main()
{

    #line 27
    entryPointParam_psMain_0 = vec4(input_v_Color_0, 1.0);

    #line 27
    return;
}
