#version 460

layout (location=0) in vec3 i_position;
layout (location=1) in vec3 i_normal;
layout (location=2) in uvec4 i_bones;
layout (location=3) in vec4 i_bone_weights;

uniform mat4 mv_matrix;
uniform mat4 norm_matrix;
uniform mat4 proj_matrix;
uniform mat4 bone_mats[256];
uniform mat4 norm_bone_mats[256];

out vec3 position;
out vec3 normal;


void main()
{
    mat4 skin_mat = mat4(0.0);
    mat4 norm_skin_mat = mat4(0.0);
    for(int i = 0; i < 4; ++i)
    {
        skin_mat += i_bone_weights[i] * bone_mats[i_bones[i]];
        norm_skin_mat += i_bone_weights[i] * norm_bone_mats[i_bones[i]];
    }

    vec4 skinned_position = skin_mat * vec4(i_position, 1);
    skinned_position.w = 1.0;   //To avoid imprecision in weight sum. Probably unneeded
	position = (mv_matrix * skinned_position).xyz;
    normal = (norm_matrix * norm_skin_mat * vec4(i_normal, 0)).xyz;
    normal = normalize(normal);
    gl_Position = proj_matrix * vec4(position, 1);
}
