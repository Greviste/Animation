#version 430

in vec3 position;
in vec3 normal;

out vec4 fragColor;

const vec3 light_dir = normalize(vec3(0.2,0.5,0.7));

void main()
{
    float light = max(0.0, dot(normalize(normal), light_dir));
    light = min(1.0, light + 0.01);
    fragColor = vec4(1,1,1,0) * light;
}
