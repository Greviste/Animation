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
uniform mat2x4 dual_quats[256];
uniform float dual_quat_part;

out vec3 position;
out vec3 normal;

mat4 dualQuatToMat(mat2x4 dual_quat)
{
    float l = 1.0 / length(dual_quat[0]);
    vec4 c0 = dual_quat[0] * l;
    vec4 ce = dual_quat[1] * l;
    return 2.0 * mat4(
        vec4(
            0.5 - c0.y * c0.y - c0.z * c0.z,
            c0.x * c0.y + c0.w * c0.z,
            c0.x * c0.z - c0.w * c0.y,
            0.0
        ),
        vec4(
            c0.x * c0.y - c0.w * c0.z,
            0.5 - c0.x * c0.x - c0.z * c0.z,
            c0.y * c0.z + c0.w * c0.x,
            0.0
        ),
        vec4(
            c0.x * c0.z + c0.w * c0.y,
            c0.y * c0.z - c0.w * c0.x,
            0.5 - c0.x * c0.x - c0.y * c0.y,
            0.0
        ),
        vec4(
            -ce.w * c0.x + ce.x * c0.w - ce.y * c0.z + ce.z * c0.y,
            -ce.w * c0.y + ce.x * c0.z + ce.y * c0.w - ce.z * c0.x,
            -ce.w * c0.z - ce.x * c0.y + ce.y * c0.x + ce.z * c0.w,
            0.5
        )
    );
}

void main()
{
    mat4 skin_mat = mat4(0.0);
    mat4 norm_skin_mat = mat4(0.0);
    mat2x4 dual_quat = mat2x4(0.0);
    for(int i = 0; i < 4; ++i)
    {
        skin_mat += i_bone_weights[i] * bone_mats[i_bones[i]];
        norm_skin_mat += i_bone_weights[i] * norm_bone_mats[i_bones[i]];
        dual_quat += i_bone_weights[i] * dual_quats[i_bones[i]];
    }
    mat4 quat_mat = dualQuatToMat(dual_quat);

    vec4 skinned_position = mix(skin_mat * vec4(i_position, 1), quat_mat * vec4(i_position, 1), dual_quat_part);
    skinned_position.w = 1.0;   //To avoid imprecision in weight sum. Probably unneeded
	position = (mv_matrix * skinned_position).xyz;
    normal = mix(norm_matrix * norm_skin_mat * vec4(i_normal, 0), norm_matrix * quat_mat * vec4(i_normal, 0), dual_quat_part).xyz;
    normal = normalize(normal);

    gl_Position = proj_matrix * vec4(position, 1);
}
