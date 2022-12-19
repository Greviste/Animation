#version 460

layout (location=0) in vec3 i_position;
layout (location=1) in vec3 i_normal;
layout (location=2) in uvec4 i_bones;
layout (location=3) in vec4 i_bone_weights;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 bone_mats[255];

out vec3 position;


void main()
{
    vec4 skinned_position = vec4(0.0);
    for(int i = 0; i < 4; ++i)
    {
        skinned_position += i_bone_weights[i] * bone_mats[i_bones[i]] * vec4(i_position, 1);
    }

    skinned_position.w = 1.0;   //To avoid imprecision in weight sum. Probably unneeded
	position = skinned_position.xyz;
    gl_Position = proj_matrix * mv_matrix * skinned_position;
}
