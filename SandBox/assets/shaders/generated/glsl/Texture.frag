#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 28 0
layout(binding = 1)
uniform sampler2D u_Texture_0;


#line 962 1
layout(location = 0)
out vec4 entryPointParam_psMain_0;


#line 962
layout(location = 0)
in vec2 input_v_TexCoord_0;


#line 31 0
void main()
{

#line 31
    entryPointParam_psMain_0 = (texture((u_Texture_0), (input_v_TexCoord_0)));

#line 31
    return;
}

