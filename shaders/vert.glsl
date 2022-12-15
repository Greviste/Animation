#version 430

layout (location=0) in vec3 i_position;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

out vec3 position;


void main()
{
	position = i_position;
    gl_Position = proj_matrix * mv_matrix * vec4(i_position,1);
}
