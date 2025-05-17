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

